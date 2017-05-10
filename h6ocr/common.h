#pragma once
using namespace std;
namespace COMMON
{
	void combineNearNumers(vector<float> &idxs, int len_threshold);
	bool isCharEqual(unsigned char a, unsigned char b);
	int minEditDistance(string word1, string word2);
	bool floatEqual(float d1, float d2, float accuracy = 0.000001);
	string stripAllSpace(string str);
	bool isFloat(string myString);
}