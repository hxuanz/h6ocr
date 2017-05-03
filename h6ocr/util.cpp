#include "util.h"

Point getLeftEndpointOfLine(Vec2f line)
{
	float rho = line[0], theta = line[1];
	return Point(0, (int)rho / sin(theta));
}

Point getRightEndpointOfLine(Vec2f line, float width)
{
	float rho = line[0], theta = line[1];
	if (COMMON::floatEqual(theta, M_PI / 2))
	{
		return Point(width, (int)rho);
	}
	float y = (-1 / tan(theta))*width + rho / sin(theta);
	return Point(width, y);
}


bool compareLine(Vec2f a, Vec2f b)
{
	return a[0] < b[0]; //升序
}
bool compareLine_theta(Vec2f a, Vec2f b)
{
	return a[1] < b[1];
}

void _filterLines(vector<Vec2f>& lines, float lo, float hi)
{
	sort(lines.begin(), lines.end(), compareLine_theta);
	vector<Vec2f>::iterator begin = lower_bound(lines.begin(), lines.end(), Vec2f(0, lo), compareLine_theta);
	vector<Vec2f>::iterator end = upper_bound(lines.begin(), lines.end(), Vec2f(0, hi), compareLine_theta);
	lines.assign(begin, end);
	sort(lines.begin(), lines.end(), compareLine);
}

void findLines_Horizonta(vector<Vec2f>& lines)
{
	_filterLines(lines, M_PI / 2 - 0.05, M_PI / 2 + 0.05);
}

void findLines_Vertical(vector<Vec2f>& lines)
{
	_filterLines(lines, -0.01, 0.01);
}


/* 横线中找到纵坐标跟值target最接近的两条直线*/
void findHorizontaLinesNearTarget(vector<Vec2f>& lines, float target)
{
	//lines 已经是排好序的
	vector<Vec2f>::iterator target_it = lower_bound(lines.begin(), lines.end(), Vec2f(target, 0), compareLine);
	lines.assign(target_it - 1, target_it + 1);
}


void drawLines(Mat &image, vector<Vec2f> &lines)
{
	for (size_t i = 0; i < lines.size(); i++)
	{
		float rho = lines[i][0], theta = lines[i][1];
		Point pt1, pt2;
		float a = cos(theta), b = sin(theta);
		float x0 = a*rho, y0 = b*rho;
		pt1.x = cvRound(x0 + 1000 * (-b));
		pt1.y = cvRound(y0 + 1000 * (a));
		pt2.x = cvRound(x0 - 1000 * (-b));
		pt2.y = cvRound(y0 - 1000 * (a));
		cv::line(image, pt1, pt2, Scalar(55, 100, 195), 1, CV_AA);
	}
}

void drawRectangles(Mat &image, vector<Rect>& rects)
{
	for (vector<Rect>::iterator it = rects.begin(); it != rects.end(); ++it)
	{
		cv::rectangle(image, *it, Scalar(255, 0, 0), 1);
	}
}


void _calCutPosition_Horizontal(const Mat& canny_image, vector<float>& position, int count_threshold)
{
	position.clear();
	position.push_back(0);
	int rows = canny_image.rows, cols = canny_image.cols;
	for (int i = 0; i < rows; ++i)
	{
		const uchar* p = canny_image.ptr<uchar>(i);
		int white_dots_count = 0;
		for (int j = 0; j < cols; ++j)
		{
			white_dots_count += p[j] > 200 ? 1 : 0;
			if (white_dots_count == count_threshold) break;
		}
		if (white_dots_count < count_threshold)
			position.push_back(i);
	}
	position.push_back(rows);
	COMMON::combineNearNumers(position, 2);
}
void _cut_Horizontal(const Mat& canny_image, Rect rect, vector<Rect>& rects, int threshold)
{
	rects.clear();
	vector<float> position;
	_calCutPosition_Horizontal(canny_image(rect), position, threshold);  // 子图

	for (int i = 1; i < position.size(); ++i)
	{
		int tmp_height = position[i] - position[i - 1];
		if (tmp_height > 10)  // 区域大小至少大于5像素
		{
			rects.push_back({ rect.x, (int)position[i - 1], rect.width, tmp_height });
		}
	}
}
int cut_Horizontal(const Mat& canny_image, Rect rect, vector<Rect>& rects)
{
	int height = canny_image.size().height;
	// 迭代改变阈值,确定合适的切割范围
	int threshold = 2;
	int try_count = 10;
	while (try_count--)
	{
		_cut_Horizontal(canny_image, rect, rects, threshold);
		float average = height / rects.size();  // 按理应该平均切割
		if (rects.size() == 1)
		{
			threshold += 1;
			continue;
		}
		int flag = 0;
		for (Rect rect : rects)
		{
			int diff_threshold = average / 2;
			if ((rect.height - average) > diff_threshold)
			{
				flag = 1; // 有个别rect多大 --> 总体数目偏小  --> 阈值增大
				break;
			}
			if ((average - rect.height) > diff_threshold)
			{
				flag = -1; // 个数偏大
				break;
			}
		}

		if (flag == 0)
		{
			break;
		}
		threshold += flag;
	}
	return try_count > 0 ? 0 : -1;
}

