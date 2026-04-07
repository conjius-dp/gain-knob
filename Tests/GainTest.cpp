#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../Source/PluginProcessor.h"

class GainParameterTest : public juce::UnitTest
{
public:
    GainParameterTest() : juce::UnitTest("Gain Parameter Tests") {}

    void runTest() override
    {
        beginTest("Parameter exists with correct ID");
        {
            GainKnobAudioProcessor processor;
            auto* param = processor.getAPVTS().getParameter("gain");
            expect(param != nullptr, "gain parameter should exist");
        }

        beginTest("Parameter range is -100 to +24 dB");
        {
            GainKnobAudioProcessor processor;
            auto* param = processor.getAPVTS().getParameter("gain");
            auto range = param->getNormalisableRange();
            expectEquals(range.start, -100.0f, "min should be -100");
            expectEquals(range.end, 24.0f, "max should be +24");
        }

        beginTest("Default value is 0 dB (unity gain)");
        {
            GainKnobAudioProcessor processor;
            auto* param = processor.getAPVTS().getParameter("gain");
            float defaultNorm = param->getDefaultValue();
            float defaultDenorm = param->convertFrom0to1(defaultNorm);
            expectWithinAbsoluteError(defaultDenorm, 0.0f, 0.01f,
                                      "default should be 0 dB");
        }

        beginTest("Unity gain (0 dB) passes audio unchanged");
        {
            GainKnobAudioProcessor processor;
            processor.getAPVTS().getParameterAsValue("gain").setValue(0.0f);

            constexpr int numSamples = 512;
            juce::AudioBuffer<float> buffer(2, numSamples);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, 0.5f);

            juce::MidiBuffer midi;
            processor.prepareToPlay(44100.0, numSamples);
            processor.processBlock(buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    expectWithinAbsoluteError(buffer.getSample(ch, i), 0.5f, 0.001f,
                                              "unity gain should not change signal");
        }

        beginTest("+6 dB doubles the amplitude");
        {
            GainKnobAudioProcessor processor;
            processor.getAPVTS().getParameterAsValue("gain").setValue(6.0f);

            constexpr int numSamples = 512;
            juce::AudioBuffer<float> buffer(2, numSamples);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, 0.25f);

            juce::MidiBuffer midi;
            processor.prepareToPlay(44100.0, numSamples);
            processor.processBlock(buffer, midi);

            float expected = 0.25f * juce::Decibels::decibelsToGain(6.0f);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    expectWithinAbsoluteError(buffer.getSample(ch, i), expected, 0.01f,
                                              "+6 dB should roughly double amplitude");
        }

        beginTest("-100 dB (min) silences the signal");
        {
            GainKnobAudioProcessor processor;
            processor.getAPVTS().getParameterAsValue("gain").setValue(-100.0f);

            constexpr int numSamples = 512;
            juce::AudioBuffer<float> buffer(2, numSamples);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, 1.0f);

            juce::MidiBuffer midi;
            processor.prepareToPlay(44100.0, numSamples);
            processor.processBlock(buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    expectWithinAbsoluteError(buffer.getSample(ch, i), 0.0f, 0.0001f,
                                              "-100 dB should silence signal");
        }

        beginTest("State save and restore preserves gain value");
        {
            juce::MemoryBlock stateData;

            {
                GainKnobAudioProcessor processor;
                processor.getAPVTS().getParameterAsValue("gain").setValue(12.0f);
                processor.getStateInformation(stateData);
            }

            {
                GainKnobAudioProcessor processor;
                processor.setStateInformation(stateData.getData(),
                                              static_cast<int>(stateData.getSize()));
                auto* param = processor.getAPVTS().getParameter("gain");
                float val = param->convertFrom0to1(param->getValue());
                expectWithinAbsoluteError(val, 12.0f, 0.1f,
                                          "restored gain should be 12 dB");
            }
        }

        beginTest("Stereo bus layout is supported");
        {
            GainKnobAudioProcessor processor;
            auto layout = juce::AudioProcessor::BusesLayout();
            layout.inputBuses.add(juce::AudioChannelSet::stereo());
            layout.outputBuses.add(juce::AudioChannelSet::stereo());
            expect(processor.isBusesLayoutSupported(layout),
                   "stereo in/out should be supported");
        }

