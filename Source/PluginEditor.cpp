#include "PluginEditor.h"
#include <cmath>

namespace
{
const auto charcoal  = juce::Colour::fromRGB(14, 13, 13);
const auto leather   = juce::Colour::fromRGB(29, 27, 25);
const auto steel     = juce::Colour::fromRGB(76, 70, 62);
const auto edge      = juce::Colour::fromRGB(111, 91, 68);
const auto gold      = juce::Colour::fromRGB(241, 166, 81);
const auto goldLight = juce::Colour::fromRGB(255, 206, 131);
const auto text      = juce::Colour::fromRGB(237, 227, 211);
const auto muted     = juce::Colour::fromRGB(173, 162, 146);
const auto deep      = juce::Colour::fromRGB(20, 19, 19);

void plate(juce::Graphics& g, juce::Rectangle<int> r, juce::String title, juce::String subtitle)
{
    auto f = r.toFloat().reduced(1.0f);
    g.setGradientFill(juce::ColourGradient(leather.brighter(0.07f), f.getTopLeft(),
                                           charcoal, f.getBottomRight(), false));
    g.fillRoundedRectangle(f, 15.0f);
    g.setColour(edge.withAlpha(0.5f));
    g.drawRoundedRectangle(f, 15.0f, 1.2f);
    g.setColour(juce::Colours::black.withAlpha(0.40f));
    g.fillRoundedRectangle(f.reduced(8.0f).removeFromTop(48.0f), 8.0f);
    g.setColour(gold);
    g.setFont(juce::Font(juce::FontOptions(19.0f, juce::Font::bold)));
    g.drawText(title, r.getX()+24, r.getY()+15, r.getWidth()-48, 24, juce::Justification::centredLeft, false);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText(subtitle, r.getX()+22, r.getY()+58, r.getWidth()-44, 22, juce::Justification::centredLeft, false);
    for (int i = 0; i < 4; ++i)
    {
        int x = (i & 1) ? r.getRight()-13 : r.getX()+13;
        int y = (i & 2) ? r.getBottom()-13 : r.getY()+13;
        g.setColour(steel);
        g.fillEllipse((float)x-3.0f,(float)y-3.0f,6.0f,6.0f);
        g.setColour(charcoal);
        g.drawLine((float)x-2.0f,(float)y+2.0f,(float)x+2.0f,(float)y-2.0f,1.0f);
    }
}

void speaker(juce::Graphics& g, juce::Rectangle<int> r)
{
    auto bounds = r.toFloat();
    g.setColour(juce::Colour::fromRGB(45, 44, 43));
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(juce::Colours::black.withAlpha(0.24f));
    for (int y = r.getY()+5; y < r.getBottom()-5; y += 7)
        g.drawHorizontalLine(y, (float) r.getX()+5.0f, (float) r.getRight()-5.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight())*0.34f;
    auto centre = bounds.getCentre();
    auto circle = juce::Rectangle<float>(2*radius,2*radius).withCentre(centre);
    g.setColour(juce::Colour::fromRGB(21,20,20));
    g.fillEllipse(circle);
    g.setColour(edge.withAlpha(0.75f));
    g.drawEllipse(circle, 3.0f);
    g.setColour(steel.darker(0.25f));
    g.fillEllipse(circle.reduced(radius*0.24f));
    g.setColour(juce::Colour::fromRGB(13,13,13));
    g.fillEllipse(circle.reduced(radius*0.58f));
    g.setColour(gold.withAlpha(0.78f));
    g.drawEllipse(circle.reduced(radius*0.58f), 1.0f);
}

void sectionText(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& headline,
                 const juce::String& details)
{
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    g.drawText(headline, r.removeFromTop(26), juce::Justification::centredLeft, false);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawFittedText(details, r, juce::Justification::topLeft, 5);
}
}

LibertyNeuralMatrixAudioProcessorEditor::HardwareLook::HardwareLook()
{
    setColour(juce::TextButton::textColourOffId, text);
    setColour(juce::TextButton::textColourOnId, goldLight);
    setColour(juce::Slider::textBoxBackgroundColourId, deep);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxTextColourId, text);
}

