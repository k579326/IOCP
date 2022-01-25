

#pragma once


#include "core/Server.h"

class IObject;

namespace async
{

class PipeServer : public Server
{
public:
    PipeServer(std::shared_ptr<Observer> ob);
    ~PipeServer();

    int Create(const char* pipename);
    void Destory();

    virtual int Listen() override;
    virtual void OnConnect(IObject* obj);
    virtual void OnDisconnect();

private:
    int _Listen(bool first);

private:
    std::string pipe_name_;
};


};


