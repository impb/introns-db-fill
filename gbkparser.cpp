#include "gbkparser.h"

#include "database.h"
#include "structures.h"

#include <QDebug>
#include <QFileInfo>
#include <QStringList>
#include <QThread>

void GbkParser::setSource(QIODevice *sourceStream, const QString &fileName)
{
    _io = sourceStream;
    _stream = new QTextStream(_io);
    _stream->setCodec("UTF-8");
    _state = State::TopLevel;
    _fileName = QFileInfo(fileName).fileName();
}

void GbkParser::setDatabase(QSharedPointer<Database> db)
{
    _db = db;
}

void GbkParser::setOverrideOrganismName(const QString &name)
{
    _overrideOrganismName = name;
}

bool GbkParser::atEnd() const
{
    return !_io || !_stream || _stream->atEnd();
}

SequencePtr GbkParser::readSequence()
{
    _state = TopLevel;
    SequencePtr seq(new Sequence);
    seq->sourceFileName = _fileName;
    QString topLevelName;
    QString topLevelValue;
    QString secondLevelName;
    QString secondLevelValue;
    while (!atEnd()) {
        QString currentLine = _stream->readLine();
        currentLine.replace('\t', "    ");
        if ("//" == currentLine.trimmed()) {
            break;
        }
        if (State::TopLevel == _state) {
            const QString prefix =
                    currentLine.length() > 12
                    ? currentLine.left(12).trimmed()
                    : currentLine.trimmed();

            const QString value =
                    currentLine.length() > 12
                    ? currentLine.mid(12).trimmed()
                    : QString();

            if (prefix.isEmpty()) {
                if (topLevelValue.length() > 0) {
                    topLevelValue.push_back('\n');
                }
                topLevelValue += value;
            }
            else {
                if (topLevelName.length() > 0) {
                    parseTopLevel(topLevelName, topLevelValue, seq);
                }
                if (State::Features == _state) {
                    secondLevelName = prefix;
                    secondLevelValue = value;
                }
                else {
                    topLevelName = prefix;
                    topLevelValue = value;
                }
            }
        }
        else if (State::Features == _state) {
            const QString prefix =
                    currentLine.length() > 21
                    ? currentLine.left(21).trimmed()
                    : currentLine.trimmed();
            const QString value =
                    currentLine.length() > 21
                    ? currentLine.mid(21).trimmed()
                    : QString();

            if (prefix.isEmpty()) {
                if (secondLevelValue.length() > 0) {
                    secondLevelValue.push_back('\n');
                }
                secondLevelValue += value;
            }
            else {
                if (secondLevelName.length() > 0) {
                    parseSecondLevel(secondLevelName, secondLevelValue, seq);
                }
                secondLevelName = prefix;
                secondLevelValue = value;
            }
            if ("ORIGIN" == prefix) {
                _state = State::Origin;
            }
        }
        else if (State::Origin == _state) {
            QString value =
                    currentLine.length() > 10
                    ? currentLine.mid(10)
                    : QString();
            value.replace(' ', "");
            value = value.toUpper();
            seq->origin.append(value.toLatin1());
        }
    }
    if (seq->genes.isEmpty() && seq->description.isEmpty()) {
        seq.clear();
    }
    else {
        fillIntronsAndExonsFromOrigin(seq);
    }
    return seq;
}

GenePtr GbkParser::findGeneMatchingLocation(
        const QList<GenePtr> &genes,
        const quint32 start, const quint32 end,
        const bool backwardChain)
{
    Q_FOREACH(GenePtr gene, genes) {
        const bool startMatch = start == gene->start;
        const bool endMatch = end == gene->end;
        const bool chainMatch = backwardChain == gene->backwardChain;
        if (startMatch && endMatch && chainMatch) {
            return gene;
        }
    }

    return GenePtr();
}

GenePtr GbkParser::findGeneContainingLocation(
        const QList<GenePtr> &genes,
        const quint32 start, const quint32 end,
        const bool backwardChain)
{
    Q_FOREACH(GenePtr gene, genes) {
        const bool startMatch = start >= gene->start;
        const bool endMatch = end <= gene->end;
        const bool chainMatch = backwardChain == gene->backwardChain;
        if (startMatch && endMatch && chainMatch) {
            return gene;
        }
    }

    return GenePtr();
}

