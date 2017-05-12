#include "blood_ocr.h"
#include <time.h> 
#include "err.h"
#include "my_log.h"

using namespace std;
using namespace cv;

Blood_OCR::Blood_OCR()
{
	initOcrEngine();
}

Blood_OCR::~Blood_OCR()
{
}

int Blood_OCR::initOcrEngine()
{
	_INFO("初始化ocr引擎");
	tess_ocr_key_.Init(NULL, "fontyp", tesseract::OEM_TESSERACT_ONLY);
	tess_ocr_key_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	tess_ocr_value_.Init(NULL, "num", tesseract::OEM_TESSERACT_ONLY);
	tess_ocr_value_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	return 0;
}


int Blood_OCR::loadDictionary(string dictionary_string)
{
	_INFO("加载血生化字典");
	Json::Reader reaeder;
	if (!reaeder.parse(dictionary_string, result_))
	{
		return -1;
	}

	Json::Value scale = result_["scale"];
	for (Json::Value obj : scale)
	{
		dictionary_.insert(pair<string,string>(obj["text"].asString(), ""));
	}
	return 0;
}


/* 找到横线 →  定位四个角 → 透视变换 */
int Blood_OCR::perspectiveTransformation(const Mat& src_image, Mat& dst_image)
{
	Size src_image_size = src_image.size();

	/* 找横线*/
	vector<Vec2f> lines;
	{
		if (detectLines(src_image, lines) != 0)
		{
			return H6OCRERROR::perspective_detectLines;
		}
		//{
		//	Mat tmp_image(src_image);
		//	drawLines(tmp_image, lines);
		//	imshow("横线识别结果", tmp_image);
		//}
		/* 找到需要的直线*/
		findHorizontaLines_MaxInterval(lines);  //这里假定：间距最大的两条直线之间的内容就是待识别区域
	}

	{	/* 定位四个角 */
		vector<Point2f> corners = {
			getLeftEndpointOfLine(lines.front()),
			getRightEndpointOfLine(lines.front(), src_image_size.width),
			getLeftEndpointOfLine(lines.back()),
			getRightEndpointOfLine(lines.back(), src_image_size.width)
		};

		Size2f dst_size = { (float)src_image_size.width, lines.back()[0] - lines.front()[0] };

		vector<Point2f> corners_trans = {
			{ 0, 0 }, { dst_size.width, 0 }, { 0, dst_size.height }, { dst_size.width, dst_size.height }
		};
		Mat warp_mat = cv::getPerspectiveTransform(corners, corners_trans);
		cv::warpPerspective(src_image, dst_image, warp_mat, dst_size);  // 透视变换
	}
	return 0;
}

/* 找到左右文字边缘 */
int Blood_OCR::findLeftAndRightEdge(const Mat& src_image, Mat& dst_image)
{
	vector<Vec2f> lines;
	{
		Mat canny_image;
		Canny(src_image, canny_image, 50, 200, 3);
		int threshold = 5;  // 阈值:5  
		cv::HoughLines(canny_image, lines, 1, CV_PI / 180, threshold, 0, 0);
		findLines_Vertical(lines);
	}

	if (lines.empty())
	{
		dst_image = src_image;
	}
	else
	{
		Vec2f left = lines.front(), right = lines.back();
		Rect rect((int)left[0], 0, right[0] - left[0] + 2, src_image.size().height);
		dst_image = src_image(rect);
	}
	return 0;
}

// 与字典尝试匹配，返回匹配成功的数量
int Blood_OCR::correctKey(vector<string> &keys)
{
	int valid_count = 0;
	for (string& key : keys)
	{
		if (dictionary_.count(key) == 1)  //直接找到
		{
			++valid_count;
			continue;
		}
		/* 根据编辑距离最小原则，纠正*/
		int minDis = 100;
		string target;
		for (auto name_pair : dictionary_)
		{
			string name = name_pair.first;
			int dis = COMMON::minEditDistance(name, key);
			if (dis < minDis)
			{
				minDis = dis;
				target = name;  //
			}
		}
		if (minDis <= key.size() / 2)
		{
			++valid_count;
			key = target;  //correct
		}
	}
	return valid_count;
}

/* 根据是否是float判断*/
int Blood_OCR::correctValue(vector<string> &values)
{
	int valid_count = 0;
	for (string& val : values)
	{
		val = COMMON::stripAllSpace(val);
		if (COMMON::isFloat(val))
		{
			++valid_count;
		}
	}
	return valid_count;
}

