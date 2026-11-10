#pragma once
#include <JuceHeader.h>
#include <cmath>

// Original Liberty cabinet preview; silhouette follows textual hints only.
// It does not claim the WAV contains measured microphone positions.
namespace libertyCabinetArtwork
{
inline int speakerCount(const juce::String& filename)
{
    const auto s = filename.toLowerCase().replaceCharacter(' ', '_');
    if (s.contains("4x12") || s.contains("412") || s.contains("4_12")) return 4;
    if (s.contains("2x12") || s.contains("212") || s.contains("2_12")) return 2;
    if (s.contains("1x12") || s.contains("112") || s.contains("1_12")) return 1;
    return 2; // visual suggestion, not an assertion about the IR source
}

inline void drawCab(juce::Graphics& g, juce::Rectangle<float> r, int slot,
                    const juce::String& filename)
{
    const bool loaded = filename.isNotEmpty() && filename != "Charger un fichier";
    const auto accent = slot == 0 ? juce::Colour(0xffe9a352) : juce::Colour(0xff7dc5dd);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff424039),r.getTopLeft(),
                                           juce::Colour(0xff0b0a09),r.getBottomRight(), false));
    g.fillRoundedRectangle(r,8.0f);
    g.setColour(juce::Colour(0xff92785e));
    g.drawRoundedRectangle(r.reduced(1.5f),7.0f,2.0f);
    auto grille = r.reduced(14.0f,17.0f);
    grille.removeFromTop(18.0f);
    grille.removeFromBottom(9.0f);
    g.setColour(juce::Colour(0xff16181b));
    g.fillRoundedRectangle(grille,5.0f);
    const int count = loaded ? speakerCount(filename) : 2;
    const int columns = count == 1 ? 1 : 2;
    const int rows = count == 4 ? 2 : 1;
    const float diameter = juce::jmin(grille.getWidth() / (columns + 0.45f),
                                      grille.getHeight() / (rows + 0.42f));
    for (int y=0; y<rows; ++y)
        for (int x=0; x<columns; ++x)
        {
            const float cx = grille.getX() + grille.getWidth() * ((float)x + 0.5f)/(float)columns;
            const float cy = grille.getY() + grille.getHeight() * ((float)y + 0.5f)/(float)rows;
            const auto speaker = juce::Rectangle<float>(diameter,diameter).withCentre({cx,cy});
            g.setColour(juce::Colour(0xff353b3e));
            g.fillEllipse(speaker);
            g.setColour(juce::Colour(0xff778088).withAlpha(0.55f));
            g.drawEllipse(speaker,1.8f);
            g.setColour(juce::Colour(0xff101112));
            g.fillEllipse(speaker.reduced(diameter*0.19f));
            g.setColour(juce::Colour(0xff424547));
            g.fillEllipse(speaker.reduced(diameter*0.36f));
        }
    g.setColour(juce::Colour(0xffddddcd).withAlpha(0.14f));
    for (float y=grille.getY()+4.0f; y < grille.getBottom(); y += 4.0f)
        g.drawHorizontalLine((int)y,grille.getX()+3,grille.getRight()-3);
    g.setColour(accent);
    g.fillEllipse(r.getRight()-19,r.getY()+10,6,6);
    g.setFont(juce::Font(juce::FontOptions(12.0f,juce::Font::bold)));
    g.drawFittedText(slot==0 ? "CAB A  /  LIBERTY" : "CAB B  /  LIBERTY",
                     r.toNearestInt().withHeight(22).reduced(12,0),juce::Justification::centredLeft,1);
}

inline void paint(juce::Graphics& g, juce::Rectangle<int> area,
                  const juce::String& filenameA,const juce::String& filenameB)
{
    auto inner=area.toFloat().reduced(12.0f);
    const float gap=12.0f;
    const float width=(inner.getWidth()-gap)/2.0f;
    drawCab(g,{inner.getX(),inner.getY(),width,inner.getHeight()},0,filenameA);
    drawCab(g,{inner.getX()+width+gap,inner.getY(),width,inner.getHeight()},1,filenameB);
}
}
