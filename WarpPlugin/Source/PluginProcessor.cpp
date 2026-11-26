#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using NR = NormalisableRange<float>;

static const char* modSrc[5] = { "lfo", "crv", "env", "m1", "m2" };
static const char* modTgt[3] = { "drive", "cut", "fb" };

// ---------------------------------------------------------------- parameters
static AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;

    auto f = [&] (const String& id, const String& name, NR r, float def, const String& unit = {})
    {
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                                                      AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto b = [&] (const String& id, const String& name, bool def)
    {
        l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    f ("inGain",  "Input",    NR (-24.f, 24.f, 0.01f), 0.f, "dB");
    f ("outGain", "Output",   NR (-24.f, 12.f, 0.01f), 0.f, "dB");
    f ("mix",     "Mix",      NR (0.f, 1.f, 0.001f),   1.f);
    f ("xover1",  "X-Over 1", NR (40.f, 2000.f, 0.f, 0.3f),   200.f,  "Hz");
    f ("xover2",  "X-Over 2", NR (400.f, 12000.f, 0.f, 0.3f), 2500.f, "Hz");

    StringArray shapes;
    for (auto* s : warp::shapeNames) shapes.add (s);

    const char* names[] = { "Low", "Mid", "High" };
    const int defP[] = { 0, 0, 2 }, defN[] = { 0, 4, 10 };
    for (int i = 0; i < 3; ++i)
    {
        String p = "b" + String (i + 1) + "_", n = String (names[i]) + " ";
        b (p + "on", n + "On", true);
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { p + "shapeP", 1 }, n + "Positive", shapes, defP[i]));
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { p + "shapeN", 1 }, n + "Negative", shapes, defN[i]));
        f (p + "drive", n + "Drive", NR (0.f, 48.f, 0.01f),   12.f, "dB");
        f (p + "bias",  n + "Bias",  NR (-1.f, 1.f, 0.001f),  0.f);
        f (p + "mix",   n + "Mix",   NR (0.f, 1.f, 0.001f),   1.f);
        f (p + "gain",  n + "Gain",  NR (-24.f, 12.f, 0.01f), 0.f, "dB");
    }

    StringArray filters;
    for (auto* fn : warp::filterNames) filters.add (fn);
    b ("pre_on", "Pre Filter On", false);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "pre_type", 1 }, "Pre Type", filters, 8));
    f ("pre_cut",   "Pre Cutoff",    NR (20.f, 20000.f, 0.f, 0.3f), 1000.f, "Hz");
    f ("pre_res",   "Pre Resonance", NR (0.f, 1.f, 0.001f), 0.2f);
    f ("pre_morph", "Pre Morph",     NR (0.f, 1.f, 0.001f), 0.f);
    b ("post_on", "Post Filter On", false);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "post_type", 1 }, "Post Type", filters, 8));
    f ("post_cut",   "Post Cutoff",    NR (20.f, 20000.f, 0.f, 0.3f), 8000.f, "Hz");
    f ("post_res",   "Post Resonance", NR (0.f, 1.f, 0.001f), 0.2f);
    f ("post_morph", "Post Morph",     NR (0.f, 1.f, 0.001f), 0.f);

    b ("fb_on", "Feedback On", false);
    f ("fb_freq", "FB Pitch",   NR (30.f, 2000.f, 0.f, 0.4f),   110.f,  "Hz");
    f ("fb_amt",  "FB Amount",  NR (0.f, 0.95f, 0.001f),        0.5f);
    f ("fb_damp", "FB Damping", NR (500.f, 16000.f, 0.f, 0.4f), 6000.f, "Hz");
    f ("fb_mix",  "FB Mix",     NR (0.f, 1.f, 0.001f),          0.5f);
    b ("fb_snap", "FB Pitch Snap", true);
    b ("fb_midi", "FB MIDI Track", false);

    f ("lfoRate", "LFO Rate",   NR (0.05f, 20.f, 0.f, 0.4f),   1.f, "Hz");
    f ("crvRate", "Curve Rate", NR (0.05f, 20.f, 0.f, 0.4f),   1.f, "Hz");
    f ("envRel",  "Follow Release", NR (10.f, 1000.f, 0.f, 0.4f), 150.f, "ms");
    f ("m1", "Macro 1", NR (0.f, 1.f, 0.001f), 0.f);
    f ("m2", "Macro 2", NR (0.f, 1.f, 0.001f), 0.f);

    const float defC[8] = { 1.f, 0.75f, 0.5f, 0.25f, 0.f, 0.25f, 0.5f, 0.75f };
    for (int i = 0; i < 8; ++i) f ("c" + String (i + 1), "Curve " + String (i + 1), NR (0.f, 1.f, 0.001f), defC[i]);

    for (auto* s : modSrc)
        for (auto* t : modTgt)
            f (String ("mod_") + s + "_" + t, String ("Mod ") + s + " > " + t, NR (-1.f, 1.f, 0.001f), 0.f);

    return l;
}