IsoformPtr GbkParser::findRnaIsoformContainingLocation(
        const QList<IsoformPtr> &isoforms,
        const quint32 start,
        const quint32 end,
        const bool backwardChain)
{
    Q_FOREACH(IsoformPtr iso, isoforms) {
        const bool startMatch = start >= iso->mrnaStart;
        const bool endMatch = end <= iso->mrnaEnd;
        const bool chainMatch = backwardChain == iso->gene.toStrongRef()->backwardChain;
        if (startMatch && endMatch && chainMatch) {
            return iso;
        }
    }

    return IsoformPtr();
}

void GbkParser::parseTopLevel(const QString &prefix, QString value, SequencePtr seq)
{
    if ("LOCUS" == prefix) {
        const QStringList words = value.split(QRegExp("\\s+"));
        seq->refSeqId = words[0];
        seq->length = words[1].toUInt();
        qDebug() << "... " << seq->refSeqId
                 << " from " << _fileName
                 << " by worker " << QThread::currentThreadId();
    }
    else if ("ORGANISM" == prefix) {
        const QStringList lines = value.split('\n', QString::SkipEmptyParts);
        const QString name = _overrideOrganismName.isEmpty()
                ? lines[0].trimmed()
                : _overrideOrganismName;
        seq->organism = _db->findOrCreateOrganism(name).toWeakRef();
        if (seq->organism.toStrongRef()->taxonomyList.size() == 0) {
            for (int i=1; i<lines.size(); ++i) {
                const QStringList words = lines[i].split(';', QString::SkipEmptyParts);
                Q_FOREACH (QString word, words) {
                    word.replace('.', "");
                    word = word.simplified();
                    seq->organism.toStrongRef()->taxonomyList.append(word);
                }
            }
        }
    }
    else if ("DEFINITION" == prefix) {
        seq->description = value.replace('\n', ' ').simplified();
    }
    else if ("FEATURES" == prefix) {
        _state = State::Features;
    }
    else if ("ORIGIN" == prefix) {
        _state = State::Origin;
    }

    // TODO interact with organisms records
}

void GbkParser::parseSecondLevel(const QString &prefix, QString value, SequencePtr seq)
{
    if ("ORIGIN" == prefix) {
        _state = State::Origin;
    }
    else if ("gene" == prefix) {
        seq->genes.append(parseGene(value, seq));
    }
    else if ("source" == prefix) {
        const auto attrs = parseFeatureAttributes(value);
        if (attrs.contains("organelle")) {
            seq->organism.toStrongRef()->dbMitochondria =
                    "mitochondrion" == attrs["organelle"];
        }
        if (attrs.contains("db_xref")) {
            seq->organism.toStrongRef()->taxonomyXref =
                    attrs["db_xref"];
        }
        if (attrs.contains("organism")) {
            //Q_ASSERT(seq->organism.toStrongRef()->name == attrs["organism"]);
        }
        if (attrs.contains("chromosome")) {
            seq->chromosome =
                    _db->findOrCreateChromosome(
                        attrs["chromosome"],
                        seq->organism.toStrongRef()
                    );
        }
        else if (attrs.contains("organelle") && "mitochondrion" == attrs["organelle"]) {
            seq->chromosome =
                    _db->findOrCreateChromosome("mitochondrion",
                                                seq->organism.toStrongRef());
        }
    }
    else if ("CDS" == prefix || prefix.endsWith("RNA")) {
        parseCdsOrRna(prefix, value, seq);
    }
}

GenePtr GbkParser::parseGene(const QString & value, SequencePtr seq)
{
    GenePtr gene(new Gene);
    parseRange(value, &gene->start, &gene->end, &gene->backwardChain, 0, 0);
    gene->sequence = seq.toWeakRef();
    const auto attrs = parseFeatureAttributes(value);
    if (attrs.contains("gene")) {
        gene->name = attrs["gene"];
    }
    gene->isPseudoGene = attrs.contains("pseudo") || attrs.contains("pseudogene");
    return gene;
}

