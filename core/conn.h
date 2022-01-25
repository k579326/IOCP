
#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <functional>

#include "event.h"
#include "Server.h"

class IObject;

namespace async
{
	class IConnection
	{
		friend class IoEventEngine;

	public:
		IConnection(Server* svr);
		virtual ~IConnection();

		virtual int Init(ConnId id, IoType io_type) = 0;
		virtual int Init(ConnId id, IObject* obj) = 0;

		int Close();

		virtual int OnSend();
		virtual int OnRecv(size_t size);
		virtual int OnAccept(IObject* obj);
		virtual int OnDisconnect();
		virtual bool IsAlive();

		IObject* GetIoObject();

		ConnId GetConnId() const;


	protected:
		virtual void InitEvents() = 0;
		int CancelAllOperation();
	protected:

		Server* server_ = nullptr;
		async::ConnId id_;
		std::map<EventType, std::shared_ptr<IoEvent>> events_;
		std::shared_ptr<IObject> obj_;
	};


	using RecvCB = std::function<int(IConnection* conn, const void* data, size_t size)>;
	using SendCB = std::function<int(IConnection* conn)>;

	class RemoteConnect : public IConnection
	{
	public:
		RemoteConnect(Server* svr, RecvCB rcb, SendCB scb);
		~RemoteConnect();

		virtual int Init(ConnId id, IoType io_type) override;
		virtual int Init(ConnId id, IObject* obj) override;

		void ResetConnId(ConnId id);
		int PostSend(const void* data, size_t size);
		int Push(const void* data, size_t size);
		int PostRecv();

		// Only used for pipe
		int PostListen();

		virtual int OnSend() override;
		virtual int OnRecv(size_t size) override;
		virtual int OnAccept(IObject* obj) override;

	private:
		virtual void InitEvents() override;

	private:

		RecvCB rcb_;
		SendCB scb_;
		char* buf_ = nullptr;
		char* read_pos_ = nullptr;
		static constexpr size_t bufsize_ = 65536;
	};

	class ListenConnect : public IConnection
	{
	public:
		ListenConnect(Server* svr);
		~ListenConnect();

		virtual int Init(ConnId id, IoType io_type) override;
		virtual int Init(ConnId id, IObject* obj) override;
		int PostListen(IObject* obj);
		virtual int OnAccept(IObject* obj) override;

	private:
		virtual void InitEvents() override;
	};

};
