

#pragma once


#include <windows.h>

#include <stdint.h>
#include <atomic>

#include "iocp_define.h"

class IObject
{
public:	
	class CompletionKey
	{
	public:
		IObject* obj;
	};

	IObject(IoType type) {
		type_ = type;
		key_.obj = this;
		io_pending_number_ = 0;
	}
	virtual ~IObject() {}
	
	virtual int Send(const void* data, size_t size, OVERLAPPED* ov) = 0;
	virtual int Recv(void* data, size_t size, OVERLAPPED* ov) = 0;
	virtual int Accept(OVERLAPPED* ov) = 0;
	virtual int Close() = 0;
	virtual bool IsAlive() = 0;
	virtual HANDLE IoHandle() = 0;
	virtual void OnEvent(EventType event, bool result) = 0;

	CompletionKey* GetKey() { return &key_; }
	IoType GetType() { return type_; }

protected:
	virtual void OnSend(bool result) = 0;
	virtual void OnRecv(bool result) = 0;
	virtual void OnAccept(bool result) = 0;
	virtual void OnDisconnect() = 0;


protected:
	IoType type_;
	uint16_t io_pending_number_;
	CompletionKey key_;
};