void GbkParser::parseCdsOrRna(const QString & prefix,
                              const QString &value, SequencePtr seq)
{    
    quint32 start = UINT32_MAX;
    quint32 end = 0;
    bool bw = false;
    QList<quint32> starts;
    QList<quint32> ends;
    parseRange(value, &start, &end, &bw, &starts, &ends);
    const QList<GenePtr> & allGenes = seq->genes;

    GenePtr targetGene;
    IsoformPtr targetIsoform;
    OrganismPtr organism = seq->organism.toStrongRef();

    if ("CDS" == prefix) {
        // CDS might have non-coding bounds inside gene
        targetGene = findGeneContainingLocation(allGenes, start, end, bw);

        if (! targetGene) {
            return;
        }

        // CDS must be linked to existing mRNA isoform
        const QList<IsoformPtr> & geneIsoforms = targetGene->isoforms;
        targetIsoform = findRnaIsoformContainingLocation(
                    geneIsoforms, start, end, bw
                    );

        if (! targetIsoform) {
            return;
        }
        targetGene->hasCDS = true;
        organism->mutex.lock();
        organism->cdsCount ++;
        organism->mutex.unlock();

        targetIsoform->cdsStart = start;
        targetIsoform->cdsEnd = end;
        targetIsoform->exonsCdsCount = starts.size();
        targetGene->isProteinButNotRna = true;
        targetGene->startCode = start;
        targetGene->endCode = end;
    }
    else {
        // *RNA range must be equal to gene location
        targetGene = findGeneMatchingLocation(allGenes, start, end, bw);

        if (! targetGene) {
            return;
        }

        if ("mRNA" == prefix) {
            targetIsoform = IsoformPtr(new Isoform);
            targetIsoform->mrnaStart = start;
            targetIsoform->mrnaEnd = end;
            targetIsoform->exonsMrnaCount = starts.size();
            targetGene->isoforms.push_back(targetIsoform);
        }
        else {
            targetGene->hasRNA = true;
            organism->mutex.lock();
            organism->rnaCount ++;
            organism->mutex.unlock();
        }
    }

    if (! targetIsoform) {
        return;
    }

    targetIsoform->gene = targetGene.toWeakRef();
    targetIsoform->sequence = targetGene->sequence;

    const auto attrs = parseFeatureAttributes(value);

    if (attrs.contains("protein_id")) {
        targetIsoform->proteinName = attrs["protein_id"];
    }
    if (attrs.contains("db_xref")) {
        targetIsoform->proteinXref = attrs["db_xref"];
    }
    if (attrs.contains("product")) {
        targetIsoform->product = attrs["product"];
    }

    if ("CDS" == prefix) {
        createIntronsAndExons(targetIsoform,
                              false,
                              bw,
                              starts, ends);
    }
}

