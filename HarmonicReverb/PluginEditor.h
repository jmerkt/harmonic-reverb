#pragma once

#include "PluginProcessor.h"

#include "../include/gui/OtherLookAndFeel.h"

//==============================================================================
class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:

    AudioPluginAudioProcessor& processorRef;
    juce::AudioProcessorValueTreeState& mParameters;

    juce::Label mHeadingLabel;
    juce::Label mVersionLabel;
    juce::Label mWebsiteLabel;

    juce::Slider mAttackSlider;
    juce::Slider mDecaySlider;
    juce::Slider mTuningSlider;
    juce::Slider mOctaveOffsetSlider;
    juce::Slider mOctaveMixSlider;
    juce::Slider mColourSlider;
    juce::Slider mSparsitySlider;
    juce::Slider mGainSlider;
    juce::Slider mMixSlider;
    juce::Slider mMasterSlider;

    OtherLookAndFeel mOtherLookAndFeel;

    void attackSliderChanged();
    void decaySliderChanged();
    void octaveOffsetSliderChanged();
    void octaveMixSliderChanged();
    void colourSliderChanged();
    void sparsitySliderChanged();
    void tuningSliderChanged();
    void gainSliderChanged();
    void mixSliderChanged();
    void masterSliderChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
