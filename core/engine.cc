
#include "engine.h"

#include <thread>
#include <assert.h>

#include "conn.h"
#include "iocp_define.h"
#include "io-object.h"

#include "logger.h"

namespace async
{


IoEventEngine::IoEventEngine() 
{
	iocp_ = INVALID_HANDLE_VALUE;
	thread_count_ = 0;
	exit_ = false;
}
IoEventEngine::~IoEventEngine() 
{
	if (iocp_ != INVALID_HANDLE_VALUE)
		CloseHandle(iocp_);
	iocp_ = INVALID_HANDLE_VALUE;
}


bool IoEventEngine::Create(size_t thread_num)
{
	HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, thread_num);
	if (iocp == NULL)
		return false;

	iocp_ = iocp;
	thread_count_ = thread_num;

	return true;
}

void IoEventEngine::Destory()
{
	Stop();
}

bool IoEventEngine::Register(IObject* obj)
{
	if (CreateIoCompletionPort(obj->IoHandle(), iocp_, (ULONG_PTR)obj->GetKey(), thread_count_) != iocp_)
		return false;

	return true;
}


void IoEventEngine::Run()
{
	std::thread connfree(&IoEventEngine::CleanConnection, this);
	std::thread* thread = new std::thread[thread_count_];
	for (int i = 0; i < thread_count_; i++)
	{
		thread[i] = std::thread(&IoEventEngine::EventLoop, this);
	}

	for (int i = 0; i < thread_count_; i++)
	{
		if (thread[i].joinable())
			thread[i].join();
	}

	delete[] thread;

	if (connfree.joinable())
		connfree.join();

	if (iocp_ != INVALID_HANDLE_VALUE)
		CloseHandle(iocp_);
	iocp_ = INVALID_HANDLE_VALUE;
}

void IoEventEngine::Stop()
{
	exit_ = true;

	size_t repeat = thread_count_;
	while (repeat--)
		assert(PostQueuedCompletionStatus(iocp_, 0xffffffff, NULL, NULL));
	cond_.notify_one();
}


void IoEventEngine::EventLoop()
{
	DWORD recv_data_size = 0;
	IObject::CompletionKey* key = nullptr;
	OVERLAPPED* ovlp = nullptr;
	WRAPPER_OVERLAPPED* wovlp = nullptr;
	BOOL ret = false;
	DWORD retcode = 0;
	/*
		1、同一个handle的不同operation可能同时被线程并发处理
		2、非阻塞模式下，写入函数要么全部字节写入缓冲区成功，要么完全失败
	*/
	while (!exit_)
	{
		ret = GetQueuedCompletionStatus(iocp_, &recv_data_size, (PULONG_PTR)&key, &ovlp, INFINITE);
		LOG_Debug("IOCP return %d, datasize %d, overlapped pointer: %p", ret, recv_data_size, ovlp);
		if (!ovlp && GetLastError() == ERROR_ABANDONED_WAIT_0)
		{
			// iocp was closed
			break;
		}

		if (ovlp == NULL && key == NULL)
			continue;

		char* p = (char*)ovlp - (size_t) & (((WRAPPER_OVERLAPPED*)0)->overlap);
		wovlp = (WRAPPER_OVERLAPPED*)p;
		IoEvent* event = wovlp->owner;
		if (!ret)
		{
			event->GetConnectObject()->GetIoObject()->OnEvent(event->GetType(),
				false);
			// event->GetConnectObject()->OnDisconnect();
			AddbrokenConnection(event->GetConnectObject());

			continue;
		}

		LOG_Debug("IOCP event %d", event->GetType());
		switch (event->GetType())
		{
		case EventType::Accept:
		{
			retcode = event->GetConnectObject()->OnAccept(nullptr);
			break;
		}
		case EventType::Read:
		{
			retcode = event->GetConnectObject()->OnRecv(recv_data_size);
			break;
		}
		case EventType::Write:
		{
			retcode = event->GetConnectObject()->OnSend();
			break;
		}
		default:
			assert(false);
			break;
		}

		event->GetConnectObject()->GetIoObject()->OnEvent(event->GetType(), 
			retcode == 0);

		cond_.notify_one();
	}
}


void IoEventEngine::CleanConnection()
{
	while (!exit_)
	{
		std::unique_lock<std::mutex> autolock(mtx_);
		cond_.wait(autolock, [this]()->bool
		{
			return !broken_conn_list_.empty() || exit_;
		});

		for (auto& it = broken_conn_list_.begin();
			it != broken_conn_list_.end(); )
		{
			if (!(*it)->IsAlive())
			{
				(*it)->OnDisconnect();
				it = broken_conn_list_.erase(it);
			}
			else {
				it++;
			}
		}
	}
}

void IoEventEngine::AddbrokenConnection(IConnection* conn)
{
	std::unique_lock<std::mutex> autolock(mtx_);
	broken_conn_list_.push_back(conn);
	cond_.notify_one();
}

};
