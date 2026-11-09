#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <filesystem>

namespace {
constexpr auto* kInput = "input";
constexpr auto* kOutput = "output";
constexpr auto* kNAMMix = "nammix";
constexpr auto* kIRMix = "irmix";
constexpr float pi = 3.14159265358979323846f;
float onePole(float frequency, double sampleRate)
{
    return 1.0f - std::exp(-2.0f * pi * frequency / static_cast<float>(juce::jmax(1.0, sampleRate)));
}
float dbToGain(float value) { return juce::Decibels::decibelsToGain(value); }

}

juce::AudioProcessorValueTreeState::ParameterLayout LibertyNeuralMatrixAudioProcessor::parameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{kInput, 1}, "Input (dB)", -24.0f, 24.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{kOutput, 1}, "Output (dB)", -24.0f, 24.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{kNAMMix, 1}, "NAM A/B", 0.0f, 1.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{kIRMix, 1}, "IR A/B", 0.0f, 1.0f, 0.0f));
    auto addFloat = [&](const char* id, const char* name, float min, float max, float initial)
    {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name, min, max, initial));
    };
    auto addBool = [&](const char* id, const char* name, bool initial)
    {
        p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id, 1}, name, initial));
    };
    // Tone controls are real post-NAM filters, not exposed as controls of a captured amplifier.
    addFloat("bass", "Graves post NAM (dB)", -12.0f, 12.0f, 0.0f);
    addFloat("middle", "Médiums post NAM (dB)", -12.0f, 12.0f, 0.0f);
    addFloat("treble", "Aigus post NAM (dB)", -12.0f, 12.0f, 0.0f);
    addFloat("presence", "Présence post NAM (dB)", -12.0f, 12.0f, 0.0f);
    addBool("ampOn", "Activer section ampli", true);
    addBool("cabOn", "Activer section baffle", true);
    addFloat("irAGain", "Niveau IR A (dB)", -24.0f, 6.0f, 0.0f);
    addFloat("irBGain", "Niveau IR B (dB)", -24.0f, 6.0f, 0.0f);
    addBool("irAPhase", "Inverser polarité IR A", false);
    addBool("irBPhase", "Inverser polarité IR B", false);
    addFloat("cabHP", "Coupe-bas baffle (Hz)", 20.0f, 250.0f, 70.0f);
    addFloat("cabLP", "Coupe-haut baffle (Hz)", 2500.0f, 18000.0f, 12000.0f);
    addBool("gateOn", "Activer gate", false);
    addFloat("gateThreshold", "Seuil gate (dBFS)", -80.0f, -20.0f, -55.0f);
    addBool("boostOn", "Boost pré NAM +6 dB", false);
    addBool("delayOn", "Activer delay", false);
    // Microphone families are original post-IR tone profiles, not physical remiking.
    const juce::StringArray micChoices { "Neutre", "Dynamique type 57", "Dynamique type 421",
                                         "Condensateur type 87", "Lampe type 47", "Ruban type 121" };
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "micAType", 1 }, "Profil micro A", micChoices, 0));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "micBType", 1 }, "Profil micro B", micChoices, 0));
    addFloat("micAOffset", "Micro A hors axe (%)", 0.0f, 100.0f, 0.0f);
    addFloat("micBOffset", "Micro B hors axe (%)", 0.0f, 100.0f, 0.0f);
    addFloat("micADistance", "Distance micro A (cm)", 0.0f, 80.0f, 10.0f);
    addFloat("micBDistance", "Distance micro B (cm)", 0.0f, 80.0f, 10.0f);
    addFloat("delayMs", "Temps delay (ms)", 60.0f, 1200.0f, 320.0f);
    addFloat("delayFeedback", "Feedback delay", 0.0f, 0.85f, 0.30f);
    addFloat("delayMix", "Mix delay", 0.0f, 0.70f, 0.20f);
    return {p.begin(), p.end()};
}

LibertyNeuralMatrixAudioProcessor::LibertyNeuralMatrixAudioProcessor()
 : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
       .withInput("Entrée", juce::AudioChannelSet::stereo(), true)
       .withOutput("Sortie", juce::AudioChannelSet::stereo(), true)),
   state(*this, nullptr, "LIBERTY_STATE", parameters())
{}

bool LibertyNeuralMatrixAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    auto in = layout.getMainInputChannelSet();
    auto out = layout.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void LibertyNeuralMatrixAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Never allocate in the audio callback. Models are reset on the host preparation thread.
    const juce::ScopedLock lock(modelMutex);
    preparedRate = sampleRate;
    preparedRateAtomic.store(sampleRate, std::memory_order_relaxed);
    gateEnvelope = 0.0f;
    gateGain = 1.0f;
    toneStates = {};
    micStates = {};
    delayPosition = 0;
    const size_t delaySize = static_cast<size_t>(std::ceil(1.25 * sampleRate)) + 16u;
    for (auto& line : delayLines) line.assign(delaySize, 0.0f);
    maxBlock = juce::jmax(64, samplesPerBlock);
    mono.resize((size_t) maxBlock);
    namA.resize((size_t) maxBlock);
    namB.resize((size_t) maxBlock);
    irScratchA.setSize(2, maxBlock);
    irScratchB.setSize(2, maxBlock);

    juce::dsp::ProcessSpec spec{sampleRate, (juce::uint32) maxBlock, 2};
    for (auto& ir : irs) ir.prepare(spec);
    for (auto& model : models)
        if (model) model->Reset(sampleRate, maxBlock);

    for (auto* smoothed : {&inputGain, &outputGain, &namBlend, &irBlend})
        smoothed->reset(sampleRate, 0.025);
    inputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(state.getRawParameterValue(kInput)->load()));
    outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(state.getRawParameterValue(kOutput)->load()));
    namBlend.setCurrentAndTargetValue(state.getRawParameterValue(kNAMMix)->load());
    irBlend.setCurrentAndTargetValue(state.getRawParameterValue(kIRMix)->load());
}

juce::Result LibertyNeuralMatrixAudioProcessor::loadFile(int slot, const juce::File& file)
{
    if (!juce::isPositiveAndBelow(slot, 4) || !file.existsAsFile())
        return juce::Result::fail("Fichier introuvable.");
    const auto extension = file.getFileExtension().toLowerCase();
    if ((slot < 2 && extension != ".nam") || (slot >= 2 && extension != ".wav"))
        return juce::Result::fail("Extension incompatible avec ce slot.");

    if (slot < 2)
    {
        try
        {
            // Expensive loading and neural pre-warming happen on the UI thread, not in processBlock.
            // Preserve French accents and other Unicode characters in model paths on Windows.
           #if JUCE_WINDOWS
            const std::filesystem::path nativePath(file.getFullPathName().toWideCharPointer());
           #else
            const std::filesystem::path nativePath(file.getFullPathName().toRawUTF8());
           #endif
            auto replacement = nam::get_dsp(nativePath);
            if (!replacement || replacement->NumInputChannels() != 1 || replacement->NumOutputChannels() != 1)
                return juce::Result::fail("Ce prototype exige un NAM mono entrée/sortie.");

            auto expectedRate = replacement->GetExpectedSampleRate();
            if (expectedRate > 0 && std::abs(expectedRate - preparedRate) > 1.0)
                return juce::Result::fail("Fréquence du NAM différente du projet. Utiliser le taux natif (souvent 48 kHz). ");
            replacement->Reset(preparedRate, maxBlock);
            std::unique_ptr<nam::DSP> previous;
            {
                const juce::ScopedLock lock(modelMutex);
                previous = std::move(models[(size_t) slot]);
                models[(size_t) slot] = std::move(replacement);
                slotPaths[(size_t) slot] = file.getFullPathName();
            }
            // Destroy old model only after releasing the audio thread lock.
        }
        catch (const std::exception& e)
        {
            return juce::Result::fail("Impossible de charger le modèle NAM : " + juce::String(e.what()));
        }
    }
    else
    {
        // JUCE convolution resamples and prepares the IR internally. No file I/O in processBlock.
        const int idx = slot - 2;
        // Read a capped preview on the message thread. No audio-thread decoding.
        std::array<float, 128> preview {};
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
        if (!reader || reader->lengthInSamples <= 0 || reader->numChannels == 0)
            return juce::Result::fail("IR WAV illisible ou vide.");
        const int previewSamples = static_cast<int>(juce::jmin<juce::int64>(reader->lengthInSamples, 192000));
        juce::AudioBuffer<float> samples(static_cast<int>(juce::jmin<unsigned int>(reader->numChannels, 2u)), previewSamples);
        if (!reader->read(&samples, 0, previewSamples, 0, true, true))
            return juce::Result::fail("Impossible de décoder cette IR.");
        for (int index = 0; index < previewSamples; ++index)
        {
            const int bin = juce::jmin(127, index * 128 / previewSamples);
            float level = std::abs(samples.getSample(0, index));
            if (samples.getNumChannels() > 1)
                level = juce::jmax(level, std::abs(samples.getSample(1, index)));
            preview[(size_t) bin] = juce::jmax(preview[(size_t) bin], level);
        }
        irs[(size_t) idx].loadImpulseResponse(file, juce::dsp::Convolution::Stereo::yes,
             juce::dsp::Convolution::Trim::yes, 0, juce::dsp::Convolution::Normalise::no);
        const juce::ScopedLock lock(modelMutex);
        slotPaths[(size_t) slot] = file.getFullPathName();
        irLoaded[(size_t) idx] = true;
        irPeaks[(size_t) idx] = preview;
    }
    return juce::Result::ok();
}

