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

#include <boost/algorithm/string.hpp>

/* 解析post的data字段，键值对。  eg：  ....=...&...=...&......=... */
void parse_params(std::string data, std::unordered_map<std::string, std::string>& params)
{
	std::vector<std::string> strs;
	boost::split(strs, data, boost::is_any_of("&"));
	for (auto str : strs)
	{
		std::vector<std::string> key_val;
		boost::split(key_val, str, boost::is_any_of("="));
		params[key_val.front()] = key_val.back();
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
			// Decode url to path.
			std::string request_path;
			if (!url_decode(req.uri, request_path))
			{
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			// 必须是以 / 开头的绝对路径
			if (request_path.empty() || request_path[0] != '/' || request_path.find("..") != std::string::npos)
			{
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			// If path ends in slash (i.e. is a directory) then add "index.html".
			//if (request_path[request_path.size() - 1] == '/')
			//{
			//	request_path += "index.html";
			//}

			// 确定文件扩展名 extension.
			//std::size_t last_slash_pos = request_path.find_last_of("/");
			//std::size_t last_dot_pos = request_path.find_last_of(".");
			//std::string extension;
			//if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
			//{
			//	extension = request_path.substr(last_dot_pos + 1);
			//}
			

			/* 分发请求，返回数据 */ 
			Json::Value result_root;
			int ret = 0;
			if (request_path == "/ocr/xhd")
			{
				/* 解析数据段*/
				std::string data;
				url_decode(req.data, data);  

				std::unordered_map<std::string, std::string> params; /* 解析 post 请求的数据段*/
				parse_params(data, params);

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
			
			result_root["error_no"] = ret;
			if (ret == H6OCRERROR::SUCCESS)
			{
				Json::FastWriter writer;
				rep.content = writer.write(result_root);
				cout << rep.content << endl;
			}
			rep.status = reply::ok;
			rep.headers.resize(2);
			rep.headers[0] = { "Content-Length", std::to_string(rep.content.size()) };
			rep.headers[1] = { "Content-Type", "application/json"};  //本应根据extension确定
		}

		bool request_handler::url_decode(const std::string& in, std::string& out)
		{
			out.clear();
			out.reserve(in.size());
			for (std::size_t i = 0; i < in.size(); ++i)
			{
				if (in[i] == '%')
				{
					if (i + 3 <= in.size())
					{
						int value = 0;
						std::istringstream is(in.substr(i + 1, 2));
						if (is >> std::hex >> value)
						{
							out += static_cast<char>(value);
							i += 2;
						}
						else
						{
							return false;
						}
					}
					else
					{
						return false;
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
			return true;
		}

	} // namespace server
} // namespace http