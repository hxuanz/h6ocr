#pragma once
#include "stdafx.h"
#include <unordered_map>

class Blood_OCR
{
public:
	static Blood_OCR& Instance()
	{
		static Blood_OCR singleton;
		return singleton;
	}
	int loadDictionary(string dictionary_string);
	int recognise(const std::vector<unsigned char>& image_buffer);
	void retrieve(Json::Value& result);

protected:	
	Blood_OCR();
	~Blood_OCR();
	
	int initOcrEngine();
	int perspectiveTransformation(const cv::Mat& src, cv::Mat& dst);
	int findLeftAndRightEdge(const cv::Mat& src, cv::Mat& dst);
	int cutAndOcr(const cv::Mat& image);

	int correctKey(std::vector<std::string> &);
	int correctValue(std::vector<std::string> &);

private:
	static Blood_OCR *instance_;

	Json::Value result_;
	unordered_map<string, string> dictionary_;

	tesseract::TessBaseAPI tess_ocr_key_;
	tesseract::TessBaseAPI tess_ocr_value_;
};

