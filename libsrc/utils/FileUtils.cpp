#include <utils/FileUtils.h>

#include <QFileInfo>
#include <QTextStream>

namespace FileUtils {

	QString getBaseName( QString sourceFile)
	{
		QFileInfo fi( sourceFile );
		return fi.fileName();
	}

	QString getDirName( QString sourceFile)
	{
		QFileInfo fi( sourceFile );
		return fi.path();
	}

	bool fileExists(const QString& path, Logger* log, bool ignError)
	{
		QFile file(path);
		if(!file.exists())
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
			ErrorIf((!ignError), log,"Can't open file: %s",QSTRING_CSTR(path));
			return false;
		}

		QTextStream in(&file);
		data = in.readAll();
		file.close();

		return true;
	}
};
