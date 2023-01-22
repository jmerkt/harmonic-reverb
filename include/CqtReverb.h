#pragma once

#include "../submodules/rt-cqt/include/SlidingCqt.h"
#include "SmoothedFloat.h"
#include "CplxWavetableOscillator.h"

using namespace std::complex_literals;
constexpr int BlockSize{256};
constexpr size_t WavetableSize{512u};

// Parameters later
constexpr double MaxToneThresholdFactor{0.25}; // sparsity
constexpr double GlobalMaxThresholdFactor{0.05};
constexpr double OctaveMeanThresholdFactor{2.0}; // sparsity

template <unsigned B, unsigned OctaveNumber>
class CqtReverb
{
public:
    CqtReverb() = default;
    ~CqtReverb() = default;

    void init(const double samplerate, const int blockSize);

    void processBlock(double* const data, const int nSamples);

    void setAttack(const double attack);
    void setDecay(const double decay);
    void setTuning(const double tuning);
    void setOctaveOffset(const double octaveOffset);
    void setOctaveMix(const double octaveMix);
    void setColour(const double colour);
    void setSparsity(const double sparsity);
private:
    static constexpr double mOneDivB{1. / static_cast<double>(B)};

    // Processing classes and buffers
    Cqt::SlidingCqt<B, OctaveNumber, false> mCqt;

    CircularBuffer<double> mInputBuffer;
	CircularBuffer<double> mOutputBuffer;
    std::vector<double> mInputData;
	std::vector<double> mOutputData;
    size_t mInputDataCounter;
	size_t mOutputDataCounter;

    SmoothedFloatUpDown<double> mSmoothedFloats[OctaveNumber][B];

    double mCqtValues[OctaveNumber][B];
    std::vector<double> mModulationData[OctaveNumber][B];
    std::vector<double> mPhaseData[OctaveNumber][B];

    StaticCplxWavetable<WavetableSize> mStaticWavetable;
    CplxWavetableOscillator<WavetableSize> mOscillators[OctaveNumber][B];
    std::vector<std::complex<double>> mOscillatorBuffer[OctaveNumber][B];
    std::vector<std::complex<double>> mSynthBuffer[OctaveNumber][B];

    double mGainSum[OctaveNumber][B];
    double mOctaveMeans[OctaveNumber];

