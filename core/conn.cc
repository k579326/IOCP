
#include "conn.h"

#include <assert.h>

#include "prot_pak.h"
#include "pipe-object.h"
#include "Server.h"

namespace async
{

IConnection::IConnection(Server* svr)
{
	obj_ = nullptr;
	server_ = svr;
}

IConnection::~IConnection()
{
}

int IConnection::OnSend()
{
	return -1;
}
int IConnection::OnRecv(size_t size)
{
	return -1;
}
int IConnection::OnAccept(IObject* obj)
{
	return -1;
}
bool IConnection::IsAlive()
{
	return obj_->IsAlive();
}

int IConnection::OnDisconnect()
{
	// remove from connection table
	CancelAllOperation();
	obj_->Close();

	if (!obj_->IsAlive())
	{
		std::shared_ptr<RemoteConnect> conn = server_->RemoveConnectFromTable(GetConnId());
		server_->GetObserver()->OnDisconnect(id_);
		if (conn == nullptr)
		{
			delete this;
		}
		else
		{
			assert(conn.get() == this);
			conn.reset();
		}

	}

	return 0;
}


IObject* IConnection::GetIoObject()
{
	return obj_.get();
}
ConnId IConnection::GetConnId() const
{
	return id_;
}
int IConnection::CancelAllOperation()
{
	return CancelIoEx(obj_->IoHandle(), nullptr) ? 0 : -GetLastError();
}

int IConnection::Close()
{
	CancelAllOperation();
	obj_->Close();
	return 0;
}



RemoteConnect::RemoteConnect(Server* svr, RecvCB rcb, SendCB scb)
	: IConnection(svr), rcb_(rcb), scb_(scb)
{
	buf_ = new char[bufsize_];
	read_pos_ = buf_;
}

RemoteConnect::~RemoteConnect()
{
	delete[] buf_;
}

int RemoteConnect::PostSend(const void* data, size_t size)
{
	if (!data || !size)
		return -1;

	// make package
	PakBuilderV1 pb;
	ProtPackage* pkg = pb.NewPackage(data, size, PAK_TYPE_RW);
	if (!pkg)
		return -1;

	auto event = events_.find(EventType::Write)->second;
	int ret = obj_->Send(pkg, size + sizeof(ProtPackage), event->GetOverlapped());
	IPakBuilder::DestoryPackage(pkg);

	return ret;
}

int RemoteConnect::Push(const void* data, size_t size)
{
	if (!data || !size)
		return -1;

	// make package
	PakBuilderV1 pb;
	ProtPackage* pkg = pb.NewPackage(data, size, PAK_TYPE_PUSH);
	if (!pkg)
		return -1;

	auto event = events_.find(EventType::Write)->second;
	int ret = obj_->Send(pkg, size + sizeof(ProtPackage), event->GetOverlapped());
	IPakBuilder::DestoryPackage(pkg);

	return ret;
}

int RemoteConnect::PostRecv()
{
	auto event = events_.find(EventType::Read)->second;
	int ret = obj_->Recv(read_pos_, bufsize_ - (read_pos_ - buf_),
		event->GetOverlapped());
	return ret;
}

int RemoteConnect::PostListen()
{
	assert(obj_->GetType() == IoType::Pipe);
	auto event = events_.find(EventType::Accept)->second;
	return obj_->Accept(event->GetOverlapped());
}

void RemoteConnect::ResetConnId(ConnId id)
{
	id_ = id;
}

int RemoteConnect::Init(ConnId id, IoType io_type)
{
	id_ = id;

	switch (io_type)
	{
	case IoType::Pipe:
		obj_.reset(new PipeObject);
		break;
	case IoType::Tcp:
		obj_ = nullptr;	// todo
		break;
	default:
		obj_ = 0;
		assert(obj_);
		break;
	}

	InitEvents();

	return obj_ == nullptr ? -1 : 0;
}

int RemoteConnect::Init(ConnId id, IObject* obj)
{
	if (!obj) {
		return -1;
	}
	obj_.reset(obj);
	id_ = id;

	InitEvents();

	return 0;
}


int RemoteConnect::OnSend()
{
	// 如果iocp发送失败，iocp如何体现这种错误
	scb_(this);
	return 0;
}

int RemoteConnect::OnRecv(size_t size)
{
	int retcode = 0;
	// analyze the buf, fetch data package
	assert(read_pos_ - buf_ + size <= bufsize_ - sizeof(ProtPackage));

	IPakBuilder* builder = IPakBuilder::CreatePakFromStream((const ProtPackage*)buf_, size_t(read_pos_ - buf_) + size);
	if (!builder) {
		read_pos_ += size;
		return PostRecv();
	}

	rcb_(this, builder->GetData(), builder->GetDataSize());
	memmove(buf_, builder->GetRemain(), builder->GetRemainSize());
	read_pos_ = buf_ + builder->GetRemainSize();

	retcode = PostRecv();
	delete builder;

	return retcode;
}

int RemoteConnect::OnAccept(IObject* obj)
{
	server_->OnConnect((IObject*)this);
	return 0;
}


void RemoteConnect::InitEvents()
{
	std::shared_ptr<IoEvent> recv_event = std::make_shared<IoEvent>();
	std::shared_ptr<IoEvent> send_event = std::make_shared<IoEvent>();

	recv_event->SetType(EventType::Read);
	recv_event->SetUserData(obj_.get());
	recv_event->AttachConnect(this);
	
	send_event->SetType(EventType::Write);
	send_event->SetUserData(obj_.get());
	send_event->AttachConnect(this);

	events_.insert(std::make_pair(recv_event->GetType(), recv_event));
	events_.insert(std::make_pair(send_event->GetType(), send_event));

	if (obj_ && obj_->GetType() == IoType::Pipe)
	{
		std::shared_ptr<IoEvent> accept_event = std::make_shared<IoEvent>();
		accept_event->SetType(EventType::Accept);
		accept_event->SetUserData(obj_.get());
		accept_event->AttachConnect(this);
		events_.insert(std::make_pair(accept_event->GetType(), accept_event));
	}
}


ListenConnect::ListenConnect(Server* svr) : IConnection(svr)
{
}
ListenConnect::~ListenConnect()
{
}

int ListenConnect::PostListen(IObject* obj)
{
	int rst = 0;
	auto event = events_.find(EventType::Accept)->second;
	
	event->SetUserData(obj);
	rst = obj_->Accept(event->GetOverlapped());
	
	if (rst == 0)
	{
		printf("begin listen...\n");
	}

	return rst;
}

int ListenConnect::OnAccept(IObject* obj)
{
	server_->OnConnect(obj);
	return 0;
}

void ListenConnect::InitEvents()
{
	std::shared_ptr<IoEvent> event = std::make_shared<IoEvent>();
	event->SetType(EventType::Accept);
	event->SetUserData(nullptr);
	event->AttachConnect(this);

	events_.insert(std::make_pair(event->GetType(), event));
}

int ListenConnect::Init(ConnId id, IoType io_type)
{
	id_ = id;

	switch (io_type)
	{
	case IoType::Pipe:
		obj_.reset(new PipeObject);
		break;
	case IoType::Tcp:
		obj_ = nullptr;	// todo
		break;
	default:
		obj_ = 0;
		assert(obj_);
		break;
	}

	InitEvents();

	return obj_ == nullptr ? -1 : 0;
}

int ListenConnect::Init(ConnId id, IObject* obj)
{
	if (!obj)
		return -1;
	id_ = id;
	obj_.reset(obj);

	return 0;
}

};
