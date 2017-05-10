#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <vector>
#include "header.hpp"

namespace http {
	namespace server {

		/// A request received from a client.
		struct request
		{
			request() : body_len(0), is_in_headers(true), data(""){}
			std::string method;
			std::string uri;
			int http_version_major;
			int http_version_minor;
			std::vector<header> headers;
			std::string data;
			int body_len;  //
			bool is_in_headers;
		};

	} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HPP