void LibertyNeuralMatrixAudioProcessorEditor::HardwareLook::drawButtonBackground(
    juce::Graphics& g, juce::Button& button, const juce::Colour&, bool hover, bool down)
{
    auto area = button.getLocalBounds().toFloat().reduced(1.0f);
    auto active = button.getToggleState();
    auto colour = active ? juce::Colour::fromRGB(82, 56, 28) : juce::Colour::fromRGB(43, 40, 37);
    if (hover) colour = colour.brighter(0.12f);
    if (down) colour = colour.brighter(0.21f);
    g.setGradientFill(juce::ColourGradient(colour.brighter(0.09f), area.getTopLeft(),
                                           colour.darker(0.14f), area.getBottomLeft(), false));
    g.fillRoundedRectangle(area, 8.0f);
    g.setColour(active ? gold : edge.withAlpha(0.64f));
    g.drawRoundedRectangle(area, 8.0f, active ? 1.8f : 1.0f);
}

void LibertyNeuralMatrixAudioProcessorEditor::HardwareLook::drawRotarySlider(
    juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    const float diameter = juce::jmax(26.0f, juce::jmin((float) w - 28.0f, (float)h - 26.0f));
    const auto cx = x + w*0.5f, cy = y+h*0.5f;
    auto knob = juce::Rectangle<float>(diameter, diameter).withCentre(juce::Point<float>(cx,cy));
    g.setColour(juce::Colours::black.withAlpha(0.60f));
    g.fillEllipse(knob.translated(0.0f,3.0f));
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(103,99,92), knob.getTopLeft(),
                                           juce::Colour::fromRGB(37,36,35), knob.getBottomRight(), false));
    g.fillEllipse(knob);
    g.setColour(edge);
    g.drawEllipse(knob, 2.0f);
    g.setColour(charcoal.withAlpha(0.65f));
    g.drawEllipse(knob.reduced(6.0f), 1.2f);
    const float radius = diameter*0.5f + 5.0f;
    juce::Path whole, active;
    whole.addCentredArc(cx,cy,radius,radius,0.0f,start,end,true);
    active.addCentredArc(cx,cy,radius,radius,0.0f,start,start+(end-start)*pos,true);
    g.setColour(steel);
    g.strokePath(whole,juce::PathStrokeType(3.0f));
    g.setColour(gold);
    g.strokePath(active,juce::PathStrokeType(3.3f));
    const float angle = start+(end-start)*pos;
    g.setColour(goldLight);
    g.drawLine(cx,cy,cx+std::sin(angle)*(diameter*0.32f),cy-std::cos(angle)*(diameter*0.32f),3.0f);
}

