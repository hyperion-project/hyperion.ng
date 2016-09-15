#include <utils/FileUtils.h>

#include <QFileInfo>

namespace FileUtils {
	
std::string getBaseName( std::string sourceFile)
{
	QFileInfo fi( sourceFile.c_str() );
	return fi.fileName().toStdString();
}
 

};