
// STL includes
#include <iostream>

// QT includes
#include <QRegularExpression>
#include <QString>
#include <QStringList>

int main()
{
	QString testString = "1-9, 11, 12,13,16-17";

	QRegularExpression overallExp("([0-9]+(\\-[0-9]+)?)(,[ ]*([0-9]+(\\-[0-9]+)?))*");
	{

		std::cout << "[1] Match found: " << (overallExp.match("5").hasMatch()?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.match("4-").hasMatch()?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.match("-4").hasMatch()?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.match("3-9").hasMatch()?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.match("1-90").hasMatch()?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.match("1-90,100").hasMatch()?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.match("1-90, 100").hasMatch()?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.match("1-90, 100-200").hasMatch()?"true":"false") << std::endl;
		std::cout << "[1] Match found: " << (overallExp.match("1-90, 100-200, 100").hasMatch()?"true":"false") << std::endl;
	}
	{
		if (!overallExp.match(testString).hasMatch()) {
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