LibertyNeuralMatrixAudioProcessorEditor::LibertyNeuralMatrixAudioProcessorEditor(
    LibertyNeuralMatrixAudioProcessor& p) : juce::AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&hardware);
    const std::array<const char*,4> titles{"AMPLI","BAFFLES","EFFETS","RÉGLAGES"};
    for(int i=0;i<4;++i)
    {
        auto& button=nav[(size_t)i];
        button.setButtonText(titles[(size_t)i]);
        button.onClick=[this,i]{showPage((Page)i);};
        addAndMakeVisible(button);
    }
    const std::array<const char*,4> loaders{"CHARGER NAM A","CHARGER NAM B","CHARGER IR A","CHARGER IR B"};
    for(int i=0;i<4;++i)
    {
        auto& b=load[(size_t)i];
        b.setButtonText(loaders[(size_t)i]);
        b.onClick=[this,i]{loadFile(i);};
        addAndMakeVisible(b);
        auto& f=filename[(size_t)i];
        f.setColour(juce::Label::textColourId,text);
        f.setJustificationType(juce::Justification::centredLeft);
        f.setFont(juce::Font(juce::FontOptions(13.0f)));
        addAndMakeVisible(f);
    }

    auto setupKnobs=[this](auto& sliders,auto& captions,auto& attachments,
                           const std::array<const char*,4>& ids,
                           const std::array<const char*,4>& names)
    {
        for(int i=0;i<4;++i)
        {
            auto& slider=sliders[(size_t)i];
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,82,23);
            slider.setVelocityModeParameters(0.9,1,0.0,false);
            addAndMakeVisible(slider);
            auto& label=captions[(size_t)i];
            label.setText(names[(size_t)i],juce::dontSendNotification);
            label.setColour(juce::Label::textColourId,text);
            label.setFont(juce::Font(juce::FontOptions(13.0f,juce::Font::bold)));
            label.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(label);
            attachments[(size_t)i]=std::make_unique<SliderAttachment>(processor.state,ids[(size_t)i],slider);
        }
    };
    setupKnobs(masterSliders,masterCaptions,masterAttachments,
               {{"input","nammix","irmix","output"}},
               {{"ENTRÉE dB","MIX NAM A/B","MIX IR A/B","SORTIE dB"}});
    setupKnobs(ampSliders,ampCaptions,ampAttachments,
               {{"bass","middle","treble","presence"}},
               {{"GRAVES dB","MÉDIUMS dB","AIGUS dB","PRÉSENCE dB"}});
    setupKnobs(cabSliders,cabCaptions,cabAttachments,
               {{"irAGain","irBGain","cabHP","cabLP"}},
               {{"NIVEAU IR A dB","NIVEAU IR B dB","COUPE-BAS Hz","COUPE-HAUT Hz"}});
    setupKnobs(fxSliders,fxCaptions,fxAttachments,
               {{"gateThreshold","delayMs","delayFeedback","delayMix"}},
               {{"SEUIL GATE dBFS","TEMPS ms","FEEDBACK","MIX DELAY"}});

    const std::array<const char*,7> switchesIds{"ampOn","cabOn","irAPhase","irBPhase","gateOn","boostOn","delayOn"};
    const std::array<const char*,7> switchesTitles{"AMPLI ACTIF","BAFFLE ACTIF","INVERSER IR A","INVERSER IR B","GATE ACTIF","BOOST +6 dB","DELAY ACTIF"};
    for(int i=0;i<7;++i)
    {
        auto& b=switches[(size_t)i];
        b.setButtonText(switchesTitles[(size_t)i]);
        b.setClickingTogglesState(true);
        addAndMakeVisible(b);
        switchAttachments[(size_t)i]=std::make_unique<ButtonAttachment>(processor.state,switchesIds[(size_t)i],b);
    }
    const std::array<const char*,3> selections{"A","50 / 50","B"};
    for(int i=0;i<3;++i)
    {
        const float value=i==0?0.0f:i==1?0.5f:1.0f;
        ampSelect[(size_t)i].setButtonText(selections[(size_t)i]);
        ampSelect[(size_t)i].onClick=[this,value]{selectBlend(true,value);};
        addAndMakeVisible(ampSelect[(size_t)i]);
        cabSelect[(size_t)i].setButtonText(selections[(size_t)i]);
        cabSelect[(size_t)i].onClick=[this,value]{selectBlend(false,value);};
        addAndMakeVisible(cabSelect[(size_t)i]);
    }
    feedback.setText("NAM A1/A2 + IR WAV · Édition temps réel · Contrôles automatisables",juce::dontSendNotification);
    feedback.setColour(juce::Label::textColourId,muted);
    feedback.setJustificationType(juce::Justification::centredLeft);
    feedback.setFont(juce::Font(juce::FontOptions(12.5f)));
    addAndMakeVisible(feedback);
    setSize(1400,840);
    setResizable(true,true);
    setResizeLimits(1180,820,1800,1080);
    showPage(Page::amp);
    startTimerHz(4);
}

LibertyNeuralMatrixAudioProcessorEditor::~LibertyNeuralMatrixAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void LibertyNeuralMatrixAudioProcessorEditor::timerCallback()
{
    refreshSelection();
}

void LibertyNeuralMatrixAudioProcessorEditor::selectBlend(bool amp,float blend)
{
    if(auto* param=processor.state.getParameter(amp?"nammix":"irmix"))
        param->setValueNotifyingHost(blend);
    refreshSelection();
}

void LibertyNeuralMatrixAudioProcessorEditor::refreshSelection()
{
    auto update=[this](auto& buttons,const char* id)
    {
        const float v=processor.state.getRawParameterValue(id)->load();
        const int selected=v<0.04f?0:v>0.96f?2:std::abs(v-0.5f)<0.04f?1:-1;
        for(int i=0;i<3;++i)
            buttons[(size_t)i].setToggleState(selected==i,juce::dontSendNotification);
    };
    update(ampSelect,"nammix");
    update(cabSelect,"irmix");
}

