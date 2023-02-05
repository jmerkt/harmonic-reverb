#pragma once

#include "../../submodules/cqt-analyzer/include/gui/MagnitudesComponent.h"



template <int B, int OctaveNumber>
class SpectralComponent    : public juce::Component, public juce::Timer
{
public:
    SpectralComponent(AudioPluginAudioProcessor& p):
	processorRef (p)
    {
		// create level meters
		const float colorFadeIncr = 1.f / (static_cast<float>(OctaveNumber * B));
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
                addAndMakeVisible(mMagnitudeMeters[octave][tone]);
                mMagnitudeMeters[octave][tone].setColour(juce::Colour{static_cast<float>(octave * B + tone) * colorFadeIncr, 0.98, 0.725, 1.f});
			}
		}

		// timer
		startTimer(15);
    }

	~SpectralComponent()
	{
		//stopTimer();
	}

    void paint (juce::Graphics& g) override
    {
        g.fillAll (mBackgroundColor);

        auto bounds = getLocalBounds().toFloat();

		// meters
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
                mMagnitudeMeters[octave][tone].paint(g);
			}
		}
    }

    void resized() override
    {
		auto meterRect = getLocalBounds().toFloat();
		const float barWidth = meterRect.getWidth() / static_cast<float>(OctaveNumber * B);
		meterRect = meterRect.withTrimmedRight(meterRect.getWidth() - barWidth);
		// create level meters
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
                mMagnitudeMeters[octave][tone].setBounds(meterRect.toNearestIntEdges());
				meterRect.translate(meterRect.getWidth(), 0.f);
			}
		}
    }

	void timerCallback() override
	{
		// Get maximum value
		// Then adapt maximum in plot automatically to data maximum
		// Minimum can be statically set very low
		mMagMaxPrev = mMagMax;
		mMagMax = mMagMin;
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
				const double value = processorRef.mCqtDataStorage[octave][tone];
				const double magLog = juce::Decibels::gainToDecibels(value);
				if(magLog > mMagMax)
					mMagMax = magLog;
			}
		}
		mMagMax = mMagMax <= mMagMin ? mMagMin + 1.0 : mMagMax; 
		mOneDivMaxMin = 1. / (mMagMax - mMagMin);

		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
				const double value = processorRef.mCqtDataStorage[octave][tone];
				double magLog = juce::Decibels::gainToDecibels(value);
				magLog = Cqt::Clip<double>(magLog, mMagMin, mMagMax);
				double magLogMapped = 1. - ((mMagMax - magLog) * mOneDivMaxMin);
				magLogMapped = Cqt::Clip<double>(magLogMapped, 0., 1.);
				mMagnitudeMeters[OctaveNumber - octave - 1][tone].setValue(magLogMapped);
			}
		}
		if(processorRef.mNewKernelFreqs)
		{
			for (int octave = 0; octave < OctaveNumber; octave++) 
			{
				for (int tone = 0; tone < B; tone++) 
				{
					mMagnitudeMeters[OctaveNumber - octave - 1][tone].setFrequency(processorRef.mKernelFreqs[octave][tone]);
				}
			}
			processorRef.mNewKernelFreqs = false;
		}
		repaint();
	}

	void remapValues()
	{
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
				const double magLogMapped = mMagnitudeMeters[OctaveNumber - octave - 1][tone].getValue();
				const double magLog = magLogMapped * (mMagMaxPrev - mMagMinPrev) + mMagMinPrev;
				if((magLog > mMagMin) && (magLog < mMagMax) && (magLog > mMagMinPrev))
				{
					const double magLogRemapped = 1. - ((mMagMax - magLog) * mOneDivMaxMin);
					mMagnitudeMeters[OctaveNumber - octave - 1][tone].setValueHard(magLogRemapped);
				}
				else if(magLog > mMagMax)
				{
					mMagnitudeMeters[OctaveNumber - octave - 1][tone].setValueHard(1.);
				}
				else
				{
					mMagnitudeMeters[OctaveNumber - octave - 1][tone].setValueHard(0.);
				}	
			}
		}
	}

	void setRangeMin(const double rangeMin)
	{
		if(static_cast<int>(rangeMin) != static_cast<int>(mMagMax))
		{
			mMagMinPrev = mMagMin;
			mMagMin = rangeMin;
			mOneDivMaxMin = 1. / (mMagMax - mMagMin);
			remapValues();
			repaint();
		}	
	}

	void setTuning(const double tuning)
	{
		mTuning = tuning;
		repaint();
	}

	void setSmoothing(const double smoothing)
	{
		const double smoothingClipped = Cqt::Clip<double>(smoothing, 0., 0.999999);
		const double smoothingUp = smoothingClipped;
		const double smoothingDown = (1. - (1. - smoothingClipped) * 0.5);
		for (int octave = 0; octave < OctaveNumber; octave++) 
		{
			for (int tone = 0; tone < B; tone++) 
			{
				mMagnitudeMeters[octave][tone].setSmoothing(smoothingUp, smoothingDown);
			}
		}
	}

	void setSmoothing(const double smoothingUp, const double smoothingDown)
	{
		for (int octave = 0; octave < OctaveNumber; octave++) 
			{
				for (int tone = 0; tone < B; tone++) 
				{
					mMagnitudeMeters[octave][tone].setSmoothing(smoothingUp, smoothingDown);
				}
			}
	}
private:
	AudioPluginAudioProcessor& processorRef;

    juce::Colour mBackgroundColor{juce::Colours::black};
    juce::Colour mMeterColour{juce::Colours::blue};
	MagnitudeMeter mMagnitudeMeters[OctaveNumber][B];
	double mMagMin{ -50. };
	double mMagMax{ 0. };
	double mMagMinPrev{ -50. };
	double mMagMaxPrev{ 0. };
	double mTuning{ 440. };
	double mOneDivMaxMin{ 1. };
};
