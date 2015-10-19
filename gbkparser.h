#ifndef GBKPARSER_H
#define GBKPARSER_H

#include "structures.h"

#include <QIODevice>
#include <QTextStream>

class Database;

class GbkParser
{
public:
    void setSource(QIODevice * sourceStream, const QString &fileName);
    void setDatabase(QSharedPointer<Database> db);
    void setOverrideOrganismName(const QString & name);
    bool atEnd() const;
    SequencePtr readSequence();

private:
    static GenePtr findGeneMatchingLocation(const QList<GenePtr> &genes,
                                            const quint32 start,
                                            const quint32 end,
                                            const bool backwardChain
                                            );

    static GenePtr findGeneContainingLocation(const QList<GenePtr> &genes,
                                              const quint32 start,
                                              const quint32 end,
                                              const bool backwardChain
                                              );

    static IsoformPtr findRnaIsoformContainingLocation(
            const QList<IsoformPtr> &isoforms,
            const QList<quint32> & starts, const QList<quint32> & ends,
            const bool backwardChain);

    void parseTopLevel(const QString & prefix, QString value, SequencePtr seq);
    void parseSecondLevel(const QString & prefix, QString value, SequencePtr seq);

    GenePtr parseGene(const QString & value, SequencePtr seq);
    void parseCdsOrRna(const QString & prefix, const QString & value, SequencePtr seq);

    void createIntronsAndExons(IsoformPtr isoform, bool rna, bool bw,
                               const QList<quint32> & starts,
                               const QList<quint32> ends);

    static QByteArray dnaReverseComplement(const QByteArray & origin, int start, int end);
    void fillIntronsAndExonsFromOrigin(SequencePtr seq);
    void fillIntronsAndExonsFromOrigin(IsoformPtr isoform, const QByteArray & origin);

    void parseRange(const QString & value, quint32 * start, quint32 * end, bool * bw,
                    QList<quint32> * starts, QList<quint32> * ends);
    QMap<QString,QString> parseFeatureAttributes(const QString & value);



    enum State {
        TopLevel, Features, Origin
    } _state = TopLevel;

    QIODevice * _io = nullptr;
    QTextStream *_stream = nullptr;
    quint32 _featureStartLineNo = 0u;
    quint32 _currentLineNo = 0u;
    QString _fileName;
    QSharedPointer<Database> _db;
    QString _overrideOrganismName;
};

#endif // GBKPARSER_H
