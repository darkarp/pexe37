#include <windows.h>
#include <stdio.h>

void SystemError(int error, char *msg)
{
	char Buffer[1024];
	int n;

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
	} else
		Buffer[0] = '\0';
	n = lstrlenA(Buffer);
	_snprintf(Buffer+n, sizeof(Buffer)-n, msg);
	MessageBoxA(GetFocus(), Buffer, NULL, MB_OK | MB_ICONSTOP);
}

extern int init(char *);
extern int start(int argc, wchar_t **argv);

int WINAPI
WinMain(HINSTANCE hInst, HINSTANCE hPrevInst,
	LPSTR lpCmdLine, int nCmdShow)
{
	int result = 0;
	int argc;
	LPWSTR *argv;
	LPWSTR cmdline = GetCommandLineW();
	argv = CommandLineToArgvW(cmdline, &argc);
	result = init("windows_exe");
	if (result)
		return result;
	return start(argc, argv);
}
