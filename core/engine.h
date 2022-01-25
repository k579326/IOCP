#pragma once

#include <windows.h>
#include <stdint.h>

#include <list>
#include <memory>
#include <mutex>
#include <condition_variable>

class IObject;

namespace async
{
class IConnection;

class IoEventEngine
{
public:
	IoEventEngine();
	virtual ~IoEventEngine();


	bool Create(size_t thread_num);
	bool Register(IObject* obj);
	void Destory();

	void Run();

private:

	void Stop();
	void EventLoop();

	void AddbrokenConnection(IConnection* conn);
	void CleanConnection();

private:
	
	std::list<IConnection*> broken_conn_list_;
	std::mutex mtx_;
	std::condition_variable cond_;

	uint8_t thread_count_;
	HANDLE	iocp_;
	bool	exit_;
};



};
