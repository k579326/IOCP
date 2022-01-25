
#pragma once

#include <map>

#include "iocp_define.h"

namespace async
{


class IConnection;
class IoEvent
{
public:
    IoEvent() {
        conn_ = nullptr;
        memset(&wo_.overlap, 0, sizeof(OVERLAPPED));
        wo_.overlap.hEvent = CreateEvent(NULL, true, true, NULL);
        wo_.owner = this;
    }
    virtual ~IoEvent() {
        CloseHandle(wo_.overlap.hEvent);
    }

    void AttachConnect(IConnection* obj) {
        conn_ = obj;
    }
    IConnection* GetConnectObject() {
        return conn_;
    }

    void SetType(EventType type) {
        wo_.type = type;
    }

    EventType GetType() {
        return wo_.type;
    }

    OVERLAPPED* GetOverlapped() {
        return &wo_.overlap;
    }

    void SetUserData(void* data) {
        wo_.data_ = data;
    }

    void* GetUserData() const {
        return wo_.data_;
    }

protected:

    IConnection* conn_;
    WRAPPER_OVERLAPPED wo_;
};

};

