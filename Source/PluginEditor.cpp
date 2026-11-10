#include "TextUtf8.h"
#include "PluginEditor.h"
#include "ArtAssets.h"
#include "AmpArtwork.h"
#include "CabArtwork.h"
#include "LibraryBrowser.h"
#include <algorithm>
#include <cmath>

namespace
{
const auto bg = juce::Colour::fromRGB(13, 12, 12);
const auto panel = juce::Colour::fromRGB(32, 29, 27);
const auto panelEdge = juce::Colour::fromRGB(91, 69, 49);
const auto bronze = juce::Colour::fromRGB(233, 154, 65);
const auto cream = juce::Colour::fromRGB(245, 232, 214);
const auto secondary = juce::Colour::fromRGB(185, 176, 162);
const auto black = juce::Colour::fromRGB(17, 17, 17);

void card(juce::Graphics& g, const juce::Rectangle<int>& r, const juce::String& title)
{
    const auto f = r.toFloat();
    g.setGradientFill(juce::ColourGradient(panel.brighter(0.1f), f.getTopLeft(), bg, f.getBottomRight(), false));
    g.fillRoundedRectangle(f, 12.0f);
    g.setColour(panelEdge);
    g.drawRoundedRectangle(f.reduced(0.5f), 11.5f, 1.0f);
    const auto top = f.withHeight(46.0f);
    g.setColour(black);
    g.fillRoundedRectangle(top.reduced(6.0f, 5.0f), 8.0f);
    g.setColour(bronze);
    g.setFont(juce::Font(juce::FontOptions(17.0f, juce::Font::bold)));
    g.drawFittedText(title, r.withHeight(42).reduced(23, 4), juce::Justification::centredLeft, 1);
}


// Distinct amp portraits are painted by AmpArtwork.h using a detected visual
// family. Hardware recognition remains probabilistic, never official.
void panelLine(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(panelEdge.withAlpha(0.8f));
    g.drawRoundedRectangle(area.toFloat().reduced(0.8f), 8.0f, 1.1f);
}

void caption(juce::Graphics& g, juce::String message, juce::Rectangle<int> r,
             int size = 13, bool bright = false)
{
    g.setColour(bright ? cream : secondary);
    g.setFont(juce::Font(juce::FontOptions((float) size)));
    g.drawFittedText(message, r, juce::Justification::centredLeft, 1);
}

template <size_t N>
juce::Image loadEmbeddedImage(const std::array<const char*, N>& segments)
{
    // Deliberately concatenate at runtime. MSVC cannot compile a single 39–45kB literal.
    juce::String encoded;
    for (const auto* segment : segments)
        encoded += segment;

    juce::MemoryOutputStream decoded;
    if (!juce::Base64::convertFromBase64(decoded, juce::StringRef(encoded))) return {};
    return juce::ImageFileFormat::loadFrom(decoded.getData(), decoded.getDataSize());
}
}

class LibertyNeuralMatrixAudioProcessorEditor::IRWaveform final : public juce::Component
{
public:
    explicit IRWaveform(LibertyNeuralMatrixAudioProcessor& p) : processor(p) {}
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().reduced(6);
        g.setColour(black); g.fillRoundedRectangle(area.toFloat(), 8.0f);
        g.setColour(panelEdge); g.drawRoundedRectangle(area.toFloat(), 8.0f, 1.0f);
        const auto waveArea = area.reduced(12, 18);
        const auto a = processor.irWaveform(0);
        const auto b = processor.irWaveform(1);
        const bool hasA = std::any_of(a.begin(), a.end(), [](float x) { return x > 0.00001f; });
        const bool hasB = std::any_of(b.begin(), b.end(), [](float x) { return x > 0.00001f; });
        if (!hasA && !hasB)
        {
            caption(g, libertyText::u8("Chargez une IR pour afficher sa forme d'onde réelle."), waveArea, 13);
            return;
        }
        const float height = static_cast<float>(waveArea.getHeight()) * 0.44f;
        const float centreY = static_cast<float>(waveArea.getCentreY());
        for (int track = 0; track < 2; ++track)
        {
            if (track == 0 && !hasA) continue;
            if (track == 1 && !hasB) continue;
            const auto& peaks = track == 0 ? a : b;
            const float peak = juce::jmax(0.01f, *std::max_element(peaks.begin(), peaks.end()));
            g.setColour(track == 0 ? bronze : juce::Colour::fromRGB(108, 194, 207));
            for (int i = 0; i < 128; ++i)
            {
                const auto x = waveArea.getX() + waveArea.getWidth() * (static_cast<float>(i) + 0.5f) / 128.0f;
                const auto amp = (peaks[(size_t) i] / peak) * height;
                g.drawLine(x, centreY - amp, x, centreY + amp, 1.25f);
            }
        }
        caption(g, "IR A", {area.getX() + 10, area.getY() + 2, 48, 16}, 11, hasA);
        caption(g, "IR B", {area.getX() + 66, area.getY() + 2, 48, 16}, 11, hasB);
    }
private:
    LibertyNeuralMatrixAudioProcessor& processor;
};

