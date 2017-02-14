#pragma once

#include <QByteArray>
#include <QString>

namespace Process {
	
void restartHyperion(bool asNewProcess=false); 
QByteArray command_exec(QString cmd, QByteArray data="");

};