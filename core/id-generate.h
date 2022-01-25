

#pragma once

#include <mutex>

class IdGenerater
{
public:
	uint64_t GenNextId() 
	{
		uint64_t local;
		mtx_.lock();
		
		if (id + 1 == -1)
			id++;
		
		local = id++;
		mtx_.unlock();
		return local;
	}
private:
	std::mutex mtx_;
	uint64_t id = 0;
};