class LibertyNeuralMatrixAudioProcessorEditor::MicPositionCanvas final : public juce::Component
{
public:
    MicPositionCanvas(LibertyNeuralMatrixAudioProcessor& p, int idx) : processor(p), index(idx) {}
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().reduced(3);
        g.setColour(black); g.fillRoundedRectangle(r.toFloat(), 9.0f);
        g.setColour(panelEdge); g.drawRoundedRectangle(r.toFloat(), 9.0f, 1.0f);
        caption(g, index == 0 ? "MICRO A : POSITION" : "MICRO B : POSITION", r.removeFromTop(19).reduced(9, 0), 11, true);
        auto plot = r.reduced(15, 6);
        const float xval = processor.state.getRawParameterValue(index == 0 ? "micAOffset" : "micBOffset")->load();
        const float dval = processor.state.getRawParameterValue(index == 0 ? "micADistance" : "micBDistance")->load();
        g.setColour(juce::Colour::fromRGB(65, 60, 54));
        g.fillEllipse(juce::Rectangle<float>(50, 50).withCentre(plot.getCentre().toFloat()));
        g.setColour(panelEdge);
        g.drawEllipse(juce::Rectangle<float>(50, 50).withCentre(plot.getCentre().toFloat()), 1.0f);
        g.drawVerticalLine(plot.getCentreX(), (float) plot.getY(), (float) plot.getBottom());
        g.drawHorizontalLine(plot.getCentreY(), (float) plot.getX(), (float) plot.getRight());
        const float px = plot.getCentreX() + (xval * 0.0041f) * plot.getWidth();
        const float py = plot.getY() + 8.0f + (dval / 80.0f) * (plot.getHeight() - 16.0f);
        const auto micColour = index == 0 ? bronze : juce::Colour::fromRGB(115, 199, 218);
        g.setColour(micColour.withAlpha(0.28f));
        g.drawLine((float)plot.getRight()-6.0f,(float)plot.getBottom()-5.0f,px,py,2.0f);
        g.setColour(micColour);
        g.fillRoundedRectangle(px - 6.5f, py - 9.0f, 13.0f, 18.0f, 4.0f);
        g.setColour(cream);
        g.drawRoundedRectangle(px - 6.5f, py - 9.0f, 13.0f, 18.0f, 3.0f, 1.0f);
        g.drawLine(px - 4, py - 3, px + 4, py - 3, 1.0f);
        g.drawLine(px - 4, py + 1, px + 4, py + 1, 1.0f);
        caption(g, "CENTRE", {plot.getX(),plot.getY(),60,15}, 10);
        caption(g, "BORD", {plot.getRight()-45,plot.getY(),45,15}, 10);
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        const char* xKey = index == 0 ? "micAOffset" : "micBOffset";
        const char* yKey = index == 0 ? "micADistance" : "micBDistance";
        for (const auto* id : {xKey, yKey})
            if (auto* p = processor.state.getParameter(id)) p->beginChangeGesture();
        applyPosition(e.position);
    }
    void mouseDrag(const juce::MouseEvent& e) override { applyPosition(e.position); }
    void mouseUp(const juce::MouseEvent&) override
    {
        const char* xKey = index == 0 ? "micAOffset" : "micBOffset";
        const char* yKey = index == 0 ? "micADistance" : "micBDistance";
        for (const auto* id : {xKey, yKey})
            if (auto* p = processor.state.getParameter(id)) p->endChangeGesture();
    }
private:
    void applyPosition(juce::Point<float> point)
    {
        auto plot = getLocalBounds().reduced(3);
        plot.removeFromTop(19);
        plot = plot.reduced(15, 6);
        const float x = 100.0f * juce::jlimit(0.0f, 1.0f,
            (point.x - plot.getCentreX()) / (0.41f * (float) juce::jmax(1, plot.getWidth())));
        const float distance = 80.0f * juce::jlimit(0.0f, 1.0f,
            (point.y - plot.getY() - 8.0f) / (float) juce::jmax(1, plot.getHeight() - 16));
        auto set = [&](const char* id, float value)
        {
            if (auto* p = processor.state.getParameter(id))
                p->setValueNotifyingHost(p->convertTo0to1(value));
        };
        set(index == 0 ? "micAOffset" : "micBOffset", x);
        set(index == 0 ? "micADistance" : "micBDistance", distance);
        repaint();
    }
    LibertyNeuralMatrixAudioProcessor& processor;
    int index;
};

LibertyNeuralMatrixAudioProcessorEditor::HardwareLook::HardwareLook()
{
    setColour(juce::TextButton::textColourOffId, cream);
    setColour(juce::TextButton::textColourOnId, cream);
    setColour(juce::Slider::textBoxBackgroundColourId, black);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxTextColourId, cream);
    setColour(juce::ComboBox::textColourId, cream);
    setColour(juce::ComboBox::backgroundColourId, black);
    setColour(juce::ComboBox::outlineColourId, panelEdge);
    setColour(juce::ComboBox::arrowColourId, bronze);
    setColour(juce::PopupMenu::backgroundColourId, panel);
    setColour(juce::PopupMenu::textColourId, cream);
}

void LibertyNeuralMatrixAudioProcessorEditor::HardwareLook::drawButtonBackground(
    juce::Graphics& g, juce::Button& button, const juce::Colour&, bool hover, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.0f);
    auto c = button.getToggleState() ? juce::Colour::fromRGB(105, 65, 30) : panel;
    if (hover) c = c.brighter(0.18f);
    if (down) c = c.brighter(0.10f);
    g.setGradientFill(juce::ColourGradient(c.brighter(0.1f), r.getTopLeft(), c.darker(0.14f), r.getBottomLeft(), false));
    g.fillRoundedRectangle(r, 7.0f);
    g.setColour(button.getToggleState() ? bronze : panelEdge);
    g.drawRoundedRectangle(r, 7.0f, 1.0f);
}

