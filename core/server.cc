

#include "Server.h"

#include <assert.h>
#include "engine.h"
#include "conn.h"


namespace async
{
Server::Server(std::shared_ptr<Observer> ob) 
	: callback_(ob)
{
	engine_.reset(new IoEventEngine);
}
Server::~Server() {}

int Server::InitEngine(size_t thread_count)
{
	return engine_->Create(thread_count) ? 0 : -1;
}
void Server::UninitEngine()
{
	engine_->Destory();
}


// 发送
int Server::Send(ConnId id, const void* data, size_t size)
{
	int rst = 0;
	table_mtx_.lock();
	rst = conntable_.find(id)->second->PostSend(data, size);
	table_mtx_.unlock();
	return rst;
}
// 推送
int Server::Push(ConnId id, const void* data, size_t size)
{
	int rst = 0;
	table_mtx_.lock();
	auto conn = conntable_.find(id);
	if (conn != conntable_.end())
		rst = conn->second->Push(data, size);
	table_mtx_.unlock();
	return rst;
}
// 断开连接
int Server::Disconnect(ConnId id)
{
	auto conn = FindConnectFromTable(id);
	if (!conn)
		return 0;

	return conn->Close();
}

Observer* Server::GetObserver()
{
	return callback_.get();
}

void Server::Run()
{
	engine_->Run();
}

void Server::Stop()
{
	// stop listener

	// close all connection
	table_mtx_.lock();
	for (auto it = conntable_.begin(); it != conntable_.end(); it++)
	{
		it->second->Close();
	}
	table_mtx_.unlock();

	// wait until connection table is empty()
	{
		std::unique_lock<std::mutex> autolock(table_mtx_);
		table_cond_.wait(autolock, [this]()->bool {
			return conntable_.empty();
		});
	}

	// close IOCP
	engine_->Destory();
}

void Server::AddConnectToTable(ConnId id, std::shared_ptr<RemoteConnect> conn)
{
	table_mtx_.lock();
	if (conntable_.find(id) != conntable_.end())
		assert(false);
	conntable_.insert(std::make_pair(id, conn));
	table_mtx_.unlock();
}

std::shared_ptr<RemoteConnect> Server::RemoveConnectFromTable(ConnId id)
{
	std::shared_ptr<RemoteConnect> rst = NULL;
	table_mtx_.lock();
	auto iterator = conntable_.find(id);
	if (iterator != conntable_.end())
	{
		rst = iterator->second;
		conntable_.erase(iterator);
		table_cond_.notify_one();
	}
	table_mtx_.unlock();
	return rst;
}

std::shared_ptr<RemoteConnect> Server::FindConnectFromTable(ConnId id)
{
	std::shared_ptr<RemoteConnect> rst = NULL;
	table_mtx_.lock();
	auto iterator = conntable_.find(id);
	if (iterator != conntable_.end())
	{
		rst = iterator->second;
	}
	table_mtx_.unlock();
	return rst;
}


int Server::RecvCallback(IConnection* conn, const void* data, size_t size)
{
	return callback_->OnRecv(conn->GetConnId(), data, size);
}
int Server::SendCallback(IConnection* conn)
{
	return callback_->OnSend(conn->GetConnId());
}
ConnId Server::ApplyConnId()
{
	return gen_.GenNextId();
}

};
