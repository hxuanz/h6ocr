// main.cpp : �������̨Ӧ�ó������ڵ㡣

#include "stdafx.h"
#include "server\server.hpp"
#include "my_log.h"
using namespace std;

int main(int argc, char* argv[])
{
	try
	{
		string port = "12345";
		if (argc == 2)
		{
			port = argv[1];
		}

		MyLog::init("d:/logs");

		http::server::server s("0.0.0.0", port, "");  // ��ʼ��
		_INFO("��������  port: " + port);
		s.run();
	}
	catch (std::exception& e)
	{
		_ERROR("exception: " + string(e.what()));
	}

	return 0;
}

