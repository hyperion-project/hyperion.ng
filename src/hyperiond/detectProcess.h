#pragma once

#include <QString>
#include <QProcess>
#include <QByteArray>

#ifdef WIN32
// psapi.h requires windows.h to be included
#include <Windows.h>
#include <Psapi.h>
#endif

unsigned int getProcessIdsByProcessName(const char *processName, QStringList &listOfPids)
{

	// Clear content of returned list of PIDS
	listOfPids.clear();

#if defined(WIN32)
	// Get the list of process identifiers.
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		return 0;

	// Calculate how many process identifiers were returned.
	cProcesses = cbNeeded / sizeof(DWORD);

	// Search for a matching name for each process
	for (i = 0; i < cProcesses; i++)
	{
		if (aProcesses[i] != 0)
		{
			char szProcessName[MAX_PATH] = {0};

			DWORD processID = aProcesses[i];

			// Get a handle to the process.
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

			// Get the process name
			if (NULL != hProcess)
			{
				HMODULE hMod;
				DWORD cbNeeded;

				if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
					GetModuleBaseNameA(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(char));

				// Release the handle to the process.
				CloseHandle(hProcess);

				if (*szProcessName != 0 && strcmp(processName, szProcessName) == 0)
					listOfPids.append(QString::number(processID));
			}
		}
	}

	return listOfPids.count();

#else

	// Run pgrep, which looks through the currently running processses and lists the process IDs
	// which match the selection criteria to stdout.
	QProcess process;
	process.start("pgrep", QStringList() << processName);
	process.waitForReadyRead();

	QByteArray bytes = process.readAllStandardOutput();

	process.terminate();
	process.waitForFinished();
	process.kill();

	// Output is something like "2472\n2323" for multiple instances
	if (bytes.isEmpty())
		return 0;

	#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
		listOfPids = QString(bytes).split("\n", Qt::SkipEmptyParts);
	#else
		listOfPids = QString(bytes).split("\n", QString::SkipEmptyParts);
	#endif

	return listOfPids.count();

#endif
}
