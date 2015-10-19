#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QString>
#include <QTextStream>
#include <QMutex>
#include <QScopedPointer>
#include <QFile>

class Logger: public QObject
{
public:
    static void init(const QString & fileName = QString());
    static void error(const QString & message);
private:
    explicit Logger(const QString & fileName = QString());

    static Logger * _logger;
    QFile * _file;
    QScopedPointer<QTextStream> _ts;
    QScopedPointer<QMutex> _mutex;

};

#endif // LOGGER_H
