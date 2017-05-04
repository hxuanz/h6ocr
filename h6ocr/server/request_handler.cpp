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

			// Request path must be absolute and not contain "..".
			if (request_path.empty() || request_path[0] != '/'
				|| request_path.find("..") != std::string::npos)
			{
				rep = reply::stock_reply(reply::bad_request);
				return;
			}

			// If path ends in slash (i.e. is a directory) then add "index.html".
			if (request_path[request_path.size() - 1] == '/')
			{
				request_path += "index.html";
			}

			// Determine the file extension.
			std::size_t last_slash_pos = request_path.find_last_of("/");
			std::size_t last_dot_pos = request_path.find_last_of(".");
			std::string extension;
			if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
			{
				extension = request_path.substr(last_dot_pos + 1);
			}
			

			/* 分发请求，返回数据 */ 
			Blood_OCR& bloodocr = Blood_OCR::Instance();//
			int ret = 0;
			if (request_path == "/ocr/blood")
			{
				std::string data;
				url_decode(req.data, data);  // url_decode
				std::vector<uchar> iamge_buffer = base64_decode(data);  //base64_decode
				ret = bloodocr.recognise(iamge_buffer); /* 调用ocr api */
			}
			else
			{
				ret = H6OCRERROR::INVILD_URI;
			}

			Json::Value result_root;
			if (ret == H6OCRERROR::SUCCESS)
			{
				bloodocr.retrieve(result_root);
			}

			{
				result_root["err_no"] = ret;
				Json::FastWriter writer;
				rep.content = writer.write(result_root);
			}
			rep.status = reply::ok;
			rep.headers.resize(2);
			rep.headers[0] = { "Content-Length", std::to_string(rep.content.size()) };
			rep.headers[1] = { "Content-Type", mime_types::extension_to_type(extension) };
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