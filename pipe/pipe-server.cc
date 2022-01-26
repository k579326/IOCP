
#include "pipe-server.h"

#include <assert.h>

#include "conn.h"
#include "pipe-object.h"

#include "engine.h"

namespace async
{
 
PipeServer::PipeServer(std::shared_ptr<Observer> ob) 
    : Server(ob)
{

}
PipeServer::~PipeServer()
{

}

int PipeServer::Create(const char* pipename)
{
    int ret = 0; 

    if ((ret = InitEngine(4)) != 0)
        return ret;

    listener_.reset(new ListenConnect(this));
    listener_->Init(0, IoType::Pipe);

    // 管道与socket不同，不需要专门用于监听的管道，这里只创建一个没有管道实例的监听连接对象
    /*
    PipeObject* pipe = dynamic_cast<PipeObject*>(listener_->GetIoObject());
    if ((ret = pipe->CreateAsServer(pipename, true)) != 0)
        goto _Exit;
    */

    pipe_name_ = pipename;
    return 0;
}


void PipeServer::Destory()
{
    listener_ = nullptr;
    UninitEngine();
}

int PipeServer::Listen()
{
    return _Listen(true);
}

int PipeServer::_Listen(bool first)
{
    PipeObject* newobj = new PipeObject;
    if (newobj->CreateAsServer(pipe_name_.c_str(), first) != 0)
    {
        delete newobj;
        return -1;
    }

    engine_->Register(newobj);

    RemoteConnect* conn = new RemoteConnect(this,
        std::bind(&Server::RecvCallback, this,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        std::bind(&Server::SendCallback, this,
            std::placeholders::_1)
    );
    if (conn->Init(0, newobj) != 0)
    {
        delete conn;
        delete newobj;
        return -1;
    }

    int rst = conn->PostListen();
    if (rst == ERROR_PIPE_CONNECTED)
    {
        OnConnect((IObject*)conn);
        rst = 0;
    }
    else if (rst == ERROR_IO_PENDING)
        rst = 0;

    return rst;
}

void PipeServer::OnConnect(IObject* obj)
{
    ConnId connid = ApplyConnId();
    // apply connid
    // add to conntable
    RemoteConnect* conn = reinterpret_cast<RemoteConnect*>(obj);
    conn->ResetConnId(connid);
    AddConnectToTable(connid, std::shared_ptr<RemoteConnect>(conn));
    if (conn->PostRecv() != 0)
        assert(false);

    // callback
    callback_->OnConnect(connid);
    if (_Listen(false) != 0)
        assert(false);
}
void PipeServer::OnDisconnect()
{

}

};

