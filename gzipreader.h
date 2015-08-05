#ifndef GZIPREADER_H
#define GZIPREADER_H

#include <zlib.h>

#include <QByteArray>
#include <QIODevice>
#include <QObject>

class GZipReader
        : public QIODevice
{
public:
    explicit GZipReader(QIODevice * compressedSource, QObject * parent = 0);
    ~GZipReader();

    bool atEnd() const override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    inline qint64 writeData(const char *, qint64 ) override { return 0; }

private:
    void readNextChunk(qint64 maxlen);

    QIODevice * _in;
    z_stream _gz;
    QByteArray _buf;

};

#endif // GZIPREADER_H
