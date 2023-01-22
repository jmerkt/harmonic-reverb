#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
        mParameters (*this, nullptr, juce::Identifier ("HarmonicReverb"), 
        {
            std::make_unique<juce::AudioParameterFloat> ("attack", "Attack", std::get<0>(AttackRange), std::get<1>(AttackRange), std::get<2>(AttackRange)),
            std::make_unique<juce::AudioParameterFloat> ("decay", "Decay", std::get<0>(DecayRange), std::get<1>(DecayRange), std::get<2>(DecayRange)),
            std::make_unique<juce::AudioParameterFloat> ("tuning", "Tuning", std::get<0>(TuningRange), std::get<1>(TuningRange), std::get<2>(TuningRange)),
            std::make_unique<juce::AudioParameterFloat> ("octaveOffset", "OctaveOffset", std::get<0>(OctaveOffsetRange), std::get<1>(OctaveOffsetRange), std::get<2>(OctaveOffsetRange)),
            std::make_unique<juce::AudioParameterFloat> ("octaveMix", "OctaveMix", std::get<0>(OctaveMixRange), std::get<1>(OctaveMixRange), std::get<2>(OctaveMixRange)),
            std::make_unique<juce::AudioParameterFloat> ("gain", "Gain", std::get<0>(GainRange), std::get<1>(GainRange), std::get<2>(GainRange)),
            std::make_unique<juce::AudioParameterFloat> ("mix", "Mix", std::get<0>(MixRange), std::get<1>(MixRange), std::get<2>(MixRange)),
            std::make_unique<juce::AudioParameterFloat> ("master", "Master", std::get<0>(MasterRange), std::get<1>(MasterRange), std::get<2>(MasterRange)),
            std::make_unique<juce::AudioParameterFloat> ("colour", "Colour", std::get<0>(ColourRange), std::get<1>(ColourRange), std::get<2>(ColourRange)),
            std::make_unique<juce::AudioParameterFloat> ("sparsity", "Sparsity", std::get<0>(SparsityRange), std::get<1>(SparsityRange), std::get<2>(SparsityRange)),
        })
{
    mAttackParameter = dynamic_cast<juce::AudioParameterFloat*>(mParameters.getParameter("attack"));
    mDecayParameter = dynamic_cast<juce::AudioParameterFloat*>(mParameters.getParameter("decay"));
    mOctaveOffsetParameter = dynamic_cast<juce::AudioParameterFloat*>(mParameters.getParameter("octaveOffset"));
    mOctaveMixParameter = dynamic_cast<juce::AudioParameterFloat*>(mParameters.getParameter("octaveMix"));
    mTuningParameter = dynamic_cast<juce::AudioParameterFloat*>(mParameters.getParameter("tuning"));
    mColourParameter = dynamic_cast<juce::AudioParameterFloat*>(mParameters.getParameter("colour"));
    mSparsityParameter = dynamic_cast<juce::AudioParameterFloat*>(mParameters.getParameter("sparsity"));
    mGainParameter = dynamic_cast<juce::AudioParameterFloat*>(mParameters.getParameter("gain"));
    mMixParameter = dynamic_cast<juce::AudioParameterFloat*>(mParameters.getParameter("mix"));
    mMasterParameter = dynamic_cast<juce::AudioParameterFloat*>(mParameters.getParameter("master"));
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

bool AudioPluginAudioProcessor::supportsDoublePrecisionProcessing () const
{
    return false;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // mutex

    for(int c = 0; c < 2; c++)
    {
        mCqtReverb[c].init(sampleRate, samplesPerBlock);
        mCqtSampleBuffer[c].resize(samplesPerBlock, 0.);
    }
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i_channel = totalNumInputChannels; i_channel < totalNumOutputChannels; ++i_channel)
        buffer.clear (i_channel, 0, buffer.getNumSamples());


    const double gain = *mGainParameter;
    const double mix = *mMixParameter;
    const double master = *mMasterParameter;

    auto* channelDataL = buffer.getWritePointer (0);
    auto* channelDataR = buffer.getWritePointer (1); 
    for(int i_sample = 0; i_sample < buffer.getNumSamples(); i_sample++)
    {
        mCqtSampleBuffer[0][i_sample] = static_cast<double>(channelDataL[i_sample]) * gain;
        mCqtSampleBuffer[1][i_sample] = static_cast<double>(channelDataR[i_sample]) * gain;
    }

    for(unsigned i_channel = 0u; i_channel < ChannelNumber; i_channel++)
    {
        mCqtReverb[i_channel].processBlock(mCqtSampleBuffer[i_channel].data(), buffer.getNumSamples());
    }
    for(int i_sample = 0; i_sample < buffer.getNumSamples(); i_sample++)
    {
        channelDataL[i_sample] = (mix * mCqtSampleBuffer[0][i_sample] + (1. - mix) * channelDataL[i_sample]) * master;
        channelDataR[i_sample] = (mix * mCqtSampleBuffer[1][i_sample] + (1. - mix) * channelDataR[i_sample]) * master;
    }

}

