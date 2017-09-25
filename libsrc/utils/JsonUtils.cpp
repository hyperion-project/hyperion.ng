#include <utils/JsonUtils.h>

namespace JsonUtils {

	bool readFile(const QString& path, QJsonObject& obj, Logger* log, bool ignError)
	{
		QString data;
		if(!FileUtils::readFile(path, data, log, ignError))
			return false;

		if(!parseJson(path, data, obj, log))
			return false;

		return true;
	}

	bool parseJson(const QString& path, QString& data, QJsonObject& obj, Logger* log)
	{
		//remove Comments in Config
		//data.remove(QRegularExpression("([^:]?\\/\\/.*)"));

		QJsonParseError error;
		QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8(), &error);

		if (error.error != QJsonParseError::NoError)
		{
			// report to the user the failure and their locations in the document.
			int errorLine(0), errorColumn(0);

			for( int i=0, count=qMin( error.offset,data.size()); i<count; ++i )
			{
				++errorColumn;
				if(data.at(i) == '\n' )
				{
					errorColumn = 0;
					++errorLine;
				}
			}
			Error(log,"Failed to parse file %s: Error: %s at Line: %i, Column: %i", QSTRING_CSTR(path), QSTRING_CSTR(error.errorString()), errorLine, errorColumn);
			return false;
		}
		obj = doc.object();
		return true;
	}
};
