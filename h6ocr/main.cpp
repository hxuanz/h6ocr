// main.cpp : 定义控制台应用程序的入口点。

#include "stdafx.h"
#include "server\server.hpp"
#include "my_log.h"
#include "arg_parser.h"
using namespace std;

int printHelp()
{
	cout << "usage: h6ocr -i port -o log_dir" << endl;
	return 0;
}

int main(int argc, char* argv[])
{
	ArgParser parser;
	parser.parse(argc, argv);
	if (parser.hasArg("h", "help"))
	{
		return printHelp();
	}

	string port = "12345";
	string log_dir = "logs";

	if (parser.hasArg("i"))
	{
		port = parser.getArg("i");
	}

	if (parser.hasArg("o"))
	{
		log_dir = parser.getArg("o");
	}

	MyLog::init(log_dir);

	try
	{
		http::server::server s("0.0.0.0", port, "");  // 初始化

		{
			string tmp = "启动服务\n端口: " + port + "\n日志目录 : " + log_dir + "\n ----------------------";
			cout << tmp << endl;
			_INFO(tmp);
		}

		s.run();
	}
	catch (std::exception& e)
	{
		_ERROR("exception: " + string(e.what()));
	}

	return 0;
}