// ---------------------------------------------------------------- presets
struct Preset { std::string name; std::vector<std::pair<std::string, float>> v; };

// Procedurally generated preset families. Each (category, index) pair is deterministic.
static Preset generatePreset (int cat, int idx)
{
    static const char* catNames[8] = { "Subtle", "Bass", "Lead", "Drums", "Rhythmic", "Resonant", "Extreme", "Lo-Fi" };
    static const std::vector<int> pools[8] = {
        { 0, 5, 7, 4, 12, 14, 15 },  { 0, 4, 7, 3, 11, 12, 13 },  { 2, 8, 10, 9, 17, 18, 26, 27 },
        { 3, 11, 1, 19, 21, 29, 22 }, { 0, 2, 1, 8, 13, 23, 24 },  { 9, 7, 2, 17, 18, 28, 16 },
        { 2, 8, 9, 10, 17, 18, 19, 20, 25, 26, 27, 29, 3, 11, 1 }, { 3, 11, 19, 29, 15, 14, 22 } };
    static const float driveLo[8] = { 2, 6, 10, 8, 8, 10, 26, 6 }, driveHi[8] = { 10, 22, 30, 26, 24, 26, 46, 20 };
    static const float postProb[8] = { 0.8f, 0.7f, 0.5f, 0.4f, 0.5f, 0.6f, 0.6f, 1.0f };
    static const float fbProb[8]   = { 0.05f, 0.1f, 0.2f, 0.1f, 0.15f, 1.0f, 0.4f, 0.05f };
    static const float notes[7]    = { 55.f, 82.4f, 110.f, 146.8f, 220.f, 293.7f, 440.f };

    Random r (1000 * (cat + 1) + idx * 7 + 13);
    Preset p;
    p.name = (String (catNames[cat]) + " " + String (idx + 1)).toStdString();
    auto set = [&] (const String& id, float v) { p.v.emplace_back (id.toStdString(), v); };
    auto rf  = [&] (float lo, float hi) { return lo + (hi - lo) * r.nextFloat(); };
    const auto& pool = pools[cat];
    auto pick = [&] { return (float) pool[(size_t) r.nextInt ((int) pool.size())]; };

    set ("xover1", cat == 1 ? rf (70.f, 160.f) : rf (120.f, 320.f));
    set ("xover2", rf (1500.f, 4500.f));
    set ("mix", cat == 0 ? rf (0.5f, 1.f) : rf (0.8f, 1.f));

    for (int b = 1; b <= 3; ++b)
    {
        const String q = "b" + String (b) + "_";
        float d = rf (driveLo[cat], driveHi[cat]);
        if (cat == 1 && b == 3) d *= 0.4f;
        if (cat == 1 && b == 1) d = jmin (48.f, d * 1.2f);
        const float sp = pick();
        set (q + "drive", d);
        set (q + "shapeP", sp);
        set (q + "shapeN", r.nextFloat() < 0.65f ? pick() : sp);
        set (q + "bias", rf (-0.15f, 0.15f));
        set (q + "mix", rf (0.7f, 1.f));
        set (q + "gain", rf (-3.f, 0.f));
    }

    const bool post = r.nextFloat() < postProb[cat];
    if (post)
    {
        static const int types[9] = { 0, 1, 8, 9, 10, 12, 13, 15, 17 };
        set ("post_on", 1.f);
        set ("post_type", cat == 7 ? (float) r.nextInt (2) : (float) types[r.nextInt (9)]);
        set ("post_cut", cat == 7 ? rf (2500.f, 7000.f) : rf (2000.f, 12000.f));
        set ("post_res", rf (0.1f, 0.5f));
        set ("post_morph", rf (0.f, 1.f));
    }
    if (r.nextFloat() < 0.35f)
    {
        static const int types[7] = { 2, 3, 4, 8, 9, 11, 14 };
        set ("pre_on", 1.f);
        set ("pre_type", (float) types[r.nextInt (7)]);
        set ("pre_cut", rf (200.f, 4000.f));
        set ("pre_res", rf (0.1f, 0.6f));
        set ("pre_morph", rf (0.f, 1.f));
    }
    const bool fb = r.nextFloat() < fbProb[cat];
    if (fb)
    {
        set ("fb_on", 1.f);
        set ("fb_freq", notes[r.nextInt (7)]);
        set ("fb_amt", rf (0.5f, 0.9f));
        set ("fb_mix", rf (0.3f, 0.8f));
        set ("fb_damp", rf (2000.f, 9000.f));
    }

    set ("lfoRate", rf (0.1f, 8.f));
    if (r.nextFloat() < 0.5f) set ("mod_lfo_drive", rf (0.05f, 0.4f) * (r.nextBool() ? 1.f : -1.f));
    if (post && r.nextFloat() < 0.3f) set ("mod_lfo_cut", rf (0.1f, 0.5f));

    if (cat == 3 || cat == 4)
    {
        for (int k = 1; k <= 8; ++k) set ("c" + String (k), r.nextFloat() < 0.5f ? 1.f : rf (0.f, 0.4f));
        static const float rates[4] = { 1.f, 2.f, 4.f, 8.f };
        set ("crvRate", rates[r.nextInt (4)]);
        set ("mod_crv_drive", rf (0.4f, 0.9f));
        if (post) set ("mod_crv_cut", rf (-0.5f, 0.5f));
    }
    if (cat == 3) { set ("envRel", rf (40.f, 200.f)); set ("mod_env_drive", rf (0.2f, 0.6f)); }
    if (cat == 2 && post) set ("mod_env_cut", rf (0.2f, 0.6f));

    // Macros drive the Play View
    set ("m1", rf (0.f, 0.3f));
    set ("mod_m1_drive", rf (0.3f, 0.8f));
    if (post) set ("mod_m2_cut", rf (-0.6f, 0.6f)); else set ("mod_m2_fb", rf (0.2f, 0.6f));
    return p;
}

