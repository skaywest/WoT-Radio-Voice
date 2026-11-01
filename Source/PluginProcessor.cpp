#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float pi = 3.14159265358979323846f;
    float getParam(juce::AudioProcessorValueTreeState& p, const char* id)
    {
        return p.getRawParameterValue(id)->load();
    }
}

WoTRadioVoiceAudioProcessor::WoTRadioVoiceAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

WoTRadioVoiceAudioProcessor::APVTS::ParameterLayout
WoTRadioVoiceAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterBool>("enabled", "Effect On", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>("distortion", "Distortion",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 72.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("tone", "Radio Tone",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 75.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("noise", "Noise",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 10.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("crackle", "Crackle",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 8.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("compression", "Compression",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 78.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("output", "Output",
        juce::NormalisableRange<float>(-18.0f, 6.0f, 0.1f), -5.0f, "dB"));
    return layout;
}

void WoTRadioVoiceAudioProcessor::prepareToPlay(double sampleRate, int)
{
    sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
    for (auto& s : state) s = {};
    random.setSeedRandomly();
}

bool WoTRadioVoiceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && in == out;
}

void WoTRadioVoiceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (! getParam(parameters, "enabled")) return;

    const float distortion = getParam(parameters, "distortion") / 100.0f;
    const float tone = getParam(parameters, "tone") / 100.0f;
    const float noise = getParam(parameters, "noise") / 100.0f;
    const float crackle = getParam(parameters, "crackle") / 100.0f;
    const float compression = getParam(parameters, "compression") / 100.0f;
    const float mix = getParam(parameters, "mix") / 100.0f;
    const float outputGain = juce::Decibels::decibelsToGain(getParam(parameters, "output"));

    // A deliberately narrow, unmistakable two-way-radio voice band.
    const float lowHz = 420.0f + tone * 260.0f;
    const float highHz = 1450.0f + tone * 950.0f;
    const float hpA = std::exp(-2.0f * pi * lowHz / static_cast<float>(sr));
    const float lpA = 1.0f - std::exp(-2.0f * pi * highHz / static_cast<float>(sr));
    const float drive = 1.0f + distortion * 34.0f;
    const float norm = std::tanh(drive);
    const float attack = std::exp(-1.0f / (0.0025f * static_cast<float>(sr)));
    const float release = std::exp(-1.0f / (0.075f * static_cast<float>(sr)));
    const int decimation = 1 + static_cast<int>(distortion * 5.0f);
    const int bitSteps = 2048 - static_cast<int>(distortion * 1900.0f);

    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto& s = state[static_cast<size_t>(ch)];
        for (int i = 0; i < samples; ++i)
        {
            const float dry = data[i];
            const float hp = hpA * (s.hpOut + dry - s.hpIn);
            s.hpIn = dry;
            s.hpOut = hp;
            s.lpOut += lpA * (hp - s.lpOut);
            float x = s.lpOut;

            // Radio transmitter saturation and deliberately coarse digital texture.
            x = std::tanh(x * drive) / juce::jmax(0.01f, norm);
            if (++s.holdCounter >= decimation)
            {
                s.holdCounter = 0;
                s.held = std::round(x * static_cast<float>(bitSteps)) / static_cast<float>(bitSteps);
            }
            x = s.held;

            const float level = std::abs(x);
            const float coeff = level > s.envelope ? attack : release;
            s.envelope = coeff * s.envelope + (1.0f - coeff) * level;
            const float threshold = 0.20f;
            const float over = juce::jmax(1.0f, s.envelope / threshold);
            x *= std::pow(over, -(0.25f + compression * 1.8f));

            // Audible but controllable receiver hiss and occasional static ticks.
            const float hiss = (random.nextFloat() * 2.0f - 1.0f) * noise * 0.055f;
            const bool click = random.nextFloat() < (crackle * 0.00012f);
            const float pop = click ? (random.nextFloat() * 2.0f - 1.0f) * crackle * 0.30f : 0.0f;
            x += hiss + pop;
            const float wet = x * outputGain;
            data[i] = dry * (1.0f - mix) + wet * mix;
        }
    }
}


juce::AudioProcessorEditor* WoTRadioVoiceAudioProcessor::createEditor()
{
    return new WoTRadioVoiceAudioProcessorEditor(*this);
}

void WoTRadioVoiceAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto stateCopy = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(stateCopy.createXml());
    copyXmlToBinary(*xml, dest);
}

void WoTRadioVoiceAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WoTRadioVoiceAudioProcessor();
}
