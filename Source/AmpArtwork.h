#pragma once
#include <JuceHeader.h>
#include "AmpVisualDetector.h"
#include <cmath>

// Original Liberty vector artwork. Inspired by broad hardware design eras;
// not photographs of specific commercial amplifiers and not official skins.
namespace libertyArtwork
{
inline void paint(juce::Graphics& g, juce::Rectangle<int> bounds, int family,
                  bool loaded, const juce::Image& customPhoto)
{
    if (bounds.getWidth() < 100 || bounds.getHeight() < 48) return;
    family = juce::jlimit(0, libertyAmp::familyCount - 1, family);
    g.saveState();
    g.reduceClipRegion(bounds);
    auto outer = bounds.toFloat().reduced(4.0f, 4.0f);
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.fillRoundedRectangle(outer.translated(0, 4), 10.0f);

    if (customPhoto.isValid())
    {
        juce::ColourGradient framing(juce::Colour::fromRGB(70, 55, 41), outer.getTopLeft(),
                                     juce::Colour::fromRGB(18, 18, 18), outer.getBottomLeft(), false);
        g.setGradientFill(framing);
        g.fillRoundedRectangle(outer, 9.0f);
        const auto imageArea = outer.reduced(8).toNearestInt();
        g.saveState();
        g.reduceClipRegion(imageArea);
        g.drawImageWithin(customPhoto, imageArea.getX(), imageArea.getY(),
                          imageArea.getWidth(), imageArea.getHeight(),
                          juce::RectanglePlacement::centred);
        g.restoreState();
        g.setColour(juce::Colour::fromRGB(237, 178, 96).withAlpha(0.85f));
        g.drawRoundedRectangle(outer, 9.0f, 1.5f);
        g.restoreState();
        return;
    }

    // Per-family chassis/fascia/grille palettes and physically distinct layouts.
    const std::array<juce::Colour, 19> chassis {{
        juce::Colour(0xff262424), juce::Colour(0xff25201b), juce::Colour(0xff191817), juce::Colour(0xff222629), juce::Colour(0xff1c2127),
        juce::Colour(0xff272729), juce::Colour(0xffb89a62), juce::Colour(0xff362b2c), juce::Colour(0xff26374b), juce::Colour(0xff292725),
        juce::Colour(0xffb74b18), juce::Colour(0xff202124), juce::Colour(0xff4b3527), juce::Colour(0xff252b33), juce::Colour(0xff1b222c),
        juce::Colour(0xff151a1e), juce::Colour(0xff282a2d), juce::Colour(0xff845b38), juce::Colour(0xff1c1d21)
    }};
    const std::array<juce::Colour, 19> faces {{
        juce::Colour(0xff66605b), juce::Colour(0xffcaa26e), juce::Colour(0xffa78b5d), juce::Colour(0xff777f89), juce::Colour(0xffb1b7be),
        juce::Colour(0xff302e2e), juce::Colour(0xffe1bd7f), juce::Colour(0xffbcb0a0), juce::Colour(0xffc0c4ce), juce::Colour(0xffb5a078),
        juce::Colour(0xffead2a0), juce::Colour(0xffebebdf), juce::Colour(0xffd1b38a), juce::Colour(0xffb2b9c1), juce::Colour(0xff90949c),
        juce::Colour(0xff393e41), juce::Colour(0xffb1b3b6), juce::Colour(0xffd2ac7b), juce::Colour(0xff9f9ca1)
    }};
    const auto shell = chassis[(size_t)family];
    const auto face = faces[(size_t)family];
    const bool combo = family == 5 || family == 6 || family == 7 || family == 16;
    const bool vented = family == 3 || family == 4 || family == 14 || family == 15 || family == 18;
    const bool wood = family == 17;
    const bool diamond = family == 7;
    const bool silverPlate = family == 4 || family == 11 || family == 13 || family == 14;

    g.setGradientFill(juce::ColourGradient(shell.brighter(0.40f), outer.getTopLeft(),
                                            shell.darker(0.45f), outer.getBottomRight(), false));
    g.fillRoundedRectangle(outer, 9.0f);
    g.setColour(juce::Colours::black.withAlpha(0.9f));
    g.drawRoundedRectangle(outer, 9.0f, 2.0f);

    // Leather/tweed/wood texture inside bezel, entirely deterministic.
    auto cloth = outer.reduced(6.0f);
    g.saveState();
    g.reduceClipRegion(cloth.toNearestInt());
    const float spacing = wood ? 10.0f : family == 6 ? 7.0f : 5.0f;
    g.setColour((wood ? juce::Colour(0xffe3b47c) : juce::Colours::white).withAlpha(wood ? 0.16f : 0.045f));
    for (float offset = -cloth.getHeight(); offset < cloth.getWidth(); offset += spacing)
        g.drawLine(cloth.getX() + offset, cloth.getBottom(),
                   cloth.getX() + offset + cloth.getHeight(), cloth.getY(), 0.65f);
    g.restoreState();

    for (float x : {outer.getX()+10.0f, outer.getRight()-10.0f})
        for (float y : {outer.getY()+10.0f, outer.getBottom()-10.0f})
        {
            g.setColour(juce::Colour(0xffa99c88));
            g.fillEllipse(x-2.5f,y-2.5f,5.0f,5.0f);
            g.setColour(juce::Colour(0xff3b332a));
            g.drawLine(x-1.4f,y+1.4f,x+1.4f,y-1.4f,0.8f);
        }

    const float w = outer.getWidth(), h = outer.getHeight();
    const auto front = outer.reduced(17, 11);
    juce::Rectangle<float> facePlate;
    juce::Rectangle<float> grille;
    if (combo)
    {
        facePlate = {front.getX(), front.getY(), front.getWidth(), h * 0.31f};
        grille = {front.getX(), facePlate.getBottom()+4, front.getWidth(),
                  juce::jmax(6.0f, front.getBottom()-facePlate.getBottom()-4)};
    }
    else
    {
        grille = {front.getX(), front.getY(), front.getWidth(), h * (vented ? 0.44f : 0.48f)};
        facePlate = {front.getX(), grille.getBottom()+3, front.getWidth(),
                     juce::jmax(10.0f, front.getBottom()-grille.getBottom()-3)};
    }

    g.setColour(juce::Colour(0xff090a0b));
    g.fillRoundedRectangle(grille, 4.0f);
    g.setColour((diamond ? juce::Colour(0xffbdac96) : juce::Colour(0xff77787a)).withAlpha(0.25f));
    g.saveState();
    g.reduceClipRegion(grille.toNearestInt());
    if (diamond)
    {
        for (float y=-grille.getWidth();y<grille.getHeight()+grille.getWidth();y+=10.0f)
        {
            g.drawLine(grille.getX(), grille.getY()+y,
                       grille.getRight(), grille.getY()+y+grille.getWidth()*0.22f,0.8f);
            g.drawLine(grille.getX(), grille.getBottom()-y,
                       grille.getRight(), grille.getBottom()-y-grille.getWidth()*0.22f,0.8f);
        }
    }
    else
    {
        for (float y = grille.getY()+4.0f; y<grille.getBottom(); y+=5.5f)
            g.drawHorizontalLine((int)y, grille.getX()+4, grille.getRight()-4);
        for (float x = grille.getX()+6; x<grille.getRight(); x+=8.0f)
            g.drawVerticalLine((int)x, grille.getY()+3, grille.getBottom()-3);
    }
    g.restoreState();

    if (combo)
    {
        const float radius = juce::jmin(grille.getHeight()*0.39f, grille.getWidth()*0.13f);
        for (int i=0;i<(family==16?2:1);++i)
        {
            const float cx = grille.getCentreX() + (family == 16 ? (i==0?-0.20f:0.20f)*grille.getWidth() : 0.0f);
            const auto speaker = juce::Rectangle<float>(radius*2, radius*2)
                                 .withCentre({cx,grille.getCentreY()});
            g.setColour(juce::Colours::black.withAlpha(0.47f));
            g.fillEllipse(speaker);
            g.setColour(juce::Colour(0xffa49a84).withAlpha(0.4f));
            g.drawEllipse(speaker,1.5f);
            g.setColour(juce::Colour(0xff202020));
            g.fillEllipse(speaker.reduced(radius*0.60f));
        }
    }
    else if (vented)
    {
        for (int i=0;i<11;++i)
        {
            const float x=grille.getX() + grille.getWidth()*(i+0.5f)/11.0f;
            g.setColour(juce::Colours::black.withAlpha(0.68f));
            g.fillRoundedRectangle(x-2,grille.getY()+6,4,grille.getHeight()-12,1.5f);
        }
    }
    else
    {
        // Visible warm tubes and iconic Liberty logo, without third-party branding.
        for(int i=0;i<4;++i)
        {
            const float x=grille.getX()+grille.getWidth()*(i+1.0f)/5.0f;
            auto bulb=juce::Rectangle<float>(7,grille.getHeight()*0.37f)
                      .withCentre({x,grille.getCentreY()+5});
            g.setColour(juce::Colour(0xffef8839).withAlpha(loaded ? 0.68f : 0.10f));
            g.fillRoundedRectangle(bulb,3.0f);
        }
    }

    g.setGradientFill(juce::ColourGradient(face.brighter(0.18f), facePlate.getTopLeft(),
                                            face.darker(0.26f), facePlate.getBottomLeft(), false));
    g.fillRoundedRectangle(facePlate, 3.0f);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.drawRoundedRectangle(facePlate, 3.0f, 1.0f);
    const bool darkFace = family == 5 || family == 15 || family == 16;
    const auto ink=darkFace?juce::Colour(0xffece4cd):juce::Colour(0xff1c1b1a);

    // Brand engraving and model-inspired fascia; names are visual families only.
    auto modelWidth = juce::jmin(120.0f, facePlate.getWidth()*0.25f);
    auto modelZone = facePlate.removeFromLeft(modelWidth);
    g.setColour(ink);
    g.setFont(juce::Font(juce::FontOptions(juce::jlimit(10.0f,17.0f,h*0.097f),juce::Font::bold)));
    g.drawFittedText("LIBERTY", modelZone.toNearestInt().reduced(5,2),
                     juce::Justification::centred,1);
    const int knobs = family == 1 ? 6 : family == 3 || family == 4 ? 9 : family == 13 ? 7 : 8;
    for(int i=0;i<knobs;++i)
    {
        const float x=facePlate.getX()+(i+0.5f)*facePlate.getWidth()/(float)knobs;
        const float y=facePlate.getCentreY()+1.5f;
        const float rad=juce::jlimit(3.4f,10.0f,facePlate.getHeight()*0.20f);
        const auto knob=juce::Rectangle<float>(rad*2,rad*2).withCentre({x,y});
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff757575),knob.getTopLeft(),
                                             juce::Colour(0xff0d0d0d),knob.getBottomRight(),false));
        g.fillEllipse(knob);
        g.setColour(juce::Colours::black);
        g.drawEllipse(knob,0.8f);
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.drawLine(x,y-1,x,y-rad+1,1.0f);
    }
    g.setColour(loaded ? juce::Colour(0xfff3a24a) : juce::Colour(0xff433a35));
    g.fillEllipse(outer.getX()+13.0f,outer.getCentreY()-3.0f,5.0f,5.0f);
    g.setColour(juce::Colours::black.withAlpha(0.50f));
    g.drawRoundedRectangle(outer, 9.0f, 1.0f);
    g.restoreState();
}
} // namespace libertyArtwork