void LibertyNeuralMatrixAudioProcessorEditor::HardwareLook::drawRotarySlider(
    juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    const auto diameter = juce::jmax(24.0f, juce::jmin((float) w - 34.0f, (float) h - 38.0f));
    const auto cx = x + w * 0.5f, cy = y + (h - 15) * 0.5f;
    const auto knob = juce::Rectangle<float>(diameter, diameter).withCentre({cx, cy});
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(140, 134, 123), knob.getTopLeft(),
                                           juce::Colour::fromRGB(42, 39, 37), knob.getBottomRight(), false));
    g.fillEllipse(knob);
    g.setColour(panelEdge); g.drawEllipse(knob, 1.5f);
    const auto radius = diameter * 0.5f + 3.0f;
    juce::Path whole, active;
    whole.addCentredArc(cx, cy, radius, radius, 0, start, end, true);
    active.addCentredArc(cx, cy, radius, radius, 0, start, start + (end - start) * pos, true);
    g.setColour(panelEdge); g.strokePath(whole, juce::PathStrokeType(3.0f));
    g.setColour(bronze); g.strokePath(active, juce::PathStrokeType(3.2f));
    const auto angle = start + (end - start) * pos;
    g.setColour(cream);
    g.drawLine(cx, cy, cx + std::sin(angle) * diameter * 0.34f,
               cy - std::cos(angle) * diameter * 0.34f, 2.4f);
}

