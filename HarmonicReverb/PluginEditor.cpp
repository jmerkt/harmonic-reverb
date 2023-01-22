#include "PluginProcessor.h"
#include "PluginEditor.h"

constexpr float LabelSize{15.f};
constexpr float HeadingSize{30.f};
constexpr float WebsiteSize{16.f};

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), processorRef (p), mParameters (vts)
{
    juce::ignoreUnused (processorRef);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (PLUGIN_WIDTH, PLUGIN_HEIGHT);
    setResizable(true, true);

    // Global LookAndFeel
    setLookAndFeel (&mOtherLookAndFeel);

    // Labels
    addAndMakeVisible(mHeadingLabel);
    addAndMakeVisible(mVersionLabel);
    addAndMakeVisible(mWebsiteLabel);

    mHeadingLabel.setText("HarmonicReverb", juce::dontSendNotification);
    mVersionLabel.setText("Version 2.0.0", juce::dontSendNotification);
    mWebsiteLabel.setText("www.ChromaDSP.com", juce::dontSendNotification);

    mHeadingLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mVersionLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    mWebsiteLabel.setColour (juce::Label::textColourId, juce::Colours::white);

    mHeadingLabel.setColour (juce::Label::backgroundColourId, juce::Colours::black);

    mHeadingLabel.setJustificationType (juce::Justification::centred);
    mVersionLabel.setJustificationType (juce::Justification::left);
    mWebsiteLabel.setJustificationType (juce::Justification::right);

    // Get parameters saved in processor
    const float attackParameter = mParameters.getParameterAsValue("attack").getValue();
    const float decayParameter = mParameters.getParameterAsValue("decay").getValue();
    const float octaveShiftParameter = mParameters.getParameterAsValue("octaveShift").getValue();
    const float octaveMixParameter = mParameters.getParameterAsValue("octaveMix").getValue();
    const float tuningParameter = mParameters.getParameterAsValue("tuning").getValue();
    const float colourParameter = mParameters.getParameterAsValue("colour").getValue();
    const float sparsityParameter = mParameters.getParameterAsValue("sparsity").getValue();
    const float mixParameter = mParameters.getParameterAsValue("mix").getValue();
    const float gainParameter = mParameters.getParameterAsValue("gain").getValue();
    const float masterParameter = mParameters.getParameterAsValue("master").getValue();

    // Controls
    addAndMakeVisible(mAttackLabel);
    addAndMakeVisible(mDecayLabel);
    addAndMakeVisible(mOctaveShiftLabel);
    addAndMakeVisible(mOctaveMixLabel);
    addAndMakeVisible(mColourLabel);
    addAndMakeVisible(mSparsityLabel);
    addAndMakeVisible(mTuningLabel);
    addAndMakeVisible(mGainLabel);
    addAndMakeVisible(mMixLabel);
    addAndMakeVisible(mMasterLabel);

    addAndMakeVisible(mAttackSlider);
    addAndMakeVisible(mDecaySlider);
    addAndMakeVisible(mOctaveShiftSlider);
    addAndMakeVisible(mOctaveMixSlider);
    addAndMakeVisible(mColourSlider);
    addAndMakeVisible(mSparsitySlider);
    addAndMakeVisible(mTuningSlider);
    addAndMakeVisible(mGainSlider);
    addAndMakeVisible(mMixSlider);
    addAndMakeVisible(mMasterSlider);

    mAttackLabel.setText("Attack", juce::dontSendNotification);
    mDecayLabel.setText("Decay", juce::dontSendNotification);
    mOctaveShiftLabel.setText("OctaveShift", juce::dontSendNotification);
    mOctaveMixLabel.setText("OctaveMix", juce::dontSendNotification);
    mTuningLabel.setText("Tuning", juce::dontSendNotification);
    mColourLabel.setText("Colour", juce::dontSendNotification);
    mSparsityLabel.setText("Sparsity", juce::dontSendNotification);
    mGainLabel.setText("Gain", juce::dontSendNotification);
    mMixLabel.setText("Mix", juce::dontSendNotification);
    mMasterLabel.setText("Master", juce::dontSendNotification);

    mAttackSlider.setRange(std::get<0>(AttackRange), std::get<1>(AttackRange), 0.01);
    mAttackSlider.setValue(attackParameter, juce::dontSendNotification);
    mAttackSlider.setTextValueSuffix (" ms");
    mAttackSlider.onValueChange = [this]{attackSliderChanged();};
    attackSliderChanged();

    mDecaySlider.setRange(std::get<0>(DecayRange), std::get<1>(DecayRange), 0.01);
    mDecaySlider.setValue(decayParameter, juce::dontSendNotification);
    mDecaySlider.setTextValueSuffix (" ms");
    mDecaySlider.onValueChange = [this]{decaySliderChanged();};
    decaySliderChanged();

    mOctaveShiftSlider.setRange(std::get<0>(OctaveShiftRange), std::get<1>(OctaveShiftRange), 0.01);
    mOctaveShiftSlider.setValue(octaveShiftParameter, juce::dontSendNotification);
    mOctaveShiftSlider.setTextValueSuffix ("");
    mOctaveShiftSlider.onValueChange = [this]{octaveShiftSliderChanged();};
    octaveShiftSliderChanged();

    mOctaveMixSlider.setRange(std::get<0>(OctaveMixRange), std::get<1>(OctaveMixRange), 0.01);
    mOctaveMixSlider.setValue(octaveMixParameter, juce::dontSendNotification);
    mOctaveMixSlider.setTextValueSuffix ("%");
    mOctaveMixSlider.onValueChange = [this]{octaveMixSliderChanged();};
    octaveMixSliderChanged();

    mTuningSlider.setRange(std::get<0>(TuningRange), std::get<1>(TuningRange), 0.01);
    mTuningSlider.setValue(tuningParameter, juce::dontSendNotification);
    mTuningSlider.setTextValueSuffix (" Hz");
    mTuningSlider.onValueChange = [this]{tuningSliderChanged();};
    tuningSliderChanged();

    mColourSlider.setRange(std::get<0>(ColourRange), std::get<1>(ColourRange), 0.01);
    mColourSlider.setValue(colourParameter, juce::dontSendNotification);
    mColourSlider.setTextValueSuffix ("");
    mColourSlider.onValueChange = [this]{colourSliderChanged();};
    colourSliderChanged();

    mSparsitySlider.setRange(std::get<0>(SparsityRange), std::get<1>(SparsityRange), 0.01);
    mSparsitySlider.setValue(sparsityParameter, juce::dontSendNotification);
    mSparsitySlider.setTextValueSuffix ("");
    mSparsitySlider.onValueChange = [this]{sparsitySliderChanged();};
    sparsitySliderChanged();

    mGainSlider.setRange(std::get<0>(GainRange), std::get<1>(GainRange), 0.01);
    mGainSlider.setValue(gainParameter, juce::dontSendNotification);
    mGainSlider.setTextValueSuffix ("");
    mGainSlider.onValueChange = [this]{gainSliderChanged();};
    gainSliderChanged();

    mMixSlider.setRange(std::get<0>(MixRange), std::get<1>(MixRange), 0.01);
    mMixSlider.setValue(mixParameter, juce::dontSendNotification);
    mMixSlider.setTextValueSuffix ("%");
    mMixSlider.onValueChange = [this]{mixSliderChanged();};
    mixSliderChanged();

    mMasterSlider.setRange(std::get<0>(MasterRange), std::get<1>(MasterRange), 0.01);
    mMasterSlider.setValue(masterParameter, juce::dontSendNotification);
    mMasterSlider.setTextValueSuffix ("");
    mMasterSlider.onValueChange = [this]{masterSliderChanged();};
    masterSliderChanged();
    
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    //juce::Colour backgroundColour = juce::Colour::fromRGBA(7u, 13u, 255u, 63u );
    juce::Colour backgroundColour = juce::Colours::black;
    g.fillAll (backgroundColour);
}

