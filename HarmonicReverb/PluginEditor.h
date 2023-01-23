#pragma once

#include "PluginProcessor.h"

#include "../include/gui/OtherLookAndFeel.h"
#include "../submodules/cqt-analyzer/include/gui/MagnitudesComponent.h"

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

    juce::Label mAttackLabel;
    juce::Label mDecayLabel;
    juce::Label mTuningLabel;
    juce::Label mOctaveShiftLabel;
    juce::Label mOctaveMixLabel;
    juce::Label mColourLabel;
    juce::Label mSparsityLabel;
    juce::Label mGainLabel;
    juce::Label mMixLabel;
    juce::Label mMasterLabel;

    juce::Slider mAttackSlider;
    juce::Slider mDecaySlider;
    juce::Slider mTuningSlider;
    juce::Slider mOctaveShiftSlider;
    juce::Slider mOctaveMixSlider;
    juce::Slider mColourSlider;
    juce::Slider mSparsitySlider;
    juce::Slider mGainSlider;
    juce::Slider mMixSlider;
    juce::Slider mMasterSlider;

    OtherLookAndFeel mOtherLookAndFeel;

    juce::TooltipWindow mFrequencyTooltip;

    MagnitudesComponent<BinsPerOctave, OctaveNumber> mMagnitudesComponent{ processorRef };

    void attackSliderChanged();
    void decaySliderChanged();
    void octaveShiftSliderChanged();
    void octaveMixSliderChanged();
    void colourSliderChanged();
    void sparsitySliderChanged();
    void tuningSliderChanged();
    void gainSliderChanged();
    void mixSliderChanged();
    void masterSliderChanged();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
