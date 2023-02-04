#pragma once

//#include <cmath>

template<typename FloatType>
class SmoothedFloat
{
public:
	SmoothedFloat() = default;
	~SmoothedFloat() = default;

	inline void init(FloatType samplerateHz) noexcept
	{ 
		mSampleRate = samplerateHz; 
		setSmoothingTime(mSmoothingTimeMs);
	};
	inline void setTargetValue(FloatType newTargetValue) noexcept
	{
		mTargetValue = newTargetValue;
		mCountdown = mSmoothingTimeSamples;
		mSmoothingStep = (mTargetValue - mCurrentValue) * mOneDivSmoothingTimeSamples;
	};
	inline FloatType getNextValue() noexcept
	{
		if (!isSmoothing()) { return mTargetValue; };
		--mCountdown;
		if (isSmoothing())
		{
			mCurrentValue += mSmoothingStep;
		}
		else
		{
			return mTargetValue;
		}
		return mCurrentValue;
	};
	inline void getNextBlock(FloatType* const data, const int blockSize) noexcept 
	{
		for (int i = 0; i < blockSize; i++)
		{
			data[i] = getNextValue();
		}
	}
	inline bool isSmoothing() noexcept { return mCountdown > 0; };
	inline void setSmoothingTime(FloatType timeMs) noexcept
	{
		mSmoothingTimeMs = timeMs;
		mSmoothingTimeSamples = static_cast<int>(mSmoothingTimeMs * mOneDivThousand * mSampleRate);
		mOneDivSmoothingTimeSamples = static_cast<FloatType>(1.) / static_cast<FloatType>(mSmoothingTimeSamples);
	};
	inline FloatType getCurrentValue() { return mCurrentValue; };

#ifdef INCLUDE_PYTHON_BINDING
	pybind11::array_t<double> Python_getNextBlock(const int blockSize) 
	{ 
		auto result = pybind11::array_t<double>(blockSize);
		pybind11::buffer_info buf = result.request();
		double* ptr = static_cast<double*>(buf.ptr);
		getNextBlock(ptr, blockSize);
		return result;
	};
#endif
private:
	const FloatType mOneDivThousand = static_cast<FloatType>(1. / 1000.);
	FloatType mSampleRate{ 48000. };
	int mCountdown{ 0 };
	int mSmoothingTimeSamples{ 0 };
	FloatType mOneDivSmoothingTimeSamples{ 0. };
	FloatType mTargetValue{ 0. };
	FloatType mCurrentValue{ 0. };
	FloatType mSmoothingTimeMs{ 20. };
	FloatType mSmoothingStep{ 0. };
};

namespace SmoothingTypes
{
	struct Linear{};
	struct Exponential{};
}

template<typename FloatType, typename SmoothingType>
class SmoothedFloatUpDown
{
public:
	SmoothedFloatUpDown() = default;
	~SmoothedFloatUpDown() = default;

