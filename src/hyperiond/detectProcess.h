#pragma once

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QRegExp>
#include <QString>
#include <QTextStream>

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

#else

	QDir dir("/proc");
	dir.setFilter(QDir::Dirs);
	dir.setSorting(QDir::Name | QDir::Reversed);

	for (const QString & pid : dir.entryList()) {
		QRegExp regexp("\\d*");
		if (!regexp.exactMatch(pid))
		{
			/* Not a number, can not be PID */
			continue;
		}

		QFile cmdline("/proc/" + pid + "/cmdline");
		if (!cmdline.open(QFile::ReadOnly | QFile::Text))
		{
			/* Can not open cmdline file */
			continue;
		}

		QTextStream in(&cmdline);
		QString command = in.readAll();
		if (command.startsWith(processName))
		{
			listOfPids.push_back(pid);
		}
	}

#endif

	return listOfPids.count();
}
