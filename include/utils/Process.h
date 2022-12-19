#pragma once

#include <QString>
#include <QByteArray>

namespace Process {

void restartHyperion(int exitCode = 0);
QByteArray command_exec(const QString& cmd, const QByteArray& data = {});

}
