#pragma once

#include <JuceHeader.h>
#include "AmpVisualDetector.h"
#include "AmpArtwork.h"
#include "TextUtf8.h"
#include <algorithm>
#include <cmath>
#include <array>
#include <functional>
#include <memory>
#include <vector>

// Offline browser: searches a folder explicitly selected by the user.
// No network requests, no model decoding during scanning and no audio-thread I/O.
class LibertyLibraryBrowser final : public juce::Component, private juce::ListBoxModel
{
public:
    std::function<void(int, const juce::File&)> onLoad;
    std::function<void(const juce::String&)> onStatus;

    LibertyLibraryBrowser()
    {
        search.setTextToShowWhenEmpty(libertyText::u8("Rechercher un ampli, une IR ou un dossier..."),
                                      juce::Colours::grey);
        search.onTextChange = [this] { updateFilter(); };
        addAndMakeVisible(search);

        filter.addItem("NAM + IR", 1);
        filter.addItem("NAM", 2);
        filter.addItem("IR WAV", 3);
        filter.addItem("FAVORIS", 4);
        filter.setSelectedId(1, juce::dontSendNotification);
        filter.onChange = [this] { updateFilter(); };
        addAndMakeVisible(filter);

        chooseRoot.setButtonText("DOSSIER...");
        chooseRoot.onClick = [this] { chooseFolder(); };
        refresh.setButtonText("ACTUALISER");
        refresh.onClick = [this] { scan(); };
        favourite.setButtonText("FAVORI");
        favourite.onClick = [this] { toggleFavourite(); };
        loadA.setButtonText("CHARGER A");
        loadA.onClick = [this] { loadSelected(0); };
        loadB.setButtonText("CHARGER B");
        loadB.onClick = [this] { loadSelected(1); };
        for (auto* button : {&chooseRoot, &refresh, &favourite, &loadA, &loadB})
            addAndMakeVisible(*button);

        list.setModel(this);
        list.setRowHeight(32);
        list.setColour(juce::ListBox::backgroundColourId, juce::Colour::fromRGB(17,16,16));
        list.setColour(juce::ListBox::outlineColourId, juce::Colour::fromRGB(95,70,43));
        list.setOutlineThickness(1);
        addAndMakeVisible(list);

        for (auto* label : {&pathLabel, &detail, &subdetail})
        {
            label->setColour(juce::Label::textColourId, juce::Colour::fromRGB(235,221,204));
            label->setFont(juce::Font(juce::FontOptions(13.0f)));
            label->setMinimumHorizontalScale(0.65f);
            addAndMakeVisible(*label);
        }
        pathLabel.setText(libertyText::u8("Choisissez un dossier contenant vos NAM et IR."), juce::dontSendNotification);
        detail.setText(libertyText::u8("Sélectionnez un fichier pour voir son aperçu."), juce::dontSendNotification);
        formats.registerBasicFormats();
        readPrefs();
        if (root.isDirectory()) scan();
        else updateButtons();
    }

    ~LibertyLibraryBrowser() override { list.setModel(nullptr); }

    void paint(juce::Graphics& g) override
    {
        const auto all = getLocalBounds();
        g.setColour(juce::Colour::fromRGB(33,29,25));
        g.fillRoundedRectangle(all.toFloat().reduced(1.0f), 10.0f);
        g.setColour(juce::Colour::fromRGB(95,70,43));
        g.drawRoundedRectangle(all.toFloat().reduced(1.0f), 10.0f, 1.0f);

        auto previewArea = preview;
        if (previewArea.isEmpty()) return;
        g.setColour(juce::Colour::fromRGB(12,12,12));
        g.fillRoundedRectangle(previewArea.toFloat(), 9.0f);
        const auto* item = selectedItem();
        if (item == nullptr) return;

        auto visual = previewArea.reduced(16, 20);
        visual.removeFromBottom(64);
        if (item->nam)
        {
            const auto family = libertyAmp::detect(item->file.getFileName().toStdString(), {},
                                                     item->file.getParentDirectory().getFileName().toStdString());
            libertyArtwork::paint(g, visual, family.family, true, juce::Image());
            g.setColour(juce::Colour::fromRGB(240,175,86));
            g.setFont(juce::Font(juce::FontOptions(13.0f)));
            g.drawFittedText(libertyText::u8("Profil suggéré : ") + libertyText::u8(libertyAmp::families[(size_t)family.family].name),
                             previewArea.reduced(18).removeFromBottom(20), juce::Justification::centredLeft, 1);
        }
        else
        {
            g.setColour(juce::Colour::fromRGB(112,197,216));
            const float centre = (float)visual.getCentreY();
            const float half = (float)visual.getHeight() * 0.42f;
            for (int i=0; i<128; ++i)
            {
                const float x = (float)visual.getX() + (float)visual.getWidth() * ((float)i + 0.5f) / 128.0f;
                const float y = irPreview[(size_t)i] * half;
                g.drawLine(x, centre-y, x, centre+y, 1.2f);
            }
            g.setColour(juce::Colour::fromRGB(230,224,213));
            g.setFont(juce::Font(juce::FontOptions(13.0f)));
            g.drawFittedText("IR WAV / FORME D'ONDE", previewArea.reduced(18).removeFromBottom(20),
                             juce::Justification::centredLeft, 1);
        }
    }

