// h6ocr.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


using namespace std;

//#define DEBUG_MODE

#ifdef DEBUG_MODE
#include "master.h"
int main(int argc, char* argv[])
{
	std::unordered_map<std::string, std::string> params;
	{
		params["dict"] = "{\"r_GT\":\"\",\"A/G\":\"\",\"ALB\":\"\",\"ALP\":\"\",\"ALT\":\"\",\"ASL\":\"\",\"CHO\":\"\",\"Crea\":\"\",\"DBILI\":\"\",\"GLB\":\"\",\"GLU\":\"\",\"HBsAg\":\"\",\"TBILI\":\"\",\"TG\":\"\",\"TP\":\"\",\"UA\":\"\",\"UREA\":\"\"}";
	}
	string data;
	{
		string path = "D://ocr//1.jpg";
		ifstream fileInput(path, ios_base::binary);
		if (!fileInput) return -1;
		//获取文件大小
		fileInput.seekg(0, std::ios_base::end);
		const size_t maxSize = fileInput.tellg();
		//重置文件指针
		fileInput.seekg(0, std::ios_base::beg);
		//读取图片文件
		char *pic = new char[maxSize];
		fileInput.read(pic, maxSize);

		data.assign((uchar*)pic, (uchar*)pic + maxSize);
		delete[] pic;
		fileInput.close();
	}

	Master* master = Master::getInstance();
	string result;
	data = base64_encode((uchar const*)data.c_str(), data.size());
	master->distribute("/ocr", params, data, result);
	master->release();

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

