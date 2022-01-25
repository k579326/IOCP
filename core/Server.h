
#pragma once
/*


*/

#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "id-generate.h"

class IObject;

namespace async
{

using ConnId = uint64_t;

class IoEventEngine;
class RemoteConnect;
class ListenConnect;
class IConnection;

class Observer
{
public:
	Observer() {}
	virtual ~Observer() {}

	virtual int OnConnect(ConnId connid) = 0; 
	virtual int OnRecv(ConnId connid, const void* data, size_t size) = 0;
	virtual int OnSend(ConnId connid) = 0;
	virtual int OnDisconnect(ConnId connid) = 0;
};


class Server
{
	friend class IConnection;
protected:
	Server(std::shared_ptr<Observer> ob);
	virtual ~Server();

public:

	// 监听
	virtual int Listen() = 0;
	virtual void OnConnect(IObject* obj) = 0;
	virtual void OnDisconnect() = 0;

	// 发送
	int Send(ConnId id, const void* data, size_t size);
	// 推送
	int Push(ConnId id, const void* data, size_t size);
	// 断开连接
	int Disconnect(ConnId id);

	void Run();
	void Stop();

	Observer* GetObserver();

	int RecvCallback(IConnection* conn, const void* data, size_t size);
	int SendCallback(IConnection* conn);

protected:
	// Init
	int InitEngine(size_t thread_count);
	void UninitEngine();

	void AddConnectToTable(ConnId id, std::shared_ptr<RemoteConnect> conn);
	std::shared_ptr<RemoteConnect> RemoveConnectFromTable(ConnId id);
	std::shared_ptr<RemoteConnect> FindConnectFromTable(ConnId id);

	ConnId ApplyConnId();

protected:
	using Conn = std::shared_ptr<RemoteConnect>;

	// 连接表
	std::map<ConnId, Conn> conntable_;
	std::mutex table_mtx_;
	std::condition_variable table_cond_;

	// 自身context
	std::shared_ptr<ListenConnect> listener_;
	std::shared_ptr<Observer> callback_;

	// iocp
	std::shared_ptr<IoEventEngine> engine_;

private:
	IdGenerater gen_;
	std::mutex id_mtx_;

};	
	
};
