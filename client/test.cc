#include <Windows.h>

#include <string>



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


int main()
{
	size_t wait_times = 3;
	HANDLE pipe = INVALID_HANDLE_VALUE;
	std::wstring real_pipename = L"\\\\.\\pipe\\LOCAL\\test-iocp-pipe";
	DWORD pipe_mode = 0;

	while (wait_times--)
	{
		if (!WaitNamedPipeW(real_pipename.c_str(), 1000))
			continue;

		pipe = CreateFileW(
					real_pipename.c_str(),    // pipe name 
					GENERIC_READ | GENERIC_WRITE,
					0,              // no sharing 
					NULL,           // default security attributes
					OPEN_EXISTING,  // opens existing pipe 
					0,              // default attributes 
					NULL);          // no template file
		if (pipe == INVALID_HANDLE_VALUE)
			continue;

		pipe_mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
		if (!SetNamedPipeHandleState(pipe, &pipe_mode, NULL, NULL))
		{
			CloseHandle(pipe);
			pipe = INVALID_HANDLE_VALUE;
		}
		else
			break;
	}

	if (pipe == INVALID_HANDLE_VALUE)
		return -1;


	ProtPackage* pkg = (ProtPackage*)malloc(sizeof(ProtPackage) + 128);
	if (!pkg)
		return -1;

	pkg->version = PROTO_VERSION_1;
	pkg->seq = 0;
	pkg->pak_type = PAK_TYPE_RW;
	pkg->magic = MAGIC_NUMBER;

	DWORD writesize = 0;
	DWORD repeat = 0;
	unsigned char buf[256];

	while (true)
	{
		memset(buf, 0, 256);
		sprintf((char*)pkg->data, "send times %d", repeat++);
		pkg->data_size = strlen((char*)pkg->data);

		WriteFile(pipe, pkg, sizeof(ProtPackage) + pkg->data_size, &writesize, NULL);
		
		if (!ReadFile(pipe, buf, 256, &writesize, NULL))
			break;

		ProtPackage* recv_pkg = (ProtPackage*)buf;
		recv_pkg->data[recv_pkg->data_size] = 0;
		printf("recv: %s\n", (char*)recv_pkg->data);
		Sleep(1);
	}

	CloseHandle(pipe);
	return 0;
}