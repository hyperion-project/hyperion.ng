#include <Windows.h>

bool openConsole(bool isShowConsole)
{
	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
		SetConsoleTitle(TEXT("Hyperion"));
		SetConsoleOutputCP(CP_UTF8);
		return true;
	}
	return false;
}
