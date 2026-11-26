#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "WarpDSP.h"

using namespace juce;
using NR = NormalisableRange<float>;

static AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;

    auto f = [&] (const String& id, const String& name, NR r, float def, const String& unit = {})
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                                                      AudioParameterFloatAttributes().withLabel (unit)));
    };

    f ("inGain",   "Input",        NR (-24.f, 24.f, 0.01f), 0.f, "dB");
    f ("outGain",  "Output",       NR (-24.f, 12.f, 0.01f), 0.f, "dB");
    f ("mix",      "Mix",          NR (0.f, 1.f, 0.001f),   1.f);
    f ("xover1",   "X-Over 1",     NR (40.f, 2000.f, 0.f, 0.3f),   200.f,  "Hz");
    f ("xover2",   "X-Over 2",     NR (400.f, 12000.f, 0.f, 0.3f), 2500.f, "Hz");
    f ("hp",       "HP",           NR (20.f, 2000.f, 0.f, 0.3f),   20.f,   "Hz");
    f ("lp",       "LP",           NR (500.f, 20000.f, 0.f, 0.3f), 20000.f, "Hz");
    f ("lfoRate",  "LFO Rate",     NR (0.05f, 20.f, 0.f, 0.4f), 1.f, "Hz");
    f ("lfoDepth", "LFO Depth",    NR (0.f, 1.f, 0.001f), 0.f);
    f ("envAmt",   "Env Amount",   NR (-1.f, 1.f, 0.001f), 0.f);
    f ("envRel",   "Env Release",  NR (10.f, 1000.f, 0.f, 0.4f), 150.f, "ms");

    StringArray shapes;
    for (auto* s : warp::shapeNames) shapes.add (s);

    const char* names[] = { "Low", "Mid", "High" };
    const int defShape[] = { 0, 4, 2 };
    for (int b = 0; b < 3; ++b)
    {
        String p = "b" + String (b + 1) + "_", n = String (names[b]) + " ";
        l.add (std::make_unique<AudioParameterBool>   (ParameterID { p + "on", 1 },    n + "On", true));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { p + "shape", 1 }, n + "Shape", shapes, defShape[b]));
        f (p + "drive", n + "Drive", NR (0.f, 48.f, 0.01f),  12.f, "dB");
        f (p + "bias",  n + "Bias",  NR (-1.f, 1.f, 0.001f), 0.f);
        f (p + "mix",   n + "Mix",   NR (0.f, 1.f, 0.001f),  1.f);
        f (p + "gain",  n + "Gain",  NR (-24.f, 12.f, 0.01f), 0.f, "dB");
    }
    return l;
}

WarpAudioProcessor::WarpAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto g = [this] (const String& id) { return apvts.getRawParameterValue (id); };
    for (int b = 0; b < 3; ++b)
    {
        String p = "b" + String (b + 1) + "_";
        bp[b] = { g (p + "on"), g (p + "shape"), g (p + "drive"), g (p + "bias"), g (p + "mix"), g (p + "gain") };
        bandIn[b] = 0.f; bandOut[b] = 0.f;
    }
    pIn = g ("inGain"); pOut = g ("outGain"); pMix = g ("mix");
    pX1 = g ("xover1"); pX2 = g ("xover2"); pHp = g ("hp"); pLp = g ("lp");
    pLfoRate = g ("lfoRate"); pLfoDepth = g ("lfoDepth"); pEnvAmt = g ("envAmt"); pEnvRel = g ("envRel");
    for (auto& s : scope) s = 0.f;
}

bool WarpAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto o = l.getMainOutputChannelSet();
    return (o == AudioChannelSet::mono() || o == AudioChannelSet::stereo()) && o == l.getMainInputChannelSet();
}

void WarpAudioProcessor::prepareToPlay (double sr, int bs)
{
    osRate = sr * 4.0;
    dsp::ProcessSpec spec   { sr,     (uint32) bs,     2 };
    dsp::ProcessSpec osSpec { osRate, (uint32) bs * 4, 2 };

    oversampler.reset();
    oversampler.initProcessing ((size_t) bs);
    const int latency = (int) std::ceil (oversampler.getLatencyInSamples());
    setLatencySamples (latency);

    mixer.prepare (spec);
    mixer.setMixingRule (dsp::DryWetMixingRule::linear);
    mixer.setWetLatency ((float) latency);

    xo1.prepare (osSpec); xo2.prepare (osSpec); apLow.prepare (osSpec);
    xo1.setType (dsp::LinkwitzRileyFilterType::lowpass);
    xo2.setType (dsp::LinkwitzRileyFilterType::lowpass);
    apLow.setType (dsp::LinkwitzRileyFilterType::allpass);

    hpF.prepare (spec); hpF.setType (dsp::StateVariableTPTFilterType::highpass);
    lpF.prepare (spec); lpF.setType (dsp::StateVariableTPTFilterType::lowpass);

    for (int b = 0; b < 3; ++b)
        for (auto* s : { &driveSm[b], &biasSm[b], &mixSm[b], &gainSm[b] })
            s->reset (osRate, 0.02);

    dcR = 1.0f - (float) (MathConstants<double>::twoPi * 10.0 / osRate);
    std::fill (&dcX[0][0], &dcX[0][0] + 6, 0.f);
    std::fill (&dcY[0][0], &dcY[0][0] + 6, 0.f);
    env = 0.f; lfoPhase = 0.f;
    lastIn  = Decibels::decibelsToGain (pIn->load());
    lastOut = Decibels::decibelsToGain (pOut->load());
    updateTargets (true);
}