LibertyNeuralMatrixAudioProcessorEditor::LibertyNeuralMatrixAudioProcessorEditor(
    LibertyNeuralMatrixAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&hardware);
    ampImage = loadEmbeddedImage(libertyArt::ampJpegChunks);
    cabImage = loadEmbeddedImage(libertyArt::cabJpegChunks);

    const std::array<juce::String,5> tabs {{ "AMPLI", "BAFFLES / IR", "EFFETS", libertyText::u8("RÉGLAGES"), libertyText::u8("BIBLIOTHÈQUE") }};
    const std::array<juce::String,4> loadNames {{ "CHARGER NAM A", "CHARGER NAM B", "CHARGER IR A", "CHARGER IR B" }};
    for (int i = 0; i < 5; ++i)
    {
        nav[(size_t)i].setButtonText(tabs[(size_t)i]);
        nav[(size_t)i].onClick = [this,i] { showPage(static_cast<Page>(i)); };
        addAndMakeVisible(nav[(size_t)i]);
    }
    for (int i = 0; i < 4; ++i)
    {
        load[(size_t)i].setButtonText(loadNames[(size_t)i]);
        load[(size_t)i].onClick = [this,i] { loadFile(i); };
        addAndMakeVisible(load[(size_t)i]);
        filename[(size_t)i].setColour(juce::Label::textColourId, cream);
        filename[(size_t)i].setFont(juce::Font(juce::FontOptions(13.0f)));
        filename[(size_t)i].setMinimumHorizontalScale(0.6f);
        filename[(size_t)i].setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(filename[(size_t)i]);
    }
    // An independent, per-NAM visual override and optional user-owned photograph.
    // The selector only changes artwork and never modifies the audio chain.
    for (int i = 0; i < 2; ++i)
    {
        auto& choice = visualChoices[(size_t)i];
        choice.addItem("AUTO", 1);
        for (int family = 0; family < libertyAmp::familyCount; ++family)
            choice.addItem(libertyText::u8(libertyAmp::families[(size_t)family].name), family + 2);
        choice.setSelectedId(1, juce::dontSendNotification);
        choice.onChange = [this, i]
        {
            const int family = visualChoices[(size_t)i].getSelectedId() - 2;
            const auto result = processor.setAmpVisualFamily(i, family);
            if (result.failed()) showFeedback(result.getErrorMessage());
            else showFeedback(libertyText::u8("Profil visuel mémorisé pour ce NAM."));
            refreshFromProcessor();
            repaint();
        };
        addAndMakeVisible(choice);

        auto& info = visualStatus[(size_t)i];
        info.setColour(juce::Label::textColourId, secondary);
        info.setJustificationType(juce::Justification::centredLeft);
        info.setFont(juce::Font(juce::FontOptions(11.4f)));
        info.setMinimumHorizontalScale(0.72f);
        addAndMakeVisible(info);

        auto& photo = photoButtons[(size_t)i];
        photo.setButtonText("PHOTO...");
        photo.onClick = [this,i] { selectAmpPhoto(i); };
        addAndMakeVisible(photo);

        auto& reset = resetPhotoButtons[(size_t)i];
        reset.setButtonText("EFFACER");
        reset.onClick = [this,i]
        {
            const auto result = processor.setAmpVisualPhoto(i, juce::File());
            showFeedback(result.failed() ? result.getErrorMessage()
                                         : libertyText::u8("Photo retirée : retour au profil visuel."));
            refreshFromProcessor();
            repaint();
        };
        addAndMakeVisible(reset);
    }

    auto createSliders = [this](auto& sliders, auto& captions, auto& attach,
                                const std::array<const char*,4>& ids,
                                const std::array<juce::String,4>& names)
    {
        for (int i = 0; i < 4; ++i)
        {
            auto& s = sliders[(size_t)i];
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 81, 22);
            addAndMakeVisible(s);
            auto& c = captions[(size_t)i];
            c.setText(names[(size_t)i], juce::dontSendNotification);
            c.setColour(juce::Label::textColourId, cream);
            c.setJustificationType(juce::Justification::centred);
            c.setFont(juce::Font(juce::FontOptions(12.7f, juce::Font::bold)));
            c.setMinimumHorizontalScale(0.75f);
            addAndMakeVisible(c);
            attach[(size_t)i] = std::make_unique<SliderAttachment>(processor.state, ids[(size_t)i], s);
        }
    };
    createSliders(masterSliders, masterCaptions, masterAttach,
                  {{"input","nammix","irmix","output"}},
                  {{libertyText::u8("ENTRÉE"),"MIX NAM A/B","MIX IR A/B","SORTIE"}});
    createSliders(ampSliders, ampCaptions, ampAttach,
                  {{"bass","middle","treble","presence"}},
                  {{"GRAVES",libertyText::u8("MÉDIUMS"),"AIGUS",libertyText::u8("PRÉSENCE")}});
    createSliders(cabSliders, cabCaptions, cabAttach,
                  {{"irAGain","irBGain","cabHP","cabLP"}},
                  {{"NIVEAU IR A","NIVEAU IR B","COUPE-BAS","COUPE-HAUT"}});
    createSliders(fxSliders, fxCaptions, fxAttach,
                  {{"gateThreshold","delayMs","delayFeedback","delayMix"}},
                  {{"SEUIL GATE","TEMPS DELAY",libertyText::u8("RÉINJECTION"),"MIX DELAY"}});
    createSliders(micSliders, micCaptions, micAttach,
                  {{"micAOffset","micADistance","micBOffset","micBDistance"}},
                  {{"MICRO A - AXE","MICRO A - CM","MICRO B - AXE","MICRO B - CM"}});

    const std::array<const char*,7> ids {{"ampOn","cabOn","irAPhase","irBPhase","gateOn","boostOn","delayOn"}};
    const std::array<juce::String,7> titles {{"AMPLI ACTIF","BAFFLE ACTIF","PHASE A","PHASE B","GATE ACTIF","BOOST +6 dB","DELAY ACTIF"}};
    for (int i = 0; i < 7; ++i)
    {
        switches[(size_t)i].setButtonText(titles[(size_t)i]);
        switches[(size_t)i].setClickingTogglesState(true);
        addAndMakeVisible(switches[(size_t)i]);
        switchAttach[(size_t)i] = std::make_unique<ButtonAttachment>(processor.state, ids[(size_t)i], switches[(size_t)i]);
    }
    const std::array<juce::String,3> modes {{"A","50 / 50","B"}};
    for (int i = 0; i < 3; ++i)
    {
        const auto val = i == 0 ? 0.0f : i == 1 ? 0.5f : 1.0f;
        ampSelect[(size_t)i].setButtonText(modes[(size_t)i]);
        ampSelect[(size_t)i].onClick = [this,val]{ selectBlend(true,val); };
        addAndMakeVisible(ampSelect[(size_t)i]);
        cabSelect[(size_t)i].setButtonText(modes[(size_t)i]);
        cabSelect[(size_t)i].onClick = [this,val]{ selectBlend(false,val); };
        addAndMakeVisible(cabSelect[(size_t)i]);
    }
    const juce::StringArray profiles {{ "Neutre", "Dynamique type 57", "Dynamique type 421",
                                       "Condensateur type 87", "Lampe type 47", "Ruban type 121" }};
    for (int i = 0; i < 2; ++i)
    {
        auto& box = micTypes[(size_t)i];
        box.addItemList(profiles, 1);
        addAndMakeVisible(box);
        micTypeAttach[(size_t)i] = std::make_unique<ComboAttachment>(
            processor.state, i == 0 ? "micAType" : "micBType", box);
        auto& label = micTypeLabels[(size_t)i];
        label.setText(i == 0 ? "TYPE MICRO A" : "TYPE MICRO B", juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, cream);
        label.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        addAndMakeVisible(label);
        micCanvases[(size_t)i] = std::make_unique<MicPositionCanvas>(processor, i);
        addAndMakeVisible(*micCanvases[(size_t)i]);
    }
    waveform = std::make_unique<IRWaveform>(processor);
    addAndMakeVisible(*waveform);
    libraryBrowser = std::make_unique<LibertyLibraryBrowser>();
    libraryBrowser->onLoad = [this](int slot, const juce::File& file)
    {
        const auto result = processor.loadFile(slot, file);
        if (result.failed()) showFeedback(libertyText::u8("Erreur de chargement : ") + result.getErrorMessage());
        else
        {
            showFeedback(libertyText::u8("Fichier chargé dans ") + (slot < 2 ? "NAM " : "IR ")
                         + (slot % 2 == 0 ? "A : " : "B : ") + file.getFileName());
            refreshFromProcessor();
            if (slot >= 2) waveform->repaint();
            repaint();
        }
    };
    addAndMakeVisible(*libraryBrowser);
    feedback.setColour(juce::Label::textColourId, secondary);
    feedback.setFont(juce::Font(juce::FontOptions(12.5f)));
    feedback.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(feedback);
    feedback.setText(libertyText::u8("Interface guitare. Profils micro = simulation après IR, non mesure acoustique."), juce::dontSendNotification);
    setSize(1560, 980);
    setResizable(true, true);
    setResizeLimits(1370, 930, 1860, 1200);
    showPage(Page::amp);
    startTimerHz(4);
}

LibertyNeuralMatrixAudioProcessorEditor::~LibertyNeuralMatrixAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void LibertyNeuralMatrixAudioProcessorEditor::showFeedback(const juce::String& s)
{
    feedback.setText(s, juce::dontSendNotification);
}

void LibertyNeuralMatrixAudioProcessorEditor::timerCallback()
{
    refreshFromProcessor();
    if (page == Page::cab)
    {
        waveform->repaint();
        for (auto& mic : micCanvases) mic->repaint();
    }
}

void LibertyNeuralMatrixAudioProcessorEditor::selectBlend(bool isNam, float blend)
{
    if (auto* param = processor.state.getParameter(isNam ? "nammix" : "irmix"))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost(blend);
        param->endChangeGesture();
    }
    refreshFromProcessor();
}

