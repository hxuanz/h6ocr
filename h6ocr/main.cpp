// main.cpp : 定义控制台应用程序的入口点。

#include "stdafx.h"
#include "server\server.hpp"

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

		http::server::server s("0.0.0.0", port, "");  // 初始化
		cout << "Run the server...." << endl << "port: " << port << endl;
		s.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "exception: " << e.what() << "\n";
	}

	return 0;
}