void GbkParser::createIntronsAndExons(IsoformPtr isoform,
                                      bool rna, bool bw,
                                      const QList<quint32> &starts,
                                      const QList<quint32> ends)
{
    Q_ASSERT(starts.size() == ends.size());
    if (starts.size() == 0) {
        return;
    }

    int startIndex = bw ? starts.size() - 1 : 0;
    int endIndex = bw ? -1 : starts.size();
    int increment = bw ? -1 : 1;

    quint8 phase = 0;

    for (int exonIndex = startIndex;
         exonIndex != endIndex;
         exonIndex += increment)
    {
        const int start = starts[exonIndex];
        const int end = ends[exonIndex];
        CodingExonPtr exon(new CodingExon);
        exon->start = start;
        exon->end = end;
        exon->isoform = isoform;
        exon->gene = isoform->gene;
        exon->sequence = isoform->sequence;
        exon->startPhase = phase;
        phase = exon->endPhase = (phase + end - start + 1) % 3;
        isoform->exons.push_back(exon);
    }

    if (1 == isoform->exons.size()) {
        CodingExonPtr exon = isoform->exons.first();
        exon->index = exon->revIndex = 0;
        exon->type = CodingExon::Type::OneExon;
    }
    else {
        for (int index = 0; index < isoform->exons.size(); ++index) {
            CodingExonPtr exon = isoform->exons.at(index);
            exon->index = index;
            exon->revIndex = isoform->exons.size() - index - 1;
            if (0 == index) {
                exon->type = CodingExon::Type::Start;
            }
            else if (isoform->exons.size()-1 == index) {
                exon->type = CodingExon::Type::End;
            }
            else {
                exon->type = CodingExon::Type::Inner;
            }
            if (index > 0) {
                CodingExonPtr prevExon = isoform->exons[index-1];
                IntronPtr intron(new Intron);
                intron->isoform = isoform;
                intron->gene = isoform->gene;
                intron->sequence = isoform->sequence;
                intron->prevExon = prevExon;
                intron->nextExon = exon;
                intron->start = bw ? exon->end + 1 : prevExon->end + 1;
                intron->end = bw ? prevExon->start - 1 : exon->start - 1;
                intron->index = index - 1;
                intron->revIndex = isoform->exons.size() - index - 2;
                intron->phase = prevExon->endPhase;
                intron->lengthPhase = (intron->end - intron->start + 1) % 3;
                const quint8 prevStartPhase = prevExon->startPhase;
                const quint8 intrStartPhase = prevExon->endPhase;
                const quint8 nextEndPhase = exon->endPhase;
                const size_t typeIndex =
                        1 +  // SQL id's starts from 1 but not 0
                        9 * prevStartPhase +  // use prev start phase as group number
                        3 * intrStartPhase +  // use intron phase as row number
                        nextEndPhase;  // use next end phase as column number
                intron->intronTypeId = typeIndex;
                isoform->introns.push_back(intron);
                prevExon->nextIntron = intron;
                exon->prevIntron = intron;
            }
        }
    }
    GenePtr gene = isoform->gene.toStrongRef();

    gene->maxIntronsCount =
            qMax(gene->maxIntronsCount, quint32(isoform->introns.size()));

    if (rna) {
        gene->isProteinButNotRna = false;
        isoform->exonsMrnaCount = isoform->exons.size();
    }
    else {
        isoform->exonsCdsCount = isoform->exons.size();
    }

    Q_FOREACH(IsoformPtr iso, gene->isoforms) {
        iso->isMaximumByIntrons =
                quint32(iso->introns.size()) == gene->maxIntronsCount;
    }
}

QByteArray GbkParser::dnaReverseComplement(const QByteArray &origin,
                                           int start, int end)
{
    if (end > start) {
        // ensure reverse indexing
        int t = start;
        start = end;
        end = t;
    }
    const int length = start - end + 1;  // inclusive both bounds
    QByteArray result(length, '?');
    for (int i=0; i<length; ++i) {
        int originIndex = start - i - 1;
        char c = origin[originIndex];
        char t = '?';
        switch (c) {
        case 'A': t = 'T'; break;
        case 'T': t = 'A'; break;
        case 'G': t = 'C'; break;
        case 'C': t = 'G'; break;
        case 'N': t = 'N'; break;
        default:
            qWarning() << "Unknown letter: " << c;
            break;
        }
        result[i] = t;
    }
    return result;
}

void GbkParser::fillIntronsAndExonsFromOrigin(SequencePtr seq)
{
    const QByteArray & origin = seq->origin;
    Q_FOREACH(GenePtr gene, seq->genes) {
        Q_FOREACH(IsoformPtr isoform, gene->isoforms) {
            fillIntronsAndExonsFromOrigin(isoform, origin);
        }
    }
}