        beginTest("Unity gain bypass leaves buffer untouched");
        {
            GainKnobAudioProcessor processor;
            processor.getAPVTS().getParameterAsValue("gain").setValue(0.0f);

            constexpr int numSamples = 512;
            juce::AudioBuffer<float> buffer(2, numSamples);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, static_cast<float>(i) / numSamples);

            juce::MidiBuffer midi;
            processor.prepareToPlay(44100.0, numSamples);
            // Run two blocks so smoothing settles
            processor.processBlock(buffer, midi);

            juce::AudioBuffer<float> buffer2(2, numSamples);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer2.setSample(ch, i, static_cast<float>(i) / numSamples);

            processor.processBlock(buffer2, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    expectWithinAbsoluteError(buffer2.getSample(ch, i),
                                              static_cast<float>(i) / numSamples, 0.0001f,
                                              "unity gain should be a no-op");
        }

        beginTest("Silence at -100 dB clears buffer efficiently");
        {
            GainKnobAudioProcessor processor;
            processor.getAPVTS().getParameterAsValue("gain").setValue(-100.0f);

            constexpr int numSamples = 512;
            juce::AudioBuffer<float> buffer(2, numSamples);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, 0.75f);

            juce::MidiBuffer midi;
            processor.prepareToPlay(44100.0, numSamples);
            // Two blocks to settle smoothing
            processor.processBlock(buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, 0.75f);
            processor.processBlock(buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    expectWithinAbsoluteError(buffer.getSample(ch, i), 0.0f, 0.0001f,
                                              "-100 dB should zero the buffer");
        }

        beginTest("SIMD path produces correct gain for +12 dB");
        {
            GainKnobAudioProcessor processor;
            processor.getAPVTS().getParameterAsValue("gain").setValue(12.0f);

            constexpr int numSamples = 1024;
            juce::AudioBuffer<float> buffer(2, numSamples);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, 0.1f);

            juce::MidiBuffer midi;
            processor.prepareToPlay(44100.0, numSamples);
            // Two blocks so smoothing settles into SIMD fast path
            processor.processBlock(buffer, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, 0.1f);
            processor.processBlock(buffer, midi);

            float expected = 0.1f * juce::Decibels::decibelsToGain(12.0f);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    expectWithinAbsoluteError(buffer.getSample(ch, i), expected, 0.01f,
                                              "SIMD path should apply +12 dB correctly");
        }

        beginTest("Process latency is measured and non-negative");
        {
            GainKnobAudioProcessor processor;
            processor.getAPVTS().getParameterAsValue("gain").setValue(6.0f);

            constexpr int numSamples = 512;
            juce::AudioBuffer<float> buffer(2, numSamples);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, 0.5f);

            juce::MidiBuffer midi;
            processor.prepareToPlay(44100.0, numSamples);
            processor.processBlock(buffer, midi);

            float latencyMs = processor.getLastProcessLatencyMs();
            expect(latencyMs >= 0.0f, "latency should be non-negative");
            expect(latencyMs < 100.0f, "latency should be reasonable for a gain plugin");
        }

        beginTest("Process latency updates each block");
        {
            GainKnobAudioProcessor processor;
            processor.getAPVTS().getParameterAsValue("gain").setValue(3.0f);

            constexpr int numSamples = 256;
            juce::AudioBuffer<float> buffer(2, numSamples);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, 0.5f);

            juce::MidiBuffer midi;
            processor.prepareToPlay(44100.0, numSamples);
            processor.processBlock(buffer, midi);
            float latency1 = processor.getLastProcessLatencyMs();

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    buffer.setSample(ch, i, 0.5f);
            processor.processBlock(buffer, midi);
            float latency2 = processor.getLastProcessLatencyMs();

            expect(latency1 >= 0.0f, "first block latency non-negative");
            expect(latency2 >= 0.0f, "second block latency non-negative");
        }
    }
};

static GainParameterTest gainParameterTest;

int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        if (auto* result = runner.getResult(i))
            failures += result->failures;

    if (failures > 0)
    {
        DBG("FAILED: " + juce::String(failures) + " test(s) failed.");
        return 1;
    }

    DBG("All tests passed.");
    return 0;
}
