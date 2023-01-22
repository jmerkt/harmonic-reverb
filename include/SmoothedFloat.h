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
		samplerate = samplerateHz; 
		setSmoothingTime(smoothingTimeMs);
	};
	inline void setTargetValue(FloatType newTargetValue) noexcept
	{
		targetValue = newTargetValue;
		countdown = smoothingTimeSamples;
		smoothingStep = (targetValue - currentValue) * oneDivSmoothingTimeSamples;
	};
	inline FloatType getNextValue() noexcept
	{
		if (!isSmoothing()) { return targetValue; };
		--countdown;
		if (isSmoothing())
		{
			currentValue += smoothingStep;
		}
		else
		{
			return targetValue;
		}
		return currentValue;
	};
	inline void getNextBlock(FloatType* const data, const int blockSize) noexcept 
	{
		for (int i = 0; i < blockSize; i++)
		{
			data[i] = getNextValue();
		}
	}
	inline bool isSmoothing() noexcept { return countdown > 0; };
	inline void setSmoothingTime(FloatType timeMs) noexcept
	{
		smoothingTimeMs = timeMs;
		smoothingTimeSamples = static_cast<int>(smoothingTimeMs * oneDivThousand * samplerate);
		oneDivSmoothingTimeSamples = static_cast<FloatType>(1.) / static_cast<FloatType>(smoothingTimeSamples);
	};
	inline FloatType getCurrentValue() { return currentValue; };

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
	const FloatType oneDivThousand = static_cast<FloatType>(1. / 1000.);
	FloatType samplerate{ 48000. };
	int countdown{ 0 };
	int smoothingTimeSamples{ 0 };
	FloatType oneDivSmoothingTimeSamples{ 0. };
	FloatType targetValue{ 0. };
	FloatType currentValue{ 0. };
	FloatType smoothingTimeMs{ 20. };
	FloatType smoothingStep{ 0. };

};


template<typename FloatType>
class SmoothedFloatUpDown
{
public:
	SmoothedFloatUpDown() = default;
	~SmoothedFloatUpDown() = default;

	inline void init(FloatType samplerateHz) noexcept
	{ 
		samplerate = samplerateHz; 
		setSmoothingTime(smoothingTimeMsUp, smoothingTimeMsDown);
	};
	inline void setTargetValue(FloatType newTargetValue) noexcept
	{
		if(newTargetValue != targetValue)
		{
			if(newTargetValue > currentValue)
			{
				targetValue = newTargetValue;
				countdown = smoothingTimeSamplesUp;
				smoothingStep = (targetValue - currentValue) * oneDivSmoothingTimeSamplesUp;
			}
			else
			{
				targetValue = newTargetValue;
				countdown = smoothingTimeSamplesDown;
				smoothingStep = (targetValue - currentValue) * oneDivSmoothingTimeSamplesDown;
			}
		}
	};
	inline FloatType getNextValue() noexcept
	{
		if (!isSmoothing()) { return targetValue; };
		--countdown;
		if (isSmoothing())
		{
			currentValue += smoothingStep;
		}
		else
		{
			return targetValue;
		}
		return currentValue;
	};
	inline void getNextBlock(FloatType* const data, const int blockSize) noexcept 
	{
		for (int i = 0; i < blockSize; i++)
		{
			data[i] = getNextValue();
		}
	}
	inline bool isSmoothing() noexcept { return countdown > 0; };
	inline void setSmoothingTime(FloatType timeMsUp, FloatType timeMsDown) noexcept
	{
		smoothingTimeMsUp = timeMsUp;
		smoothingTimeSamplesUp = static_cast<int>(smoothingTimeMsUp * oneDivThousand * samplerate);
		oneDivSmoothingTimeSamplesUp = static_cast<FloatType>(1.) / static_cast<FloatType>(smoothingTimeSamplesUp);

		smoothingTimeMsDown = timeMsDown;
		smoothingTimeSamplesDown = static_cast<int>(smoothingTimeMsDown * oneDivThousand * samplerate);
		oneDivSmoothingTimeSamplesDown = static_cast<FloatType>(1.) / static_cast<FloatType>(smoothingTimeSamplesDown);
	};
	inline FloatType getCurrentValue() { return currentValue; };
private:
	const FloatType oneDivThousand = static_cast<FloatType>(1. / 1000.);
	FloatType samplerate{ 48000. };
	int countdown{ 0 };
	FloatType targetValue{ 0. };
	FloatType currentValue{ 0. };
	FloatType smoothingStep{ 0. };
	int smoothingTimeSamplesUp{ 0 };
	int smoothingTimeSamplesDown{ 0 };
	FloatType smoothingTimeMsUp{ 20. };
	FloatType smoothingTimeMsDown{ 20. };
	FloatType oneDivSmoothingTimeSamplesUp{ 0. };
	FloatType oneDivSmoothingTimeSamplesDown{ 0. };

};