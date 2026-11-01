#include "PluginEditor.h"

WoTRadioVoiceAudioProcessorEditor::WoTRadioVoiceAudioProcessorEditor(
    WoTRadioVoiceAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(720, 430);
    enabledButton.setClickingTogglesState(true);
    enabledAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.parameters, "enabled", enabledButton);
    addAndMakeVisible(enabledButton);

    for (size_t i = 0; i < sliders.size(); ++i)
    {
        auto& s = sliders[i];
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 86, 22);
        s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffd19a42));
        s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff454b50));
        s.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffd58a));
        addAndMakeVisible(s);

        labels[i].setText(names[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].setColour(juce::Label::textColourId, juce::Colour(0xffe5d5b7));
        labels[i].setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(labels[i]);

        attachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.parameters, ids[i], s));
    }
}

void WoTRadioVoiceAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff171b1e));
    g.setColour(juce::Colour(0xff292f33));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(10.0f), 12.0f);
    g.setColour(juce::Colour(0xffd19a42));
    g.setFont(juce::Font(25.0f, juce::Font::bold));
    g.drawText("WoT RADIO VOICE", 24, 16, 420, 36, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff9ca6aa));
    g.setFont(12.0f);
    g.drawText("CREW COMMUNICATION // VOICE PROCESSOR", 26, 49, 430, 20,
               juce::Justification::centredLeft);
}

void WoTRadioVoiceAudioProcessorEditor::resized()
{
    enabledButton.setBounds(545, 22, 145, 34);
    const int left = 22, top = 92, cellW = 168, cellH = 155;
    for (size_t i = 0; i < sliders.size(); ++i)
    {
        const int col = static_cast<int>(i % 4);
        const int row = static_cast<int>(i / 4);
        const int x = left + col * cellW;
        const int y = top + row * 160;
        labels[i].setBounds(x, y, 150, 22);
        sliders[i].setBounds(x + 15, y + 23, 120, 125);
    }
}
