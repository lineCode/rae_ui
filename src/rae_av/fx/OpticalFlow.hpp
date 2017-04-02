#pragma once

#include <iostream>

#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/video.hpp"
//#include "opencv2/cudaoptflow.hpp"
//#include "opencv2/cudaarithm.hpp"

// From opencv_contrib
#include "opencv2/optflow.hpp"

extern "C"
{
	#include <libavcodec/avcodec.h>
}

#include "rae/image/ImageBuffer.hpp"

namespace rae
{
namespace av
{

enum class OpticalFlowMethod
{
	DeepFlow, // Actually usable quality and quite fast (7 seconds versus 1 min for DualTVL1)
	DualTVL1, // Slower, possibly better quality than Farneback, not really good quality though. Much worse than DeepFlow.
	Farneback, // Faster
	CudaBrox // Doesn't work yet.
};

enum class EffectNodeState
{
	Nothing,
	WaitingForData,
	Processing,
	Done
};

class OpticalFlow
{
public:
	OpticalFlow(OpticalFlowMethod method = OpticalFlowMethod::DeepFlow);

	void update(double time, double deltaTime);

	void setMethod(OpticalFlowMethod method) { m_opticalFlowMethod = method; }

	EffectNodeState getState() { return m_state; }
	void reset() { setState(EffectNodeState::Nothing); }
	void waitForData() { setState(EffectNodeState::WaitingForData); }

	void pushFrame(AVFrame* frameRGB);
	void replaceFrame0(AVFrame* frameRGB);
	const cv::Mat& getOutputAtTime(float lerpTime);

	//JONDE MOVE TO UTILS OR OTHER CLASS:
	void copyMatToImage(const cv::Mat& mat, ImageBuffer& image);
	void copyAVFrameToMat(AVFrame* frameRGB, cv::Mat& mat);
	void writeMatToPng(String filepath, const cv::Mat& mat);

private:

	void process();

	void setState(EffectNodeState state)
	{
		std::cout << "Optical flow State: " << (int)state << "\n";
		m_state = state;
	}

	OpticalFlowMethod m_opticalFlowMethod;

	int frameCount = 0;

	cv::Mat m_frame0;
	cv::Mat m_frame1;
	cv::Mat m_output;
	
	cv::Mat_<cv::Point2f> m_flowForward; // optical flow result vectors
	cv::Mat_<cv::Point2f> m_flowBackward; // optical flow backwards result vectors
	float m_duration = 2.0f;

	EffectNodeState m_state = EffectNodeState::Nothing;
	bool m_error = false;
};

}
}
