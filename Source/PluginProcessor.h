#pragma once
#include <JuceHeader.h>
#include <get_dsp.h>
#include <array>
#include <memory>
#include <vector>

class LibertyNeuralMatrixAudioProcessor final
    : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    LibertyNeuralMatrixAudioProcessor();
    ~LibertyNeuralMatrixAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int blockSize) override;
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

    juce::Result loadFile(int slot, const juce::File& file);
    juce::String slotDisplayName(int slot) const;
    juce::AudioProcessorValueTreeState state;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout parameters();
    void handleAsyncUpdate() override;

    juce::CriticalSection modelMutex;
    std::array<std::unique_ptr<nam::DSP>, 2> models;
    std::array<juce::String, 4> slotPaths;
    std::array<juce::dsp::Convolution, 2> irs;
    std::array<bool, 2> irLoaded {{false, false}};
    juce::AudioBuffer<float> irScratchA, irScratchB;
    std::vector<float> mono, namA, namB;
    juce::SmoothedValue<float> inputGain, outputGain, namBlend, irBlend;
    double preparedRate = 48000.0;
    int maxBlock = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibertyNeuralMatrixAudioProcessor)
};
