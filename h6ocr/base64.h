#pragma once

#include <vector>
#include <string>

typedef unsigned char uchar;

std::string base64_encode(uchar const* buf, unsigned int bufLen);
std::vector<uchar> base64_decode(std::string const&);