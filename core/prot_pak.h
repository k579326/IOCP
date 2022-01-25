#pragma once

#include <stdint.h>

#define PROTO_VERSION_1 	0x10000001
#define MAGIC_NUMBER 		'CPDP'

#define PAK_TYPE_PUSH	0
#define PAK_TYPE_RW		1


struct ProtPackage
{
	uint32_t version;
	uint32_t magic;
	uint32_t pak_type;
	uint32_t seq;
	uint32_t data_size;
	uint8_t  data[0];
};


class IPakBuilder
{
public:
	IPakBuilder();
	virtual ~IPakBuilder();
	
	IPakBuilder(const IPakBuilder& o) = delete;
	IPakBuilder(IPakBuilder&& o) = delete;
	IPakBuilder& operator=(const IPakBuilder& o) = delete;
	
	virtual ProtPackage* NewPackage(const void* data, size_t length, uint32_t pak_type) = 0;
	
	/* 
		返回值代表完整包缺失字节数。
		=0表示pak至少包含一个完整包;
		>0代表pak作为一个完整包缺失的字节数;
		<0代表pak太短
	*/
	virtual int32_t  AnalyzePackage(const ProtPackage* pak, size_t length) = 0;
	virtual uint32_t GetVersion() = 0;
	virtual uint32_t GetPakType() = 0;
	virtual uint32_t GetSequeue() = 0;
	virtual uint8_t* GetData() = 0;
	virtual uint32_t GetDataSize() = 0;
	virtual uint8_t* GetRemain() = 0;
	virtual uint32_t GetRemainSize() = 0;
	
	uint32_t ApplySeqNumber();
	
	static IPakBuilder* CreatePakFromStream(const ProtPackage* pak, size_t length);
	static void DestoryPackage(const ProtPackage* pak);
protected:
	static uint32_t seq_;
	ProtPackage* pak_ = nullptr;
	size_t size_ = 0;
};


class PakBuilderV1 : public IPakBuilder
{
public:
	PakBuilderV1();
	virtual ~PakBuilderV1();
	
	virtual ProtPackage* NewPackage(const void* data, size_t length, uint32_t pak_type) override;
	virtual int32_t  AnalyzePackage(const ProtPackage* pak, size_t length) override;
	virtual uint32_t GetVersion() override;
	virtual uint32_t GetPakType() override;
	virtual uint32_t GetSequeue() override;
	virtual uint8_t* GetData() override;
	virtual uint32_t GetDataSize() override;
	virtual uint8_t* GetRemain() override;
	virtual uint32_t GetRemainSize() override;
private:	
	
	const uint32_t version_;
};


















