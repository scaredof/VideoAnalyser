#ifndef ANALYZER_ALGORITHMOVYOLO_H
#define ANALYZER_ALGORITHMOVYOLO_H

#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <openvino/openvino.hpp> //openvino header file
#include "Algorithm.h"

namespace AVSAnalyzer {
	class Config;

	class AlgorithmOvYolo : public Algorithm
	{
	public:
		AlgorithmOvYolo(Config* config, std::string& modelPath,std::vector<std::string>& classNames);
		virtual ~AlgorithmOvYolo();
	public:
		virtual bool objectDetect(cv::Mat& image, std::vector<DetectObject>& detects, const std::string& targetObject = "");
	private:
		std::vector<std::string> mClassNames;
		std::mutex mMtx;
		ov::Core* core;
		ov::CompiledModel compiled_model;
		ov::InferRequest  infer_request;
	};
}
#endif //ANALYZER_ALGORITHMOVYOLO_H

