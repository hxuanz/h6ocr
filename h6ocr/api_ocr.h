#pragma once
#include "stdafx.h"

class Api_OCR
{
public:
	static Api_OCR& Instance()
	{
		static Api_OCR singleton;
		return singleton;
	}
	int recognise(const std::vector<unsigned char>& image_buffer, std::string result);
	void Api_OCR::retrieve(Json::Value& result);

protected:	
	Api_OCR();
	~Api_OCR();
	void init();
	int perspectiveTransformation(const cv::Mat& src, cv::Mat& dst);
	int findLeftAndRightEdge(const cv::Mat& src, cv::Mat& dst);
	int cutAndOcr(const cv::Mat& image);

	int correctKey(std::vector<std::string> &);
	int correctValue(std::vector<std::string> &);

private:
	static Api_OCR *instance_;

	Json::Value result_;
	tesseract::TessBaseAPI tess_ocr_key_;
	tesseract::TessBaseAPI tess_ocr_value_;
};

