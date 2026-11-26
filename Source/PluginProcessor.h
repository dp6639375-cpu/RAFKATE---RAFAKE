#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class WarpAudioProcessor : public juce::AudioProcessor
{
public:
    WarpAudioProcessor();
    ~WarpAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "WARP"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // visualisation data (audio thread writes, GUI reads)
    static constexpr int scopeSize = 1024;
    std::array<std::atomic<float>, scopeSize> scope;
    std::atomic<int> scopePos { 0 };
    std::atomic<float> bandIn[3], bandOut[3];

private:
    struct BandP { std::atomic<float> *on, *shape, *drive, *bias, *mix, *gain; } bp[3];
    std::atomic<float> *pIn, *pOut, *pMix, *pX1, *pX2, *pHp, *pLp,
                       *pLfoRate, *pLfoDepth, *pEnvAmt, *pEnvRel;

    juce::dsp::Oversampling<float> oversampler { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    juce::dsp::LinkwitzRileyFilter<float> xo1, xo2, apLow;
    juce::dsp::StateVariableTPTFilter<float> hpF, lpF;
    juce::dsp::DryWetMixer<float> mixer { 64 };

    juce::SmoothedValue<float> driveSm[3], biasSm[3], mixSm[3], gainSm[3];
    float dcX[3][2] {}, dcY[3][2] {};
    float dcR = 0.9997f, env = 0.0f, lfoPhase = 0.0f, lastIn = 1.0f, lastOut = 1.0f;
    double osRate = 192000.0;

    void updateTargets (bool jump);
    void processShaper (juce::dsp::AudioBlock<float>& block);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarpAudioProcessor)
};
