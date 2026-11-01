#pragma once
#include <JuceHeader.h>
#include <array>

class WoTRadioVoiceAudioProcessor : public juce::AudioProcessor
{
public:
    WoTRadioVoiceAudioProcessor();
    ~WoTRadioVoiceAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "WoT Radio Voice"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.15; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;

private:
    struct ChannelState
    {
        float hpIn = 0.0f, hpOut = 0.0f, lpOut = 0.0f;
        float envelope = 0.0f;
        float held = 0.0f;
        int holdCounter = 0;
    };
    std::array<ChannelState, 2> state {};
    double sr = 44100.0;
    juce::Random random;

    static APVTS::ParameterLayout createParameterLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WoTRadioVoiceAudioProcessor)
};
