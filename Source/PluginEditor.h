#pragma once
#include "PluginProcessor.h"
#include <array>

class LibertyNeuralMatrixAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit LibertyNeuralMatrixAudioProcessorEditor(LibertyNeuralMatrixAudioProcessor&);
    ~LibertyNeuralMatrixAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class Theme final : public juce::LookAndFeel_V4
    {
    public:
        Theme();
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float,
                              float, float, juce::Slider&) override;
    } theme;

    LibertyNeuralMatrixAudioProcessor& processor;
    std::array<juce::TextButton, 4> loadButtons;
    std::array<juce::Label, 4> names;
    std::array<std::unique_ptr<juce::FileChooser>, 4> choosers;
    std::array<juce::Slider, 4> controls;
    std::array<juce::Label, 4> captions;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> attachments;
    juce::Label status;

    void openFile(int slot);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibertyNeuralMatrixAudioProcessorEditor)
};
