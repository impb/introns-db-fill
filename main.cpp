#include "database.h"
#include "iniparser.h"
#include "gbkparser.h"
#include "gzipreader.h"
#include "logger.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QSemaphore>
#include <QSharedPointer>
#include <QString>
#include <QThread>


struct Arguments {
    QString databaseHost;  // --host=...
    QString databaseUser;  // --user=...
    QString databasePass;  // --pass=...
    QString databaseName;  // --db=...

    QString sequencesDir;  // --seqdir=...
    QString translationsDir;  // --transdir=...

    quint16 maxThreads = 1;  // --threads=...

    QStringList sourceFileNames;    // positional parameters
    QString extraDataFile;  // --use-data=...

    QString loggerFileName; // --logfile=...
};


Arguments parseArguments()
{
    Arguments result;
    const QStringList args = qApp->arguments().mid(1);
    Q_FOREACH (const QString & arg, args) {
        if (arg.startsWith("--host=")) {
            result.databaseHost = arg.mid(7);
        }
        else if (arg.startsWith("--user=")) {
            result.databaseUser = arg.mid(7);
        }
        else if (arg.startsWith("--pass=")) {
            result.databasePass = arg.mid(7);
        }
        else if (arg.startsWith("--db=")) {
            result.databaseName = arg.mid(5);
        }
        else if (arg.startsWith("--seqdir=")) {
            result.sequencesDir = arg.mid(9);
        }
        else if (arg.startsWith("--transdir=")) {
            result.translationsDir = arg.mid(11);
        }
        else if (arg.startsWith("--threads=")) {
            result.maxThreads = arg.mid(10).toUShort();
        }
        else if (arg.startsWith("--use-data=")) {
            result.extraDataFile = arg.mid(11);
        }
        else if (arg.startsWith("--logfile=")) {
            result.loggerFileName = arg.mid(10);
        }
        else if (!arg.startsWith("-")) {
            result.sourceFileNames.push_back(arg);
        }
    }

    if (result.databaseHost.isEmpty()) {
        qWarning() << "DB host name not specified. Using 'localhost'.";
        result.databaseHost = "localhost";
    }
    if (result.databaseName.isEmpty()) {
        qWarning() << "DB name not specified. Using 'introns'.";
        result.databaseName = "introns";
    }
    if (result.databaseUser.isEmpty()) {
        qWarning() << "DB user name not specified. Using 'root'.";
        result.databaseUser = "root";
    }
    if (result.sequencesDir.isEmpty()) {
        qWarning() << "Directory for storing sequneces not specified. Origins will not be stored!";
    }
    if (result.translationsDir.isEmpty()) {
        qWarning() << "Directory for storing translations not specified. Translations will not be stored!";
    }
    if (result.loggerFileName.isEmpty()) {
        qWarning() << "Log file name not specified. Errors will be printed at STDERR.";
    }
    if (0 == result.maxThreads) {
        result.maxThreads = qMin(QThread::idealThreadCount(), result.sourceFileNames.size());
        qWarning() << "Threads count not specified. " << result.maxThreads << " cores will be utilized.";
    }
    return result;
}


class Worker
        : public QThread
{
public:
    explicit Worker(const Arguments & args, int from, int to);
    void launch();
private:
    void processOneFile();
    void run() override;
    const Arguments & _args;
    int _index = -1;
    const int _from;
    const int _to;
    QSemaphore _semaphore;
};

Worker::Worker(const Arguments &args, int from, int to)
    : QThread()
    , _args(args)
    , _from(from)
    , _to(to)
{
}

void Worker::run()
{
    qDebug() << "Created thread " << QThread::currentThreadId();
    _semaphore.acquire();
    for (_index = _from; _index < _to; ++_index) {
        const QString &fileName = _args.sourceFileNames.at(_index);
        qDebug() << "Start processing file " << fileName
                 << " by worker " << QThread::currentThreadId();
        processOneFile();
        qDebug() << "Done processing file " << fileName
                 << " by worker " << QThread::currentThreadId();
    }
    qDebug() << "Finished thread " << QThread::currentThreadId();
}

void Worker::launch()
{
    _semaphore.release();
}

void Worker::processOneFile()
{
    const QString inputFileName = _args.sourceFileNames.at(_index);
    QIODevice * inputSource = nullptr;
    QFile * inputFile = new QFile(inputFileName);
    GZipReader * gzipReader = nullptr;

    if (inputFileName.endsWith(".gz") && inputFile->open(QIODevice::ReadOnly)) {
        gzipReader = new GZipReader(inputFile);
        gzipReader->open(QIODevice::ReadOnly|QIODevice::Text);
        inputSource = gzipReader;
    }
    else if (inputFile->open(QIODevice::ReadOnly|QIODevice::Text)) {
        inputSource = inputFile;
    }
    else {
        qWarning() << "Can't open file " << inputFileName << ". Skipped!";
    }

    if (inputSource) {
        QSharedPointer<GbkParser> parser(new GbkParser);
        QSharedPointer<IniParser> supplParser(new IniParser);
        QSharedPointer<Database> db(Database::open(
                                        _args.databaseHost,
                                        _args.databaseUser,
                                        _args.databasePass,
                                        _args.databaseName,
                                        _args.sequencesDir,
                                        _args.translationsDir
                                        ));
        parser->setDatabase(db);
        parser->setSource(inputSource, inputFileName);
        QString supplFileName = _args.extraDataFile;
        if (supplFileName.isEmpty()) {
            supplFileName =
                QFileInfo(inputFileName).absoluteDir()
                .absoluteFilePath(
                    QFileInfo(inputFileName).baseName() + ".ini"
                    );
        }
        if (!supplFileName.isEmpty() && QFile(supplFileName).exists()) {
            supplParser->setSourceFileName(supplFileName);
            supplParser->setDatabase(db);
            parser->setOverrideOrganismName(
                        supplParser->value("organisms", "name").toString()
                        );
        }
        while (!parser->atEnd()) {
            SequencePtr seq = parser->readSequence();
            if (!seq) {
                continue;
            }
            supplParser->updateOrganism(seq->organism);
            supplParser->updateOrganismTaxonomy(seq->organism);
            db->storeOrigin(seq);
            db->addSequence(seq);
            if (seq->organism) {
                db->updateOrganism(seq->organism);
            }
        }
    }

    if (gzipReader) {
        gzipReader->close();
        delete gzipReader;
    }
    if (inputFile) {
        inputFile->close();
        delete inputFile;
    }
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    const Arguments args = parseArguments();
    Logger::init(args.loggerFileName);

    const quint32 filesPerWorker = args.sourceFileNames.size() / args.maxThreads;

    QList<Worker*> pool;

    for (quint16 threadNo = 0; threadNo < args.maxThreads; ++threadNo) {
        int start = threadNo * filesPerWorker;
        int end = start + filesPerWorker;
        if (args.maxThreads-1 == threadNo) {
            end = args.sourceFileNames.size();
        }
        Worker * worker = new Worker(args, start, end);
        worker->start();
        pool.append(worker);
    }

    Q_FOREACH(Worker * worker, pool) {
        worker->launch();
    }

    Q_FOREACH(Worker * worker, pool) {
        worker->wait();
        delete worker;
    }

    return 0;
}
