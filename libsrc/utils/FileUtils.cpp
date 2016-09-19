#include <utils/FileUtils.h>

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <QFileInfo>

namespace FileUtils {
	
std::string getBaseName( std::string sourceFile)
{
	QFileInfo fi( sourceFile.c_str() );
	return fi.fileName().toStdString();
}
 
std::string command_exec(const char* cmd)
{
	char buffer[128];
	std::string result = "";
	std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
	if (pipe) 
	{
	while (!feof(pipe.get()))
	{
		if (fgets(buffer, 128, pipe.get()) != NULL)
			result += buffer;
	}
	}
	
	return result;
}

};