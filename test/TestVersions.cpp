
// STL includes
#include <iostream>

// QT includes
#include <QString>

#include <utils/version.hpp>
using namespace semver;

void test(const QString& a, const QString& b, char exp)
{
	semver::version verA (a.toStdString());
	semver::version verB (b.toStdString());

	std::cout << "[" << a.toStdString() << "] : " << (verA.isValid() ? "" : "not ") << "valid" << std::endl;
	std::cout << "[" << b.toStdString() << "] : " << (verB.isValid() ? "" : "not ") << "valid" << std::endl;

	if ( verA.isValid() && verB.isValid())
	{
		std::cout << "[" << a.toStdString() << "] < [" << b.toStdString() << "]: " << (verA < verB)  << " " << ((exp == '<') ? ((verA < verB)  ? "OK" : "NOK") : "") << std::endl;
		std::cout << "[" << a.toStdString() << "] > [" << b.toStdString() << "]: " << (verA > verB)  << " " << ((exp == '>') ? ((verA > verB)  ? "OK" : "NOK") : "") << std::endl;
		std::cout << "[" << a.toStdString() << "] = [" << b.toStdString() << "]: " << (verA == verB) << " " << ((exp == '=') ? ((verA == verB) ? "OK" : "NOK") : "") << std::endl;
	}
	std::cout << "" << std::endl;
};

int main()
{
	test ("2.12.0", "2.12.0", '=');
	test ("2.0.0-alpha.11", "2.12.0", '<');
	test ("2.11.1", "2.12.0", '<');
	test ("2.12.0", "2.12.1", '<');

	test ("2.12.0+PR4711", "2.12.0+PR4712", '=');
	test ("2.12.0+nighly20211012ac1dad7", "2.12.0+nighly20211012ac1dad8", '=');

	test ("2.0.0+nighly20211012ac1dad7", "2.0.12+nighly20211012ac1dad8", '<');

	test ("2.0.0-alpha.11+nighly20211012ac1dad7", "2.0.0-alpha.11+nighly20211012ac1dad8", '=');
	test ("2.0.0-alpha.11+PR1354", "2.0.0-alpha.11+PR1355", '=');

	test ("2.0.0-alpha.11+nighly20211012ac1dad7", "2.0.12+nighly20211012ac1dad8", '<');
	test ("2.0.0-alpha.11+nighly20211012ac1dad7", "2.0.12", '<');
	test ("2.0.0-alpha-11", "2.12.0", '<');

	test ("2.0.0-alpha.10.1", "2.0.0-alpha.10", '>');

	return 0;
}
