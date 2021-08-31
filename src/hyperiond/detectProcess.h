#pragma once

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>

#ifdef WIN32
// psapi.h requires windows.h to be included
#include <Windows.h>
#include <Psapi.h>
#include <tlhelp32.h>
#endif

QStringList getProcessIdsByProcessName(const char *processName)
{
	QStringList listOfPids;

#if defined(WIN32)
	// https://docs.microsoft.com/en-us/windows/win32/toolhelp/taking-a-snapshot-and-viewing-processes
	/* Take a snapshot of all processes in the system */
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return {};
	}

	PROCESSENTRY32 pe32{};
	pe32.dwSize = sizeof(PROCESSENTRY32);

	/* Retrieve information about the first process */
	if(!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);
		return {};
	}

	/* Walk through the snapshot of processes */
	do
	{
		if (QString::compare(processName, QString::fromUtf16(reinterpret_cast<char16_t*>(pe32.szExeFile)), Qt::CaseInsensitive) == 0)
			listOfPids.append(QString::number(pe32.th32ProcessID));

	} while(Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);

#else

	QDir dir("/proc");
	dir.setFilter(QDir::Dirs);
	dir.setSorting(QDir::Name | QDir::Reversed);

	for (const QString & pid : dir.entryList()) {
		QRegularExpression regexp("^\\d*$");
		if (!regexp.match(pid).hasMatch())
		{
			/* Not a number, can not be PID */
			continue;
		}

		QFile cmdline("/proc/" + pid + "/comm");
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

	return listOfPids;
}
