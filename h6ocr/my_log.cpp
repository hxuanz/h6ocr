#include "my_log.h"
#include <ctime>


using namespace std;

string MyLog::_file_name = "";
ofstream MyLog::_file;
string MyLog::_dir = ".";

string time2string(time_t rawtime, string format)
{
	struct tm * timeinfo;
	char buffer[80];
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), format.c_str(), timeinfo);
	return string(buffer);
}


void MyLog::init(string dir){
	_dir = dir;

	time_t rawtime;
	time(&rawtime);
	string time_str = time2string(rawtime, "%Y%m%d_%H");
	_file_name = _dir + "/" + time_str + ".log";

	if (_dir.back() == '/')
	{
		_dir.pop_back();
	}
	/*  打开对应的日志文件 */
	_file.open(_file_name.c_str(), ios::app | ios::out);

	if (!_file){
		cerr << "Log file open failed!" << endl;
		exit(0);
	}
	return;
}

void MyLog::write(string msg, string type){
	//pthread_mutex_lock(&_locker);
	time_t rawtime;
	time(&rawtime);
	string time_str = time2string(rawtime, "%Y%m%d_%H");
	string file_name = _dir + "/" + time_str + ".log";

	if (file_name != _file_name)
	{
		_file_name = file_name;
		_file.close();
		_file.open(_file_name.c_str(), ios::app | ios::out);
		if (!_file){
			cerr << "Log file open failed!" << endl;
			exit(0);
		}
	}
	//write log
	_file << "[" << time2string(rawtime, "%Y%m%d_%H:%M:%S") << "]" << type << msg << endl;
	//pthread_mutex_unlock(&_locker);
	return;
}