#pragma once

#include "PluginProcessor.h"
#include <array>
#include <memory>

class LibertyNeuralMatrixAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                      private juce::Timer
{
public:
    explicit LibertyNeuralMatrixAudioProcessorEditor(LibertyNeuralMatrixAudioProcessor&);
    ~LibertyNeuralMatrixAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    class HardwareLook final : public juce::LookAndFeel_V4
    {
    public:
        HardwareLook();
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    } hardware;

    class IRWaveform;
    class MicPositionCanvas;

    enum class Page { amp, cab, fx, settings };
    Page page = Page::amp;
    LibertyNeuralMatrixAudioProcessor& processor;

    std::array<juce::TextButton, 4> nav;
    std::array<juce::TextButton, 4> load;
    std::array<juce::Label, 4> filename;
    std::array<std::unique_ptr<juce::FileChooser>, 4> chooser;
    std::array<juce::TextButton, 3> ampSelect, cabSelect;

    std::array<juce::Slider, 4> masterSliders, ampSliders, cabSliders, fxSliders, micSliders;
    std::array<juce::Label, 4> masterCaptions, ampCaptions, cabCaptions, fxCaptions, micCaptions;
    std::array<std::unique_ptr<SliderAttachment>, 4> masterAttach, ampAttach, cabAttach, fxAttach, micAttach;
    std::array<juce::TextButton, 7> switches;
    std::array<std::unique_ptr<ButtonAttachment>, 7> switchAttach;

    std::array<juce::ComboBox, 2> micTypes;
    std::array<std::unique_ptr<ComboAttachment>, 2> micTypeAttach;
    std::array<juce::Label, 2> micTypeLabels;

    std::unique_ptr<IRWaveform> waveform;
    std::array<std::unique_ptr<MicPositionCanvas>, 2> micCanvases;
    juce::Image ampImage, cabImage;
    juce::Label feedback;

    void showPage(Page);
    void refreshFromProcessor();
    void showFeedback(const juce::String&);
    void loadFile(int);
    void selectBlend(bool isNam, float value);
    void timerCallback() override;
    void layoutRow(juce::Rectangle<int>, std::array<juce::Slider,4>&,
                   std::array<juce::Label,4>&);
    void layoutGrid(juce::Rectangle<int>, std::array<juce::Slider,4>&,
                    std::array<juce::Label,4>&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibertyNeuralMatrixAudioProcessorEditor)
};
