#include "arg_parser.h"


ArgParser::ArgParser()
{
}


ArgParser::~ArgParser()
{
}


void ArgParser::parse(int argc, char* argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			string key = argv[i] + ((argv[i][1] == '-') ? 2 : 1);

			if (i + 1 < argc && argv[i + 1][0] != '-')
			{
				_args[key] = argv[i + 1];
				++i;  // skip
			}
			else
				_args[key] = "";
		}
		else
		{
			_free_options.push_back(argv[i]);
		}
	}
}

string ArgParser::getArg(string key)
{
	if (hasArg(key))
	{
		return _args[key];
	}
	return "";
}

bool ArgParser::hasArg(string key)
{
	return _args.count(key) == 1;
}