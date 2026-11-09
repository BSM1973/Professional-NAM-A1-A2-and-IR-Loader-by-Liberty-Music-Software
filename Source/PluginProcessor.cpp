#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace {
constexpr auto* kInput = "input";
constexpr auto* kOutput = "output";
constexpr auto* kNAMMix = "nammix";
constexpr auto* kIRMix = "irmix";
}

juce::AudioProcessorValueTreeState::ParameterLayout LibertyNeuralMatrixAudioProcessor::parameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{kInput, 1}, "Entrée (dB)", -24.0f, 24.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{kOutput, 1}, "Sortie (dB)", -24.0f, 24.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{kNAMMix, 1}, "NAM A/B", 0.0f, 1.0f, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{kIRMix, 1}, "IR A/B", 0.0f, 1.0f, 0.0f));
    return {p.begin(), p.end()};
}

LibertyNeuralMatrixAudioProcessor::LibertyNeuralMatrixAudioProcessor()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
          .withInput("Entrée", juce::AudioChannelSet::stereo(), true)
          .withOutput("Sortie", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "LIBERTY_STATE", parameters())
{
}

bool LibertyNeuralMatrixAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto in = layout.getMainInputChannelSet();
    const auto out = layout.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void LibertyNeuralMatrixAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Preparation is called by the host, never from processBlock.
    const juce::ScopedLock lock(modelMutex);
    preparedRate = sampleRate;
    maxBlock = juce::jmax(64, samplesPerBlock);
    mono.resize((size_t)maxBlock);
    namA.resize((size_t)maxBlock);
    namB.resize((size_t)maxBlock);
    irScratchA.setSize(2, maxBlock);
    irScratchB.setSize(2, maxBlock);
    juce::dsp::ProcessSpec spec{sampleRate, (juce::uint32)maxBlock, 2};
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
        return juce::Result::fail("Extension incompatible avec cet emplacement.");

    if (slot < 2)
    {
        try
        {
            // Decode outside the audio callback. Future: fully asynchronous loader.
           #if JUCE_WINDOWS
            const std::filesystem::path nativePath(file.getFullPathName().toWideCharPointer());
           #else
            const std::filesystem::path nativePath(file.getFullPathName().toRawUTF8());
           #endif
            auto replacement = nam::get_dsp(nativePath);
            if (!replacement || replacement->NumInputChannels() != 1 || replacement->NumOutputChannels() != 1)
                return juce::Result::fail("Ce prototype nécessite un modèle NAM mono.");
            const auto rate = replacement->GetExpectedSampleRate();
            if (rate > 0 && std::abs(rate - preparedRate) > 1.0)
                return juce::Result::fail("Fréquence NAM différente de celle du projet. Utiliser la fréquence native.");
            replacement->Reset(preparedRate, maxBlock);
            std::unique_ptr<nam::DSP> previous;
            {
                const juce::ScopedLock lock(modelMutex);
                previous = std::move(models[(size_t)slot]);
                models[(size_t)slot] = std::move(replacement);
                slotPaths[(size_t)slot] = file.getFullPathName();
            }
            // Destruct old model after releasing the audio mutex.
        }
        catch (const std::exception& e)
        {
            return juce::Result::fail("Erreur de lecture NAM : " + juce::String(e.what()));
        }
    }
    else
    {
        const int idx = slot - 2;
        irs[(size_t)idx].loadImpulseResponse(
            file, juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::yes, 0,
            juce::dsp::Convolution::Normalise::no);
        const juce::ScopedLock lock(modelMutex);
        slotPaths[(size_t)slot] = file.getFullPathName();
        irLoaded[(size_t)idx] = true;
    }
    return juce::Result::ok();
}

juce::String LibertyNeuralMatrixAudioProcessor::slotDisplayName(int slot) const
{
    if (!juce::isPositiveAndBelow(slot, 4)) return {};
    const juce::ScopedLock lock(const_cast<juce::CriticalSection&>(modelMutex));
    return slotPaths[(size_t)slot].isEmpty()
        ? "Aucun fichier"
        : juce::File(slotPaths[(size_t)slot]).getFileName();
}

void LibertyNeuralMatrixAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int nCh = buffer.getNumChannels();
    const int nIn = juce::jmin(getTotalNumInputChannels(), nCh);
    if (nCh == 0 || nIn == 0) { buffer.clear(); return; }

    inputGain.setTargetValue(juce::Decibels::decibelsToGain(state.getRawParameterValue(kInput)->load()));
    outputGain.setTargetValue(juce::Decibels::decibelsToGain(state.getRawParameterValue(kOutput)->load()));
    namBlend.setTargetValue(state.getRawParameterValue(kNAMMix)->load());
    irBlend.setTargetValue(state.getRawParameterValue(kIRMix)->load());

    // Prototype: avoid blocking audio if UI loading holds the lock.
    juce::ScopedTryLock lock(modelMutex);
    if (!lock.isLocked()) return;

    for (int offset = 0; offset < buffer.getNumSamples(); offset += maxBlock)
    {
        const int count = juce::jmin(maxBlock, buffer.getNumSamples() - offset);
        const float* l = buffer.getReadPointer(0, offset);
        const float* r = nIn > 1 ? buffer.getReadPointer(1, offset) : nullptr;
        for (int i = 0; i < count; ++i)
            mono[(size_t)i] = (r ? 0.5f * (l[i] + r[i]) : l[i]) * inputGain.getNextValue();

        auto renderNAM = [&](int idx, std::vector<float>& dst)
        {
            if (models[(size_t)idx])
            {
                float* inputs[] = {mono.data()};
                float* outputs[] = {dst.data()};
                models[(size_t)idx]->process(inputs, outputs, count);
            }
            else std::copy_n(mono.data(), count, dst.data());
        };
        const bool loadedA = (bool)models[0], loadedB = (bool)models[1];
        if (loadedA) renderNAM(0, namA);
        if (loadedB) renderNAM(1, namB);
        for (int i = 0; i < count; ++i)
        {
            const float t = namBlend.getNextValue();
            float x = mono[(size_t)i];
            if (loadedA && loadedB)
                x = (1.0f - t) * namA[(size_t)i] + t * namB[(size_t)i];
            else if (loadedA)
                x = namA[(size_t)i];
            else if (loadedB)
                x = namB[(size_t)i];
            for (int ch = 0; ch < nCh; ++ch)
                buffer.setSample(ch, offset + i, x);
        }

        const bool hasIRA = irLoaded[0], hasIRB = irLoaded[1];
        if (hasIRA || hasIRB)
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                const int sourceCh = juce::jmin(ch, nCh - 1);
                if (hasIRA) irScratchA.copyFrom(ch, 0, buffer, sourceCh, offset, count);
                if (hasIRB) irScratchB.copyFrom(ch, 0, buffer, sourceCh, offset, count);
            }
            if (hasIRA)
            {
                juce::dsp::AudioBlock<float> block(irScratchA);
                auto sub = block.getSubBlock(0, (size_t)count);
                juce::dsp::ProcessContextReplacing<float> context(sub);
                irs[0].process(context);
            }
            if (hasIRB)
            {
                juce::dsp::AudioBlock<float> block(irScratchB);
                auto sub = block.getSubBlock(0, (size_t)count);
                juce::dsp::ProcessContextReplacing<float> context(sub);
                irs[1].process(context);
            }
            for (int i = 0; i < count; ++i)
            {
                const float t = irBlend.getNextValue();
                for (int ch = 0; ch < nCh; ++ch)
                {
                    const float a = hasIRA ? irScratchA.getSample(ch, i) : 0.0f;
                    const float b = hasIRB ? irScratchB.getSample(ch, i) : 0.0f;
                    const float result = (hasIRA && hasIRB) ? a * (1.0f - t) + b * t : hasIRA ? a : b;
                    buffer.setSample(ch, offset + i, result);
                }
            }
        }
        else irBlend.skip(count);

        for (int i = 0; i < count; ++i)
        {
            const float gain = outputGain.getNextValue();
            for (int ch = 0; ch < nCh; ++ch)
                buffer.setSample(ch, offset + i, buffer.getSample(ch, offset + i) * gain);
        }
    }
}

void LibertyNeuralMatrixAudioProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    auto tree = state.copyState();
    {
        const juce::ScopedLock lock(modelMutex);
        for (int i = 0; i < 4; ++i)
            tree.setProperty("path" + juce::String(i), slotPaths[(size_t)i], nullptr);
    }
    if (auto xml = tree.createXml()) copyXmlToBinary(*xml, destination);
}

void LibertyNeuralMatrixAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (auto tree = juce::ValueTree::fromXml(*xml); tree.isValid())
        {
            state.replaceState(tree);
            {
                const juce::ScopedLock lock(modelMutex);
                for (int i = 0; i < 4; ++i)
                    slotPaths[(size_t)i] = tree.getProperty("path" + juce::String(i)).toString();
            }
            triggerAsyncUpdate();
        }
}

void LibertyNeuralMatrixAudioProcessor::handleAsyncUpdate()
{
    std::array<juce::String, 4> paths;
    {
        const juce::ScopedLock lock(modelMutex);
        paths = slotPaths;
    }
    for (int i = 0; i < 4; ++i)
        if (paths[(size_t)i].isNotEmpty())
            (void)loadFile(i, juce::File(paths[(size_t)i]));
}

juce::AudioProcessorEditor* LibertyNeuralMatrixAudioProcessor::createEditor()
{
    return new LibertyNeuralMatrixAudioProcessorEditor(*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LibertyNeuralMatrixAudioProcessor();
}