static const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = []
    {
        std::vector<Preset> v = {
            { "Init", {} },
            { "Warm Tape", { { "b1_drive", 8 }, { "b2_drive", 9 }, { "b3_drive", 6 },
                             { "b2_shapeN", 4 }, { "b3_shapeP", 5 }, { "b3_shapeN", 5 },
                             { "post_on", 1 }, { "post_cut", 9000 }, { "post_type", 0 } } },
            { "Fuzz Wall", { { "b1_drive", 30 }, { "b2_drive", 28 }, { "b3_drive", 24 },
                             { "b1_shapeP", 8 }, { "b2_shapeP", 8 }, { "b3_shapeP", 8 },
                             { "b1_shapeN", 1 }, { "b2_shapeN", 1 }, { "b3_shapeN", 1 },
                             { "post_on", 1 }, { "post_type", 1 }, { "post_cut", 5000 }, { "post_res", 0.2f },
                             { "mod_env_drive", 0.2f } } },
            { "Wavefold Lead", { { "b2_drive", 22 }, { "b2_shapeP", 2 }, { "b2_shapeN", 10 },
                                 { "b3_drive", 18 }, { "b3_shapeP", 2 }, { "b3_shapeN", 2 },
                                 { "mod_lfo_drive", 0.15f }, { "lfoRate", 0.4f } } },
            { "Bass Crusher", { { "xover1", 150 }, { "b1_drive", 14 }, { "b1_shapeP", 3 }, { "b1_shapeN", 11 },
                                { "b2_drive", 20 }, { "b2_shapeP", 11 }, { "b2_shapeN", 3 },
                                { "b3_drive", 6 }, { "mix", 0.9f } } },
            { "Metal Resonator", { { "b1_drive", 16 }, { "b2_drive", 16 }, { "b3_drive", 16 },
                                   { "b1_shapeP", 9 }, { "b2_shapeP", 9 }, { "b3_shapeP", 9 },
                                   { "b1_shapeN", 7 }, { "b2_shapeN", 7 }, { "b3_shapeN", 7 },
                                   { "fb_on", 1 }, { "fb_freq", 220 }, { "fb_amt", 0.8f }, { "fb_mix", 0.7f },
                                   { "fb_damp", 4000 }, { "pre_on", 1 }, { "pre_cut", 2500 },
                                   { "pre_morph", 0.5f }, { "pre_res", 0.5f } } },
            { "Rhythmic Gate", { { "b1_drive", 14 }, { "b2_drive", 14 }, { "b3_drive", 14 },
                                 { "b1_shapeN", 1 }, { "b2_shapeN", 1 }, { "b3_shapeN", 1 },
                                 { "c1", 1 }, { "c2", 0 }, { "c3", 1 }, { "c4", 0 },
                                 { "c5", 1 }, { "c6", 0 }, { "c7", 1 }, { "c8", 0 },
                                 { "crvRate", 4 }, { "mod_crv_drive", 0.8f } } },
        };
        for (int c = 0; c < 8; ++c)
            for (int i = 0; i < 40; ++i)
                v.push_back (generatePreset (c, i));
        return v;
    }();
    return p;
}

