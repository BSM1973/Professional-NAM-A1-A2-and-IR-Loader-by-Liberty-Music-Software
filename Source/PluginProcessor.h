#pragma once
#include <JuceHeader.h>
#include <get_dsp.h>
#include "AmpVisualDetector.h"
#include <array>
#include <memory>
#include <atomic>
#include <vector>

class LibertyNeuralMatrixAudioProcessor final : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    LibertyNeuralMatrixAudioProcessor();
    ~LibertyNeuralMatrixAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Liberty Neural Matrix"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // Slots 0/1 are .nam, slots 2/3 are .wav. These operations run off the audio callback.
    juce::Result loadFile(int slot, const juce::File& file);
    juce::String slotDisplayName(int slot) const;

    struct AmpVisualInfo
    {
        bool loaded = false;
        libertyAmp::Result detected;
        int manualFamily = -1; // -1 means automatic; >=0 forces a visual family
        juce::String customPhotoPath; // optional user-owned local artwork
        int displayedFamily() const noexcept
        {
            return manualFamily >= 0 ? manualFamily : detected.family;
        }
    };
    AmpVisualInfo ampVisualInfo(int slot) const;
    juce::Result setAmpVisualFamily(int slot, int familyOrAuto);
    juce::Result setAmpVisualPhoto(int slot, const juce::File& file);
    std::array<float, 128> irWaveform(int index) const;
    double getPreparedSampleRate() const noexcept { return preparedRateAtomic.load(); }
    juce::AudioProcessorValueTreeState state;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout parameters();
    void handleAsyncUpdate() override;
    juce::CriticalSection modelMutex;
    std::array<std::unique_ptr<nam::DSP>, 2> models;
    std::array<juce::String, 4> slotPaths;
    std::array<AmpVisualInfo, 2> visualInfo {};
    std::array<int, 2> pendingProjectFamily {{-1,-1}};
    std::array<juce::String, 2> pendingProjectPhoto {};
    bool hasPendingProjectVisuals = false;
    std::array<juce::dsp::Convolution, 2> irs;
    std::array<bool, 2> irLoaded {{ false, false }};
    std::array<std::array<float, 128>, 2> irPeaks {};

    juce::AudioBuffer<float> irScratchA, irScratchB;
    std::vector<float> mono, namA, namB;
    juce::SmoothedValue<float> inputGain, outputGain, namBlend, irBlend;
    struct ToneState
    {
        float low = 0, midLower = 0, midUpper = 0, upper = 0, presence = 0;
        float cabinetHP = 0, cabinetLP = 0;
    };
    std::array<ToneState, 2> toneStates;
    struct MicColourState
    {
        float low = 0.0f;
        float high = 0.0f;
    };
    std::array<std::array<MicColourState, 2>, 2> micStates {}; // [IR slot][channel]

    std::array<std::vector<float>, 2> delayLines;
    size_t delayPosition = 0;
    float gateEnvelope = 0.0f, gateGain = 1.0f;
    std::atomic<double> preparedRateAtomic { 48000.0 };
    double preparedRate = 48000.0;
    int maxBlock = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibertyNeuralMatrixAudioProcessor)
};