void WarpAudioProcessor::updateTargets (bool jump)
{
    auto set = [jump] (SmoothedValue<float>& s, float v)
    {
        if (jump) s.setCurrentAndTargetValue (v); else s.setTargetValue (v);
    };
    for (int b = 0; b < 3; ++b)
    {
        set (driveSm[b], Decibels::decibelsToGain (bp[b].drive->load()));
        set (biasSm[b],  bp[b].bias->load());
        set (mixSm[b],   bp[b].mix->load());
        set (gainSm[b],  Decibels::decibelsToGain (bp[b].gain->load()));
    }
}

void WarpAudioProcessor::processShaper (dsp::AudioBlock<float>& blk)
{
    const int n   = (int) blk.getNumSamples();
    const int nch = (int) blk.getNumChannels();
    float* ch[2] = { blk.getChannelPointer (0), nch > 1 ? blk.getChannelPointer (1) : nullptr };

    updateTargets (false);

    int  type[3]; bool on[3];
    for (int b = 0; b < 3; ++b)
    {
        type[b] = (int) bp[b].shape->load();
        on[b]   = bp[b].on->load() > 0.5f;
    }

    const float x1 = pX1->load();
    const float x2 = jmax (pX2->load(), x1 * 1.25f);
    xo1.setCutoffFrequency (x1);
    xo2.setCutoffFrequency (x2);
    apLow.setCutoffFrequency (x2);

    const float lfoInc   = pLfoRate->load() / (float) osRate;
    const float lfoDepth = pLfoDepth->load();
    const float envAmt   = pEnvAmt->load();
    const float relCoef  = std::exp (-1.0f / (pEnvRel->load() * 0.001f * (float) osRate));

    for (int i = 0; i < n; ++i)
    {
        // modulation: LFO + envelope follower -> drive offset in dB
        const float lfo = std::sin (lfoPhase * MathConstants<float>::twoPi);
        lfoPhase += lfoInc; if (lfoPhase >= 1.f) lfoPhase -= 1.f;

        float inAbs = std::abs (ch[0][i]);
        if (ch[1]) inAbs = jmax (inAbs, std::abs (ch[1][i]));
        env = inAbs > env ? inAbs : env * relCoef;

        const float modDb = 24.0f * (lfo * lfoDepth + envAmt * jmin (env, 1.0f));
        const float modGain = Decibels::decibelsToGain (modDb);

        float d[3], bi[3], mx[3], g[3];
        for (int b = 0; b < 3; ++b)
        {
            d[b]  = driveSm[b].getNextValue() * modGain;
            bi[b] = biasSm[b].getNextValue();
            mx[b] = mixSm[b].getNextValue();
            g[b]  = gainSm[b].getNextValue();
        }

        for (int c = 0; c < nch; ++c)
        {
            float low, rest, mid, high;
            xo1.processSample (c, ch[c][i], low, rest);
            xo2.processSample (c, rest, mid, high);
            low = apLow.processSample (c, low);   // phase-align low band with the 2nd crossover

            const float bands[3] = { low, mid, high };
            float sum = 0.f;

            for (int b = 0; b < 3; ++b)
            {
                const float in = bands[b];
                if (! on[b]) { sum += in; continue; }

                const float y = warp::shape (in * d[b] + bi[b], type[b]);

                if (i == n - 1 && c == 0)
                {
                    bandIn[b]  = jlimit (-1.f, 1.f, in);
                    bandOut[b] = y;
                }

                const float dc = y - dcX[b][c] + dcR * dcY[b][c];   // DC blocker
                dcX[b][c] = y; dcY[b][c] = dc;

                sum += (in + (dc - in) * mx[b]) * g[b];
            }
            ch[c][i] = sum;
        }
    }
}

void WarpAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n   = buffer.getNumSamples();
    const int nch = jmin (buffer.getNumChannels(), 2);

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    const float inG = Decibels::decibelsToGain (pIn->load());
    buffer.applyGainRamp (0, n, lastIn, inG);
    lastIn = inG;

    dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) nch, (size_t) n);

    mixer.setWetMixProportion (pMix->load());
    mixer.pushDrySamples (block);

    auto osBlock = oversampler.processSamplesUp (block);
    processShaper (osBlock);
    oversampler.processSamplesDown (block);

    dsp::ProcessContextReplacing<float> ctx (block);
    const float hp = pHp->load(), lp = pLp->load();
    if (hp > 21.f)     { hpF.setCutoffFrequency (hp); hpF.process (ctx); }
    if (lp < 19900.f)  { lpF.setCutoffFrequency (lp); lpF.process (ctx); }

    mixer.mixWetSamples (block);

    const float outG = Decibels::decibelsToGain (pOut->load());
    buffer.applyGainRamp (0, n, lastOut, outG);
    lastOut = outG;

    int pos = scopePos.load();
    const float* r = buffer.getReadPointer (0);
    for (int i = 0; i < n; i += 4)
    {
        scope[(size_t) pos] = r[i];
        pos = (pos + 1) % scopeSize;
    }
    scopePos = pos;
}

AudioProcessorEditor* WarpAudioProcessor::createEditor() { return new WarpAudioProcessorEditor (*this); }

void WarpAudioProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void WarpAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WarpAudioProcessor(); }