int WarpAudioProcessor::getNumPrograms() { return (int) presets().size(); }
const String WarpAudioProcessor::getProgramName (int i) { return String (presets()[(size_t) jlimit (0, getNumPrograms() - 1, i)].name); }
void WarpAudioProcessor::setCurrentProgram (int i)
{
    curProgram = jlimit (0, getNumPrograms() - 1, i);
    applyPreset (curProgram);
}

void WarpAudioProcessor::applyPreset (int index)
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (p))
            if (rp->paramID != "inGain" && rp->paramID != "outGain")
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (auto& [id, value] : presets()[(size_t) index].v)
        if (auto* p = apvts.getParameter (String (id)))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void WarpAudioProcessor::randomize()
{
    Random r;
    for (auto* p : getParameters())
    {
        auto* rp = dynamic_cast<RangedAudioParameter*> (p);
        if (rp == nullptr) continue;
        const String id = rp->paramID;
        if (id == "inGain" || id == "outGain" || id == "mix" || id == "xover1" || id == "xover2"
            || id == "fb_midi" || id == "fb_snap" || (id.startsWith ("b") && id.endsWith ("_on")))
            continue;

        float v = r.nextFloat();
        if (id.endsWith ("_drive"))        v = 0.15f + 0.45f * v;
        else if (id.endsWith ("_mix"))     v = 0.5f + 0.5f * v;
        else if (id == "fb_amt")           v *= 0.7f;
        else if (id.endsWith ("_res"))     v *= 0.7f;
        else if (id.startsWith ("mod_"))   v = r.nextFloat() < 0.65f ? 0.5f : 0.5f + (v - 0.5f) * 0.6f;
        else if (id == "fb_on")            v = r.nextFloat() < 0.3f ? 1.f : 0.f;
        else if (id == "pre_on")           v = r.nextFloat() < 0.3f ? 1.f : 0.f;
        else if (id == "post_on")          v = r.nextFloat() < 0.5f ? 1.f : 0.f;
        rp->setValueNotifyingHost (v);
    }
}

