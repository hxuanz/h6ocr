#pragma once

#include <cstring>
#include <cstdio>
#include <unordered_map>
#include <vector>
using namespace std;

class ArgParser
{
public:
	ArgParser();
	ArgParser::~ArgParser();

	void parse(int argc, char* argv[]);
	string getArg(string key);
	bool hasArg(string key);
	bool hasArg(string key1, string key2){ return hasArg(key1) || hasArg(key2); }


	string getPositionalArgByIndex(int i) {
		return _free_options[i];
	}

private:

	unordered_map<string, string> _args;
	vector<string> _free_options;

};
