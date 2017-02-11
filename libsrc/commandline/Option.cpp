#include "commandline/Option.h"
#include "commandline/Parser.h"

using namespace commandline;

bool Option::validate(Parser & parser, QString &value)
{
	/* By default everything is accepted */
	return true;
}

QString Option::value(Parser &parser)
{
	return parser.value(*this);
}

std::string Option::getStdString(Parser &parser)
{
	return value(parser).toStdString();
}

std::wstring Option::getStdWString(Parser &parser)
{
	return value(parser).toStdWString();
}

const char* Option::getCString(Parser &parser)
{
	return value(parser).toLocal8Bit().constData();
}

