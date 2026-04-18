#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "KnobDesign.h"

class BoostorAudioProcessor : public juce::AudioProcessor
{
public:
    BoostorAudioProcessor();
    ~BoostorAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Host-integrated bypass. DAWs that drive a host-side bypass parameter
    // will flip this, and processBlock early-returns on the same APVTS
    // entry so the in-plugin power button stays in sync.
    juce::AudioParameterBool* getBypassParameter() const override
    {
        return dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("bypass"));
    }

    // Wall-clock time the last processBlock took (CPU load indicator).
    float getLastProcessLatencyMs() const { return lastProcessLatencyMs.load(std::memory_order_relaxed); }

    // Algorithmic latency in milliseconds — the buffering delay this plugin
    // introduces. Boostor does per-sample gain only (no FFT, no delay line),
    // so it's exactly zero. Kept as a method for symmetry with tiptoe and
    // in case a future feature adds actual buffering.
    float getAlgorithmicLatencyMs() const { return 0.0f; }

    // Editor size persistence
    std::atomic<int> editorWidth  { KnobDesign::defaultSize };
    std::atomic<int> editorHeight { KnobDesign::defaultHeight };

private:
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float>* gainParam = nullptr; // cached pointer — no hashmap lookup per block
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothedGain;
    std::atomic<float> lastProcessLatencyMs { 0.0f };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoostorAudioProcessor)
};
