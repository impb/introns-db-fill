#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <QtGlobal>

#include <QDateTime>
#include <QList>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QWeakPointer>

struct IntronType;
struct TaxKingdom;
struct TaxGroup1;
struct TaxGroup2;
struct OrthologousGroup;
struct Organism;
struct Chromosome;
struct Sequence;
struct Gene;
struct Isoform;
struct Exon;
struct Intron;

struct Range {
    quint32 start;
    quint32 end;

    inline static QList<Range> createList(const QList<quint32> &starts,
                                          const QList<quint32> &ends)
    {
        Q_ASSERT(starts.size() == ends.size());
        QList<Range> result;
        for (int i=0; i<starts.size(); ++i) {
            Range range;
            range.start = starts[i];
            range.end = ends[i];
            result.push_back(range);
        }
        return result;
    }


    inline bool contains(const Range & other) const {
        return this->start <= other.start && this->end >= other.end;
    }
};

typedef QSharedPointer<IntronType> IntronTypePtr;
typedef QSharedPointer<TaxKingdom> TaxKingdomPtr;
typedef QSharedPointer<TaxGroup1> TaxGroup1Ptr;
typedef QSharedPointer<TaxGroup2> TaxGroup2Ptr;
typedef QSharedPointer<OrthologousGroup> OrthologousGroupPtr;
typedef QSharedPointer<Organism> OrganismPtr;
typedef QSharedPointer<Chromosome> ChromosomePtr;
typedef QSharedPointer<Sequence> SequencePtr;
typedef QSharedPointer<Gene> GenePtr;
typedef QSharedPointer<Isoform> IsoformPtr;
typedef QSharedPointer<Exon> ExonPtr;
typedef QSharedPointer<Intron> IntronPtr;

typedef QWeakPointer<IntronType> IntronTypeWPtr;
typedef QWeakPointer<TaxKingdom> TaxKingdomWPtr;
typedef QWeakPointer<TaxGroup1> TaxGroup1WPtr;
typedef QWeakPointer<TaxGroup2> TaxGroup2WPtr;
typedef QWeakPointer<OrthologousGroup> OrthologousGroupWPtr;
typedef QWeakPointer<Organism> OrganismWPtr;
typedef QWeakPointer<Chromosome> ChromosomeWPtr;
typedef QWeakPointer<Sequence> SequenceWPtr;
typedef QWeakPointer<Gene> GeneWPtr;
typedef QWeakPointer<Isoform> IsoformWPtr;
typedef QWeakPointer<Exon> CodingExonWPtr;
typedef QWeakPointer<Intron> IntronWPtr;


struct IntronType {
    QString         representation;
};


struct TaxKingdom {
    qint32          id = 0;
    QString         name;
};



struct TaxGroup1 {
    qint32          id = 0;
    TaxKingdomWPtr  kingdomPtr;
    QString         name;
    QString         type;
};



struct TaxGroup2 {
    qint32          id = 0;
    TaxKingdomWPtr  kingdomPtr;
    TaxGroup1WPtr   taxGroup1Ptr;
    QString         name;
    QString         type;
};



struct OrthologousGroup {
    QString         name;
    QString         fullName;
};


struct Organism
{
    QMutex          mutex; // for multithreaded processing
    qint32          id = 0;
    QString         name;
    QString         refSeqAssemblyId;
    QString         annotationRelease;
    QDate           annotationDate;
    QString         taxonomyXref;
    QStringList     taxonomyList;
    TaxKingdomWPtr  kingdom;
    TaxGroup1WPtr   taxGroup1;
    TaxGroup2WPtr   taxGroup2;
    quint32         realChromosomeCount = 0;
    quint32         dbChromosomeCount = 0;
    bool            realMitochondria = false;
    bool            dbMitochondria = false;
    quint32         unknownSequencesCount = 0;
    quint64         totalSequencesLength = 0;
    quint32         bGenesCount = 0;
    quint32         rGenesCount = 0;
    quint32         cdsCount = 0;
    quint32         rnaCount = 0;
    quint32         unknownProtGenesCount = 0;
    quint32         unknownProtCdsCount = 0;
    quint32         unknownRnaCdsCount = 0;
    quint32         exonsCount = 0;
    quint32         intronsCount = 0;
};