juce::String LibertyNeuralMatrixAudioProcessor::slotDisplayName(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, 4)) return {};
    const juce::ScopedLock lock(const_cast<juce::CriticalSection&>(modelMutex));
    return slotPaths[(size_t) slot].isEmpty() ? "Charger un fichier" : juce::File(slotPaths[(size_t) slot]).getFileName();
}

std::array<float, 128> LibertyNeuralMatrixAudioProcessor::irWaveform(int index) const
{
    if (!juce::isPositiveAndBelow(index, 2)) return {};
    const juce::ScopedLock lock(const_cast<juce::CriticalSection&>(modelMutex));
    return irPeaks[(size_t) index];
}

void LibertyNeuralMatrixAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = buffer.getNumChannels();
    const int nIn = juce::jmin(getTotalNumInputChannels(), nCh);
    if (nIn == 0 || nCh == 0) { buffer.clear(); return; }

    const auto value = [this](const char* id) { return state.getRawParameterValue(id)->load(); };
    inputGain.setTargetValue(dbToGain(value(kInput)));
    outputGain.setTargetValue(dbToGain(value(kOutput)));
    namBlend.setTargetValue(value(kNAMMix));
    irBlend.setTargetValue(value(kIRMix));

    const bool ampOn = value("ampOn") > 0.5f;
    const bool cabOn = value("cabOn") > 0.5f;
    const bool gateOn = value("gateOn") > 0.5f;
    const bool boostOn = value("boostOn") > 0.5f;
    const bool delayOn = value("delayOn") > 0.5f;
    const float gateThreshold = dbToGain(value("gateThreshold"));
    const float boostGain = boostOn ? dbToGain(6.0f) : 1.0f;
    const float bassGain = dbToGain(value("bass"));
    const float middleGain = dbToGain(value("middle"));
    const float trebleGain = dbToGain(value("treble"));
    const float presenceGain = dbToGain(value("presence"));
    const float irAGain = dbToGain(value("irAGain")) * (value("irAPhase") > 0.5f ? -1.0f : 1.0f);
    const float irBGain = dbToGain(value("irBGain")) * (value("irBPhase") > 0.5f ? -1.0f : 1.0f);

    // Simple, transparent post-IR microphone tone profiles. These do not recover the
    // acoustic information of a microphone captured at a different physical position.
    struct MicProfile
    {
        float cutoff;
        float low;
        float body;
        float air;
    };
    const std::array<MicProfile, 6> profiles {{
        { 18000.0f, 1.00f, 1.00f, 1.00f },
        { 9800.0f, 1.05f, 1.03f, 0.96f },
        { 8100.0f, 1.11f, 1.00f, 0.86f },
        { 15300.0f, 0.96f, 1.00f, 1.10f },
        { 11600.0f, 1.08f, 1.03f, 0.96f },
        { 6500.0f, 1.14f, 0.96f, 0.75f }
    }};
    struct MicCoefficients
    {
        float lowGain = 1.0f, bodyGain = 1.0f, highGain = 1.0f, lowCoef = 0.0f, highCoef = 0.0f;
        bool bypass = true;
    };
    std::array<MicCoefficients, 2> mic {};
    for (int mi = 0; mi < 2; ++mi)
    {
        const char* type = mi == 0 ? "micAType" : "micBType";
        const char* offset = mi == 0 ? "micAOffset" : "micBOffset";
        const char* distance = mi == 0 ? "micADistance" : "micBDistance";
        const auto typeIndex = juce::jlimit(0, 5, static_cast<int>(std::round(value(type))));
        const auto offAxis = juce::jlimit(0.0f, 1.0f, value(offset) * 0.01f);
        const auto metres = juce::jlimit(0.0f, 1.0f, value(distance) / 80.0f);
        auto profile = profiles[(size_t) typeIndex];
        auto& c = mic[(size_t) mi];
        c.bypass = typeIndex == 0 && offAxis < 0.001f && std::abs(value(distance) - 10.0f) < 0.01f;
        // Off-axis and distance darken the upper spectrum, as an approximation only.
        const auto cutoff = juce::jlimit(1600.0f, 18000.0f, profile.cutoff * (1.0f - offAxis * 0.73f) * (1.0f - metres * 0.24f));
        c.lowCoef = onePole(210.0f, preparedRate);
        c.highCoef = onePole(cutoff, preparedRate);
        c.lowGain = profile.low * (1.0f + 0.28f * (1.0f - metres));
        c.bodyGain = profile.body;
        c.highGain = profile.air * (1.0f - 0.52f * offAxis);
        if (typeIndex == 0) c.lowGain = 1.0f + 0.16f * (0.125f - metres);
    }
    const auto colourMic = [&](int ir, int ch, float sample)
    {
        const auto& c = mic[(size_t) ir];
        if (c.bypass) return sample;
        auto& st = micStates[(size_t) ir][(size_t) juce::jmin(ch, 1)];
        st.low += c.lowCoef * (sample - st.low);
        st.high += c.highCoef * (sample - st.high);
        return st.low * c.lowGain + (st.high - st.low) * c.bodyGain + (sample - st.high) * c.highGain;
    };
    const float hpCoeff = onePole(value("cabHP"), preparedRate);
    const float lpCoeff = onePole(value("cabLP"), preparedRate);
    const float lowCoeff = onePole(170.0f, preparedRate);
    const float midLowCoeff = onePole(350.0f, preparedRate);
    const float midHighCoeff = onePole(1900.0f, preparedRate);
    const float upperCoeff = onePole(1800.0f, preparedRate);
    const float presenceCoeff = onePole(4300.0f, preparedRate);
    const float feedback = value("delayFeedback");
    const float delayMix = delayOn ? value("delayMix") : 0.0f;
    const size_t delaySize = delayLines[0].size();
    const size_t delaySamples = juce::jlimit<size_t>(1, delaySize > 1 ? delaySize - 1 : 1,
                                    static_cast<size_t>(value("delayMs") * preparedRate * 0.001));
    const float attack = onePole(85.0f, preparedRate);
    const float release = onePole(5.0f, preparedRate);
    const float gateSmoothing = onePole(45.0f, preparedRate);

    // No file I/O, model creation, memory allocation or coefficient objects in audio callback.
    juce::ScopedTryLock lock(modelMutex);
    if (!lock.isLocked()) return; // Existing nonblocking fallback during a model swap.

    for (int offset = 0; offset < buffer.getNumSamples(); offset += maxBlock)
    {
        const int count = juce::jmin(maxBlock, buffer.getNumSamples() - offset);
        const auto* l = buffer.getReadPointer(0, offset);
        const auto* r = nIn > 1 ? buffer.getReadPointer(1, offset) : nullptr;

        for (int i = 0; i < count; ++i)
        {
            const float dry = r != nullptr ? 0.5f * (l[i] + r[i]) : l[i];
            const float absolute = std::abs(dry);
            gateEnvelope += (absolute > gateEnvelope ? attack : release) * (absolute - gateEnvelope);
            const float desiredGain = (!gateOn || gateEnvelope >= gateThreshold) ? 1.0f : 0.0f;
            gateGain += gateSmoothing * (desiredGain - gateGain);
            mono[(size_t) i] = dry * gateGain * boostGain * inputGain.getNextValue();
        }

        const bool loadedA = ampOn && (bool) models[0];
        const bool loadedB = ampOn && (bool) models[1];
        auto renderNAM = [&](int idx, std::vector<float>& destination)
        {
            float* inputs[] { mono.data() };
            float* outputs[] { destination.data() };
            models[(size_t) idx]->process(inputs, outputs, count);
        };
        if (loadedA) renderNAM(0, namA);
        if (loadedB) renderNAM(1, namB);

        for (int i = 0; i < count; ++i)
        {
            const float t = namBlend.getNextValue();
            float monoResult = mono[(size_t) i];
            if (loadedA && loadedB) monoResult = (1.0f - t) * namA[(size_t) i] + t * namB[(size_t) i];
            else if (loadedA) monoResult = namA[(size_t) i];
            else if (loadedB) monoResult = namB[(size_t) i];

            for (int ch = 0; ch < nCh; ++ch)
            {
                float x = monoResult;
                if (ampOn)
                {
                    auto& eq = toneStates[(size_t) juce::jmin(ch, 1)];
                    eq.low += lowCoeff * (x - eq.low);
                    eq.midLower += midLowCoeff * (x - eq.midLower);
                    eq.midUpper += midHighCoeff * (x - eq.midUpper);
                    eq.upper += upperCoeff * (x - eq.upper);
                    eq.presence += presenceCoeff * (x - eq.presence);
                    x += (bassGain - 1.0f) * eq.low
                       + (middleGain - 1.0f) * (eq.midUpper - eq.midLower)
                       + (trebleGain - 1.0f) * (x - eq.upper)
                       + (presenceGain - 1.0f) * (x - eq.presence);
                }
                buffer.setSample(ch, offset + i, x);
            }
        }

        const bool hasIRA = cabOn && irLoaded[0];
        const bool hasIRB = cabOn && irLoaded[1];
        if (hasIRA || hasIRB)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                const int src = juce::jmin(ch, nCh - 1);
                if (hasIRA) irScratchA.copyFrom(ch, 0, buffer, src, offset, count);
                if (hasIRB) irScratchB.copyFrom(ch, 0, buffer, src, offset, count);
            }
            if (hasIRA)
            {
                juce::dsp::AudioBlock<float> block(irScratchA);
                auto sub = block.getSubBlock(0, (size_t) count);
                juce::dsp::ProcessContextReplacing<float> context(sub);
                irs[0].process(context);
            }
            if (hasIRB)
            {
                juce::dsp::AudioBlock<float> block(irScratchB);
                auto sub = block.getSubBlock(0, (size_t) count);
                juce::dsp::ProcessContextReplacing<float> context(sub);
                irs[1].process(context);
            }
            for (int i = 0; i < count; ++i)
            {
                const float t = irBlend.getNextValue();
                for (int ch = 0; ch < nCh; ++ch)
                {
                    const float a = hasIRA ? colourMic(0, ch, irScratchA.getSample(ch, i)) * irAGain : 0.0f;
                    const float b = hasIRB ? colourMic(1, ch, irScratchB.getSample(ch, i)) * irBGain : 0.0f;
                    float y = hasIRA && hasIRB ? a * (1.0f - t) + b * t : hasIRA ? a : b;
                    auto& eq = toneStates[(size_t) juce::jmin(ch, 1)];
                    eq.cabinetHP += hpCoeff * (y - eq.cabinetHP);
                    y -= eq.cabinetHP;
                    eq.cabinetLP += lpCoeff * (y - eq.cabinetLP);
                    buffer.setSample(ch, offset + i, eq.cabinetLP);
                }
            }
        }
        else irBlend.skip(count);

        for (int i = 0; i < count; ++i)
        {
            const float outGain = outputGain.getNextValue();
            for (int ch = 0; ch < nCh; ++ch)
            {
                float x = buffer.getSample(ch, offset + i);
                if (delaySize > 0)
                {
                    auto& line = delayLines[(size_t) juce::jmin(ch, 1)];
                    const size_t readPosition = (delayPosition + delaySize - delaySamples) % delaySize;
                    const float delayed = line[readPosition];
                    line[delayPosition] = delayOn ? (x + delayed * feedback) : 0.0f;
                    x = x * (1.0f - delayMix) + delayed * delayMix;
                }
                buffer.setSample(ch, offset + i, x * outGain);
            }
            if (delaySize > 0 && ++delayPosition >= delaySize) delayPosition = 0;
        }
    }
}

void LibertyNeuralMatrixAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto tree = state.copyState();
    {
        const juce::ScopedLock lock(modelMutex);
        for (int i = 0; i < 4; ++i) tree.setProperty("path" + juce::String(i), slotPaths[(size_t) i], nullptr);
    }
    auto xml = tree.createXml();
    if (xml) copyXmlToBinary(*xml, destination);
}

void LibertyNeuralMatrixAudioProcessor::setStateInformation(const void* source, int size)
{
    if (auto xml = getXmlFromBinary(source, size))
        if (auto tree = juce::ValueTree::fromXml(*xml); tree.isValid())
        {
            state.replaceState(tree);
            {
                const juce::ScopedLock lock(modelMutex);
                for (int i = 0; i < 4; ++i)
                    slotPaths[(size_t) i] = tree.getProperty("path" + juce::String(i)).toString();
            }
            triggerAsyncUpdate(); // Never decode NAM/IR inside DAW state callback.
        }
}

void LibertyNeuralMatrixAudioProcessor::handleAsyncUpdate()
{
    std::array<juce::String,4> paths;
    {
        const juce::ScopedLock lock(modelMutex);
        paths = slotPaths;
    }
    for (int i = 0; i < 4; ++i)
        if (paths[(size_t)i].isNotEmpty()) (void) loadFile(i, juce::File(paths[(size_t) i]));
}

juce::AudioProcessorEditor* LibertyNeuralMatrixAudioProcessor::createEditor()
{
    return new LibertyNeuralMatrixAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LibertyNeuralMatrixAudioProcessor();
}
