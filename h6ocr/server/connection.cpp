#include "connection.hpp"
#include <utility>
#include <vector>
#include "connection_manager.hpp"
#include "request_handler.hpp"

namespace http {
	namespace server {

		connection::connection(boost::asio::ip::tcp::socket socket,
			connection_manager& manager, request_handler& handler)
			: socket_(std::move(socket)),
			connection_manager_(manager),
			request_handler_(handler)
		{
		}

		void connection::start()
		{
			do_read();
		}

		void connection::stop()
		{
			socket_.close();
		}

		void connection::do_read()
		{
			auto self(shared_from_this());
			socket_.async_read_some(boost::asio::buffer(buffer_),
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred)  //Lambda
			{
				if (!ec)  // error code
				{
					if (request_.is_in_headers)  // 数据还处于header范围
					{
						std::string data(buffer_.data(), buffer_.data() + bytes_transferred);
						int found = data.find("\r\n\r\n");
						if (found != std::string::npos)  //找到\t\n\t\n， header接收完成
						{
							request_.is_in_headers = false;
							/* 将header 转为 request_内的对象*/
							std::string header_str = request_.data + data.substr(0, found);
							std::cout << std::endl << "------- header------" << std::endl;
							std::cout << std::endl << header_str << std::endl;   //v
							request_parser_.parse(request_, header_str.c_str(), header_str.c_str() + header_str.size());
							for (header h : request_.headers)
							{

								request_.params[h.name] = h.value;
							}
							request_.body_len = stoi(request_.params["Content-Length"]);
							request_.data = data.substr(found + 4);
							
						}
						else  // header没接收完， 用request_.data暂存
						{
							request_.data += data;
						}
					}  // 数据还处于data范围
					else
					{
						request_.data += std::string(buffer_.data(), bytes_transferred);
						if (request_.data.size() == request_.body_len)
						{
							request_handler_.handle_request(request_, reply_);  // 处理请求
							std::cout << std::endl << "------- result------" << std::endl;
							std::cout << reply_.content << std::endl;
							do_write();
							/*  重新初始化*/
							request_.params.clear();
							request_.data = ""; //
							request_.is_in_headers = true;
						}
					}
					do_read();
				}
				else if (ec != boost::asio::error::operation_aborted)
				{
					connection_manager_.stop(shared_from_this());
				}
			});
		}

		void connection::do_write()
		{
			auto self(shared_from_this());
			boost::asio::async_write(socket_, reply_.to_buffers(),
				[this, self](boost::system::error_code ec, std::size_t)
			{
				if (!ec)
				{
					// Initiate graceful connection closure.
					boost::system::error_code ignored_ec;
					socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
						ignored_ec);
				}

				if (ec != boost::asio::error::operation_aborted)
				{
					connection_manager_.stop(shared_from_this());
				}
			});
		}

	} // namespace server
} // namespace http