// ---------------------------------------------------------------- construction
WarpAudioProcessor::WarpAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto g = [this] (const String& id) { return apvts.getRawParameterValue (id); };
    for (int i = 0; i < 3; ++i)
    {
        String p = "b" + String (i + 1) + "_";
        bp[i] = { g (p + "on"), g (p + "shapeP"), g (p + "shapeN"), g (p + "drive"),
                  g (p + "bias"), g (p + "mix"), g (p + "gain") };
        bandIn[i] = 0.f; bandOut[i] = 0.f;
    }
    pIn = g ("inGain"); pOut = g ("outGain"); pMix = g ("mix"); pX1 = g ("xover1"); pX2 = g ("xover2");
    preOn = g ("pre_on"); preType = g ("pre_type"); preCut = g ("pre_cut"); preRes = g ("pre_res"); preMorph = g ("pre_morph");
    postOn = g ("post_on"); postType = g ("post_type"); postCut = g ("post_cut"); postRes = g ("post_res"); postMorph = g ("post_morph");
    fbOn = g ("fb_on"); fbFreq = g ("fb_freq"); fbAmt = g ("fb_amt"); fbDamp = g ("fb_damp");
    fbMix = g ("fb_mix"); fbSnap = g ("fb_snap"); fbMidi = g ("fb_midi");
    pLfoRate = g ("lfoRate"); pCrvRate = g ("crvRate"); pEnvRel = g ("envRel"); pM1 = g ("m1"); pM2 = g ("m2");
    for (int i = 0; i < 8; ++i) pC[i] = g ("c" + String (i + 1));
    for (int s = 0; s < 5; ++s)
        for (int t = 0; t < 3; ++t)
            modAmt[s][t] = g (String ("mod_") + modSrc[s] + "_" + modTgt[t]);
    for (auto& s : scope) s = 0.f;
    for (auto& s : fftRing) s = 0.f;
}

bool WarpAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto o = l.getMainOutputChannelSet();
    return (o == AudioChannelSet::mono() || o == AudioChannelSet::stereo()) && o == l.getMainInputChannelSet();
}

void WarpAudioProcessor::prepareToPlay (double sampleRate, int bs)
{
    sr = sampleRate;
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

    preF.prepare (sr); postF.prepare (sr);

    for (int i = 0; i < 3; ++i)
        for (auto* s : { &driveSm[i], &biasSm[i], &mixSm[i], &gainSm[i] })
            s->reset (osRate, 0.02);
    fbFreqSm.reset (sr, 0.03);
    fbFreqSm.setCurrentAndTargetValue (fbFreq->load());

    fbSize = (int) (sr / 20.0) + 8;
    for (auto& v : fbBuf) v.assign ((size_t) fbSize, 0.f);
    fbW = 0; fbLp[0] = fbLp[1] = 0.f;

    modDrive.assign ((size_t) bs, 0.f); modCut.assign ((size_t) bs, 0.f); modFb.assign ((size_t) bs, 0.f);

    dcR = 1.0f - (float) (MathConstants<double>::twoPi * 10.0 / osRate);
    std::fill (&dcX[0][0], &dcX[0][0] + 6, 0.f);
    std::fill (&dcY[0][0], &dcY[0][0] + 6, 0.f);
    env = 0.f; lfoPh = 0.f; crvPh = 0.f;
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
    for (int i = 0; i < 3; ++i)
    {
        set (driveSm[i], Decibels::decibelsToGain (bp[i].drive->load()));
        set (biasSm[i],  bp[i].bias->load());
        set (mixSm[i],   bp[i].mix->load());
        set (gainSm[i],  Decibels::decibelsToGain (bp[i].gain->load()));
    }
}

