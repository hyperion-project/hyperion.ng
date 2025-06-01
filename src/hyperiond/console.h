#include <Windows.h>

void openConsole(bool isShowConsole)
{
	if (AttachConsole(ATTACH_PARENT_PROCESS) || (isShowConsole && AllocConsole())) {
		FILE* fp;
		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
		SetConsoleTitle(TEXT("Hyperion"));
		SetConsoleOutputCP(CP_UTF8);
	}
}