void LibertyNeuralMatrixAudioProcessorEditor::refreshFromProcessor()
{
    for (int i = 0; i < 4; ++i)
    {
        const auto name = processor.slotDisplayName(i);
        if (name != filename[(size_t)i].getText()) filename[(size_t)i].setText(name, juce::dontSendNotification);
        filename[(size_t)i].setTooltip(name);
        if (i < 2)
        {
            const auto visual = processor.ampVisualInfo(i);
            const auto family = visual.displayedFamily();
            const juce::String familyName = libertyText::u8(libertyAmp::families[(size_t)
                juce::jlimit(0, libertyAmp::familyCount-1, family)].name);
            const auto confidence = visual.detected.confidence == 3
                ? libertyText::u8("élevée") : visual.detected.confidence == 2
                ? libertyText::u8("moyenne") : visual.detected.confidence == 1
                ? libertyText::u8("faible") : libertyText::u8("inconnue");
            const auto sourceLabel = visual.detected.source == "metadata" ? libertyText::u8("métadonnées")
                : visual.detected.source == "filename" ? "nom"
                : visual.detected.source == "directory" ? "dossier" : "aucun indice";
            const juce::String info = !visual.loaded ? libertyText::u8("Aucun NAM chargé")
                : (visual.manualFamily >= 0 ? "MANUEL : " : "AUTO : ") + familyName;
            visualStatus[(size_t)i].setText(info, juce::dontSendNotification);
            visualStatus[(size_t)i].setTooltip(
                familyName + " / " + libertyText::u8("confiance ") + confidence
                + " / " + libertyText::u8("source : ") + sourceLabel);
            const int selected = visual.manualFamily < 0 ? 1 : visual.manualFamily + 2;
            if (visualChoices[(size_t)i].getSelectedId() != selected)
                visualChoices[(size_t)i].setSelectedId(selected, juce::dontSendNotification);
            if (ampPhotoPaths[(size_t)i] != visual.customPhotoPath)
            {
                ampPhotoPaths[(size_t)i] = visual.customPhotoPath;
                ampPhotos[(size_t)i] = visual.customPhotoPath.isNotEmpty()
                    ? juce::ImageFileFormat::loadFrom(juce::File(visual.customPhotoPath))
                    : juce::Image();
                repaint();
            }
        }
    }
    auto refresh = [this](auto& group, const char* paramId)
    {
        auto* value = processor.state.getRawParameterValue(paramId);
        if (value == nullptr) return;
        const float v = value->load();
        const int active = v < 0.03f ? 0 : v > 0.97f ? 2 : std::abs(v-0.5f) < 0.03f ? 1 : -1;
        for (int i = 0; i < 3; ++i)
            group[(size_t)i].setToggleState(i == active, juce::dontSendNotification);
    };
    refresh(ampSelect, "nammix");
    refresh(cabSelect, "irmix");
}

void LibertyNeuralMatrixAudioProcessorEditor::showPage(Page next)
{
    page = next;
    for (int i = 0; i < 5; ++i)
        nav[(size_t)i].setToggleState(static_cast<int>(page) == i, juce::dontSendNotification);
    libraryBrowser->setVisible(page == Page::library);
    for (int i = 0; i < 4; ++i)
    {
        // Existing audio controls are hidden while browsing the library.
        // Each page owns its own area; no child control overlaps the library.

        const bool visible = (page == Page::amp && i < 2) || (page == Page::cab && i >= 2);
        load[(size_t)i].setVisible(visible);
        filename[(size_t)i].setVisible(visible);
        ampSliders[(size_t)i].setVisible(page == Page::amp);
        ampCaptions[(size_t)i].setVisible(page == Page::amp);
        cabSliders[(size_t)i].setVisible(page == Page::cab);
        cabCaptions[(size_t)i].setVisible(page == Page::cab);
        fxSliders[(size_t)i].setVisible(page == Page::fx);
        fxCaptions[(size_t)i].setVisible(page == Page::fx);
        micSliders[(size_t)i].setVisible(page == Page::cab);
        micCaptions[(size_t)i].setVisible(page == Page::cab);
        masterSliders[(size_t)i].setVisible(page != Page::cab && page != Page::library);
        masterCaptions[(size_t)i].setVisible(page != Page::cab && page != Page::library);
    }
    for (int i = 0; i < 2; ++i)
    {
        visualChoices[(size_t)i].setVisible(page == Page::amp);
        visualStatus[(size_t)i].setVisible(page == Page::amp);
        photoButtons[(size_t)i].setVisible(page == Page::amp);
        resetPhotoButtons[(size_t)i].setVisible(page == Page::amp);
    }
    for (int i = 0; i < 3; ++i)
    {
        ampSelect[(size_t)i].setVisible(page == Page::amp);
        cabSelect[(size_t)i].setVisible(page == Page::cab);
    }
    for (int i = 0; i < 7; ++i)
    {
        const bool visible = (page == Page::amp && i == 0)
                          || (page == Page::cab && i >= 1 && i <= 3)
                          || (page == Page::fx && i >= 4)
                          || (page == Page::settings && i <= 1);
        switches[(size_t)i].setVisible(visible);
    }
    for (int i = 0; i < 2; ++i)
    {
        micTypes[(size_t)i].setVisible(page == Page::cab);
        micTypeLabels[(size_t)i].setVisible(page == Page::cab);
        micCanvases[(size_t)i]->setVisible(page == Page::cab);
    }
    waveform->setVisible(page == Page::cab);
    resized();
    repaint();
}

