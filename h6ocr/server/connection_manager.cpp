// 管理所有连接
#include "connection_manager.hpp"
#include "../my_log.h"
namespace http {
	namespace server {

		connection_manager::connection_manager()
		{
		}

		void connection_manager::start(connection_ptr c)
		{
			_INFO("新的请求");
			_INFO("对方地址： " + c->get_address());
			connections_.insert(c);
			c->start();
		}

		void connection_manager::stop(connection_ptr c)
		{
			_INFO("本次请求结束");
			connections_.erase(c);
			c->stop();
		}

		void connection_manager::stop_all()
		{
			_INFO("所有请求结束");
			for (auto c : connections_)
				c->stop();
			connections_.clear();
		}

	} // namespace server
} // namespace http