void GbkParser::fillIntronsAndExonsFromOrigin(IsoformPtr isoform,
                                              const QByteArray &origin)
{
    qint32 start = qMin(isoform->cdsStart, isoform->mrnaStart);
    qint32 end = qMax(isoform->cdsEnd, isoform->mrnaEnd);

    isoform->startCodon = origin.mid(start, 3);
    isoform->endCodon = origin.mid(end-4, 3);

    bool bw = isoform->gene.toStrongRef()->backwardChain;

    const QByteArray isoformOrigin = bw
            ? dnaReverseComplement(origin, start, end)
            : origin.mid(start-1, end-start+1);

    isoform->startCodon = isoformOrigin.left(3);
    isoform->endCodon = isoformOrigin.right(3);

    Q_FOREACH(CodingExonPtr exon, isoform->exons) {
        const qint32 exonStart = exon->start;
        const qint32 exonEnd = exon->end;

        exon->origin = bw
                ? dnaReverseComplement(origin, exonStart, exonEnd)
                : origin.mid(exonStart-1, exonEnd-exonStart+1);

        exon->startCodon = exon->origin.left(3);
        exon->endCodon = exon->origin.right(3);
        exon->errorNInSequence = exon->origin.contains('N');
        if (exon->errorNInSequence) {
            exon->isoform.toStrongRef()->errorInCodingExon = true;
            exon->isoform.toStrongRef()->errorMain = true;
        }
    }

    Q_FOREACH(IntronPtr intron, isoform->introns) {
        const qint32 intronStart = intron->start;
        const qint32 intronEnd = intron->end;
        Q_ASSERT(intronStart > start);
        Q_ASSERT(intronEnd < end);

        intron->origin = bw
                ? dnaReverseComplement(origin, intronStart, intronEnd)
                : origin.mid(intronStart-1, intronEnd-intronStart+1);

        intron->startDinucleotide = intron->origin.left(2);
        intron->endDinucleotide = intron->origin.right(2);

        intron->errorInStartDinucleotide = "GT" != intron->startDinucleotide;
        intron->errorInEndDinucleotide = "AG" != intron->endDinucleotide;
        intron->errorMain =
                intron->errorMain ||
                intron->errorInStartDinucleotide ||
                intron->errorInEndDinucleotide;
        if (intron->errorMain) {
            intron->isoform.toStrongRef()->errorInIntron = true;
            intron->isoform.toStrongRef()->errorMain = true;
        }
        intron->warningNInSequence = intron->origin.contains('N');
    }

}

void GbkParser::parseRange(const QString &value,
                           quint32 *start, quint32 *end,
                           bool *bw,
                           QList<quint32> * starts, QList<quint32> * ends)
{
    *start = UINT32_MAX;
    *end = 0;
    bool complement = value.trimmed().startsWith("complement(");
    bool join =
            value.trimmed().startsWith("join(") ||
            value.trimmed().startsWith("complement(join(");
    int startPos = 0;
    int endPos = 0;
    if (!complement && !join) {
        startPos = 0;
        endPos = value.indexOf('\n');
    }
    else if ((complement && !join) || (!complement && join)) {
        startPos = value.indexOf('(') + 1;
        endPos = value.indexOf(")\n");
    }
    else if (complement && join) {
        startPos = value.indexOf("(join(") + 6;
        endPos = value.indexOf("))\n");
    }

    const QStringList rangesStrs =
            value.mid(startPos, endPos-startPos).split(QRegExp(",\\s*"));

    Q_FOREACH(const QString & rangeStr, rangesStrs) {
        QStringList words = rangeStr.split("..");
        Q_ASSERT(words.size() == 2 || words.size() == 1);
        if (1 == words.size()) {
            words.append(words[0]);
        }
        words[0].remove(QRegExp("[<>]"));
        words[1].remove(QRegExp("[<>]"));
        quint32 st = words[0].toUInt();
        quint32 en = words[1].toUInt();
        if (starts && ends) {
            starts->append(st);
            ends->append(en);
        }
        *start = qMin(*start, st);
        *end = qMax(*end, en);
    }
    *bw = complement;
}

QMap<QString, QString> GbkParser::parseFeatureAttributes(const QString &value)
{
    QRegExp rxAttr("/(\\S+)=\\\"(.+)\\\"");
    QRegExp rxFlags("/(\\S+)");
    rxAttr.setMinimal(true);
    QMap<QString,QString> result;
    int pos = 0;
    Q_FOREVER {
        pos = rxAttr.indexIn(value, pos);
        if (-1 == pos) {
            break;
        }
        else {
            ++pos;
        }
        const QString key = rxAttr.cap(1);
        QString value = rxAttr.cap(2);
        value.replace('\n', " ");        
        result[key] = value.simplified();
    }
    pos = 0;
    Q_FOREVER {
        pos = rxFlags.indexIn(value, pos);
        if (-1 == pos) {
            break;
        }
        else {
            ++pos;
        }
        const QString key = rxFlags.cap(1);
        if (!result.count(key)) {
            result[key] = "";
        }
    }
    return result;
}

