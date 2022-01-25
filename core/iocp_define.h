#pragma once

#include <windows.h>


enum class ConnType
{
	Client = 0,
	Server,
};

enum class IoType
{
    Pipe = 0,
    Tcp,
};

enum class EventType
{
    Accept = 0,
    Read,
    Write,

    AllCount,
};

namespace async {
    class IoEvent;
};

class WRAPPER_OVERLAPPED
{
public:
    OVERLAPPED overlap = { 0 };
    async::IoEvent* owner = nullptr;
    EventType type = EventType::Accept;
    void* data_ = nullptr;

    virtual ~WRAPPER_OVERLAPPED() {
        owner = nullptr;
    }
};






