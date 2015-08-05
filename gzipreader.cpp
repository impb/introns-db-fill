#undef QT_NO_DEBUG
#include "gzipreader.h"

extern "C" {
#include <string.h>
}

static const int GZIP_WINDOWS_BIT  = 15 + 16;
static const qint64 GZIP_CHUNK_SIZE  = 32 * 1024;

GZipReader::GZipReader(QIODevice * compressedSource, QObject * parent)
    : QIODevice(parent)
    , _in(compressedSource)
{
    if (!compressedSource->isOpen()) {
        compressedSource->open(ReadOnly);
    }
    _gz.zalloc = Z_NULL;
    _gz.zfree = Z_NULL;
    _gz.opaque = Z_NULL;
    _gz.avail_in = 0;
    _gz.next_in = Z_NULL;
    inflateInit2(&_gz, GZIP_WINDOWS_BIT);
}

GZipReader::~GZipReader()
{
    inflateEnd(&_gz);
}

bool GZipReader::atEnd() const
{
    return _in->atEnd() && _buf.isEmpty();
}

qint64 GZipReader::readData(char *data, qint64 maxlen)
{
    if (0 == _in->bytesAvailable() && _buf.isEmpty()) {
        return 0;
    }
    if (_buf.isEmpty()) {
        readNextChunk(maxlen);
    }
    qint64 chunkSize = qMin(maxlen, qint64(_buf.size()));
    memcpy(data, _buf.constData(), chunkSize);
    _buf.remove(0, chunkSize);
    return chunkSize;
}

void GZipReader::readNextChunk(qint64 maxlen)
{
    qint64 chunkSize = qMin(maxlen, GZIP_CHUNK_SIZE);
    QByteArray inBuff(chunkSize, 0);
    _buf.resize(inBuff.size() * 16);
    _buf.fill(0);

    chunkSize = _in->read(inBuff.data(), chunkSize);

    _gz.next_in = (unsigned char*) inBuff.data();
    _gz.avail_in = chunkSize;
    _gz.next_out = (unsigned char*) _buf.data();
    _gz.avail_out = _buf.size();
    Q_ASSERT(Z_STREAM_ERROR != inflate(&_gz, Z_NO_FLUSH));
    qint64 bytesRead = _buf.size() - _gz.avail_out;
    _buf.resize(bytesRead);
    Q_UNUSED(bytesRead);
}

