#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float pi = 3.14159265358979323846f;
    float getParam(juce::AudioProcessorValueTreeState& p, const char* id)
    { return p.getRawParameterValue(id)->load(); }
    struct Coeff { float b0, b1, b2, a1, a2; };
    Coeff highPass(float fs, float f)
    {
        const float w=2.0f*pi*f/fs, c=std::cos(w), s=std::sin(w), a=s/(2.0f*0.7071f), a0=1.0f+a;
        return {(1+c)*0.5f/a0, -(1+c)/a0, (1+c)*0.5f/a0, -2*c/a0, (1-a)/a0};
    }
    Coeff lowPass(float fs, float f)
    {
        const float w=2.0f*pi*f/fs, c=std::cos(w), s=std::sin(w), a=s/(2.0f*0.7071f), a0=1.0f+a;
        return {(1-c)*0.5f/a0, (1-c)/a0, (1-c)*0.5f/a0, -2*c/a0, (1-a)/a0};
    }
    float filter(float x, const Coeff& c, float& z1, float& z2)
    {
        const float y=c.b0*x+z1;
        z1=c.b1*x-c.a1*y+z2;
        z2=c.b2*x-c.a2*y;
        return y;
    }
}

WoTRadioVoiceAudioProcessor::WoTRadioVoiceAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout()) {}

WoTRadioVoiceAudioProcessor::APVTS::ParameterLayout WoTRadioVoiceAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout p;
    p.add(std::make_unique<juce::AudioParameterBool>("enabled", "Effect On", true));
    p.add(std::make_unique<juce::AudioParameterFloat>("distortion", "Distortion", juce::NormalisableRange<float>(0,100,0.1f), 28, "%"));
    p.add(std::make_unique<juce::AudioParameterFloat>("tone", "Radio Tone", juce::NormalisableRange<float>(0,100,0.1f), 58, "%"));
    p.add(std::make_unique<juce::AudioParameterFloat>("noise", "Noise", juce::NormalisableRange<float>(0,100,0.1f), 5, "%"));
    p.add(std::make_unique<juce::AudioParameterFloat>("crackle", "Crackle", juce::NormalisableRange<float>(0,100,0.1f), 2, "%"));
    p.add(std::make_unique<juce::AudioParameterFloat>("compression", "Compression", juce::NormalisableRange<float>(0,100,0.1f), 62, "%"));
    p.add(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", juce::NormalisableRange<float>(0,100,0.1f), 100, "%"));
    p.add(std::make_unique<juce::AudioParameterFloat>("output", "Output", juce::NormalisableRange<float>(-18,6,0.1f), 0, "dB"));
    return p;
}

void WoTRadioVoiceAudioProcessor::prepareToPlay(double sampleRate, int)
{
    sr=sampleRate>1000 ? sampleRate : 44100;
    for (auto& s: state) s={};
    random.setSeedRandomly();
}

bool WoTRadioVoiceAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto in=l.getMainInputChannelSet(), out=l.getMainOutputChannelSet();
    return (in==juce::AudioChannelSet::mono() || in==juce::AudioChannelSet::stereo()) && in==out;
}

void WoTRadioVoiceAudioProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals guard;
    if (!getParam(parameters,"enabled")) return;
    const int channels=juce::jmin(2,b.getNumChannels()), n=b.getNumSamples();
    const float dist=getParam(parameters,"distortion")*0.01f;
    const float tone=getParam(parameters,"tone")*0.01f;
    const float noise=getParam(parameters,"noise")*0.01f;
    const float crackle=getParam(parameters,"crackle")*0.01f;
    const float comp=getParam(parameters,"compression")*0.01f;
    const float mix=getParam(parameters,"mix")*0.01f;
    const float out=juce::Decibels::decibelsToGain(getParam(parameters,"output"));
    const float fs=(float)sr;
    const auto hp=highPass(fs,juce::jmin(260.0f+tone*180.0f,fs*0.2f));
    const auto lp=lowPass(fs,juce::jmin(2850.0f+tone*550.0f,fs*0.4f));
    const float drive=1.0f+dist*7.0f, norm=std::tanh(drive);
    const float attack=std::exp(-1.0f/(0.004f*fs)), release=std::exp(-1.0f/(0.105f*fs));
    const float threshold=0.22f-comp*0.10f, amount=0.35f+comp*2.2f;
    const int steps=4096-(int)(dist*2600.0f), decim=1+(int)(dist*2.0f);
    for (int ch=0; ch<channels; ++ch)
    {
        auto* d=b.getWritePointer(ch); auto& s=state[(size_t)ch];
        for (int i=0;i<n;++i)
        {
            const float dry=d[i];
            float x=filter(dry,hp,s.hpZ1,s.hpZ2);
            x=filter(x,lp,s.lpZ1,s.lpZ2);
            x=std::tanh(x*drive)/juce::jmax(0.001f,norm);
            if (++s.holdCounter>=decim) { s.holdCounter=0; s.held=std::round(x*(float)steps)/(float)steps; }
            x=s.held;
            const float level=std::abs(x), coeff=level>s.envelope?attack:release;
            s.envelope=coeff*s.envelope+(1.0f-coeff)*level;
            const float over=juce::jmax(1.0f,s.envelope/threshold);
            x*=std::pow(over,-amount);
            const float gate=juce::jlimit(0.0f,1.0f,s.envelope*10.0f);
            const float hiss=(random.nextFloat()*2.0f-1.0f)*noise*0.012f*gate;
            const bool click=random.nextFloat()<crackle*0.000018f;
            const float pop=click?(random.nextFloat()*2.0f-1.0f)*crackle*0.035f:0.0f;
            d[i]=dry*(1.0f-mix)+(x+hiss+pop)*out*mix;
        }
    }
}

juce::AudioProcessorEditor* WoTRadioVoiceAudioProcessor::createEditor() { return new WoTRadioVoiceAudioProcessorEditor(*this); }
void WoTRadioVoiceAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{ auto copy=parameters.copyState(); std::unique_ptr<juce::XmlElement> xml(copy.createXml()); copyXmlToBinary(*xml,dest); }
void WoTRadioVoiceAudioProcessor::setStateInformation(const void* data,int size)
{ std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data,size)); if(xml && xml->hasTagName(parameters.state.getType())) parameters.replaceState(juce::ValueTree::fromXml(*xml)); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WoTRadioVoiceAudioProcessor(); }
