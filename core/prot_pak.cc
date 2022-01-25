

#include "prot_pak.h"

#include <mutex>

uint32_t IPakBuilder::seq_ = 0;
static std::mutex g_mtx;

IPakBuilder::IPakBuilder() {}
IPakBuilder::~IPakBuilder() {}

IPakBuilder* IPakBuilder::CreatePakFromStream(const ProtPackage* pak, size_t length)
{
	if (length < 4) {
		return nullptr;
	}
	
	switch (pak->version)
	{
	case PROTO_VERSION_1:
	{
		if (length < sizeof(ProtPackage))
			return nullptr;
		PakBuilderV1* builder = new PakBuilderV1();
		if (builder->AnalyzePackage(pak, length) != 0)
		{
			delete builder;
			return nullptr;
		}
		return builder;
	}
	default:
		break;
	}
	
	return nullptr;
}

void IPakBuilder::DestoryPackage(const ProtPackage* pak)
{
	delete[] pak;
	return;
}

uint32_t IPakBuilder::ApplySeqNumber()
{
	std::lock_guard<std::mutex> autolock(g_mtx);
	
	seq_++;
	if (!seq_) {
		seq_++;
	}
	return seq_;
}


PakBuilderV1::PakBuilderV1() : version_(PROTO_VERSION_1) {}
PakBuilderV1::~PakBuilderV1() {}

ProtPackage*  PakBuilderV1::NewPackage(const void* data, size_t length, uint32_t paktype)
{
	if (!data || !length)
		return nullptr;

	ProtPackage* pak = (ProtPackage*)new char[sizeof(ProtPackage) + length];
	pak->version = version_;
	pak->magic = MAGIC_NUMBER;
	pak->seq = 0;
	pak->data_size = length;
	pak->pak_type = paktype;
	memcpy(pak->data, data, length);
	
	pak_ = pak;
	size_ = length + sizeof(ProtPackage);
	return pak;
}
int32_t PakBuilderV1::AnalyzePackage(const ProtPackage* pak, size_t length)
{
	pak_ = nullptr;
	
	if (length < sizeof(ProtPackage))
		return -1;
	
	if (length < pak->data_size + sizeof(ProtPackage))
		return pak->data_size + sizeof(ProtPackage) - length;
	
	pak_ = const_cast<ProtPackage*>(pak);
	size_ = length;
	
	return 0;
}

uint32_t PakBuilderV1::GetVersion()
{
	return pak_->version;
}
uint32_t PakBuilderV1::GetSequeue()
{
	return pak_->seq;
}
uint8_t* PakBuilderV1::GetData()
{
	return pak_->data;
}
uint32_t PakBuilderV1::GetPakType()
{
	return pak_->pak_type;
}
uint32_t PakBuilderV1::GetDataSize()
{
	return pak_->data_size;
}
uint8_t* PakBuilderV1::GetRemain()
{
	if (size_ <= sizeof(ProtPackage) + pak_->data_size)
		return nullptr;
	
	return (uint8_t*)pak_ + sizeof(ProtPackage) + pak_->data_size;
}
uint32_t PakBuilderV1::GetRemainSize()
{
	if (size_ < sizeof(ProtPackage) + pak_->data_size)
		return size_;
	
	return size_ - sizeof(ProtPackage) - pak_->data_size;
}