void _calcutPosition_Vertical(const Mat& canny_image, vector<float>& position, int count_threshold)
{
	int rows = canny_image.rows, cols = canny_image.cols;
	position.clear();
	position.push_back(0);
	uchar* data = canny_image.data;
	for (int j = 0; j < cols; ++j)
	{
		uchar* p = data + j;
		int white_dots_count = 0;
		for (int i = 0; i < rows; ++i)
		{
			uchar val = *(p + i *cols);
			white_dots_count += val > 200 ? 1 : 0;
			if (white_dots_count == count_threshold) break;
		}
		if (white_dots_count < count_threshold)
			position.push_back(j);
	}
	position.push_back(cols);
	COMMON::combineNearNumers(position, 5);
}
int cut_Vertical(const Mat& canny_image, Rect rect, vector<Rect>& rects)
{
	rects.clear();
	vector<float> position;
	int threshold = 8;
	_calcutPosition_Vertical(canny_image(rect), position, threshold);

	for (int i = 1; i < position.size(); ++i)
	{
		int tmp_width = position[i] - position[i - 1];
		if (tmp_width > 10)
		{
			rects.push_back({ (int)position[i - 1], rect.y, tmp_width, rect.height });
		}
	}
	return 0;
}


Rect cutByRange(Rect rect, RangeLR range)
{
	return{ range.left, rect.y, range.right - range.left, rect.height };
}

void stripRight(string &str)
{
	int n = str.size(), i;
	for (i = n - 1; i > -1; --i)
	{
		if (str[i] != '\n')
		{
			str.resize(i + 1);
			break;
		}
	}
}


void batchOCR(tesseract::TessBaseAPI& tess_ocr, const Mat& image, const vector<Rect>& areas, vector<string> &result)
{
	result.clear();
	for (Rect rect : areas)
	{
		tess_ocr.SetRectangle(rect.x, rect.y, rect.width, rect.height);
		string text = tess_ocr.GetUTF8Text();
		stripRight(text);
		result.push_back(text);
	}
}

RangeLR findEndPointsOfLine(Mat image, int idx)
{
	RangeLR res;
	int n = image.cols;
	uchar* data = image.data + n*idx;

	for (int j = 0; j < n; j++)
	{
		if (*(data + j)  < 128)  // black 
		{
			res.left = j;
			break;
		}
	}
	for (int j = n - 1; j >-1; j--)
	{
		if (*(data + j)  < 128)  // black 
		{
			res.right = j;
			break;
		}
	}
	return res;
}

/* 迭代改变阈值，检测直线*/
int detectLines(const Mat& src_image, vector<Vec2f>& lines)
{
	Mat canny_image;
	Canny(src_image, canny_image, 50, 200, 3);  /* 边缘检测 */

	int width = canny_image.size().width;
	float threshold = width * 0.3;
	float step = width * 0.03;

	int try_count = 8;
	while (try_count--)
	{
		cv::HoughLines(canny_image, lines, 1, CV_PI / 180, (int)threshold, 0, 0); // 霍夫线直线检测

		findLines_Horizonta(lines);
		int len = lines.size();
		if (len > 10)
		{
			threshold += step;
		}
		else if (len < 3)
		{
			threshold -= step;  // 目标个数过少，减小阈值
			continue;
		}
		else  /* 3~10 */
		{
			break;
		}
	}
	return try_count > 0 ? 0 : -1;
}