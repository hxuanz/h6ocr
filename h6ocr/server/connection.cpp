// 处理单个连接
#include "connection.hpp"
#include <utility>
#include <vector>
#include <unordered_map>
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

		/* 接受的是http post 请求，
		数据分为两部分：header 和 data。 两部分用 \t\n\t\n 隔开
		*/
		void connection::do_read()
		{
			auto self(shared_from_this());
			socket_.async_read_some(boost::asio::buffer(buffer_),  
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred)  //Lambda
			{
				if (!ec) 
				{
					if (request_.is_in_headers) 
					{
						std::string data(buffer_.data(), buffer_.data() + bytes_transferred);
						int found = data.find("\r\n\r\n");
						if (found != std::string::npos)  //出现 \t\n\t\n  -> header接收完成
						{
							request_.is_in_headers = false;

							std::string header_str = request_.data + data.substr(0, found);
							request_parser_.parse(request_, header_str.c_str(), header_str.c_str() + header_str.size());
							for (header h : request_.headers)
							{
								if (h.name == "Content-Length")
								{
									request_.body_len = stoi(h.value);
									break;
								}
							}
							request_.data = data.substr(found + 4);  // 跳过4个字节 \r\n\r\n
						}
						else  // header没接收完，用request_.data暂存
						{
							request_.data += data; 
						}
					}  
					else  
					{
						request_.data += std::string(buffer_.data(), bytes_transferred);
						if (request_.data.size() == request_.body_len)  //data接受完毕
						{
							request_handler_.handle_request(request_, reply_);  // 处理请求， 结果写入reply_对象中
							/* 发送给客户端 */
							do_write();

							/*  重新初始化*/
							request_.data = ""; 
							request_.is_in_headers = true;
						}
					}
					do_read(); // 再次读取
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