int Blood_OCR::cutAndOcr(const Mat& image)
{
	Size image_size = image.size();

	Mat canny_image;
	Canny(image, canny_image, 50, 200, 3);  /* 边缘检测 -> 变成黑白图像[利于计算] */

	_INFO("竖向切割...");
	/* 竖向切割*/
	vector<Rect> rects;
	{
		Rect image_rect = { 0, 0, image_size.width, image_size.height };
		cut_Vertical(canny_image, image_rect, rects);
		if (rects.empty())
		{
			return H6OCRERROR::cutAndOcr_cut_Vertical;
		}
		//Mat tmp;
		//cv::cvtColor(image, tmp, CV_GRAY2BGR);
		//drawRectangles(tmp, rects);
		//imshow("竖向切割区域识别", tmp);
	}

		
	vector<string> keys, values;

	tess_ocr_key_.SetImage(image.data, image.cols, image.rows, 1, image.step);
	tess_ocr_value_.SetImage(image.data, image.cols, image.rows, 1, image.step);

	int key_count_flag = 0; // 还没找到对应value列的key的列数
	int tmp_idx = 0;
	vector<string> tmp_keys;
	_INFO("横向切割...");
	for (Rect rect : rects)
	{
		++tmp_idx;
		/* 横着切割， 得到最终的识别小区域 */
		vector<Rect> areas;
		if (cut_Horizontal(canny_image, rect, areas) != 0)
		{
			continue; //
		}
		/* debug*/
		//{
		//	Mat tmp_image;
		//	cv::cvtColor(image, tmp_image, CV_GRAY2BGR);
		//	drawRectangles(tmp_image, areas);
		//	//imshow("识别区域" + std::to_string(tmp_idx), tmp_image);
		//}


		/*
		尝试ocr --> 判断是否是需要的数据
		key 在 value 的左边, 并且一一对应。有多少key列就有多少value列
		*/
		vector<string> result;
		int valid_count = 0;
		if (key_count_flag == 0)
		{
			batchOCR(tess_ocr_key_, image, areas, result);
			valid_count = correctKey(result);
			if (valid_count > result.size() / 3)  // 匹配度超过1/3 -> 可认为是key
			{
				++key_count_flag;
				tmp_keys.assign(result.begin(), result.end());
			}
		}
		else
		{
			batchOCR(tess_ocr_value_, image, areas, result);
			valid_count = correctValue(result);
			if (valid_count > result.size() / 2)  // 匹配度超过一半 -> 可认为是value
			{
				--key_count_flag;
				/* 确保key value 列相同*/
				keys.insert(keys.end(), tmp_keys.begin(), tmp_keys.end());
				values.insert(values.end(), result.begin(), result.end());
			}
		}

	}
	if (keys.empty())
	{
		return H6OCRERROR::cutAndOcr_keys_null;
	}
	if (values.empty())
	{
		return H6OCRERROR::cutAndOcr_values_null;
	}
	if ( keys.size() > values.size())
	{
		return H6OCRERROR::cutAndOcr_keys_greater;;
	}
	if (keys.size() < values.size())
	{
		return H6OCRERROR::cutAndOcr_values_greater;;
	}

	for (int i = 0; i < keys.size(); ++i)
	{
		string key = keys[i];
		if (dictionary_.count(key) == 1)
		{
			dictionary_[key] = values[i];
		}
	}
	return 0;
}


/*
可识别对象要求： 至少有两条横线； 然后按列分隔，key和value的列一一对应
基本识别思路：
	识别横线，取横线间距最大的两条作为识别区域
	根据两条横线的四个角，投射变换得到规整的矩形区域；
	按列切割；
	针对每一列，尝试识别；如果识别率达到一定阈值确定其实key或者value。
*/
int Blood_OCR::recognise(const vector<unsigned char>& image_buffer)
{
	_INFO("recognise...");
	Mat src_image = imdecode(Mat(image_buffer), 0);
	if (src_image.data == NULL)
	{
		return INVILD_IMAGE;
	}
	//imshow("【原图】", src_image);

	_INFO("透射变换...");
	int ret;
	Mat transform_image;
	ret = perspectiveTransformation(src_image, transform_image);
	if (ret != 0) return ret;
	//imshow("【透射变换结果】", transform_image);

	_INFO("定位左右边界...");
	Mat dst_image;
	ret = findLeftAndRightEdge(transform_image, dst_image);
	if (ret != 0) return ret;
	//imshow("【定位左右边界结果】", dst_image);

	_INFO("切割和识别r...");
	ret = cutAndOcr(dst_image);

	return ret;
}

void Blood_OCR::retrieve(Json::Value& result)
{
	_INFO("取结果...");
	Json::Value& scale = result_["scale"];
	for (auto& obj : scale)
	{
		string text = obj["text"].asString();
		if (dictionary_.count(text) == 1)
		{
			obj["value"] = dictionary_[text];
		}
	}
	result["scaleData"] = result_;
}
