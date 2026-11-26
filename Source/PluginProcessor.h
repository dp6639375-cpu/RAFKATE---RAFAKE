#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>
#include "WarpDSP.h"

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
    const juce::String getName() const override { return "RAF-KATE"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return curProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void randomize();

    juce::AudioProcessorValueTreeState apvts;

    // visualisation data (audio thread writes, GUI reads)
    static constexpr int scopeSize = 1024;
    std::array<std::atomic<float>, scopeSize> scope;
    std::atomic<int> scopePos { 0 };
    std::atomic<float> bandIn[3], bandOut[3];
    std::atomic<float> curvePhase { 0.0f };
    std::array<std::atomic<float>, 4096> fftRing;   // full-rate output ring for the spectrum view
    std::atomic<int> fftPos { 0 };

private:
    struct BandP { std::atomic<float> *on, *shapeP, *shapeN, *drive, *bias, *mix, *gain; } bp[3];
    std::atomic<float> *pIn, *pOut, *pMix, *pX1, *pX2;
    std::atomic<float> *preOn, *preType, *preCut, *preRes, *preMorph, *postOn, *postType, *postCut, *postRes, *postMorph;
    std::atomic<float> *fbOn, *fbFreq, *fbAmt, *fbDamp, *fbMix, *fbSnap, *fbMidi;
    std::atomic<float> *pLfoRate, *pCrvRate, *pEnvRel, *pM1, *pM2, *pC[8], *modAmt[5][3];

    juce::dsp::Oversampling<float> oversampler { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    juce::dsp::LinkwitzRileyFilter<float> xo1, xo2, apLow;
    juce::dsp::DryWetMixer<float> mixer { 64 };
    warp::FilterFx preF, postF;

    juce::SmoothedValue<float> driveSm[3], biasSm[3], mixSm[3], gainSm[3], fbFreqSm;
    std::vector<float> modDrive, modCut, modFb;   // per base-rate sample
    std::vector<float> fbBuf[2];
    int fbW = 0, fbSize = 1;
    float fbLp[2] {};
    float dcX[3][2] {}, dcY[3][2] {};
    float dcR = 0.9997f, env = 0.0f, lfoPh = 0.0f, crvPh = 0.0f, lastIn = 1.0f, lastOut = 1.0f;
    float noteFreq = 110.0f; bool haveNote = false;
    double sr = 48000.0, osRate = 192000.0;
    int curProgram = 0;

    void applyPreset (int index);
    void computeMod (const juce::AudioBuffer<float>& buf, int n);
    void updateTargets (bool jump);
    void processShaper (juce::dsp::AudioBlock<float>& block);
    void processFeedback (juce::dsp::AudioBlock<float>& block);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarpAudioProcessor)
};
