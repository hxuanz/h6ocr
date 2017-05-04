#pragma once
#include "stdafx.h"
#include <unordered_map>

//struct BK_TREE;
//typedef std::unordered_map<int, BK_TREE*> bk_childs;
//struct BK_TREE
//{
//	BK_TREE(string w) :word(w){}
//
//	std::string word;
//	bk_childs childs;
//};

class Blood_OCR
{
public:
	static Blood_OCR& Instance()
	{
		static Blood_OCR singleton;
		return singleton;
	}
	int recognise(const std::vector<unsigned char>& image_buffer);
	void Blood_OCR::retrieve(Json::Value& result);

protected:	
	Blood_OCR();
	~Blood_OCR();
	int loadDictionary(); 
	int initOcrEngine();
	//void buildBKTree();
	//void destoryBKTree(BK_TREE*);
	//void searchInBkTree(BK_TREE* tree, int threshold, string& word);

	int perspectiveTransformation(const cv::Mat& src, cv::Mat& dst);
	int findLeftAndRightEdge(const cv::Mat& src, cv::Mat& dst);
	int cutAndOcr(const cv::Mat& image);

	int correctKey(std::vector<std::string> &);
	int correctValue(std::vector<std::string> &);

private:
	static Blood_OCR *instance_;

	Json::Value result_;
	//BK_TREE* bk_tree_;

	tesseract::TessBaseAPI tess_ocr_key_;
	tesseract::TessBaseAPI tess_ocr_value_;
};