	inline void init(FloatType sampleRateHz) noexcept
	{ 
		mSampleRate = sampleRateHz; 
		setSmoothingTime(mSmoothingTimeMsUp, mSmoothingTimeMsDown);
	};
	inline void setTargetValue(FloatType newTargetValue) noexcept
	{
		if(newTargetValue != mTargetValue)
		{
			if(newTargetValue > mCurrentValue)
			{
				mTargetValue = newTargetValue;
				mCountdown = mSmoothingTimeSamplesUp;
				if constexpr (std::is_same_v<SmoothingType, SmoothingTypes::Linear>)
				{
					mSmoothingStep = (mTargetValue - mCurrentValue) * mOneDivSmoothingTimeSamplesUp;
				}
				else
				{
					mSmoothingStep = std::exp((std::log(std::abs(mTargetValue)) - std::log(std::abs(mCurrentValue))) * mOneDivSmoothingTimeSamplesUp);
				}
			}
			else
			{
				mTargetValue = newTargetValue;
				mCountdown = mSmoothingTimeSamplesDown;		
				if constexpr (std::is_same_v<SmoothingType, SmoothingTypes::Linear>)
				{
					mSmoothingStep = (mTargetValue - mCurrentValue) * mOneDivSmoothingTimeSamplesDown;
				}
				else
				{
					mSmoothingStep = std::exp((std::log(std::abs(mTargetValue)) - std::log(std::abs(mCurrentValue))) * mOneDivSmoothingTimeSamplesDown);
				}
			}
		}
	};
	inline FloatType getNextValue() noexcept
	{
		if (!isSmoothing()) { return mTargetValue; };
		--mCountdown;
		if (isSmoothing())
		{
			if constexpr (std::is_same_v<SmoothingType, SmoothingTypes::Linear>)
			{
				mCurrentValue += mSmoothingStep;
			}
			else
			{
				mCurrentValue *= mSmoothingStep;
			}
		}
		else
		{
			return mTargetValue;
		}
		return mCurrentValue;
	};
	inline void getNextBlock(FloatType* const data, const int blockSize) noexcept 
	{
		for (int i = 0; i < blockSize; i++)
		{
			data[i] = getNextValue();
		}
	}
	inline bool isSmoothing() noexcept { return mCountdown > 0; };
	inline void setSmoothingTime(FloatType timeMsUp, FloatType timeMsDown) noexcept
	{
		mSmoothingTimeMsUp = timeMsUp;
		mSmoothingTimeSamplesUp = static_cast<int>(mSmoothingTimeMsUp * mOneDivThousand * mSampleRate);
		mOneDivSmoothingTimeSamplesUp = static_cast<FloatType>(1.) / static_cast<FloatType>(mSmoothingTimeSamplesUp);

		mSmoothingTimeMsDown = timeMsDown;
		mSmoothingTimeSamplesDown = static_cast<int>(mSmoothingTimeMsDown * mOneDivThousand * mSampleRate);
		mOneDivSmoothingTimeSamplesDown = static_cast<FloatType>(1.) / static_cast<FloatType>(mSmoothingTimeSamplesDown);
	};
	inline FloatType getCurrentValue() { return mCurrentValue; };
private:
	const FloatType mOneDivThousand = static_cast<FloatType>(1. / 1000.);
	FloatType mSampleRate{ 48000. };
	int mCountdown{ 0 };
	FloatType mTargetValue{ 0. };
	FloatType mCurrentValue{ 0. };
	FloatType mSmoothingStep{ 0. };
	int mSmoothingTimeSamplesUp{ 0 };
	int mSmoothingTimeSamplesDown{ 0 };
	FloatType mSmoothingTimeMsUp{ 20. };
	FloatType mSmoothingTimeMsDown{ 20. };
	FloatType mOneDivSmoothingTimeSamplesUp{ 0. };
	FloatType mOneDivSmoothingTimeSamplesDown{ 0. };

};


template<typename FloatType>
class OnePoleUpDown
{
public:
	OnePoleUpDown() = default;
	~OnePoleUpDown() = default;

	inline void init(FloatType sampleRateHz) noexcept
	{ 
		mSampleRate = sampleRateHz; 
	};
	inline void setTargetValue(FloatType newTargetValue) noexcept
	{
		if(newTargetValue > mTargetValue)
			mSmoothingUp = true;
		else
			mSmoothingUp = false;
		mTargetValue = newTargetValue;
	};
	inline FloatType getNextValue() noexcept
	{
		if(mSmoothingUp)
		{
			mCurrentValue = mFactorUp * mTargetValue + (static_cast<FloatType>(1.) - mFactorUp) * mCurrentValue;
		}
		else
		{
			mCurrentValue = mFactorDown * mTargetValue + (static_cast<FloatType>(1.) - mFactorDown) * mCurrentValue;
		}
		return mCurrentValue;
	};
	inline void getNextBlock(FloatType* const data, const int blockSize) noexcept 
	{
		for (int i = 0; i < blockSize; i++)
		{
			data[i] = getNextValue();
		}
	}
	inline void setSmoothingFactors(FloatType factorUp, FloatType factorDown) noexcept
	{
		mFactorUp = factorUp;
		mFactorDown = factorDown;
		mFactorUp = Cqt::Clip<FloatType>(mFactorUp, 0., 0.9999999999);
		mFactorDown = Cqt::Clip<FloatType>(mFactorDown, 0., 0.9999999999);
	};
	inline FloatType getCurrentValue() { return mCurrentValue; };
private:
	FloatType mSampleRate{ 48000. };
	FloatType mTargetValue{ 0. };
	FloatType mCurrentValue{ 0. };
	FloatType mFactorUp{ 0. };
	FloatType mFactorDown{ 0. };
	bool mSmoothingUp{ true };

};