void LibertyNeuralMatrixAudioProcessorEditor::loadFile(int slot)
{
    juce::Component::SafePointer<LibertyNeuralMatrixAudioProcessorEditor> safe(this);
    chooser[(size_t)slot] = std::make_unique<juce::FileChooser>(
        slot < 2 ? "Choisir une capture NAM" : libertyText::u8("Choisir une réponse impulsionnelle WAV"),
        juce::File {}, slot < 2 ? "*.nam" : "*.wav");
    chooser[(size_t)slot]->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe, slot](const juce::FileChooser& selected)
        {
            if (safe == nullptr) return;
            const auto file = selected.getResult();
            if (!file.existsAsFile()) return;
            const auto result = safe->processor.loadFile(slot, file);
            if (result.wasOk())
            {
                safe->showFeedback(libertyText::u8("Chargement réussi : ") + file.getFileName());
                safe->refreshFromProcessor();
                if (slot >= 2) safe->waveform->repaint();
                safe->repaint(); // Refresh both independent model visuals.
            }
            else safe->showFeedback("Erreur : " + result.getErrorMessage());
        });
}

void LibertyNeuralMatrixAudioProcessorEditor::selectAmpPhoto(int slot)
{
    juce::Component::SafePointer<LibertyNeuralMatrixAudioProcessorEditor> safe(this);
    photoChoosers[(size_t)slot] = std::make_unique<juce::FileChooser>(
        libertyText::u8("Choisir une photo personnelle de cet ampli (JPG/PNG)"),
        juce::File(), "*.jpg;*.jpeg;*.png");
    photoChoosers[(size_t)slot]->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe,slot](const juce::FileChooser& selected)
        {
            if (safe == nullptr) return;
            const auto imageFile = selected.getResult();
            if (!imageFile.existsAsFile()) return;
            const auto result = safe->processor.setAmpVisualPhoto(slot, imageFile);
            safe->showFeedback(result.failed() ? result.getErrorMessage()
                : libertyText::u8("Photo associée au NAM et mémorisée pour les prochains chargements."));
            safe->refreshFromProcessor();
            safe->repaint();
        });
}

void LibertyNeuralMatrixAudioProcessorEditor::layoutRow(
    juce::Rectangle<int> zone, std::array<juce::Slider,4>& sliders, std::array<juce::Label,4>& labels)
{
    const int gap = 10;
    const int w = (zone.getWidth() - gap * 3) / 4;
    for (int i = 0; i < 4; ++i)
    {
        auto cell = juce::Rectangle<int>(zone.getX() + i * (w+gap), zone.getY(), w, zone.getHeight());
        labels[(size_t)i].setBounds(cell.removeFromTop(18));
        sliders[(size_t)i].setBounds(cell.reduced(2, 0));
    }
}

void LibertyNeuralMatrixAudioProcessorEditor::layoutGrid(
    juce::Rectangle<int> zone, std::array<juce::Slider,4>& sliders, std::array<juce::Label,4>& labels)
{
    const int gap = 8;
    const int w = (zone.getWidth() - gap) / 2;
    const int h = (zone.getHeight() - gap) / 2;
    for (int i = 0; i < 4; ++i)
    {
        auto cell = juce::Rectangle<int>(zone.getX()+(i % 2)*(w+gap),
                                         zone.getY()+(i / 2)*(h+gap), w, h);
        labels[(size_t)i].setBounds(cell.removeFromTop(19));
        sliders[(size_t)i].setBounds(cell.reduced(3, 0));
    }
}

