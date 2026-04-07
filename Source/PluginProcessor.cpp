#include "PluginProcessor.h"
#ifndef GAINKNOB_TESTS
#include "PluginEditor.h"
#endif

GainKnobAudioProcessor::GainKnobAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    gainParam = apvts.getRawParameterValue("gain");
}

juce::AudioProcessorValueTreeState::ParameterLayout
GainKnobAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"gain", 1},
        "Gain",
        juce::NormalisableRange<float>(-100.0f, 24.0f, 0.1f, 3.0f),
        0.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")));

    return { params.begin(), params.end() };
}

void GainKnobAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    smoothedGain.reset(sampleRate, 0.02);
    float gainDB = gainParam->load();
    smoothedGain.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(gainDB, -100.0f));
}

void GainKnobAudioProcessor::releaseResources() {}

bool GainKnobAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void GainKnobAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& /*midiMessages*/)
{
    const auto startTicks = juce::Time::getHighResolutionTicks();
    juce::ScopedNoDenormals noDenormals;

    const auto numChannels = getTotalNumInputChannels();
    const auto numOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto i = numChannels; i < numOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    const float gainDB = gainParam->load();
    const float targetGain = juce::Decibels::decibelsToGain(gainDB, -100.0f);
    smoothedGain.setTargetValue(targetGain);

    if (smoothedGain.isSmoothing())
    {
        // Smoothing path: hoist write pointers out of per-sample loop
        float* channelPtrs[2];
        const auto chCount = juce::jmin(numChannels, 2);
        for (int ch = 0; ch < chCount; ++ch)
            channelPtrs[ch] = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            const float g = smoothedGain.getNextValue();
            for (int ch = 0; ch < chCount; ++ch)
                channelPtrs[ch][i] *= g;
        }
    }
    else
    {
        const float g = smoothedGain.getCurrentValue();

        // Fast path: unity gain — skip entirely
        if (g == 1.0f)
        {
            lastProcessLatencyMs.store(
                static_cast<float>(juce::Time::highResolutionTicksToSeconds(
                    juce::Time::getHighResolutionTicks() - startTicks) * 1000.0),
                std::memory_order_relaxed);
            return;
        }

        // Fast path: silence — clear is cheaper than multiply
        if (g == 0.0f)
        {
            buffer.clear();
            lastProcessLatencyMs.store(
                static_cast<float>(juce::Time::highResolutionTicksToSeconds(
                    juce::Time::getHighResolutionTicks() - startTicks) * 1000.0),
                std::memory_order_relaxed);
            return;
        }

        // SIMD-accelerated gain via FloatVectorOperations
        for (int ch = 0; ch < numChannels; ++ch)
            juce::FloatVectorOperations::multiply(
                buffer.getWritePointer(ch), g, numSamples);
    }

    lastProcessLatencyMs.store(
        static_cast<float>(juce::Time::highResolutionTicksToSeconds(
            juce::Time::getHighResolutionTicks() - startTicks) * 1000.0),
        std::memory_order_relaxed);
}

juce::AudioProcessorEditor* GainKnobAudioProcessor::createEditor()
{
#ifndef GAINKNOB_TESTS
    return new GainKnobAudioProcessorEditor(*this);
#else
    return nullptr;
#endif
}

bool GainKnobAudioProcessor::hasEditor() const { return true; }

const juce::String GainKnobAudioProcessor::getName() const { return "GainKnob"; }
bool GainKnobAudioProcessor::acceptsMidi() const { return true; }
bool GainKnobAudioProcessor::producesMidi() const { return false; }
bool GainKnobAudioProcessor::isMidiEffect() const { return false; }
double GainKnobAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int GainKnobAudioProcessor::getNumPrograms() { return 1; }
int GainKnobAudioProcessor::getCurrentProgram() { return 0; }
void GainKnobAudioProcessor::setCurrentProgram(int /*index*/) {}
const juce::String GainKnobAudioProcessor::getProgramName(int /*index*/) { return {}; }
void GainKnobAudioProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/) {}

void GainKnobAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GainKnobAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GainKnobAudioProcessor();
}