    void resized() override
    {
        const auto r = getLocalBounds().reduced(22);
        const int top = r.getY();
        const int w = r.getWidth();
        const int searchWidth = juce::jmax(200, w * 35 / 100);
        chooseRoot.setBounds(r.getX(), top, 142, 34);
        refresh.setBounds(r.getX()+152, top, 126, 34);
        search.setBounds(r.getX()+290, top, searchWidth, 34);
        filter.setBounds(r.getRight()-166, top, 166, 34);
        pathLabel.setBounds(r.getX()+2, top+39, w-4, 26);
        const int contentTop = top + 74;
        const int contentBottom = r.getBottom() - 62;
        const int listWidth = (w-20) * 58 / 100;
        list.setBounds(r.getX(), contentTop, listWidth, contentBottom-contentTop);
        preview = {r.getX()+listWidth+20, contentTop, w-listWidth-20, contentBottom-contentTop};
        detail.setBounds(preview.getX()+12, preview.getBottom()-61, preview.getWidth()-24, 27);
        subdetail.setBounds(preview.getX()+12, preview.getBottom()-35, preview.getWidth()-24, 23);
        const int buttonY = r.getBottom()-44;
        favourite.setBounds(r.getX(),buttonY,180,35);
        loadA.setBounds(r.getRight()-300,buttonY,145,35);
        loadB.setBounds(r.getRight()-145,buttonY,145,35);
    }

private:
    struct Item { juce::File file; bool nam = false; };
    std::vector<Item> entries;
    std::vector<int> visible;
    juce::StringArray favourites;
    juce::File root;
    juce::TextEditor search;
    juce::ComboBox filter;
    juce::TextButton chooseRoot, refresh, favourite, loadA, loadB;
    juce::ListBox list;
    juce::Label pathLabel, detail, subdetail;
    std::unique_ptr<juce::FileChooser> folderChooser;
    juce::AudioFormatManager formats;
    juce::Rectangle<int> preview;
    std::array<float,128> irPreview {};