// ---------------------------------------------------------------- modulation (base rate)
void WarpAudioProcessor::computeMod (const AudioBuffer<float>& buf, int n)
{
    if ((int) modDrive.size() < n) { modDrive.resize ((size_t) n); modCut.resize ((size_t) n); modFb.resize ((size_t) n); }

    float amt[5][3], pts[8];
    for (int s = 0; s < 5; ++s) for (int t = 0; t < 3; ++t) amt[s][t] = modAmt[s][t]->load();
    for (int k = 0; k < 8; ++k) pts[k] = pC[k]->load();

    const float lfoInc  = pLfoRate->load() / (float) sr;
    const float crvInc  = pCrvRate->load() / (float) sr;
    const float relCoef = std::exp (-1.0f / (pEnvRel->load() * 0.001f * (float) sr));
    const float m1 = pM1->load(), m2 = pM2->load();

    const float* r0 = buf.getReadPointer (0);
    const float* r1 = buf.getNumChannels() > 1 ? buf.getReadPointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        const float lfo = std::sin (lfoPh * MathConstants<float>::twoPi);
        lfoPh += lfoInc; if (lfoPh >= 1.f) lfoPh -= 1.f;
        const float crv = warp::curveValue (pts, crvPh) * 2.f - 1.f;
        crvPh += crvInc; if (crvPh >= 1.f) crvPh -= 1.f;

        float a = std::abs (r0[i]);
        if (r1) a = jmax (a, std::abs (r1[i]));
        env = a > env ? a : env * relCoef;

        const float src[5] = { lfo, crv, jmin (env, 1.f), m1, m2 };
        float d = 0.f, c = 0.f, f = 0.f;
        for (int s = 0; s < 5; ++s) { d += src[s] * amt[s][0]; c += src[s] * amt[s][1]; f += src[s] * amt[s][2]; }
        modDrive[(size_t) i] = 24.f * d;     // dB
        modCut[(size_t) i]   = 4.f * c;      // octaves
        modFb[(size_t) i]    = 0.5f * f;     // feedback amount offset
    }
    curvePhase = crvPh;
}

// ---------------------------------------------------------------- bipolar multiband shaper (4x oversampled)
void WarpAudioProcessor::processShaper (dsp::AudioBlock<float>& blk)
{
    const int n   = (int) blk.getNumSamples();
    const int nch = (int) blk.getNumChannels();
    float* ch[2] = { blk.getChannelPointer (0), nch > 1 ? blk.getChannelPointer (1) : nullptr };

    updateTargets (false);

    int tP[3], tN[3]; bool on[3];
    for (int b = 0; b < 3; ++b)
    {
        tP[b] = (int) bp[b].shapeP->load();
        tN[b] = (int) bp[b].shapeN->load();
        on[b] = bp[b].on->load() > 0.5f;
    }

    const float x1 = pX1->load();
    const float x2 = jmax (pX2->load(), x1 * 1.25f);
    xo1.setCutoffFrequency (x1);
    xo2.setCutoffFrequency (x2);
    apLow.setCutoffFrequency (x2);

    float modGain = 1.f;
    for (int i = 0; i < n; ++i)
    {
        if ((i & 3) == 0)
            modGain = Decibels::decibelsToGain (modDrive[(size_t) (i >> 2)]);

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

                const float v = in * d[b] + bi[b];
                const float y = warp::shape (v, v >= 0.f ? tP[b] : tN[b]);   // bipolar: separate engines per polarity

                if (i == n - 1 && c == 0) { bandIn[b] = jlimit (-1.f, 1.f, in); bandOut[b] = y; }

                const float dc = y - dcX[b][c] + dcR * dcY[b][c];   // DC blocker
                dcX[b][c] = y; dcY[b][c] = dc;

                sum += (in + (dc - in) * mx[b]) * g[b];
            }
            ch[c][i] = sum;
        }
    }
}

