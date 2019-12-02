#include <windows.h>
#include <stdio.h>

void SystemError(int error, char *msg)
{
	char Buffer[1024];

	if (msg)
		fprintf(stderr, msg);
	if (error) {
		LPVOID lpMsgBuf;
		FormatMessageA( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&lpMsgBuf,
			0,
			NULL 
			);
		strncpy(Buffer, lpMsgBuf, sizeof(Buffer));
		LocalFree(lpMsgBuf);
		fprintf(stderr, Buffer);
	}
}

extern int init(char *);
extern int start(int argc, wchar_t **argv);


int wmain (int argc, wchar_t **argv)
{
	int result;
	result = init("console_exe");
	if (result)
		return result;
	return start(argc, argv);
}
