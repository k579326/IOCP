

#include "pipe-server.h"


using namespace async;

class MyOb : public Observer
{
public:
	MyOb() : ps_(nullptr) {}
	~MyOb() {}
	
	void SetServer(PipeServer* ps) {
		ps_ = ps;
	}

	virtual int OnConnect(ConnId connid) override
	{
		printf("connection %llu established\n", connid);
		// ps_->Disconnect(connid);
		return 0;
	}
	virtual int OnRecv(ConnId connid, const void* data, size_t size) override
	{
		// printf("recv %s from %llu\n", std::string((char*)data, size).c_str(), connid);
		ps_->Send(connid, "success", 7);
		return 0;
	}
	virtual int OnSend(ConnId connid) override
	{
		return 0;
	}
	virtual int OnDisconnect(ConnId connid) override
	{
		printf("connect %llu break\n", connid);
		return 0;
	}

private:
	PipeServer* ps_;
};



void BreakConnection(PipeServer* ps, ConnId id)
{
	ps->Disconnect(id);
}
void PushMessage(PipeServer* ps, ConnId id, const std::string& msg)
{
	ps->Push(id, msg.c_str(), msg.size());
}

void execute_command(PipeServer* ps)
{
	char buf[64] = {};
	while (true)
	{
		gets_s(buf, 64);

		switch (buf[0])
		{
		case 'b':		// break connid
			BreakConnection(ps, std::atoll(buf+2));
			break;
		case 'p':		// push connid ${content}
		{
			char* space = strchr(buf + 2, '\x20');
			std::string id;
			id.assign(buf+2, space);
			PushMessage(ps, std::atoll(id.c_str()), space + 1);
			break;
		}
		case 'e':		// 
			ps->Stop();
			return;
		default:
			break;
		}


	}
}



int main()
{
	std::shared_ptr<MyOb> testob = 
		std::make_shared<MyOb>();
	
	PipeServer ps(testob);
	
	testob->SetServer(&ps);

	ps.Create("\\\\.\\pipe\\LOCAL\\test-iocp-pipe");

	auto loop_func = [](PipeServer* ps)->void{
		ps->Run();
	};

	std::thread loop(loop_func, &ps);

	ps.Listen();
	
	execute_command(&ps);

	if (loop.joinable())
		loop.join();

	return 0;
}
