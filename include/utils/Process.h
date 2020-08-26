#pragma once

#include <QString>
#include <QByteArray>

namespace Process {

void restartHyperion(bool asNewProcess=false);
QByteArray command_exec(const QString& cmd, const QByteArray& data = {});

}
