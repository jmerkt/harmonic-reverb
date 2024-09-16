#pragma once

#include "../submodules/rt-cqt/include/SlidingCqt.h"
#include "../submodules/rt-cqt/submodules/audio-utils/include/SmoothedFloat.h"
#include "../submodules/rt-cqt/submodules/audio-utils/include/CplxWavetableOscillator.h"

using namespace std::complex_literals;
constexpr int BlockSize{256};
constexpr size_t WavetableSize{512u};

// Parameters later
constexpr double MaxToneThresholdFactor{0.05}; // sparsity
constexpr double GlobalMaxThresholdFactor{0.05};
constexpr double OctaveMeanThresholdFactor{.75}; // sparsity

template <unsigned B, unsigned OctaveNumber>
class CqtReverb
{
public:
    CqtReverb() = default;
    ~CqtReverb() = default;

    void init(const double samplerate, const int blockSize);

    void processBlock(double *const data, const int nSamples);

    const double *getOctaveValues(const int octave) { return mGainsIllustration[octave]; };
    inline double *getOctaveBinFreqs(const int octave) { return mCqt.getOctaveBinFreqs(octave); };

    void setAttack(const double attack);
    void setDecay(const double decay);
    void setTuning(const double tuning);
    void setOctaveShift(const double octaveShift);
    void setOctaveMix(const double octaveMix);
    void setColour(const double colour);
    void setSparsity(const double sparsity);

private:
    static constexpr double mOneDivB{1. / static_cast<double>(B)};

    // Processing classes and buffers
    Cqt::SlidingCqt<B, OctaveNumber, false> mCqt;

    audio_utils::CircularBuffer<double> mInputBuffer;
    audio_utils::CircularBuffer<double> mOutputBuffer;
    std::vector<double> mInputData;
    std::vector<double> mOutputData;
    size_t mInputDataCounter;
    size_t mOutputDataCounter;

    // SmoothedFloatUpDown<double, SmoothingTypes::Linear> mSmoothedFloats[OctaveNumber][B];
    audio_utils::OnePoleUpDown<double> mSmoothedFloats[OctaveNumber][B];

    double mCqtValues[OctaveNumber][B];
    std::vector<double> mModulationData[OctaveNumber][B];
    std::vector<double> mPhaseData[OctaveNumber][B];

    audio_utils::StaticCplxWavetable<WavetableSize> mStaticWavetable;
    audio_utils::CplxWavetableOscillator<WavetableSize> mOscillators[OctaveNumber][B];
    std::vector<std::complex<double>> mOscillatorBuffer[OctaveNumber][B];
    std::vector<std::complex<double>> mSynthBuffer[OctaveNumber][B];

    double mGainSum[OctaveNumber][B];
    double mGainSumShifted[OctaveNumber][B];
    double mGainSumMixed[OctaveNumber][B];
    double mGainsIllustration[OctaveNumber][B];

    audio_utils::SmoothedFloat<double> mBaseOctaveTracker;

    // Thresholding
    double mOctaveMean[OctaveNumber];
    double mOctaveMax[OctaveNumber];
    double mOctaveMeanCurrent[OctaveNumber];
    double mOctaveMaxCurrent[OctaveNumber];

    // Controlable parameters
    double mAttack{.25};
    double mDecay{0.05};
    double mTuning{440.};
    double mOctaveShift{1.};
    double mOctaveMix{0.3};
    double mColour{1.};
    double mSparsity{1.};