    static juce::File prefsPath()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("Liberty Music Software").getChildFile("NeuralMatrixLibrary.json");
    }

    void readPrefs()
    {
        const auto path = prefsPath();
        if (!path.existsAsFile() || path.getSize() > 1024*1024) return;
        const auto var = juce::JSON::parse(path);
        auto* object = var.getDynamicObject();
        if (object == nullptr) return;
        const auto savedRoot = object->getProperty("root").toString();
        if (savedRoot.isNotEmpty()) root = juce::File(savedRoot);
        if (auto* array = object->getProperty("favourites").getArray())
            for (const auto& value : *array)
                if (value.toString().isNotEmpty()) favourites.addIfNotAlreadyThere(value.toString());
    }

    void writePrefs()
    {
        const auto path = prefsPath();
        if (path.getParentDirectory().createDirectory().failed()) return;
        auto* object = new juce::DynamicObject();
        object->setProperty("root", root.getFullPathName());
        juce::Array<juce::var> items;
        for (const auto& value : favourites) items.add(value);
        object->setProperty("favourites", juce::var(items));
        (void)path.replaceWithText(juce::JSON::toString(juce::var(object), true));
    }

    void chooseFolder()
    {
        folderChooser = std::make_unique<juce::FileChooser>(libertyText::u8("Choisir un dossier NAM et IR"), root);
        juce::Component::SafePointer<LibertyLibraryBrowser> safe(this);
        folderChooser->launchAsync(juce::FileBrowserComponent::openMode
                                   | juce::FileBrowserComponent::canSelectDirectories,
            [safe](const juce::FileChooser& chooser)
            {
                if (safe == nullptr) return;
                const auto selected = chooser.getResult();
                if (!selected.isDirectory()) return;
                safe->root = selected;
                safe->writePrefs();
                safe->scan();
            });
    }

    void scan()
    {
        entries.clear();
        if (root.isDirectory())
        {
            // Capped local scan: no hidden network and no background access to other folders.
            int visited = 0;
            for (const auto& entry : juce::RangedDirectoryIterator(root, true, "*", juce::File::findFiles))
            {
                // A huge media drive must not freeze the plugin UI indefinitely.
                if (++visited > 20000) break;
                const auto file = entry.getFile();
                const auto ext = file.getFileExtension().toLowerCase();
                if (ext == ".nam" || ext == ".wav") entries.push_back({file, ext == ".nam"});
                if (entries.size() >= 2000) break;
            }
            std::sort(entries.begin(), entries.end(), [](const Item& a, const Item& b)
            { return a.file.getFileName().compareIgnoreCase(b.file.getFileName()) < 0; });
            pathLabel.setText(root.getFullPathName() + "   /   " + juce::String((int)entries.size()) + " NAM / IR",
                              juce::dontSendNotification);
        }
        else pathLabel.setText(libertyText::u8("Dossier introuvable. Choisissez un dossier."), juce::dontSendNotification);
        updateFilter();
    }

    void updateFilter()
    {
        visible.clear();
        const auto term = search.getText().trim();
        for (size_t i=0; i<entries.size(); ++i)
        {
            const auto& entry = entries[i];
            const int id = filter.getSelectedId();
            if (id == 2 && !entry.nam) continue;
            if (id == 3 && entry.nam) continue;
            if (id == 4 && !favourites.contains(entry.file.getFullPathName())) continue;
            if (term.isNotEmpty() && !entry.file.getFullPathName().containsIgnoreCase(term)) continue;
            visible.push_back((int)i);
        }
        list.updateContent();
        if (!visible.empty()) list.selectRow(0);
        else { list.deselectAllRows(); selectedRowsChanged(-1); }
        repaint();
    }

    const Item* selectedItem() const
    {
        const int row = list.getSelectedRow();
        if (!juce::isPositiveAndBelow(row, (int)visible.size())) return nullptr;
        return &entries[(size_t)visible[(size_t)row]];
    }

    int getNumRows() override { return (int)visible.size(); }

    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override
    {
        if (!juce::isPositiveAndBelow(row, (int)visible.size())) return;
        const auto& item = entries[(size_t)visible[(size_t)row]];
        g.fillAll(selected ? juce::Colour::fromRGB(92,60,29) : juce::Colour::fromRGB(23,22,21));
        const auto fav = favourites.contains(item.file.getFullPathName());
        g.setColour(item.nam ? juce::Colour::fromRGB(246,165,69) : juce::Colour::fromRGB(116,207,224));
        g.setFont(juce::Font(juce::FontOptions(12.5f)));
        g.drawFittedText(juce::String(fav ? "[+] " : "    ") + (item.nam ? "NAM  " : "IR     ")
                         + item.file.getFileName(), 8, 0, width-15, height, juce::Justification::centredLeft, 1);
    }

    void selectedRowsChanged(int) override
    {
        irPreview.fill(0.0f);
        if (const auto* item = selectedItem())
        {
            detail.setText(item->file.getFileName(), juce::dontSendNotification);
            subdetail.setText(item->nam ? libertyText::u8("NAM / MODÈLE AUDIO") : libertyText::u8("IR / RÉPONSE IMPULSIONNELLE"), juce::dontSendNotification);
            if (!item->nam)
            {
                std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(item->file));
                if (reader && reader->lengthInSamples > 0)
                {
                    const auto frames = (int) juce::jmin<juce::int64>(reader->lengthInSamples, 65536);
                    juce::AudioBuffer<float> buffer(1, frames);
                    if (reader->read(&buffer, 0, frames, 0, true, false))
                        for (int i=0; i<frames; ++i)
                            irPreview[(size_t)juce::jmin(127,i*128/frames)] = juce::jmax(
                                irPreview[(size_t)juce::jmin(127,i*128/frames)], std::abs(buffer.getSample(0,i)));
                }
            }
        }
        else
        {
            detail.setText(libertyText::u8("Aucun fichier sélectionné"), juce::dontSendNotification);
            subdetail.setText({}, juce::dontSendNotification);
        }
        updateButtons();
        repaint();
    }

    void updateButtons()
    {
        const auto* item = selectedItem();
        const bool hasItem = item != nullptr;
        favourite.setEnabled(hasItem);
        loadA.setEnabled(hasItem);
        loadB.setEnabled(hasItem);
        if (hasItem)
        {
            const bool fav = favourites.contains(item->file.getFullPathName());
            favourite.setButtonText(fav ? "RETIRER FAVORI" : "AJOUTER FAVORI");
            loadA.setButtonText(item->nam ? "CHARGER NAM A" : "CHARGER IR A");
            loadB.setButtonText(item->nam ? "CHARGER NAM B" : "CHARGER IR B");
        }
    }

    void toggleFavourite()
    {
        const auto* item = selectedItem();
        if (!item) return;
        const auto path = item->file.getFullPathName();
        if (favourites.contains(path)) favourites.removeString(path);
        else favourites.addIfNotAlreadyThere(path);
        writePrefs();
        if (filter.getSelectedId() == 4) updateFilter();
        else { list.repaint(); updateButtons(); }
    }

    void loadSelected(int side)
    {
        const auto* item = selectedItem();
        if (!item || !onLoad) return;
        onLoad((item->nam ? 0 : 2) + side, item->file);
    }
};
