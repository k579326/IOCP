
#include <string>
#include <mutex>

#include "io-object.h"


class PipeObject : public IObject
{
public:
	PipeObject();
	~PipeObject();
	
	int CreateAsServer(const char* pipename, bool first);
	int CreateAsClient(const char* pipename);
	std::string GetPipeName();
	virtual bool IsAlive() override;

	virtual int Close() override;
	virtual int Send(const void* data, size_t size, OVERLAPPED* ov) override;
	virtual int Recv(void* data, size_t size, OVERLAPPED* ov) override;
	int Accept(OVERLAPPED* ov) override;

	virtual void OnEvent(EventType event, bool result) override;
	virtual HANDLE IoHandle() override;

private:
	virtual void OnSend(bool result) override;
	virtual void OnRecv(bool result) override;
	virtual void OnAccept(bool result) override;
	virtual void OnDisconnect() override;

private:
	HANDLE pipe_ = INVALID_HANDLE_VALUE;
	std::string name_;
	std::mutex mtx_;
};