void LibertyNeuralMatrixAudioProcessorEditor::showPage(Page page)
{
    currentPage=page;
    for(int i=0;i<4;++i)
    {
        nav[(size_t)i].setToggleState(i==(int)page,juce::dontSendNotification);
        const bool visible=(page==Page::amp&&i<2)||(page==Page::cab&&i>=2);
        load[(size_t)i].setVisible(visible);
        filename[(size_t)i].setVisible(visible);
    }
    for(auto& button:ampSelect)button.setVisible(page==Page::amp);
    for(auto& button:cabSelect)button.setVisible(page==Page::cab);
    auto showKnobs=[&](auto& sliders,auto& captions,bool visible)
    {
        for(int i=0;i<4;++i)
        {
            sliders[(size_t)i].setVisible(visible);
            captions[(size_t)i].setVisible(visible);
        }
    };
    showKnobs(ampSliders,ampCaptions,page==Page::amp);
    showKnobs(cabSliders,cabCaptions,page==Page::cab);
    showKnobs(fxSliders,fxCaptions,page==Page::fx);
    for(int i=0;i<7;++i)
    {
        const bool visible=(page==Page::amp&&i==0)||(page==Page::cab&&i>=1&&i<=3)
                           ||(page==Page::fx&&i>=4)||(page==Page::settings&&i<=1);
        switches[(size_t)i].setVisible(visible);
    }
    resized();
    repaint();
}

void LibertyNeuralMatrixAudioProcessorEditor::showFeedback(const juce::String& message)
{
    feedback.setText(message,juce::dontSendNotification);
}

void LibertyNeuralMatrixAudioProcessorEditor::loadFile(int slot)
{
    juce::Component::SafePointer<LibertyNeuralMatrixAudioProcessorEditor> safe(this);
    chooser[(size_t)slot]=std::make_unique<juce::FileChooser>(
        slot<2?"Choisir une capture NAM":"Choisir une réponse impulsionnelle WAV",
        juce::File{},slot<2?"*.nam":"*.wav");
    chooser[(size_t)slot]->launchAsync(
        juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [safe,slot](const juce::FileChooser& selected)
        {
            if(safe==nullptr)return;
            const auto file=selected.getResult();
            if(!file.existsAsFile())return;
            const auto result=safe->processor.loadFile(slot,file);
            if(result.wasOk())
            {
                safe->filename[(size_t)slot].setText(file.getFileName(),juce::dontSendNotification);
                safe->showFeedback("Chargement réussi : "+file.getFileName());
            }
            else safe->showFeedback("Échec du chargement : "+result.getErrorMessage());
        });
}

void LibertyNeuralMatrixAudioProcessorEditor::layoutSliderGrid(
    juce::Rectangle<int> zone,std::array<juce::Slider,4>& sliders,std::array<juce::Label,4>& labels)
{
    const int gap=12;
    const int cellWidth=(zone.getWidth()-gap)/2;
    const int cellHeight=(zone.getHeight()-gap)/2;
    for(int i=0;i<4;++i)
    {
        auto cell=juce::Rectangle<int>(zone.getX()+(i%2)*(cellWidth+gap),
                                       zone.getY()+(i/2)*(cellHeight+gap),cellWidth,cellHeight);
        labels[(size_t)i].setBounds(cell.removeFromTop(23));
        sliders[(size_t)i].setBounds(cell.reduced(4,0));
    }
}

void LibertyNeuralMatrixAudioProcessorEditor::layoutMaster(juce::Rectangle<int> zone)
{
    const int gap=10;
    const int cellWidth=(zone.getWidth()-gap*3)/4;
    for(int i=0;i<4;++i)
    {
        auto cell=juce::Rectangle<int>(zone.getX()+i*(cellWidth+gap),zone.getY(),cellWidth,zone.getHeight());
        masterCaptions[(size_t)i].setBounds(cell.removeFromTop(22));
        masterSliders[(size_t)i].setBounds(cell.reduced(12,0));
    }
}

