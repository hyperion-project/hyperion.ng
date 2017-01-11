#include <utils/FileUtils.h>

#include <QFileInfo>

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
 

};