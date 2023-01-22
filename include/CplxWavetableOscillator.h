#pragma once

#include "../submodules/rt-cqt/include/Utils.h"
#include <complex>
using namespace std::complex_literals;


template<size_t WavetableSize>
class StaticCplxWavetable
{
public:
	StaticCplxWavetable();
	~StaticCplxWavetable() = default;
    inline const double* const getSineWavetable(){ return mSineWavetable; };
    inline const double* const getCosineWavetable(){ return mCosineWavetable; };
private:
	double mSineWavetable[WavetableSize];
    double mCosineWavetable[WavetableSize];
};


template<size_t WavetableSize>
StaticCplxWavetable<WavetableSize>::StaticCplxWavetable()
{
	if (WavetableSize > 0)
	{
		const double phaseIncr = Cqt::TwoPi<double>() / static_cast<double>(WavetableSize);
		double phase = 0.;
		for (size_t i = 0u; i < WavetableSize; i++)
		{
			mSineWavetable[i] = std::sin(phase);
            mCosineWavetable[i] = std::cos(phase);
			phase += phaseIncr;
		}
	}
}

template<size_t WavetableSize>
class CplxWavetableOscillator
{
public:
	CplxWavetableOscillator() = default;
	~CplxWavetableOscillator() = default;
	void init(const double samplerate, StaticCplxWavetable<WavetableSize>* staticWavetable);
	void setFrequency(const double frequency);

	std::complex<double> generateSample();
	void generateBlock(std::complex<double>* const data, const int blockSize);
private:
	void updateIncrement();
    std::complex<double> interpolateWavetable();
	double mFrequency{ 440.0 };
	double mPhase{ 0. };
	double mSampleRate{ 44100. };
	double mPhaseIncrement{ 0. };
    StaticCplxWavetable<WavetableSize>* mStaticWavetable;
	const unsigned mWavetableSizeMinOne{ WavetableSize - 1 };
};

template<size_t WavetableSize>
inline void CplxWavetableOscillator<WavetableSize>::init(const double samplerate, StaticCplxWavetable<WavetableSize>* staticWavetable)
{
	mSampleRate = samplerate;
    mStaticWavetable = staticWavetable;
	updateIncrement();
	mPhase = 0.;
}

template<size_t WavetableSize>
inline void CplxWavetableOscillator<WavetableSize>::setFrequency(const double frequency)
{
	mFrequency = frequency;
	updateIncrement();
}

template<size_t WavetableSize>
inline void CplxWavetableOscillator<WavetableSize>::updateIncrement()
{
	mPhaseIncrement = mFrequency * static_cast<double>(WavetableSize) / mSampleRate;
}

template<size_t WavetableSize>
inline std::complex<double> CplxWavetableOscillator<WavetableSize>::generateSample() 
{
    return interpolateWavetable();
}

template<size_t WavetableSize>
inline void CplxWavetableOscillator<WavetableSize>::generateBlock(std::complex<double>* const data, const int blockSize)
{
    for (int i = 0; i < blockSize; i++)
    {
        data[i] = interpolateWavetable();
    }
}

template<size_t WavetableSize>
inline std::complex<double> CplxWavetableOscillator<WavetableSize>::interpolateWavetable() 
{
	// Linear interpolation
	const double idxLowDouble = std::floor(mPhase);
	const unsigned idxLow = static_cast<unsigned>(idxLowDouble);
	const unsigned idxHigh = idxLow + 1;
	const double frac = mPhase - idxLowDouble;
    const double OneMinFrac = (1. - frac);
	// Wrap around
	mPhase += mPhaseIncrement;
	mPhase = static_cast<double>(static_cast<unsigned>(mPhase) & mWavetableSizeMinOne) + frac;
	// Return
    const double* const cosineWavetable = mStaticWavetable->getCosineWavetable();
    const double* const sineWavetable = mStaticWavetable->getSineWavetable();
    const std::complex<double> value = {cosineWavetable[idxLow] * OneMinFrac + cosineWavetable[idxHigh] * frac,
                                        -(sineWavetable[idxLow] * OneMinFrac + sineWavetable[idxHigh] * frac)};
	return value;
}