// ---------------------------------------------------------------- tuned feedback resonator (base rate)
void WarpAudioProcessor::processFeedback (dsp::AudioBlock<float>& blk)
{
    const int n   = (int) blk.getNumSamples();
    const int nch = (int) blk.getNumChannels();
    float* ch[2] = { blk.getChannelPointer (0), nch > 1 ? blk.getChannelPointer (1) : nullptr };

    float f = fbFreq->load();
    if (fbSnap->load() > 0.5f) f = warp::snapHz (f);
    if (fbMidi->load() > 0.5f && haveNote) f = noteFreq;
    fbFreqSm.setTargetValue (f);

    const float amt = fbAmt->load(), mixv = fbMix->load();
    const float a = 1.f - std::exp (-MathConstants<float>::twoPi * fbDamp->load() / (float) sr);

    for (int i = 0; i < n; ++i)
    {
        const float fr = fbFreqSm.getNextValue();
        const float d  = jlimit (2.f, (float) fbSize - 4.f, (float) sr / fr);
        float rd = (float) fbW - d; if (rd < 0.f) rd += (float) fbSize;
        const int i0 = (int) rd;
        int i1 = i0 + 1; if (i1 >= fbSize) i1 = 0;
        const float frac = rd - (float) i0;
        const float am = jlimit (0.f, 0.97f, amt + modFb[(size_t) i]);

        for (int c = 0; c < nch; ++c)
        {
            auto& buf = fbBuf[c];
            const float del = buf[(size_t) i0] + (buf[(size_t) i1] - buf[(size_t) i0]) * frac;
            fbLp[c] += a * (del - fbLp[c]);
            const float x = ch[c][i];
            const float part = std::tanh (am * fbLp[c]);   // soft-limited feedback path keeps the loop stable
            buf[(size_t) fbW] = x + part;
            ch[c][i] = x + mixv * part;
        }
        if (++fbW >= fbSize) fbW = 0;
    }
}

// ---------------------------------------------------------------- processBlock
void WarpAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n   = buffer.getNumSamples();
    const int nch = jmin (buffer.getNumChannels(), 2);

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            noteFreq = (float) MidiMessage::getMidiNoteInHertz (msg.getNoteNumber());
            haveNote = true;
        }
    }

    const float inG = Decibels::decibelsToGain (pIn->load());
    buffer.applyGainRamp (0, n, lastIn, inG);
    lastIn = inG;

    computeMod (buffer, n);

    dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) nch, (size_t) n);

    mixer.setWetMixProportion (pMix->load());
    mixer.pushDrySamples (block);

    if (preOn->load() > 0.5f)
    {
        preF.set ((int) preType->load(), preCut->load(), preRes->load(), preMorph->load());
        for (int c = 0; c < nch; ++c)
        {
            float* d = block.getChannelPointer ((size_t) c);
            for (int i = 0; i < n; ++i) d[i] = preF.process (c, d[i]);
        }
    }

    auto osBlock = oversampler.processSamplesUp (block);
    processShaper (osBlock);
    oversampler.processSamplesDown (block);

    if (fbOn->load() > 0.5f)
        processFeedback (block);

    if (postOn->load() > 0.5f)
    {
        const int pType = (int) postType->load();
        const float cut = postCut->load(), res = postRes->load(), morph = postMorph->load();
        for (int i = 0; i < n; ++i)
        {
            postF.set (pType, cut * std::exp2 (modCut[(size_t) i]), res, morph);
            for (int c = 0; c < nch; ++c)
            {
                float* d = block.getChannelPointer ((size_t) c);
                d[i] = postF.process (c, d[i]);
            }
        }
    }

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

    int fp = fftPos.load();
    for (int i = 0; i < n; ++i)
    {
        fftRing[(size_t) fp] = r[i];
        fp = (fp + 1) & 4095;
    }
    fftPos = fp;
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
