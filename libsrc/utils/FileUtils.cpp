#include <utils/FileUtils.h>

// qt incl
#include <QDir>
#include <QFileInfo>

// hyperion include
#include <hyperion/Hyperion.h>

namespace FileUtils {

	QString getBaseName(const QString& sourceFile)
	{
		QFileInfo fi(sourceFile);
		return fi.fileName();
	}

	QString getDirName(const QString& sourceFile)
	{
		QFileInfo fi(sourceFile);
		return fi.path();
	}

	bool removeDir(const QString& path, Logger* log)
	{
		if(!QDir(path).removeRecursively())
		{
			Error(log, "Failed to remove directory: %s", QSTRING_CSTR(path));
			return false;
		}
		return true;
	}

	bool fileExists(const QString& path, Logger* log, bool ignError)
	{
		if(!QFile::exists(path))
		{
			ErrorIf((!ignError), log,"File does not exist: %s",QSTRING_CSTR(path));
			return false;
		}
		return true;
	}

	bool readFile(const QString& path, QString& data, Logger* log, bool ignError)
	{
		QFile file(path);
		if(!fileExists(path, log, ignError))
		{
			return false;
		}

		if(!file.open(QFile::ReadOnly | QFile::Text))
		{
			if(!ignError)
				resolveFileError(file,log);
			return false;
		}
		data = QString(file.readAll());
		file.close();

		return true;
	}

	bool writeFile(const QString& path, const QByteArray& data, Logger* log)
	{
		QFile file(path);
		if (!file.open(QFile::WriteOnly | QFile::Truncate))
		{
			resolveFileError(file,log);
			return false;
		}

		if(file.write(data) == -1)
		{
			resolveFileError(file,log);
			return false;
		}

		file.close();
		return true;
	}

	bool removeFile(const QString& path, Logger* log, bool ignError)
	{
		QFile file(path);
		if(!file.remove())
		{
			if(!ignError)
				resolveFileError(file,log);
			return false;
		}
		return true;
	}

	void resolveFileError(const QFile& file, Logger* log)
	{
		QFile::FileError error = file.error();
		const char* fn = QSTRING_CSTR(file.fileName());
		switch(error)
		{
			case QFileDevice::NoError:
				Debug(log,"No error occurred while procesing file: %s",fn);
				break;
			case QFileDevice::ReadError:
				Error(log,"Can't read file: %s",fn);
				break;
			case QFileDevice::WriteError:
				Error(log,"Can't write file: %s",fn);
				break;
			case QFileDevice::FatalError:
				Error(log,"Fatal error while processing file: %s",fn);
				break;
			case QFileDevice::ResourceError:
				Error(log,"Resource Error while processing file: %s",fn);
				break;
			case QFileDevice::OpenError:
				Error(log,"Can't open file: %s",fn);
				break;
			case QFileDevice::AbortError:
				Error(log,"Abort Error while processing file: %s",fn);
				break;
			case QFileDevice::TimeOutError:
				Error(log,"Timeout Error while processing file: %s",fn);
				break;
			case QFileDevice::UnspecifiedError:
				Error(log,"Unspecified Error while processing file: %s",fn);
				break;
			case QFileDevice::RemoveError:
				Error(log,"Failed to remove file: %s",fn);
				break;
			case QFileDevice::RenameError:
				Error(log,"Failed to rename file: %s",fn);
				break;
			case QFileDevice::PositionError:
				Error(log,"Position Error while processing file: %s",fn);
				break;
			case QFileDevice::ResizeError:
				Error(log,"Resize Error while processing file: %s",fn);
				break;
			case QFileDevice::PermissionsError:
				Error(log,"Permission Error at file: %s",fn);
				break;
			case QFileDevice::CopyError:
				Error(log,"Error during file copy of file: %s",fn);
				break;
			default:
				break;
		}
	}
};