    // Octave shift
    int mLowerOctaveShift{0};
    int mHigherOctaveShift{0};
    double mLowerShiftFrac{0.};
    double mHigherShiftFrac{0.};
};

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::init(const double samplerate, const int nSamples)
{
    mCqt.init(samplerate, BlockSize);

    // buffers
    mInputBuffer.changeSize(nSamples + BlockSize);
    mOutputBuffer.changeSize(nSamples + BlockSize);
    mInputData.resize(BlockSize, 0.);
    mOutputData.resize(nSamples, 0.);
    mInputDataCounter = 0u;
    mOutputDataCounter = 0u;

    // smoothed values
    for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
    {
        const double octaveRate = mCqt.getOctaveSampleRate(i_octave);
        const int octaveSize = mCqt.getOctaveBlockSize(i_octave);
        const double *const binFreqs = mCqt.getOctaveBinFreqs(i_octave);
        for (unsigned i_tone = 0u; i_tone < B; i_tone++)
        {
            mSmoothedFloats[i_octave][i_tone].init(octaveRate);
            mSmoothedFloats[i_octave][i_tone].setSmoothingFactors(mAttack, mDecay);

            mOscillators[i_octave][i_tone].init(octaveRate, &mStaticWavetable);
            mOscillators[i_octave][i_tone].setFrequency(binFreqs[i_tone]);
            mOscillatorBuffer[i_octave][i_tone].resize(octaveSize, {0., 0.});
            mSynthBuffer[i_octave][i_tone].resize(octaveSize, {0., 0.});

            mModulationData[i_octave][i_tone].resize(octaveSize, 0.);
        }
    }
    const double blockRate = static_cast<double>(BlockSize) / samplerate;
    mBaseOctaveTracker.init(blockRate);
    mBaseOctaveTracker.setSmoothingTime(1000.);
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::processBlock(double *const data, const int nSamples)
{
    mInputBuffer.pushBlock(data, nSamples);
    mInputDataCounter += nSamples;
    while (mInputDataCounter >= BlockSize)
    {
        mInputBuffer.pullDelayBlock(mInputData.data(), mInputDataCounter - 1, BlockSize);
        mInputDataCounter -= BlockSize;
        mCqt.inputBlock(mInputData.data(), BlockSize);

        for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            CircularBuffer<std::complex<double>> *octaveCqtBuffer = mCqt.getOctaveCqtBuffer(i_octave);

            // acquire cqt values for feature calculations
            for (unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mCqtValues[i_octave][i_tone] = std::abs(octaveCqtBuffer[i_tone].pullDelaySample(0));
            }
        }
        for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            for (unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mGainSum[i_octave][i_tone] = 0.;
                mGainSumShifted[i_octave][i_tone] = 0.;
                mGainSumMixed[i_octave][i_tone] = 0.;
                mGainsIllustration[i_octave][i_tone] = 0.;
            }
        }

        // Determine current base (max) octave
        double maxOctaveValue = 0.;
        unsigned maxOctave = 0;
        for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            double octaveSum = 0.;
            for (unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                octaveSum += mSmoothedFloats[i_octave][i_tone].getCurrentValue();
            }
            if (octaveSum > maxOctaveValue)
            {
                maxOctaveValue = octaveSum;
                maxOctave = i_octave;
            }
        }
        mBaseOctaveTracker.setTargetValue(static_cast<double>(maxOctave));

        // Parameters for thresholding
        double globalMax = 0.;
        double globalMaxCurrent = 0.;
        for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            for (unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                if (mCqtValues[i_octave][i_tone] > globalMax)
                    globalMax = mCqtValues[i_octave][i_tone];
                if (mSmoothedFloats[i_octave][i_tone].getCurrentValue() > globalMaxCurrent)
                    globalMaxCurrent = mSmoothedFloats[i_octave][i_tone].getCurrentValue();
            }
        }
        for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            for (unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mOctaveMean[i_octave] += mCqtValues[i_octave][i_tone];
                mOctaveMeanCurrent[i_octave] += mSmoothedFloats[i_octave][i_tone].getCurrentValue();
            }
            mOctaveMean[i_octave] *= mOneDivB;
            mOctaveMeanCurrent[i_octave] *= mOneDivB;
        }
        for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            mOctaveMax[i_octave] = 0.;
            mOctaveMaxCurrent[i_octave] = 0.;
            for (unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                if (mCqtValues[i_octave][i_tone] > mOctaveMax[i_octave])
                    mOctaveMax[i_octave] = mCqtValues[i_octave][i_tone];
                if (mSmoothedFloats[i_octave][i_tone].getCurrentValue() > mOctaveMaxCurrent[i_octave])
                    mOctaveMaxCurrent[i_octave] = mSmoothedFloats[i_octave][i_tone].getCurrentValue();
            }
        }

        // Thresholding and summation of gains
        for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            const double threshold = mOctaveMax[i_octave] * MaxToneThresholdFactor * mSparsity;
            const double globalMaxThreshold = globalMax * GlobalMaxThresholdFactor * mSparsity;
            const double octaveMeanTreshold = mOctaveMean[i_octave] * OctaveMeanThresholdFactor * mSparsity;

            const double thresholdCurrent = mOctaveMaxCurrent[i_octave] * MaxToneThresholdFactor * mSparsity;
            const double globalMaxThresholdCurrent = globalMaxCurrent * GlobalMaxThresholdFactor * mSparsity;
            const double octaveMeanTresholdCurrent = mOctaveMeanCurrent[i_octave] * OctaveMeanThresholdFactor * mSparsity;

            for (unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                if (
                    mCqtValues[i_octave][i_tone] > threshold &&
                    mCqtValues[i_octave][i_tone] > globalMaxThreshold &&
                    mCqtValues[i_octave][i_tone] > octaveMeanTreshold &&
                    mCqtValues[i_octave][i_tone] > thresholdCurrent &&
                    mCqtValues[i_octave][i_tone] > globalMaxThresholdCurrent &&
                    mCqtValues[i_octave][i_tone] > octaveMeanTresholdCurrent)
                {
                    mGainSum[i_octave][i_tone] += mCqtValues[i_octave][i_tone];
                }
            }
        }

        // Octave shift and mixing
        for (int i_octave = 0; i_octave < OctaveNumber; i_octave++)
        {
            for (int i_tone = 0; i_tone < B; i_tone++)
            {
                mGainSumShifted[i_octave][i_tone] = 0.;
            }
        }
        for (int i_octave = 0; i_octave < OctaveNumber; i_octave++)
        {
            for (int i_tone = 0; i_tone < B; i_tone++)
            {
                const int shiftOctaveLow = Cqt::Clip<int>(i_octave + mLowerOctaveShift, 0, OctaveNumber - 1);
                const int shiftOctaveHigh = Cqt::Clip<int>(i_octave + mHigherOctaveShift, 0, OctaveNumber - 1);
                mGainSumShifted[i_octave][i_tone] += mGainSum[shiftOctaveLow][i_tone] * mLowerShiftFrac;
                mGainSumShifted[i_octave][i_tone] += mGainSum[shiftOctaveHigh][i_tone] * mHigherShiftFrac;
            }
        }
        for (int i_octave = 0; i_octave < OctaveNumber; i_octave++)
        {
            for (int i_tone = 0; i_tone < B; i_tone++)
            {
                mGainSumMixed[i_octave][i_tone] = mGainSum[i_octave][i_tone] * (1. - mOctaveMix) + mGainSumShifted[i_octave][i_tone] * mOctaveMix;
            }
        }

        // Apply color parameter equalization
        for (int i_octave = 0; i_octave < OctaveNumber; i_octave++)
        {
            const double baseOctave = mBaseOctaveTracker.getCurrentValue();
            const double octaveDouble = static_cast<double>(i_octave);
            const double octaveNumberDouble = static_cast<double>(OctaveNumber);
            double octaveFactor = 1.0;
            if (octaveDouble < baseOctave) // Smaller octaves are the higher ones
            {
                if (mColour > 0.)
                {
                    octaveFactor = 1.0 + std::abs(octaveDouble - baseOctave) / OctaveNumber * std::abs(mColour);
                }
                else
                {
                    octaveFactor = 1.0 - std::abs(octaveDouble - baseOctave) / OctaveNumber * std::abs(mColour);
                }
            }
            else
            {
                if (mColour > 0.)
                {
                    octaveFactor = 1.0 - std::abs(octaveDouble - baseOctave) / OctaveNumber * std::abs(mColour);
                }
                else
                {
                    octaveFactor = 1.0 + std::abs(octaveDouble - baseOctave) / OctaveNumber * std::abs(mColour);
                }
            }
            for (int i_tone = 0; i_tone < B; i_tone++)
            {
                mGainSumMixed[i_octave][i_tone] *= octaveFactor;
            }
        }

        // Set smoother's target values
        for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            for (unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mSmoothedFloats[i_octave][i_tone].setTargetValue(mGainSumMixed[i_octave][i_tone]);
            }
        }

        // Process cqt data
        for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            const size_t nSamplesOctave = mCqt.getSamplesToProcess(i_octave);
            CircularBuffer<std::complex<double>> *octaveCqtBuffer = mCqt.getOctaveCqtBuffer(i_octave);

            // synthesis
            // #pragma omp simd
            for (unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mSmoothedFloats[i_octave][i_tone].getNextBlock(mModulationData[i_octave][i_tone].data(), nSamplesOctave);
                mOscillators[i_octave][i_tone].generateBlock(mOscillatorBuffer[i_octave][i_tone].data(), nSamplesOctave);
            }
            for (unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                octaveCqtBuffer[i_tone].pullBlock(mSynthBuffer[i_octave][i_tone].data(), nSamplesOctave);
                for (size_t i_sample = 0u; i_sample < nSamplesOctave; i_sample++)
                {
                    mSynthBuffer[i_octave][i_tone][i_sample] = mOscillatorBuffer[i_octave][i_tone][i_sample] * mModulationData[i_octave][i_tone][i_sample];
                }
                octaveCqtBuffer[i_tone].pushBlock(mSynthBuffer[i_octave][i_tone].data(), nSamplesOctave);
            }
        }
        // output data
        const double *const dataOut = mCqt.outputBlock(BlockSize);
        mOutputBuffer.pushBlock(dataOut, BlockSize);
        mOutputDataCounter += BlockSize;
    }
    if (mOutputDataCounter >= nSamples)
    {
        mOutputBuffer.pullDelayBlock(mOutputData.data(), nSamples - 1, nSamples);
        mOutputDataCounter -= nSamples;
    }
    else
    {
        for (int i_sample = 0; i_sample < nSamples; i_sample++)
        {
            mOutputData[i_sample] = 0.;
        }
    }
    for (int i_sample = 0; i_sample < nSamples; i_sample++)
    {
        data[i_sample] = mOutputData[i_sample];
    }

    // Spectral display
    for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
    {
        for (unsigned i_tone = 0u; i_tone < B; i_tone++)
        {
            mGainsIllustration[i_octave][i_tone] = mSmoothedFloats[i_octave][i_tone].getCurrentValue();
        }
    }
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setAttack(const double attack)
{
    mAttack = Cqt::Clip(attack, 0.0, 1.0);
    mAttack = 1.0 - mAttack;
    for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
    {
        for (unsigned i_tone = 0u; i_tone < B; i_tone++)
        {
            mSmoothedFloats[i_octave][i_tone].setSmoothingFactors(mAttack, mDecay);
        }
    }
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setDecay(const double decay)
{
    mDecay = Cqt::Clip(decay, 0.0, 1.0);
    mDecay = 1.0 - mDecay;
    for (unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
    {
        for (unsigned i_tone = 0u; i_tone < B; i_tone++)
        {
            mSmoothedFloats[i_octave][i_tone].setSmoothingFactors(mAttack, mDecay);
        }
    }
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setTuning(const double tuning)
{
    mTuning = tuning;
    mCqt.setConcertPitch(mTuning);
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setOctaveShift(const double octaveShift)
{
    mOctaveShift = octaveShift;
    const double shiftFloor = std::floor(mOctaveShift);
    const double shiftCeil = shiftFloor + 1.;
    mLowerShiftFrac = 1. - (mOctaveShift - shiftFloor);
    mHigherShiftFrac = 1. - mLowerShiftFrac;
    mLowerOctaveShift = static_cast<int>(shiftFloor);
    mHigherOctaveShift = static_cast<int>(shiftCeil);
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setOctaveMix(const double octaveMix)
{
    mOctaveMix = octaveMix;
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setColour(const double colour)
{
    mColour = colour;
    mColour = audio_utils::Clip<double>(mColour, -1., 1.);
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setSparsity(const double sparsity)
{
    mSparsity = sparsity;
}
