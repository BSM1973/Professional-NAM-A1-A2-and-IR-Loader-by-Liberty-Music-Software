#include "PluginEditor.h"
#include <cmath>

namespace {
const auto midnight = juce::Colour::fromRGB(10, 17, 29);
const auto panel    = juce::Colour::fromRGB(19, 31, 48);
const auto surface  = juce::Colour::fromRGB(26, 43, 61);
const auto accent   = juce::Colour::fromRGB(57, 183, 216);
const auto light    = juce::Colour::fromRGB(232, 243, 249);
const auto subdued  = juce::Colour::fromRGB(150, 170, 190);
}

LibertyNeuralMatrixAudioProcessorEditor::Theme::Theme()
{
    setColour(juce::TextButton::textColourOffId, light);
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::Slider::textBoxTextColourId, light);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, surface);
}

void LibertyNeuralMatrixAudioProcessorEditor::Theme::drawButtonBackground(
    juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hover, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(down ? accent.withAlpha(0.30f) : hover ? surface.brighter(0.13f) : surface);
    g.fillRoundedRectangle(r, 10.0f);
    g.setColour(hover ? accent : accent.withAlpha(0.48f));
    g.drawRoundedRectangle(r, 10.0f, hover ? 1.8f : 1.0f);
}

void LibertyNeuralMatrixAudioProcessorEditor::Theme::drawRotarySlider(
    juce::Graphics& g, int x, int y, int w, int h, float position, float start, float end, juce::Slider&)
{
    const float diameter = (float)juce::jmin(w, h) - 21.0f;
    const float cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float rad = diameter * 0.5f;
    const auto bounds = juce::Rectangle<float>(cx - rad, cy - rad, diameter, diameter);
    g.setColour(surface.brighter(0.08f));
    g.fillEllipse(bounds);
    g.setColour(juce::Colour::fromRGB(63, 88, 107));
    g.drawEllipse(bounds, 1.4f);
    juce::Path arc;
    arc.addCentredArc(cx, cy, rad - 6.0f, rad - 6.0f, 0.0f, start,
                      start + (end - start) * position, true);
    g.setColour(accent);
    g.strokePath(arc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
    const float angle = start + (end - start) * position;
    const auto tip = juce::Point<float>(cx + std::sin(angle) * (rad - 15.0f),
                                        cy - std::cos(angle) * (rad - 15.0f));
    g.setColour(light);
    g.drawLine(cx, cy, tip.x, tip.y, 2.8f);
}

LibertyNeuralMatrixAudioProcessorEditor::LibertyNeuralMatrixAudioProcessorEditor(
    LibertyNeuralMatrixAudioProcessor& p) : juce::AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&theme);
    const std::array<juce::String, 4> buttonNames{
        "CHARGER NAM A", "CHARGER NAM B", "CHARGER IR A", "CHARGER IR B"};
    for (int i = 0; i < 4; ++i)
    {
        auto& b = loadButtons[(size_t)i];
        b.setButtonText(buttonNames[(size_t)i]);
        b.onClick = [this, i] { openFile(i); };
        addAndMakeVisible(b);
        auto& n = names[(size_t)i];
        n.setText(processor.slotDisplayName(i), juce::dontSendNotification);
        n.setColour(juce::Label::textColourId, subdued);
        n.setJustificationType(juce::Justification::centredLeft);
        n.setFont(juce::Font(juce::FontOptions(14.0f)));
        addAndMakeVisible(n);
    }

    const std::array<juce::String, 4> captionsFR{"ENTRÉE", "NAM A/B", "IR A/B", "SORTIE"};
    const std::array<juce::String, 4> ids{"input", "nammix", "irmix", "output"};
    for (int i = 0; i < 4; ++i)
    {
        auto& slider = controls[(size_t)i];
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 86, 28);
        addAndMakeVisible(slider);
        auto& c = captions[(size_t)i];
        c.setText(captionsFR[(size_t)i], juce::dontSendNotification);
        c.setJustificationType(juce::Justification::centred);
        c.setColour(juce::Label::textColourId, light);
        c.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
        addAndMakeVisible(c);
        attachments[(size_t)i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.state, ids[(size_t)i], slider);
    }

    status.setText("NAM A1/A2 · IR WAV · Prototype de développement v0.2", juce::dontSendNotification);
    status.setJustificationType(juce::Justification::centredLeft);
    status.setColour(juce::Label::textColourId, subdued);
    status.setFont(juce::Font(juce::FontOptions(13.0f)));
    addAndMakeVisible(status);
    setSize(1220, 720);
    setResizable(true, true);
    setResizeLimits(1050, 700, 1650, 960); // protects against overlap at minimum height
}

