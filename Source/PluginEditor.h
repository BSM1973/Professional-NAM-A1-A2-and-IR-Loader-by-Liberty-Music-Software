#pragma once
#include "PluginProcessor.h"
#include <array>

class LibertyNeuralMatrixAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                     private juce::Timer
{
public:
    explicit LibertyNeuralMatrixAudioProcessorEditor(LibertyNeuralMatrixAudioProcessor&);
    ~LibertyNeuralMatrixAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class HardwareLook final : public juce::LookAndFeel_V4
    {
    public:
        HardwareLook();
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    } hardware;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    LibertyNeuralMatrixAudioProcessor& processor;

    enum class Page { amp, cab, fx, settings };
    Page currentPage = Page::amp;
    std::array<juce::TextButton, 4> nav;
    std::array<juce::TextButton, 4> load;
    std::array<juce::Label, 4> filename;
    std::array<std::unique_ptr<juce::FileChooser>, 4> chooser;
    std::array<juce::TextButton, 3> ampSelect, cabSelect;

    std::array<juce::Slider, 4> masterSliders, ampSliders, cabSliders, fxSliders;
    std::array<juce::Label, 4> masterCaptions, ampCaptions, cabCaptions, fxCaptions;
    std::array<std::unique_ptr<SliderAttachment>, 4> masterAttachments, ampAttachments, cabAttachments, fxAttachments;
    std::array<juce::TextButton, 7> switches;
    std::array<std::unique_ptr<ButtonAttachment>, 7> switchAttachments;
    juce::Label feedback;

    void timerCallback() override;
    void loadFile(int slot);
    void showFeedback(const juce::String&);
    void selectBlend(bool amp, float blend);
    void showPage(Page);
    void refreshSelection();
    void layoutSliderGrid(juce::Rectangle<int>, std::array<juce::Slider,4>&, std::array<juce::Label,4>&);
    void layoutMaster(juce::Rectangle<int>);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibertyNeuralMatrixAudioProcessorEditor)
};