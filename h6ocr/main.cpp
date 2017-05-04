// h6ocr.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"


using namespace std;

//#define DEBUG_MODE

#ifdef DEBUG_MODE

#include "base64.h"
#include "blood_ocr.h"
#include "json\json.h"
int main(int argc, char* argv[])
{
	std::vector<uchar> buffer;
	{
		string path = "D://ocr//1.jpg";
		ifstream fileInput(path, ios_base::binary);
		if (!fileInput) return -1;
		//��ȡ�ļ���С
		fileInput.seekg(0, std::ios_base::end);
		const size_t maxSize = fileInput.tellg();
		//�����ļ�ָ��
		fileInput.seekg(0, std::ios_base::beg);
		//��ȡͼƬ�ļ�
		char *pic = new char[maxSize];
		fileInput.read(pic, maxSize);

		buffer.assign((uchar*)pic, (uchar*)pic + maxSize);
		delete[] pic;
		fileInput.close();
	}
	Blood_OCR& bloodocr = Blood_OCR::Instance();//
	int ret = bloodocr.recognise(buffer); /* ����ocr api */
	if (ret != 0)
	{
		cerr << ret << endl;
		return -1;
	}
	Json::Value result_root;
	bloodocr.retrieve(result_root);
	Json::FastWriter writer;
	string result = writer.write(result_root);

	cout << result;
	cv::waitKey(0);
	return 0;
}

#else

#include "server\server.hpp"
int main(int argc, char* argv[])
{
	try
	{
		string address, port;
		if (argc != 3)
		{
			address = "0.0.0.0";
			port = "12345";
		}
		else
		{
			address = argv[1];
			port = argv[2];
		}

		// Initialise the server.
		http::server::server s(address, port, "");

		// Run the server until stopped.
		cout << "Run the server...." << "port: " << port << endl;
		s.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "exception: " << e.what() << "\n";
	}

	return 0;
}
#endif

