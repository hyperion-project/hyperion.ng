
// STL includes
#include <iostream>

// QT includes
#include <QRegExp>
#include <QString>
#include <QStringList>

int main()
{
	QString testString = "1-9, 11, 12,13,16-17";

	QRegExp overallExp("([0-9]+(\\-[0-9]+)?)(,[ ]*([0-9]+(\\-[0-9]+)?))*");
	{

		std::cout << "[1] Match found: " << (overallExp.exactMatch("5")?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.exactMatch("4-")?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.exactMatch("-4")?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.exactMatch("3-9")?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.exactMatch("1-90")?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.exactMatch("1-90,100")?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.exactMatch("1-90, 100")?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.exactMatch("1-90, 100-200")?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.exactMatch("1-90, 100-200, 100")?"true":"false") << std::endl;
	}
	{
		if (!overallExp.exactMatch(testString)) {
			std::cout << "No correct match" << std::endl;
			return -1;
		}
		QStringList splitString = testString.split(QChar(','));
		for (int i=0; i<splitString.size(); ++i) {
			if (splitString[i].contains("-"))
			{
				QStringList str = splitString[i].split("-");
				int startInd = str[0].toInt();
				int endInd   = str[1].toInt();
				std::cout << "==> " << startInd << "-" << endInd << std::endl;
			}
			else
			{
				int index = splitString[i].toInt();
				std::cout << "==> " << index << std::endl;
			}
		}
	}

	return 0;
}
