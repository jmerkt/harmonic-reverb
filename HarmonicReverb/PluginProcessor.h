#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../include/CqtReverb.h"
#include "../submodules/audio-utils/include/SmoothedFloat.h"

constexpr unsigned BinsPerOctave{12};
constexpr unsigned OctaveNumber{9};
constexpr unsigned ChannelNumber{2};

// min, max, default
constexpr std::tuple<float, float, float> AttackRange{0.f, 1.f, 0.25f};
constexpr std::tuple<float, float, float> DecayRange{0.f, 1.f, 0.5f};
constexpr std::tuple<float, float, float> OctaveShiftRange{-3.f, 3.f, 1.f};
constexpr std::tuple<float, float, float> OctaveMixRange{0.f, 1.f, 0.3f};
constexpr std::tuple<float, float, float> ColourRange{-1.f, 1.f, 0.0f};
constexpr std::tuple<float, float, float> SparsityRange{0.f, 10.f, 1.0f};
constexpr std::tuple<float, float, float> TuningRange{415.305f, 466.164f, 440.f};
constexpr std::tuple<float, float, float> GainRange{-20.f, 20.f, 0.f};
constexpr std::tuple<float, float, float> MixRange{0.f, 1.f, 0.3f};
constexpr std::tuple<float, float, float> MasterRange{-20.f, 20.f, 0.f};

// TODO:
//  - Smoothed parameters

//==============================================================================
class AudioPluginAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    // void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    bool supportsDoublePrecisionProcessing() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    //==============================================================================
    double mCqtDataStorage[OctaveNumber][BinsPerOctave]; // For spectral display
    double mKernelFreqs[OctaveNumber][BinsPerOctave];    // For spectral display
    bool mNewKernelFreqs{false};                         // For spectral display

    void setAttack(const double attack);
    void setDecay(const double decay);
    void setOctaveShift(const double octaveShift);
    void setOctaveMix(const double octaveMix);
    void setGain(const double gain);
    void setMix(const double mix);
    void setMaster(const double master);
    void setColour(const double colour);
    void setSparsity(const double sparsity);
    void setTuning(const double tuning);

private:
    //==============================================================================
    std::vector<double> mCqtSampleBuffer[2];
    CqtReverb<BinsPerOctave, OctaveNumber> mCqtReverb[2];

    juce::AudioProcessorValueTreeState mParameters;
    juce::AudioParameterFloat *mAttackParameter{nullptr};
    juce::AudioParameterFloat *mDecayParameter{nullptr};
    juce::AudioParameterFloat *mOctaveShiftParameter{nullptr};
    juce::AudioParameterFloat *mOctaveMixParameter{nullptr};
    juce::AudioParameterFloat *mGainParameter{nullptr};
    juce::AudioParameterFloat *mMixParameter{nullptr};
    juce::AudioParameterFloat *mMasterParameter{nullptr};
    juce::AudioParameterFloat *mColourParameter{nullptr};
    juce::AudioParameterFloat *mSparsityParameter{nullptr};
    juce::AudioParameterFloat *mTuningParameter{nullptr};

    audio_utils::SmoothedFloat<double> mGain;
    audio_utils::SmoothedFloat<double> mMaster;
    audio_utils::SmoothedFloat<double> mWet;
    audio_utils::SmoothedFloat<double> mDry;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
