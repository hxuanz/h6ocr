#include "api_ocr.h"
#include <time.h> 
#include "err.h"
using namespace std;
using namespace cv;

Api_OCR::Api_OCR()
{
	init();
}

Api_OCR::~Api_OCR()
{
}

void Api_OCR::init()
{
	string dict = "{\"r_GT\":\"\",\"A/G\":\"\",\"ALB\":\"\",\"ALP\":\"\",\"ALT\":\"\",\"ASL\":\"\",\"CHO\":\"\",\"Crea\":\"\",\"DBILI\":\"\",\"GLB\":\"\",\"GLU\":\"\",\"HBsAg\":\"\",\"TBILI\":\"\",\"TG\":\"\",\"TP\":\"\",\"UA\":\"\",\"UREA\":\"\"}";
	Json::Reader reader;
	if (!reader.parse(dict, result_))
	{
		cerr << "init fail!" << endl;
	}
	{
		tess_ocr_key_.Init(NULL, "fontyp", tesseract::OEM_TESSERACT_ONLY);
		tess_ocr_key_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	}
	{
		tess_ocr_value_.Init(NULL, "num", tesseract::OEM_TESSERACT_ONLY);
		tess_ocr_value_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	}
	cout << "init ocr api success!" << endl;
}

/* 找到横线 →  定位四个角 → 透视变换 */
int Api_OCR::perspectiveTransformation(const Mat& src_image, Mat& dst_image)
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
		findHorizontaLinesNearTarget(lines, src_image_size.height / 2);  // 中间位置，上下扩散 
	}

	{	/* 透射变换 */
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
		warpPerspective(src_image, dst_image, warp_mat, dst_size);
	}

	return 0;
}

/* 找到左右文字边缘 */
int Api_OCR::findLeftAndRightEdge(const Mat& src_image, Mat& dst_image)
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
int Api_OCR::correctKey(vector<string> &keys)
{
	vector<string> &names = result_.getMemberNames();
	int valid_count = 0;
	for (string& key : keys)
	{
		if (result_.isMember(key))  //直接找到
		{
			++valid_count;
			continue;
		}
		/* 根据编辑距离最小原则，纠正*/
		int minDis = 100;
		string target;
		for (vector<string>::iterator it = names.begin(); it != names.end(); ++it)
		{
			int dis = COMMON::minEditDistance(*it, key);
			if (dis < minDis)
			{
				minDis = dis;
				target = *it;  //
			}
		}
		if (minDis <= key.size() / 2)
		{
			++valid_count;
			key = target;
		}
	}
	return valid_count;
}

/* 根据是否是float判断*/
int Api_OCR::correctValue(vector<string> &values)
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

int Api_OCR::cutAndOcr(const Mat& image)
{
	Size image_size = image.size();

	Mat canny_image;
	Canny(image, canny_image, 50, 200, 3);  /* 边缘检测 -> 变成黑白图像[利于计算] */

	/* 竖向切割  --> 得到大区域 */
	vector<Rect> rects;
	{
		Rect image_rect = { 0, 0, image_size.width, image_size.height };
		cut_Vertical(canny_image, image_rect, rects);

		//Mat tmp;
		//cv::cvtColor(image, tmp, CV_GRAY2BGR);
		//drawRectangles(tmp, rects);
		//imshow("竖向切割区域识别", tmp);
	}
	if (rects.empty())
	{
		return H6OCRERROR::cutAndOcr_cut_Vertical;
	}
		
	vector<string> keys, values;

	tess_ocr_key_.SetImage(image.data, image.cols, image.rows, 1, image.step);
	tess_ocr_value_.SetImage(image.data, image.cols, image.rows, 1, image.step);

	int key_count_flag = 0; // key的列数
	int tmp_idx = 0;
	vector<string> tmp_keys;

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
		{
			Mat tmp_image;
			cv::cvtColor(image, tmp_image, CV_GRAY2BGR);
			drawRectangles(tmp_image, areas);
			//imshow("识别区域" + std::to_string(tmp_idx), tmp_image);
		}
		/*
		尝试ocr --> 判断是否是需要的数据
		key 在 value 的左边, 并且一一对应。有多少key列就有多少value列
		*/

		vector<string> result;
		int valid_count = 0;
		batchOCR(tess_ocr_key_, image, areas, result);
		valid_count = correctKey(result);
		if (valid_count > result.size() / 2)  // 匹配度超过一半 -> 可认为是key
		{
			tmp_keys.assign(result.begin(), result.end());
			++key_count_flag;
			continue;
		}
		if (key_count_flag > 0)
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
		if (result_.isMember(key))
		{
			result_[key] = values[i];
		}
	}
	return 0;
}


int Api_OCR::recognise(const vector<unsigned char>& image_buffer)
{
	/* 处理输入参数*/
	Mat src_image = imdecode(Mat(image_buffer), 0);
	if (src_image.data == NULL)
	{
		return INVILD_IMAGE;
	}
	//imshow("【原图】", src_image);

	/**************** 开始 ****************/
	int ret;
	Mat transform_image;
	ret = perspectiveTransformation(src_image, transform_image);
	if (ret != 0) return ret;
	//imshow("【透射变换结果】", transform_image);

	Mat dst_image;
	ret = findLeftAndRightEdge(transform_image, dst_image);
	if (ret != 0) return ret;
	//imshow("【左右边界定位结果】", dst_image);

	ret = cutAndOcr(dst_image);
	if (ret != 0) return ret;

	/**************** *** ****************/
	return 0;
}

void Api_OCR::retrieve(Json::Value& result)
{
	result["data"] = result_;
}