LibertyNeuralMatrixAudioProcessorEditor::~LibertyNeuralMatrixAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void LibertyNeuralMatrixAudioProcessorEditor::openFile(int slot)
{
    const auto safe = juce::Component::SafePointer<LibertyNeuralMatrixAudioProcessorEditor>(this);
    const auto pattern = slot < 2 ? "*.nam" : "*.wav";
    choosers[(size_t)slot] = std::make_unique<juce::FileChooser>(
        "Choisir un modèle NAM ou une réponse impulsionnelle", juce::File{}, pattern);
    choosers[(size_t)slot]->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe, slot](const juce::FileChooser& chooser)
        {
            if (!safe) return;
            const auto file = chooser.getResult();
            if (!file.existsAsFile()) return;
            const auto result = safe->processor.loadFile(slot, file);
            safe->status.setText(result.wasOk()
                ? "Chargé : " + file.getFileName()
                : result.getErrorMessage(), juce::dontSendNotification);
            if (result.wasOk())
                safe->names[(size_t)slot].setText(file.getFileName(), juce::dontSendNotification);
        });
}

void LibertyNeuralMatrixAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(midnight);
    g.setColour(juce::Colour::fromRGB(12, 22, 37));
    g.fillRect(0, 0, getWidth(), 88);
    g.setColour(accent);
    g.fillRect(0, 85, getWidth(), 3);
    g.setColour(light);
    g.setFont(juce::Font(juce::FontOptions(31.0f, juce::Font::bold)));
    g.drawText("LIBERTY  /  NEURAL MATRIX", 29, 18, getWidth() - 60, 45,
               juce::Justification::centredLeft);
    g.setColour(subdued);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("NEURAL AMP MODELER  ·  CABINET IMPULSE RESPONSE", 30, 61,
               getWidth() - 60, 19, juce::Justification::centredLeft);

    constexpr int gutter = 26, top = 110, sectionH = 317;
    const int colW = (getWidth() - gutter * 3) / 2;
    const juce::Rectangle<float> neural((float)gutter, (float)top, (float)colW, (float)sectionH);
    const juce::Rectangle<float> impulse((float)(gutter * 2 + colW), (float)top,
                                         (float)colW, (float)sectionH);
    for (const auto& rect : {neural, impulse})
    {
        g.setColour(panel);
        g.fillRoundedRectangle(rect, 14.0f);
        g.setColour(accent.withAlpha(0.20f));
        g.drawRoundedRectangle(rect, 14.0f, 1.0f);
    }
    g.setColour(light);
    g.setFont(juce::Font(juce::FontOptions(21.0f, juce::Font::bold)));
    g.drawText("NEURAL ENGINE", gutter + 22, top + 19, colW - 44, 30, juce::Justification::centredLeft);
    g.drawText("IMPULSE MATRIX", gutter * 2 + colW + 22, top + 19,
               colW - 44, 30, juce::Justification::centredLeft);
    g.setColour(subdued);
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText("Deux captures indépendantes · A1 / A2", gutter + 22, top + 53,
               colW - 44, 22, juce::Justification::centredLeft);
    g.drawText("Deux baffles WAV en parallèle", gutter * 2 + colW + 22, top + 53,
               colW - 44, 22, juce::Justification::centredLeft);

    for (int side = 0; side < 2; ++side)
        for (int row = 0; row < 2; ++row)
        {
            const int xx = gutter + side * (colW + gutter) + 17;
            const int yy = top + 93 + row * 103;
            g.setColour(surface.withAlpha(0.55f));
            g.fillRoundedRectangle((float)xx, (float)yy, (float)colW - 34, 87.0f, 10.0f);
        }

    constexpr int bottom = 453;
    const auto footer = juce::Rectangle<float>((float)gutter, (float)bottom,
                        (float)getWidth() - 2 * gutter, (float)getHeight() - bottom - 58);
    g.setColour(panel);
    g.fillRoundedRectangle(footer, 14.0f);
    g.setColour(accent.withAlpha(0.20f));
    g.drawRoundedRectangle(footer, 14.0f, 1.0f);
    g.setColour(light);
    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.drawText("MIXAGE & NIVEAUX", gutter + 22, bottom + 16, 330, 28,
               juce::Justification::centredLeft);
    g.setColour(subdued);
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText("A/B : 0 = A   ·   1 = B", getWidth() - 310, bottom + 17,
               260, 27, juce::Justification::centredRight);
}

void LibertyNeuralMatrixAudioProcessorEditor::resized()
{
    constexpr int gutter = 26, bottom = 453, knobW = 160;
    const int colW = (getWidth() - 3 * gutter) / 2;
    for (int side = 0; side < 2; ++side)
        for (int row = 0; row < 2; ++row)
        {
            const int idx = side * 2 + row;
            const int xx = gutter + side * (colW + gutter) + 30;
            const int yy = 214 + row * 103;
            loadButtons[(size_t)idx].setBounds(xx, yy, 183, 45);
            names[(size_t)idx].setBounds(xx + 197, yy + 3, colW - 249, 39);
        }
    const int available = getWidth() - 2 * gutter - 64;
    const int areaH = getHeight() - bottom - 58;
    for (int i = 0; i < 4; ++i)
    {
        const int centre = gutter + 32 + (available * (2 * i + 1)) / 8;
        captions[(size_t)i].setBounds(centre - knobW / 2, bottom + 51, knobW, 26);
        controls[(size_t)i].setBounds(centre - knobW / 2, bottom + 77, knobW,
                                     juce::jmax(110, areaH - 92));
    }
    status.setBounds(30, getHeight() - 46, getWidth() - 60, 34);
}