struct Chromosome {
    qint32          id = 0;
    QMutex          mutex;
    OrganismWPtr    organism;
    QString         name;
    quint32         length = 0;
};



struct Sequence {
    qint32          id = 0;
    QString         sourceFileName;
    QString         refSeqId;
    QString         version;
    QString         description;
    quint32         length = 0;
    OrganismWPtr    organism;
    ChromosomeWPtr  chromosome;
    QString         originFileName;
    QByteArray      origin;

    QList<GenePtr>  genes;
};



struct Gene {
    qint32          id = 0;
    SequenceWPtr    sequence;
    OrthologousGroupWPtr orthologousGroup;
    QString         name;
    QString         note;
    bool            backwardChain = false;
    bool            isProteinButNotRna = false;
    bool            isPseudoGene = false;
    quint32         start = UINT32_MAX;
    quint32         end = 0;
    quint32         startCode = UINT32_MAX;
    quint32         endCode = 0;
    quint32         maxIntronsCount = 0;

    QList<IsoformPtr> isoforms;
    bool            hasCDS = false;
    bool            hasRNA = false;
};



struct Isoform {
    qint32          id = 0;
    enum Type {
        MRNA = 0, CDS = 1, Other = 255
    }               type = Other;
    GeneWPtr        gene;
    SequenceWPtr    sequence;
    QString         proteinXref;
    QString         proteinId;
    QString         product;
    QString         note;
    quint32         cdsStart = UINT32_MAX;
    quint32         cdsEnd = 0;
    quint32         mrnaStart = UINT32_MAX;
    quint32         mrnaEnd = 0;
    quint32         exonsCdsCount = 0;
    quint32         exonsMrnaCount = 0;
    quint32         exonsLength = 0;
    QByteArray      startCodon;
    QByteArray      endCodon;
    bool            errorInLength = false;
    bool            errorInStartCodon = false;
    bool            errorInEndCodon = false;
    bool            errorInIntron = false;
    bool            errorInCodingExon = false;
    bool            errorMain = false;
    QString         errorComment;
    bool            isMaximumByIntrons = false;

    QList<ExonPtr>    exons;
    QList<IntronPtr>  introns;
    bool              hasCDS = false;
    QString         translation;

    // Fields required to match CDS/mRNA
    QList<Range>    mRnaRanges;

};



struct Exon {
    qint32          id = 0;
    IsoformWPtr     isoform;
    GeneWPtr        gene;
    SequenceWPtr    sequence;
    quint32         start = 0;
    quint32         end = 0;
    enum Type {
        OneExon = 0, Start = 1 , End = 2, Inner = 3, Unknown = 4
    }               type = Unknown;
    quint8          startPhase = 0;
    quint8          endPhase = 0;
    quint8          lengthPhase = 0;
    quint32         index = 0;
    quint32         revIndex = 0;
    QByteArray      startCodon;
    QByteArray      endCodon;
    IntronWPtr      prevIntron;
    IntronWPtr      nextIntron;
    QByteArray      origin;

    bool            errorInPseudoFlag = false;
    bool            errorNInSequence = false;
};


struct Intron {
    qint32          id = 0;
    IsoformWPtr     isoform;
    GeneWPtr        gene;
    SequenceWPtr    sequence;
    CodingExonWPtr  prevExon;
    CodingExonWPtr  nextExon;
    QByteArray      startDinucleotide;
    QByteArray      endDinucleotide;
    quint32         start = 0;
    quint32         end = 0;
    quint32         index = 0;
    quint32         revIndex = 0;
    quint8          lengthPhase = 0;
    quint8          phase = 0;
    bool            errorInStartDinucleotide = false;
    bool            errorInEndDinucleotide = false;
    bool            errorMain = false;
    bool            warningNInSequence = false;
    qint32          intronTypeId = 0;
    QByteArray      origin;
};



#endif