void AudioPluginAudioProcessorEditor::resized()
{
    const float headingYFrac = 0.08f;
    auto b = getLocalBounds().toFloat();

    auto headingRect = b.withTrimmedTop((1.f - headingYFrac) * b.getHeight());

    // Heading
    const float sideGap = 0.02f;
    mHeadingLabel.setBounds(headingRect.toNearestIntEdges());
    mVersionLabel.setBounds(headingRect.withTrimmedLeft(sideGap * headingRect.getWidth()).toNearestIntEdges());
    mWebsiteLabel.setBounds(headingRect.withTrimmedRight(sideGap * headingRect.getWidth()).toNearestIntEdges());

    const float labelScaling = 1.f / static_cast<float>(PLUGIN_HEIGHT) * b.getHeight();
    mHeadingLabel.setFont (juce::Font (HeadingSize * labelScaling, juce::Font::bold));
    mVersionLabel.setFont (juce::Font (WebsiteSize * labelScaling, juce::Font::bold));
    mWebsiteLabel.setFont (juce::Font (WebsiteSize * labelScaling, juce::Font::bold));

    // Controls
    constexpr size_t N_CONTROLS = 10u;
    constexpr size_t N_ROWS = 2u;
    constexpr size_t N_COLUMNS= N_CONTROLS / N_ROWS;
    const float controlY = 1.f - headingYFrac;
    const float nControls = static_cast<float>(N_CONTROLS);
    const float nControlRows = static_cast<float>(N_ROWS);
    const float nControlsPerRow = static_cast<float>(N_COLUMNS);
    const float xPerControl = b.getWidth() / nControlsPerRow;
    const float yPerControl = (b.getHeight() - headingYFrac * b.getHeight()) / nControlRows;
    juce::Slider* sliderArray[N_CONTROLS] = {&mAttackSlider, &mDecaySlider, &mTuningSlider, &mColourSlider, &mMixSlider,
                                                &mOctaveShiftSlider, &mOctaveMixSlider, &mSparsitySlider, &mGainSlider, &mMasterSlider};
    juce::Label* labelArray[N_CONTROLS] = {&mAttackLabel, &mDecayLabel, &mTuningLabel, &mColourLabel, &mMixLabel,
                                                &mOctaveShiftLabel, &mOctaveMixLabel, &mSparsityLabel, &mGainLabel, &mMasterLabel};

    size_t count = 0u;
    for(size_t row = 0u; row < N_ROWS; row++)
    {
        for(size_t column = 0u; column < N_COLUMNS; column++)
        {
            auto controlB = b;
            controlB.setLeft(static_cast<float>(column) * xPerControl);
            controlB.setRight(static_cast<float>(column + 1) * xPerControl);
            controlB.setTop(static_cast<float>(row) * yPerControl);
            controlB.setBottom(static_cast<float>(row + 1) * yPerControl);

            auto labelB = controlB;
            auto sliderB = controlB;
            labelB = controlB.withTrimmedBottom(controlB.getHeight() / 2.f);
            sliderB = controlB.withTrimmedTop(controlB.getHeight() / 2.f);

            labelArray[count]->setBounds(labelB.toNearestIntEdges());
            sliderArray[count]->setBounds(sliderB.toNearestIntEdges());
            count++;
        }
    }
    
    
}

