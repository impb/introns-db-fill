#include "logger.h"
extern "C" {
#include <stdio.h>
#include <stdlib.h>
}

#include <QtGlobal>
#include <QCoreApplication>

Logger * Logger::_logger = 0;


void messageHandle(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s\n", msg);
        break;
    case QtWarningMsg:
        Logger::error(msg);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg);
        abort();
    }
}

void Logger::init(const QString &fileName)
{
    _logger = new Logger(fileName);
    qInstallMsgHandler(messageHandle);
}

void Logger::error(const QString &message)
{
    _logger->_mutex->lock();
    *(_logger->_ts.data()) << "Error: " << message << "\n";
    _logger->_ts->flush();
    _logger->_mutex->unlock();
}

Logger::Logger(const QString & fileName)
    : QObject()
    , _file(new QFile(this))
    , _ts(new QTextStream)
    , _mutex(new QMutex)
{
    if (fileName.isEmpty()) {
        _file->open(stderr, QIODevice::WriteOnly);
    }
    else {
        _file->setFileName(fileName);
        _file->open(QIODevice::WriteOnly|QIODevice::Append);
    }
    _ts->setDevice(_file);
    _ts->setCodec("UTF-8");
}