void LibertyNeuralMatrixAudioProcessorEditor::resized()
{
    auto inner=getLocalBounds().reduced(17);
    inner.removeFromTop(77);
    auto navArea=inner.removeFromTop(48).reduced(5,6);
    const int navGap=10;
    const int navW=(navArea.getWidth()-3*navGap)/4;
    for(int i=0;i<4;++i)
        nav[(size_t)i].setBounds(navArea.getX()+i*(navW+navGap),navArea.getY(),navW,navArea.getHeight());

    auto foot=inner.removeFromBottom(30);
    feedback.setBounds(foot.reduced(14,0));
    auto master=inner.removeFromBottom(164);
    layoutMaster(master.reduced(30,40).withTrimmedTop(8));
    auto main=inner.reduced(0,8);

    const int half=(main.getWidth()-16)/2;
    auto left=main.removeFromLeft(half);
    main.removeFromLeft(16);
    auto right=main;

    if(currentPage==Page::amp || currentPage==Page::cab)
    {
        const bool amp=currentPage==Page::amp;
        const int first=amp?0:2;
        auto cards=left.reduced(22,84);
        const int gap=16;
        const int cardH=(cards.getHeight()-gap)/2;
        for(int i=0;i<2;++i)
        {
            auto card=juce::Rectangle<int>(cards.getX(),cards.getY()+i*(cardH+gap),cards.getWidth(),cardH);
            const int inset=amp?18:130;
            load[(size_t)(first+i)].setBounds(card.getX()+inset,card.getY()+45,
                                                juce::jmax(80,card.getWidth()-inset-18),36);
            filename[(size_t)(first+i)].setBounds(card.getX()+inset,card.getY()+84,
                                                    juce::jmax(80,card.getWidth()-inset-18),25);
        }
        auto selectorArea=right.reduced(25,84);
        selectorArea.removeFromTop(12);
        auto buttons=selectorArea.removeFromTop(36);
        auto& controls=amp?ampSelect:cabSelect;
        const int width=(buttons.getWidth()-20)/3;
        for(int i=0;i<3;++i)
            controls[(size_t)i].setBounds(buttons.getX()+i*(width+10),buttons.getY(),width,36);
        selectorArea.removeFromTop(15);
        auto knobArea=selectorArea.removeFromTop(juce::jmax(170,selectorArea.getHeight()-65));
        if(amp)layoutSliderGrid(knobArea,ampSliders,ampCaptions);
        else layoutSliderGrid(knobArea,cabSliders,cabCaptions);
        if(amp)
            switches[0].setBounds(right.getX()+right.getWidth()-165,right.getBottom()-55,145,31);
        else
        {
            const int swY=right.getBottom()-57;
            switches[1].setBounds(right.getX()+18,swY,140,31);
            switches[2].setBounds(right.getX()+168,swY,140,31);
            switches[3].setBounds(right.getX()+318,swY,140,31);
        }
    }
    else if(currentPage==Page::fx)
    {
        auto gate=left.reduced(28,90);
        switches[4].setBounds(gate.removeFromTop(38).removeFromLeft(155));
        gate.removeFromTop(14);
        switches[5].setBounds(gate.removeFromTop(38).removeFromLeft(155));
        auto threshold=gate.removeFromTop(160);
        fxCaptions[0].setBounds(threshold.removeFromTop(24));
        fxSliders[0].setBounds(threshold.reduced(60,0));
        auto delay=right.reduced(25,90);
        switches[6].setBounds(delay.removeFromTop(38).removeFromLeft(160));
        delay.removeFromTop(16);
        auto middle=delay.reduced(0,0);
        const int w=(middle.getWidth()-16)/3;
        for(int i=1;i<4;++i)
        {
            auto cell=juce::Rectangle<int>(middle.getX()+(i-1)*(w+8),middle.getY(),w,juce::jmin(210,middle.getHeight()));
            fxCaptions[(size_t)i].setBounds(cell.removeFromTop(24));
            fxSliders[(size_t)i].setBounds(cell.reduced(4,0));
        }
    }
    else
    {
        switches[0].setBounds(left.getX()+36,left.getY()+125,200,39);
        switches[1].setBounds(left.getX()+36,left.getY()+182,200,39);
    }
}

void LibertyNeuralMatrixAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(charcoal);
    auto area=getLocalBounds().reduced(17);
    auto header=area.removeFromTop(77);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(70,48,30),
                                           header.getTopLeft().toFloat(),leather,
                                           header.getBottomRight().toFloat(),false));
    g.fillRoundedRectangle(header.toFloat(),15.0f);
    g.setColour(edge);
    g.drawRoundedRectangle(header.toFloat(),15.0f,1.2f);
    g.setColour(text);
    g.setFont(juce::Font(juce::FontOptions(28.0f,juce::Font::bold)));
    g.drawText("LIBERTY   /   NEURAL MATRIX",header.getX()+22,header.getY()+11,header.getWidth()-240,34,
               juce::Justification::centredLeft,false);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.drawText("CAPTURES NAM · CABINET IR · GUITAR TONE LAB",header.getX()+23,header.getY()+49,
               header.getWidth()-260,18,juce::Justification::centredLeft,false);
    g.setColour(goldLight);
    g.setFont(juce::Font(juce::FontOptions(14.0f,juce::Font::bold)));
    g.drawText("GUITAR EDITION 0.4",header.getRight()-209,header.getY()+24,190,26,
               juce::Justification::centredRight,false);

    area.removeFromTop(48);
    area.removeFromBottom(30);
    auto footer=area.removeFromBottom(164);
    plate(g,footer,"MASTER CONTROL","Entrée · fondu NAM · mélange IR · sortie");
    auto main=area.reduced(0,8);
    const int half=(main.getWidth()-16)/2;
    auto left=main.removeFromLeft(half);
    main.removeFromLeft(16);
    auto right=main;
    if(currentPage==Page::amp)
    {
        plate(g,left,"DUAL AMP HEADS","Deux captures NAM indépendantes · A1 et A2 selon les modèles disponibles");
        plate(g,right,"AMP TONE STACK","Égalisation post-NAM réelle : graves, médiums, aigus, présence");
        auto cards=left.reduced(22,84);
        const int gap=16;
        const int cardH=(cards.getHeight()-gap)/2;
        for(int i=0;i<2;++i)
        {
            auto card=juce::Rectangle<int>(cards.getX(),cards.getY()+i*(cardH+gap),cards.getWidth(),cardH);
            g.setGradientFill(juce::ColourGradient(steel,card.getTopLeft().toFloat(),leather,card.getBottomLeft().toFloat(),false));
            g.fillRoundedRectangle(card.toFloat(),9.0f);
            g.setColour(edge);
            g.drawRoundedRectangle(card.toFloat(),9.0f,1.0f);
            g.setColour(goldLight);
            g.setFont(juce::Font(juce::FontOptions(16.0f,juce::Font::bold)));
            g.drawText(i==0?"AMP A":"AMP B",card.getX()+18,card.getY()+10,160,26,juce::Justification::centredLeft,false);
            g.fillEllipse((float)card.getRight()-30.0f,(float)card.getY()+14.0f,9.0f,9.0f);
        }
        sectionText(g,right.reduced(25,84).removeFromBottom(59),"Sélecteur A/B",
                    "A ou B, ou mélange 50/50. Le gros bouton MIX NAM reste automatisable dans le DAW.");
    }
    else if(currentPage==Page::cab)
    {
        plate(g,left,"CABINET ROOM","Deux réponses impulsionnelles WAV réelles, en parallèle");
        plate(g,right,"CAB CONTROL","Niveaux, polarités indépendantes et filtrage coupe-bas/coupe-haut");
        auto cards=left.reduced(22,84);
        int gap=16,cardH=(cards.getHeight()-gap)/2;
        for(int i=0;i<2;++i)
        {
            auto card=juce::Rectangle<int>(cards.getX(),cards.getY()+i*(cardH+gap),cards.getWidth(),cardH);
            speaker(g,{card.getX(),card.getY(),110,card.getHeight()});
            g.setColour(goldLight);
            g.setFont(juce::Font(juce::FontOptions(16.0f,juce::Font::bold)));
            g.drawText(i==0?"CAB A":"CAB B",card.getX()+128,card.getY()+12,160,26,
                       juce::Justification::centredLeft,false);
        }
    }
    else if(currentPage==Page::fx)
    {
        plate(g,left,"GATE & BOOST","Deux traitements fonctionnels avant le moteur NAM");
        plate(g,right,"STUDIO DELAY","Delay à feedback, traité après le baffle");
        sectionText(g,left.reduced(32,0).removeFromBottom(76),"Gate et boost pré-ampli",
                    "Seuil réglable · Boost fixe +6 dB. Le seuil n’agit que lorsque le gate est actif.");
        sectionText(g,right.reduced(32,0).removeFromBottom(76),"Delay stéréo",
                    "Temps, réinjection et proportion de signal traité. Activation indépendante.");
    }
    else
    {
        plate(g,left,"SIGNAL PATH","Active ou contourne les sections principales du signal");
        plate(g,right,"INFORMATIONS AUDIO","Indicateurs du moteur et limites connues de cette bêta");
        sectionText(g,left.reduced(34,0).withTrimmedTop(252),"Chaîne de signal",
                    "Entrée → Gate / Boost → NAM A/B → Tone Stack → IR A/B → Filtres CAB → Delay → Sortie.");
        sectionText(g,right.reduced(34,82),"Moteur audio JUCE + NeuralAmpModelerCore",
                    "Fréquence du moteur : "+juce::String(processor.getPreparedSampleRate(),0)+" Hz. "
                    "NAM mono traité en entrée commune. Chargement des modèles non temps réel. "
                    "TONE3000, morphing spectral et alignement IR automatique non encore intégrés.");
    }
}