// void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer,
//                                               juce::MidiBuffer& midiMessages)
// {
//     juce::ignoreUnused (midiMessages);

//     juce::ScopedNoDenormals noDenormals;
//     auto totalNumInputChannels  = getTotalNumInputChannels();
//     auto totalNumOutputChannels = getTotalNumOutputChannels();
    
//     for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
//         buffer.clear (i, 0, buffer.getNumSamples());


// }

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this, mParameters);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = mParameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
 
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (mParameters.state.getType()))
            mParameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}

//==============================================================================
void AudioPluginAudioProcessor::setAttack(const double attack)
{
    *mAttackParameter = attack;
    for(unsigned i_channel = 0u; i_channel < ChannelNumber; i_channel++)
    {
        mCqtReverb[i_channel].setAttack(attack);
    }
}

void AudioPluginAudioProcessor::setDecay(const double decay)
{
    *mDecayParameter = decay;
    for(unsigned i_channel = 0u; i_channel < ChannelNumber; i_channel++)
    {
        mCqtReverb[i_channel].setDecay(decay);
    }
}

void AudioPluginAudioProcessor::setOctaveOffset(const double octaveOffset)
{
    *mOctaveOffsetParameter = octaveOffset;
    for(unsigned i_channel = 0u; i_channel < ChannelNumber; i_channel++)
    {
        mCqtReverb[i_channel].setOctaveOffset(octaveOffset);
    }
}

void AudioPluginAudioProcessor::setOctaveMix(const double octaveMix)
{
    *mOctaveMixParameter = octaveMix;
    for(unsigned i_channel = 0u; i_channel < ChannelNumber; i_channel++)
    {
        mCqtReverb[i_channel].setOctaveMix(octaveMix);
    }
}

void AudioPluginAudioProcessor::setSparsity(const double sparsity)
{
    *mSparsityParameter = sparsity;
    for(unsigned i_channel = 0u; i_channel < ChannelNumber; i_channel++)
    {
        mCqtReverb[i_channel].setSparsity(sparsity);
    }
}

void AudioPluginAudioProcessor::setTuning(const double tuning)
{
    *mTuningParameter = tuning;
    for(unsigned i_channel = 0u; i_channel < ChannelNumber; i_channel++)
    {
        mCqtReverb[i_channel].setTuning(tuning);
    }
}

void AudioPluginAudioProcessor::setGain(const double gain)
{
   *mGainParameter = gain; 
}

void AudioPluginAudioProcessor::setMix(const double mix)
{
   *mMixParameter = mix; 
}

void AudioPluginAudioProcessor::setMaster(const double master)
{
   *mMasterParameter = master; 
}

void AudioPluginAudioProcessor::setColour(const double colour)
{
   *mColourParameter = colour; 
}

//==============================================================================