void LibertyNeuralMatrixAudioProcessorEditor::resized()
{
    auto pageArea = getLocalBounds().reduced(16);
    pageArea.removeFromTop(76);
    auto navigation = pageArea.removeFromTop(48).reduced(8, 7);
    for (int i = 0; i < 5; ++i)
    {
        const int space = 8;
        const int width = (navigation.getWidth() - 4 * space) / 5;
        nav[(size_t)i].setBounds(navigation.removeFromLeft(width));
        if (i < 4) navigation.removeFromLeft(space);
    }
    auto status = pageArea.removeFromBottom(30);
    feedback.setBounds(status.reduced(13, 3));
    if (page == Page::library)
    {
        libraryBrowser->setBounds(pageArea.reduced(0, 8));
        return;
    }
    auto footer = pageArea.removeFromBottom(170);
    layoutRow(footer.reduced(30, 6).withTrimmedTop(46),
              page == Page::cab ? cabSliders : masterSliders,
              page == Page::cab ? cabCaptions : masterCaptions);
    auto content = pageArea.reduced(0, 8);
    const int gap = 16;
    const int column = (content.getWidth() - gap) / 2;
    auto left = content.removeFromLeft(column);
    content.removeFromLeft(gap);
    auto right = content;

    if (page == Page::amp)
    {
        // Reserve fixed rows at the bottom for file + detection + manual override.
        // At minimum window size all controls remain inside the left card.
        const int rowTop = left.getBottom() - 174;
        const int inset = 23;
        const int available = left.getWidth() - 2*inset;
        const int fileButtonWidth = juce::jmin(161, available / 4);
        const int infoWidth = available * 30 / 100;
        const int choiceWidth = available * 33 / 100;
        const int photoWidth = available * 20 / 100;
        const int clearWidth = available - infoWidth - choiceWidth - photoWidth - 12;
        for (int i = 0; i < 2; ++i)
        {
            const int y = rowTop + 8 + 78*i;
            load[(size_t)i].setBounds(left.getX()+inset, y,
                                      fileButtonWidth, 28);
            filename[(size_t)i].setBounds(left.getX()+inset+fileButtonWidth+10,
                                          y, available-fileButtonWidth-10, 28);
            const int under = y + 33;
            visualStatus[(size_t)i].setBounds(left.getX()+inset,under,infoWidth,27);
            visualChoices[(size_t)i].setBounds(left.getX()+inset+infoWidth+4,under,
                                               choiceWidth-4,27);
            photoButtons[(size_t)i].setBounds(left.getX()+inset+infoWidth+choiceWidth+4,
                                              under,photoWidth-4,27);
            resetPhotoButtons[(size_t)i].setBounds(left.getX()+inset+infoWidth+choiceWidth+photoWidth+4,
                                                   under,clearWidth-4,27);
        }
        layoutGrid(right.reduced(38, 115).withTrimmedBottom(35), ampSliders, ampCaptions);
        const auto modeWidth = (right.getWidth() - 108) / 3;
        for (int i = 0; i < 3; ++i)
            ampSelect[(size_t)i].setBounds(right.getX()+28+i*(modeWidth+14), right.getBottom()-90, modeWidth, 34);
        switches[0].setBounds(right.getX()+28, right.getBottom()-48, 166, 30);
    }
    else if (page == Page::cab)
    {
        const int top = left.getY();
        const int photoHeight = juce::jmax(188, left.getHeight() / 2 - 6);
        waveform->setBounds(left.getX()+19, top+photoHeight+59, left.getWidth()-38, 88);
        const int y = top + photoHeight + 156;
        const int micGap = 8;
        const int micWidth = (left.getWidth() - 44 - micGap) / 2;
        micCanvases[0]->setBounds(left.getX()+20, y, micWidth, left.getBottom()-y-16);
        micCanvases[1]->setBounds(left.getX()+20+micWidth+micGap, y, micWidth, left.getBottom()-y-16);

        for (int i = 0; i < 2; ++i)
        {
            const int rowY = right.getY()+70+i*62;
            load[(size_t)(2+i)].setBounds(right.getX()+24, rowY, 160, 34);
            filename[(size_t)(2+i)].setBounds(right.getX()+196, rowY-1, right.getWidth()-222, 36);
            micTypeLabels[(size_t)i].setBounds(right.getX()+26, right.getY()+205+i*54, 142, 20);
            micTypes[(size_t)i].setBounds(right.getX()+171, right.getY()+205+i*54, right.getWidth()-194, 29);
        }
        layoutGrid({right.getX()+24, right.getY()+332, right.getWidth()-48,
                    juce::jmax(180, right.getHeight()-351)}, micSliders, micCaptions);
        switches[1].setBounds(right.getX()+25,right.getY()+170,150,30);
        switches[2].setBounds(right.getX()+186,right.getY()+170,130,30);
        switches[3].setBounds(right.getX()+327,right.getY()+170,130,30);
    }
    else if (page == Page::fx)
    {
        switches[4].setBounds(left.getX()+28,left.getY()+94,180,32);
        switches[5].setBounds(left.getX()+225,left.getY()+94,180,32);
        auto gate = left.reduced(34, 138).withTrimmedTop(22);
        fxCaptions[0].setBounds(gate.removeFromTop(23));
        fxSliders[0].setBounds(gate.removeFromTop(170).reduced(110,0));
        switches[6].setBounds(right.getX()+28,right.getY()+94,180,32);
        const int x = right.getX()+30;
        const int y = right.getY()+172;
        const int w = (right.getWidth()-76)/3;
        for (int i=1;i<4;++i)
        {
            auto cell = juce::Rectangle<int>(x+(i-1)*(w+8),y,w,200);
            fxCaptions[(size_t)i].setBounds(cell.removeFromTop(22));
            fxSliders[(size_t)i].setBounds(cell.reduced(3,0));
        }
    }
    else
    {
        switches[0].setBounds(left.getX()+34,left.getY()+132,210,38);
        switches[1].setBounds(left.getX()+34,left.getY()+187,210,38);
    }
}

void LibertyNeuralMatrixAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    auto area = getLocalBounds().reduced(16);
    auto header = area.removeFromTop(76);
    auto navArea = area.removeFromTop(48);
    auto status = area.removeFromBottom(30);
    auto footer = area.removeFromBottom(170);
    auto main = area.reduced(0,8);
    const int gap = 16;
    const int width = (main.getWidth()-gap)/2;
    auto left = main.removeFromLeft(width);
    main.removeFromLeft(gap);
    auto right = main;

    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(74,46,25),header.getTopLeft().toFloat(),
        panel,header.getBottomRight().toFloat(),false));
    g.fillRoundedRectangle(header.toFloat(),12.0f);
    g.setColour(panelEdge);g.drawRoundedRectangle(header.toFloat(),12.0f,1.2f);
    g.setColour(cream);g.setFont(juce::Font(juce::FontOptions(28.0f,juce::Font::bold)));
    g.drawText("LIBERTY  /  NEURAL MATRIX",header.getX()+22,header.getY()+7,header.getWidth()-245,36,juce::Justification::centredLeft,false);
    caption(g,"GUITAR EDITION 0.8  -  NAM A1/A2  -  CABINET IR",{header.getX()+24,header.getY()+45,header.getWidth()-290,21},12);
    caption(g,"LIBERTY MUSIC SOFTWARE",{header.getRight()-236,header.getY()+22,217,31},13,true);
    g.setColour(black);g.fillRoundedRectangle(navArea.toFloat(),11.0f);
    if (page != Page::library)
        card(g,footer,page == Page::cab ? "IR - NIVEAUX ET FILTRES" : "MASTER - CONTROLES PRINCIPAUX");
    g.setColour(black);g.fillRoundedRectangle(status.toFloat(),8.0f);

    if(page==Page::library)
    {
        // The browser paints its own full-width dedicated surface.
        return;
    }
    if(page==Page::amp)
    {
        card(g,left,"AMPLIFICATEUR ET CAPTURES NAM");
        card(g,right,libertyText::u8("ÉGALISATION POST-NAM"));
        const int rowTop = left.getBottom() - 174;
        const int h = juce::jmax(180, rowTop - left.getY() - 64);
        const auto art = juce::Rectangle<int>(left.getX()+22,left.getY()+54,left.getWidth()-44,h);
        g.setColour(black);g.fillRoundedRectangle(art.toFloat(),9.0f);
        if(ampImage.isValid())
        {
            g.saveState();g.reduceClipRegion(art);
            g.setOpacity(0.28f);
            g.drawImageWithin(ampImage,art.getX(),art.getY(),art.getWidth(),art.getHeight(),juce::RectanglePlacement::centred);
            g.restoreState();
        }
        const auto headRegion = art.reduced(13, 6);
        const int headGap = 8;
        const int headHeight = (headRegion.getHeight() - headGap) / 2;
        for (int i = 0; i < 2; ++i)
        {
            const auto visual = processor.ampVisualInfo(i);
            const auto faceRect = juce::Rectangle<int>(headRegion.getX(),
                headRegion.getY() + i * (headHeight + headGap),
                headRegion.getWidth(), headHeight);
            libertyArtwork::paint(g, faceRect, visual.displayedFamily(),
                                  visual.loaded, ampPhotos[(size_t)i]);
            auto tag = faceRect.reduced(10, 4).removeFromTop(19);
            g.setColour(juce::Colours::black.withAlpha(0.76f));
            g.fillRoundedRectangle(tag.toFloat(), 5.0f);
            g.setColour(bronze);
            g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
            const auto familyName = libertyText::u8(libertyAmp::families[(size_t)
                juce::jlimit(0, libertyAmp::familyCount-1, visual.displayedFamily())].name);
            g.drawFittedText(juce::String(i == 0 ? "NAM A  /  " : "NAM B  /  ") + familyName,
                             tag.reduced(7,0),juce::Justification::centredLeft,1);
        }
        panelLine(g,{left.getX()+19,rowTop-2,left.getWidth()-38,170});
        caption(g,libertyText::u8("Mélange entre deux captures - commandes automatisables."),
                {right.getX()+29,right.getBottom()-116,right.getWidth()-58,21},13);
        caption(g,libertyText::u8("L'EQ agit après le modèle NAM, sans modifier ses poids."),
                {right.getX()+226,right.getBottom()-38,right.getWidth()-248,20},12);
    }
    else if(page==Page::cab)
    {
        card(g,left,"BAFFLE - VISUALISATION IR");
        card(g,right,"CAPTURES, MICROS ET PLACEMENT");
        const int h=juce::jmax(188,left.getHeight()/2-6);
        auto art=juce::Rectangle<int>(left.getX()+22,left.getY()+55,left.getWidth()-44,h-9);
        g.setColour(black);g.fillRoundedRectangle(art.toFloat(),9.0f);
        libertyCabinetArtwork::paint(g, art, processor.slotDisplayName(2), processor.slotDisplayName(3));
        caption(g,"FORMES D'ONDE WAV : IR A (ambre) / IR B (cyan)",
                {left.getX()+25,left.getY()+h+35,left.getWidth()-50,18},12,true);
        caption(g,libertyText::u8("IR WAV réelles - profils de micros simulés après la convolution."),
                {right.getX()+25,right.getY()+48,right.getWidth()-50,19},12);
        caption(g,libertyText::u8("Le déplacement modifie le timbre, pas la position acoustique enregistrée."),
                {right.getX()+25,right.getY()+315,right.getWidth()-50,17},11);
    }
    else if(page==Page::fx)
    {
        card(g,left,"GATE ET BOOST");card(g,right,"DELAY STUDIO");
        caption(g,"Seuil du gate et boost fixe de +6 dB avant le NAM.",
                {left.getX()+28,left.getY()+60,left.getWidth()-56,21},13);
        caption(g,libertyText::u8("Delay avec temps, réinjection et mélange après les IR."),
                {right.getX()+28,right.getY()+60,right.getWidth()-56,21},13);
    }
    else
    {
        card(g,left,"CONFIGURATION AUDIO");card(g,right,libertyText::u8("MOTEUR ET COMPATIBILITÉ"));
        caption(g,"Activez ou contournez les sections ampli et baffle.",
                {left.getX()+30,left.getY()+71,left.getWidth()-60,20},13);
        caption(g,libertyText::u8("Interface UTF-8 - accents français vérifiés."),
                {right.getX()+30,right.getY()+82,right.getWidth()-60,23},13,true);
        caption(g,"Moteur JUCE + NeuralAmpModelerCore. Les profils micros",
                {right.getX()+30,right.getY()+125,right.getWidth()-60,23},13);
        caption(g,libertyText::u8("sont des filtres après l'IR, pas des réponses mesurées."),
                {right.getX()+30,right.getY()+151,right.getWidth()-60,23},13);
        caption(g,libertyText::u8("Échantillonnage hôte : ") + juce::String(processor.getPreparedSampleRate(),0) + " Hz",
                {right.getX()+30,right.getY()+199,right.getWidth()-60,24},13,true);
        caption(g,libertyText::u8("TONE3000 et interpolation d'IR multi-positions : à développer."),
                {right.getX()+30,right.getY()+247,right.getWidth()-60,24},13);
    }
}
