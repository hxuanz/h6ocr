#include "blood_ocr.h"
#include <time.h> 
#include "err.h"
using namespace std;
using namespace cv;

Blood_OCR::Blood_OCR()
{
	loadDictionary();
	initOcrEngine();
	//buildBKTree();
	cout << "init ocr api success!" << endl;
}

Blood_OCR::~Blood_OCR()
{
	//destoryBKTree(bk_tree_);
}

int Blood_OCR::initOcrEngine()
{
	tess_ocr_key_.Init(NULL, "fontyp", tesseract::OEM_TESSERACT_ONLY);
	tess_ocr_key_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	tess_ocr_value_.Init(NULL, "num", tesseract::OEM_TESSERACT_ONLY);
	tess_ocr_value_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	return 0;
}

// ��ȡ�����ļ��� result_ key��ֵ


int Blood_OCR::loadDictionary()
{
	ifstream ifs("blood.dict", std::ifstream::in);
	string line;
	while (!ifs.eof()){
		std::getline(ifs, line);
		result_[line] = ""; // 
	}
	ifs.close();
	return 0;
}

///* ����BK�� */
//void Blood_OCR::buildBKTree()
//{
//	
//	vector<string> words = result_.getMemberNames();
//	
//	for (string word : words)
//	{
//		if (bk_tree_ == NULL)  // ��һ��������Ϊ��
//		{
//			bk_tree_ = new BK_TREE(word);
//			continue;
//		}
//		/* ����*/
//		BK_TREE* node = bk_tree_;
//		while (true)
//		{
//			int dis = COMMON::minEditDistance(word, node->word);
//			if (node->childs.count(dis) == 0)  //�������ֵ�Ǹýڵ㴦ͷһ�γ��֣�����һ���µĶ��ӽڵ�
//			{
//				BK_TREE* tmp = new BK_TREE(word);
//				node->childs[dis] = tmp;
//				break;
//			}
//			else  //�ݹ�
//			{
//				node = node->childs[dis];
//			}
//		}
//	}
//}
//
///* �ݹ��ͷ�BK�� */
//void Blood_OCR::destoryBKTree(BK_TREE* tree)
//{
//	if (tree == NULL) return;
//	bk_childs& childs = tree->childs;
//	delete tree;
//	for (auto x : childs)
//	{
//		BK_TREE* node = x.second;
//		destoryBKTree(node);
//	}
//}
///* ���㵥������ڵ�ı༭����d��
//Ȼ��ݹ����ÿ���ӽڵ���Ϊd-n��d+n���������ıߡ�
//���类���Ľڵ����������ʵľ���dС��n���򷵻ظýڵ㲢������ѯ��*/
//void Blood_OCR::searchInBkTree(BK_TREE* tree, int threshold, string& word)
//{
//	if (tree == NULL){
//		return;
//	}
//	int dis = COMMON::minEditDistance(tree->word, word);
//	if (dis <= threshold)
//	{
//		word = tree->word;
//		return;
//	}
//	//�ݹ����ÿ���ӽڵ���Ϊd - n��d + n���������ı�
//	int lo = dis - threshold;
//	int hi = dis + threshold;
//	for (int i = lo; i <= hi; ++i)
//	{
//		if (tree->childs.count(i) == 1)
//			searchInBkTree(tree->childs.at(i), threshold, word);
//	}
//}
//

// ���ֵ䳢��ƥ�䣬����ƥ��ɹ�������
//int Blood_OCR::correctKey(vector<string> &keys)
//{
//	vector<string> &names = result_.getMemberNames();
//	int valid_count = 0;
//	for (string& key : keys)
//	{
//		if (result_.isMember(key))  //ֱ���ҵ�
//		{
//			++valid_count;
//			continue;
//		}
//		/* ���ݱ༭������Сԭ�򣬾���*/
//		int threshold = 2;
//		string tmp = key;
//		searchInBkTree(bk_tree_, threshold, key);
//		if (tmp != key)
//		{
//			++valid_count;
//		}
//	}
//	return valid_count;
//}