void AudioPluginAudioProcessorEditor::attackSliderChanged()
{
    processorRef.setAttack(mAttackSlider.getValue());
}

void AudioPluginAudioProcessorEditor::decaySliderChanged()
{
    processorRef.setDecay(mDecaySlider.getValue());
}

void AudioPluginAudioProcessorEditor::octaveShiftSliderChanged()
{
    processorRef.setOctaveShift(mOctaveShiftSlider.getValue());
}

void AudioPluginAudioProcessorEditor::octaveMixSliderChanged()
{
    processorRef.setOctaveMix(mOctaveMixSlider.getValue());
}

void AudioPluginAudioProcessorEditor::colourSliderChanged()
{
    processorRef.setColour(mColourSlider.getValue());
}

void AudioPluginAudioProcessorEditor::sparsitySliderChanged()
{
    processorRef.setSparsity(mSparsitySlider.getValue());
}

void AudioPluginAudioProcessorEditor::tuningSliderChanged()
{
    processorRef.setTuning(mTuningSlider.getValue());
}

void AudioPluginAudioProcessorEditor::gainSliderChanged()
{
    processorRef.setGain(mGainSlider.getValue());
}

void AudioPluginAudioProcessorEditor::mixSliderChanged()
{
    processorRef.setMix(mMixSlider.getValue());
}

void AudioPluginAudioProcessorEditor::masterSliderChanged()
{
    processorRef.setMaster(mMasterSlider.getValue());
}



