#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"

#include <unordered_map>
#include "../base64.h"
#include "../blood_ocr.h"

#include "json/json.h"
#include "../err.h"
#include "../my_log.h"
#include <boost/algorithm/string.hpp>

/// Perform URL-decoding on a string.
std::string url_decode(std::string in)
{
	std::string out;
	out.reserve(in.size());
	for (std::size_t i = 0; i < in.size(); ++i)
	{
		if (in[i] == '%')
		{
			if (i + 3 <= in.size())
			{
				int value = 0;
				std::istringstream is(in.substr(i + 1, 2));
				if (is >> std::hex >> value)  // 十六进制
				{
					out += static_cast<char>(value);
					i += 2;
				}
				else
				{
					return "";
				}
			}
			else
			{
				return "";
			}
		}
		else if (in[i] == '+')
		{
			out += ' ';
		}
		else
		{
			out += in[i];
		}
	}
	return out;
}


/* 解析post的data字段，键值对。  eg：  ....=...&...=...&......=... */
void parse_params(std::string data, std::unordered_map<std::string, std::string>& params)
{
	std::vector<std::string> strs;
	boost::split(strs, data, boost::is_any_of("&"));
	for (auto str : strs)
	{
		std::vector<std::string> key_val;
		boost::split(key_val, str, boost::is_any_of("="));
		/* 参数解析之后再URL解码 */
		params[url_decode(key_val.front())] = url_decode(key_val.back());
	}
}

namespace http {
	namespace server {

		request_handler::request_handler(const std::string& doc_root)
			: doc_root_(doc_root)
		{
			
		}

		void request_handler::handle_request(const request& req, reply& rep)
		{
			std::string request_path = url_decode(req.uri);
			// 必须是以 / 开头的绝对路径
			if (request_path.empty() || request_path[0] != '/' || request_path.find("..") != std::string::npos)
			{
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			/* 分发请求，返回数据 */ 
			Json::Value result_root;
			int ret = 0;
			if (request_path == "/ocr/xhd")
			{
				_INFO("解析数据段");
				/* 解析数据段*/
				std::unordered_map<std::string, std::string> params; /* 解析 post 请求的数据段*/
				parse_params(req.data, params);
				if (params.count("imageData") == 0 || params.count("scaleData") == 0)
				{
					_ERROR("请求缺少参数");
					ret = H6OCRERROR::INVILD_PARAMS;
					goto BREAK;
				}
				if (params.at("imageData").size() == 0)
				{
					ret = H6OCRERROR::INVILD_PARAMS_imageData;
					_ERROR("imageData 有问题");
					goto BREAK;
				}
				if (params.at("scaleData").size() == 0)
				{
					ret = H6OCRERROR::INVILD_PARAMS_scaleData;
					_ERROR("scaleData 有问题");
					goto BREAK;
				}

				std::vector<uchar> iamge_buffer = base64_decode(params.at("imageData"));  //base64_decode

				Blood_OCR& bloodocr = Blood_OCR::Instance();
				ret = bloodocr.loadDictionary(params.at("scaleData"));
				if (ret != 0) 
					goto BREAK;
				ret = bloodocr.recognise(iamge_buffer);
				if (ret != 0) 
					goto BREAK;

				bloodocr.retrieve(result_root);
			}
			else /*  后续可能会有其他类型的请求 */
			{
				ret = H6OCRERROR::INVILD_URI;
			}

		BREAK:
			if (ret != SUCCESS)
			{
				std::cout << "fail!" << std::endl;
				_ERROR("本次OCR失败");
				_ERROR("错误代码: " + to_string(ret));
			}
			else
			{
				std::cout << "success!" << std::endl;
			}
			result_root["error_no"] = ret;
			Json::FastWriter writer;
			rep.content = writer.write(result_root);
		
			rep.status = reply::ok;
			rep.headers.resize(2);
			rep.headers[0] = { "Content-Length", std::to_string(rep.content.size()) };
			rep.headers[1] = { "Content-Type", "application/json"};  //本应根据extension确定
		}
	} // namespace server
} // namespace http