#ifndef __MYLOG_H__
#define __MYLOG_H__

#include <sstream>
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>


class MyLog{
public:
	static void write(std::string msg, std::string type);
	static void init(std::string dir);

private:
	static std::string _file_name;
	static std::ofstream _file;
	static std::string _dir;
};

#define _INFO(msg) MyLog::write(msg, " [INFO  ] ")
#define _ERROR(msg) MyLog::write(msg, " [ERROR] ")
#endif //__MYLOG_H__