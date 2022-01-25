

#include "pipe-object.h"

#include <assert.h>


PipeObject::PipeObject() : IObject(IoType::Pipe) {}
PipeObject::~PipeObject()
{
	if (pipe_ != INVALID_HANDLE_VALUE)
		CloseHandle(pipe_);
}

int PipeObject::CreateAsServer(const char* pipename, bool first)
{
	HANDLE h = CreateNamedPipeA(pipename,
		(first ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0) | PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		65535,
		65535,
		0,
		NULL);

	if (h == INVALID_HANDLE_VALUE)
		return -GetLastError();

	pipe_ = h;
	name_ = pipename;

	return 0;
}

int PipeObject::CreateAsClient(const char* pipename)
{
	HANDLE h = INVALID_HANDLE_VALUE;
	size_t repeat = 3;

	while (repeat--)
	{
		if (!WaitNamedPipeA(pipename, 1000))
			continue;

		h = CreateFileA(
			pipename,		// pipe name 
			GENERIC_READ | GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file
		if (h == INVALID_HANDLE_VALUE)
			continue;

		DWORD pipe_mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
		if (!SetNamedPipeHandleState(h, &pipe_mode, NULL, NULL))
		{
			CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
		else
		{
			// success
			break;
		}
	}

	if (h == INVALID_HANDLE_VALUE)
		return -GetLastError();

	pipe_ = h;
	name_ = pipename;

	return 0;
}

std::string PipeObject::GetPipeName()
{
	return name_;
}

bool PipeObject::IsAlive()
{
	std::lock_guard<std::mutex> autolock(mtx_);
	return io_pending_number_ != 0;
}

int PipeObject::Close()
{
	std::lock_guard<std::mutex> autolock(mtx_);

	if (pipe_ == INVALID_HANDLE_VALUE)
		return 0;

	DisconnectNamedPipe(pipe_);
	CloseHandle(pipe_);
	pipe_ = INVALID_HANDLE_VALUE;

	return 0;
}


int PipeObject::Send(const void* data, size_t size, OVERLAPPED* ov)
{
	DWORD retcode = 0;

	std::lock_guard<std::mutex> autolock(mtx_);
	if (pipe_ == INVALID_HANDLE_VALUE)
	{
		retcode = -1;
		goto _Send_Exit;
	}

	if (WriteFile(pipe_, data, size, nullptr, ov))
	{
		retcode = 0;
		io_pending_number_++;
		goto _Send_Exit;
	}

	retcode = GetLastError();
	if (retcode == ERROR_SUCCESS || retcode == ERROR_IO_PENDING)
	{
		retcode = 0;
		io_pending_number_++;
	}
	else {
		retcode = -retcode;
	}

_Send_Exit:
	return retcode;
}

int PipeObject::Recv(void* data, size_t size, OVERLAPPED* ov)
{
	DWORD retcode = 0;

	std::lock_guard<std::mutex> autolock(mtx_);
	if (pipe_ == INVALID_HANDLE_VALUE)
	{
		retcode = -1;
		goto _Recv_Exit;
	}

	if (ReadFile(pipe_, data, size, nullptr, ov))
	{
		retcode = 0;
		io_pending_number_++;
		goto _Recv_Exit;
	}
	retcode = GetLastError();
	if (retcode != ERROR_SUCCESS && retcode != ERROR_IO_PENDING)
	{
		retcode = -retcode;
	}
	else {
		retcode = 0;
		io_pending_number_++;
		goto _Recv_Exit;
	}

_Recv_Exit:
	return retcode;
}

HANDLE PipeObject::IoHandle()
{
	return pipe_;
}

void PipeObject::OnEvent(EventType event, bool result)
{
	std::lock_guard<std::mutex> autolock(mtx_);
	switch (event)
	{
	case EventType::Accept:
		OnAccept(result);
		break;
	case EventType::Write:
		OnSend(result);
		break;
	case EventType::Read:
		OnRecv(result);
		break;
	default:
		break;
	}
	io_pending_number_--;
}

int PipeObject::Accept(OVERLAPPED* ov)
{
	DWORD retcode = 0;

	std::lock_guard<std::mutex> autolock(mtx_); 
	
	if (pipe_ == INVALID_HANDLE_VALUE)
	{
		retcode = -1;
		goto _Accept_Exit;
	}

	if (ConnectNamedPipe(pipe_, ov))
	{
		io_pending_number_++;
		retcode = 0;
		goto _Accept_Exit;
	}

	retcode = GetLastError();
	if (retcode == ERROR_IO_PENDING || retcode == ERROR_PIPE_CONNECTED)
	{
		// don't reset the retcode, the caller will check that the return value of this function
		// is ERROR_IO_PENDING or ERROR_PIPE_CONNECTED
		// retcode = 0;
		io_pending_number_++;
	}

_Accept_Exit:
	return retcode;
}

void PipeObject::OnSend(bool success)
{
}
void PipeObject::OnRecv(bool success)
{
	
}
void PipeObject::OnAccept(bool success)
{
	
}
void PipeObject::OnDisconnect()
{
}





