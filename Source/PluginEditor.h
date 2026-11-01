#pragma once
#include "PluginProcessor.h"
#include <array>

class WoTRadioVoiceAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit WoTRadioVoiceAudioProcessorEditor(WoTRadioVoiceAudioProcessor&);
    ~WoTRadioVoiceAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    WoTRadioVoiceAudioProcessor& processor;
    juce::ToggleButton enabledButton { "RADIO ON" };
    std::array<juce::Slider, 7> sliders;
    std::array<juce::Label, 7> labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment;
    const std::array<const char*, 7> ids {
        "distortion", "tone", "noise", "crackle", "compression", "mix", "output"
    };
    const std::array<const char*, 7> names {
        "DISTORTION", "RADIO TONE", "NOISE", "CRACKLE", "COMPRESSION", "MIX", "OUTPUT"
    };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WoTRadioVoiceAudioProcessorEditor)
};
