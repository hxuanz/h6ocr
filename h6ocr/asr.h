#pragma once
#include <vector>
using namespace std;
class ASR
{
public:
	ASR();
	~ASR();

	int convertFromat(unsigned char *src_data, unsigned char *dst_data);
	int recognise(vector<char>& av_data);
	string retrieve() { return result_; }

private:
	string result_;
};