    // Controlable parameters
    double mAttack{ 50. };
    double mDecay{ 1000. };
    double mTuning{ 440. };
    double mOctaveOffset{ 1. };
    double mOctaveMix{ 0.3 };
    double mColour{ 1. };
    double mSparsity{ 1. };
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
    for(unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
    {
        const double octaveRate = mCqt.getOctaveSampleRate(i_octave);
        const int octaveSize = mCqt.getOctaveBlockSize(i_octave);
        const double* const binFreqs = mCqt.getOctaveBinFreqs(i_octave);
        for(unsigned i_tone = 0u; i_tone < B; i_tone++)
        {
            mSmoothedFloats[i_octave][i_tone].init(octaveRate);
            mSmoothedFloats[i_octave][i_tone].setSmoothingTime(mAttack, mDecay);

            mOscillators[i_octave][i_tone].init(octaveRate, &mStaticWavetable);
            mOscillators[i_octave][i_tone].setFrequency(binFreqs[i_tone]);
            mOscillatorBuffer[i_octave][i_tone].resize(octaveSize, {0., 0.});
            mSynthBuffer[i_octave][i_tone].resize(octaveSize, {0., 0.});

            mModulationData[i_octave][i_tone].resize(octaveSize, 0.);
        }
    }
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::processBlock(double* const data, const int nSamples)
{
    mInputBuffer.pushBlock(data, nSamples);
    mInputDataCounter += nSamples;
    while(mInputDataCounter >= BlockSize)
	{
        mInputBuffer.pullDelayBlock(mInputData.data(), mInputDataCounter - 1, BlockSize);
        mInputDataCounter -= BlockSize;
        mCqt.inputBlock(mInputData.data(), BlockSize);

        for(unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            CircularBuffer<std::complex<double>>* octaveCqtBuffer = mCqt.getOctaveCqtBuffer(i_octave);

            // acquire cqt values for feature calculations
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mCqtValues[i_octave][i_tone] = std::abs(octaveCqtBuffer[i_tone].pullDelaySample(0));
            }
        }
        for(unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mGainSum[i_octave][i_tone] = 0.;
            }
        }
        
        // Global max
        double globalMax = 0.;
        for(unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                if(mCqtValues[i_octave][i_tone] > globalMax)
                    globalMax = mCqtValues[i_octave][i_tone];
            }
        }
        for(unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mOctaveMeans[i_octave] += mCqtValues[i_octave][i_tone];
            }
            mOctaveMeans[i_octave] *= mOneDivB;
        }

        // Mean per octave, const int blockSize
        for(unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            double max_magnitude = 0.;
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                if(mCqtValues[i_octave][i_tone] > max_magnitude)
                    max_magnitude = mCqtValues[i_octave][i_tone];
            }
            const double threshold = max_magnitude * MaxToneThresholdFactor;
            const double globalMaxThreshold = globalMax * GlobalMaxThresholdFactor;
            const double octaveMeanTreshold = mOctaveMeans[i_octave] * OctaveMeanThresholdFactor;
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                if
                (
                mCqtValues[i_octave][i_tone] > threshold && 
                mCqtValues[i_octave][i_tone] > globalMaxThreshold &&
                mCqtValues[i_octave][i_tone] > octaveMeanTreshold
                )
                    mGainSum[i_octave][i_tone] += mCqtValues[i_octave][i_tone];
            }
        }
        // TODO: Octaves, etc all adds to GainSum


        for(unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mSmoothedFloats[i_octave][i_tone].setTargetValue(mGainSum[i_octave][i_tone]);
            }
        }

        // process cqt data 
        for(unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            // TODO Optimization:
            // - only process tone selection?

            const size_t nSamplesOctave = mCqt.getSamplesToProcess(i_octave);
            CircularBuffer<std::complex<double>>* octaveCqtBuffer = mCqt.getOctaveCqtBuffer(i_octave);

            // synthesis
            // #pragma omp simd
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mSmoothedFloats[i_octave][i_tone].getNextBlock(mModulationData[i_octave][i_tone].data(), nSamplesOctave);
                mOscillators[i_octave][i_tone].generateBlock(mOscillatorBuffer[i_octave][i_tone].data(), nSamplesOctave);
            }
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                octaveCqtBuffer[i_tone].pullBlock(mSynthBuffer[i_octave][i_tone].data(), nSamplesOctave);
                for(size_t i_sample = 0u; i_sample < nSamplesOctave; i_sample++)
                {
                    mSynthBuffer[i_octave][i_tone][i_sample] = mOscillatorBuffer[i_octave][i_tone][i_sample] * mModulationData[i_octave][i_tone][i_sample];
                }
                octaveCqtBuffer[i_tone].pushBlock(mSynthBuffer[i_octave][i_tone].data(), nSamplesOctave);
            }
        }
        // output data
        const double* const dataOut = mCqt.outputBlock(BlockSize);
        mOutputBuffer.pushBlock(dataOut, BlockSize);
        mOutputDataCounter += BlockSize;
    }
    if(mOutputDataCounter >= nSamples)
    {
        mOutputBuffer.pullDelayBlock(mOutputData.data(), nSamples - 1, nSamples);
        mOutputDataCounter -= nSamples; 
    }
    else
    {
        for(int i_sample = 0; i_sample < nSamples; i_sample++)
        {
            mOutputData[i_sample] = 0.;
        }
    }
    for(int i_sample = 0; i_sample < nSamples; i_sample++)
    {
        data[i_sample] = mOutputData[i_sample];
    }

    // return mOutputData.data();
    //const double* cqtOut = mCqt.outputBlock(nSamples); 
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setAttack(const double attack)
{
    mAttack = attack;
    for(unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mSmoothedFloats[i_octave][i_tone].setSmoothingTime(mAttack, mDecay);
            }
        }
}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setDecay(const double decay)
{
    mDecay = decay;
    for(unsigned i_octave = 0u; i_octave < OctaveNumber; i_octave++)
        {
            for(unsigned i_tone = 0u; i_tone < B; i_tone++)
            {
                mSmoothedFloats[i_octave][i_tone].setSmoothingTime(mAttack, mDecay);
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
inline void CqtReverb<B, OctaveNumber>::setOctaveOffset(const double octaveOffset){}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setOctaveMix(const double octaveMix){}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setColour(const double colour){}

template <unsigned B, unsigned OctaveNumber>
inline void CqtReverb<B, OctaveNumber>::setSparsity(const double sparsity){}