/* �ҵ����� ��  ��λ�ĸ��� �� ͸�ӱ任 */
int Blood_OCR::perspectiveTransformation(const Mat& src_image, Mat& dst_image)
{
	Size src_image_size = src_image.size();

	/* �Һ���*/
	vector<Vec2f> lines;
	{
		if (detectLines(src_image, lines) != 0)
		{
			return H6OCRERROR::perspective_detectLines;
		}
		//{
		//	Mat tmp_image(src_image);
		//	drawLines(tmp_image, lines);
		//	imshow("����ʶ����", tmp_image);
		//}
		/* �ҵ���Ҫ��ֱ��*/
		findHorizontaLinesNearTarget(lines, src_image_size.height / 2);  // �м�λ�ã�������ɢ 
	}

	{	/* ͸��任 */
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

/* �ҵ��������ֱ�Ե */
int Blood_OCR::findLeftAndRightEdge(const Mat& src_image, Mat& dst_image)
{
	vector<Vec2f> lines;
	{
		Mat canny_image;
		Canny(src_image, canny_image, 50, 200, 3);
		int threshold = 5;  // ��ֵ:5  
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



// ���ֵ䳢��ƥ�䣬����ƥ��ɹ�������
int Blood_OCR::correctKey(vector<string> &keys)
{
	vector<string> &names = result_.getMemberNames();
	int valid_count = 0;
	for (string& key : keys)
	{
		if (result_.isMember(key))  //ֱ���ҵ�
		{
			++valid_count;
			continue;
		}
		/* ���ݱ༭������Сԭ�򣬾���*/
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
/* �����Ƿ���float�ж�*/
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
	Canny(image, canny_image, 50, 200, 3);  /* ��Ե��� -> ��ɺڰ�ͼ��[���ڼ���] */

	/* �����и�  --> �õ������� */
	vector<Rect> rects;
	{
		Rect image_rect = { 0, 0, image_size.width, image_size.height };
		cut_Vertical(canny_image, image_rect, rects);

		//Mat tmp;
		//cv::cvtColor(image, tmp, CV_GRAY2BGR);
		//drawRectangles(tmp, rects);
		//imshow("�����и�����ʶ��", tmp);
	}
	if (rects.empty())
	{
		return H6OCRERROR::cutAndOcr_cut_Vertical;
	}
		
	vector<string> keys, values;

	tess_ocr_key_.SetImage(image.data, image.cols, image.rows, 1, image.step);
	tess_ocr_value_.SetImage(image.data, image.cols, image.rows, 1, image.step);

	int key_count_flag = 0; // key������
	int tmp_idx = 0;
	vector<string> tmp_keys;

	for (Rect rect : rects)
	{
		++tmp_idx;
		/* �����и �õ����յ�ʶ��С���� */
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
		//	//imshow("ʶ������" + std::to_string(tmp_idx), tmp_image);
		//}
		/*
		����ocr --> �ж��Ƿ�����Ҫ������
		key �� value �����, ����һһ��Ӧ���ж���key�о��ж���value��
		*/

		vector<string> result;
		int valid_count = 0;
		batchOCR(tess_ocr_key_, image, areas, result);
		valid_count = correctKey(result);
		if (valid_count > result.size() / 3)  // ƥ��ȳ���һ�� -> ����Ϊ��key
		{
			tmp_keys.assign(result.begin(), result.end());
			++key_count_flag;
			continue;
		}
		if (key_count_flag > 0)
		{
			batchOCR(tess_ocr_value_, image, areas, result);
			valid_count = correctValue(result);
			if (valid_count > result.size() / 2)  // ƥ��ȳ���һ�� -> ����Ϊ��value
			{
				--key_count_flag;
				/* ȷ��key value ����ͬ*/
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


int Blood_OCR::recognise(const vector<unsigned char>& image_buffer)
{
	/* �����������*/
	Mat src_image = imdecode(Mat(image_buffer), 0);
	if (src_image.data == NULL)
	{
		return INVILD_IMAGE;
	}
	//imshow("��ԭͼ��", src_image);

	/**************** ��ʼ ****************/
	int ret;
	Mat transform_image;
	ret = perspectiveTransformation(src_image, transform_image);
	if (ret != 0) return ret;
	//imshow("��͸��任�����", transform_image);

	Mat dst_image;
	ret = findLeftAndRightEdge(transform_image, dst_image);
	if (ret != 0) return ret;
	//imshow("�����ұ߽綨λ�����", dst_image);

	ret = cutAndOcr(dst_image);
	if (ret != 0) return ret;

	/**************** *** ****************/
	return 0;
}

void Blood_OCR::retrieve(Json::Value& result)
{
	result["data"] = result_;
}
