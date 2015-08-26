#include "database.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QThread>

QMap<QString, OrganismPtr> Database::_organisms;
QMutex Database::_organismsMutex;

QMap<QPair<OrganismPtr,QString>, ChromosomePtr> Database::_chromosomes;
QMutex Database::_chromosomesMutex;

QMutex Database::_taxMutex;
QMap<QString, TaxKingdomPtr> Database::_kingdoms;
QMap<QPair<QString,QString>, TaxGroup1Ptr> Database::_taxGroups1;
QMap<QPair<QString,QString>, TaxGroup2Ptr> Database::_taxGroups2;

QMutex Database::_connectionsMutex;
QMap<Qt::HANDLE,QSqlDatabase> Database::_connections;

QSharedPointer<Database> Database::open(const QString &host,
                         const QString &userName, const QString &password,
                         const QString &dbName, const QString &sequencesStoreDir)
{
    QSharedPointer<Database> result(new Database);

    QMutexLocker lock(&_connectionsMutex);
    const Qt::HANDLE threadId = QThread::currentThreadId();

    if (_connections.contains(threadId)) {
        result->_db = &_connections[threadId];
    }
    else {
        _connections[threadId] = QSqlDatabase::addDatabase(
                    "QMYSQL",
                    QString("introns_db_fill_pid%1_thread%2")
                    .arg(qApp->applicationPid())
                    .arg(qint64(QThread::currentThreadId()))
                    .toLatin1()
                    );

        _connections[threadId].setHostName(host);
        _connections[threadId].setUserName(userName);
        _connections[threadId].setPassword(password);
        _connections[threadId].setDatabaseName(dbName);
        _connections[threadId].setConnectOptions("CLIENT_COMPRESS=1");
        result->_db = &_connections[threadId];
    }
    result->_sequencesStoreDir = QDir::root();

    if (sequencesStoreDir.length() > 0) {
        const QString absPath = QDir(sequencesStoreDir).absolutePath();
        if (QDir::root().mkpath(absPath)) {
            result->_sequencesStoreDir = QDir(absPath);
        }
    }

    if (!result->_db->open()) {
        result.clear();
    }

    return result;
}

OrganismPtr Database::findOrCreateOrganism(const QString &name)
{
    OrganismPtr organism;
    QMutexLocker lock(&_organismsMutex);

    if (_organisms.contains(name)) {
        organism = _organisms[name];
    }    

    if (organism) {
        return organism;
    }

    QSqlQuery selectQuery("", *_db);
    selectQuery.prepare("SELECT * FROM organisms WHERE name=:name");
    selectQuery.bindValue(":name", name);

    if (!selectQuery.exec()) {
        qWarning() << selectQuery.lastError();
        qWarning() << selectQuery.lastError().text();
        qWarning() << selectQuery.lastQuery();
    }
    else {
        Q_ASSERT(selectQuery.size() == 1 || selectQuery.size() == 0);  // Unique values in table
        if (1 == selectQuery.size()) {
            selectQuery.seek(0);
            QSqlRecord organismRecord = selectQuery.record();
            organism = OrganismPtr(new Organism);
            organism->id = organismRecord.field("id").value().toInt();
            organism->name = organismRecord.field("name").value().toString();
            organism->refSeqAssemblyId = organismRecord.field("ref_seq_assembly_id").value().toString();
            organism->annotationRelease = organismRecord.field("annotation_release").value().toString();
            organism->annotationDate = organismRecord.field("annotation_date").value().toDate();
            organism->taxonomyXref = organismRecord.field("taxonomy_xref").value().toString();
            organism->taxonomyList = organismRecord.field("taxonomy_list").value().toString()
                    .split(QRegExp(";\\s+"), QString::SkipEmptyParts);
            organism->realChromosomeCount = organismRecord.field("real_chromosome_count").value().toInt();
            organism->dbChromosomeCount = organismRecord.field("db_chromosome_count").value().toInt();
            organism->realMitochondria = organismRecord.field("real_mitichondria").value().toBool();
            organism->dbMitochondria = organismRecord.field("db_mitichondria").value().toBool();
            organism->unknownSequencesCount = organismRecord.field("unknown_sequences_count").value().toInt();
            organism->totalSequencesLength = organismRecord.field("total_sequences_length").value().toInt();
            organism->bGenesCount = organismRecord.field("b_genes_count").value().toInt();
            organism->rGenesCount = organismRecord.field("r_genes_count").value().toInt();
            organism->cdsCount = organismRecord.field("cds_count").value().toInt();
            organism->rnaCount = organismRecord.field("rna_count").value().toInt();
            organism->unknownProtGenesCount = organismRecord.field("unknown_prot_genes_count").value().toInt();
            organism->unknownProtCdsCount = organismRecord.field("unknown_prot_cds_count").value().toInt();
            organism->exonsCount = organismRecord.field("exons_count").value().toInt();
            organism->intronsCount = organismRecord.field("introns_count").value().toInt();
        }
        else if (0 == selectQuery.size()) {
            // Insert into table new one
            organism = OrganismPtr(new Organism);
            organism->name = name;
            QSqlQuery insertQuery("", *_db);
            insertQuery.prepare("INSERT INTO organisms(name) VALUES(:name)");
            insertQuery.bindValue(":name", name);
            if (!insertQuery.exec()) {
                qWarning() << insertQuery.lastError();
                qWarning() << insertQuery.lastError().text();
                qWarning() << insertQuery.lastQuery();
            }
            else {
                organism->id = insertQuery.lastInsertId().toInt();
            }
            _db->commit();
        }
    }

    _organisms[name] = organism;

    return organism;
}

ChromosomePtr Database::findOrCreateChromosome(const QString &name, OrganismPtr organism)
{
    ChromosomePtr chromosome;
    QMutexLocker locker(&_chromosomesMutex);
    QMutexLocker locker2(&organism->mutex);

    const QPair<OrganismPtr,QString> key(organism, name);

    if (_chromosomes.contains(key)) {
        chromosome = _chromosomes[key];
    }

    if (chromosome) {
        return chromosome;
    }

    QSqlQuery selectQuery("", *_db);
    selectQuery.prepare("SELECT * FROM chromosomes WHERE name=:name AND id_organisms=:org_id");
    selectQuery.bindValue(":name", name);
    selectQuery.bindValue(":org_id", organism->id);

    if (!selectQuery.exec()) {
        qWarning() << selectQuery.lastError();
        qWarning() << selectQuery.lastError().text();
        qWarning() << selectQuery.lastQuery();
    }
    else {
        Q_ASSERT(selectQuery.size() == 1 || selectQuery.size() == 0);  // Unique values in table
        if (1 == selectQuery.size()) {
            selectQuery.seek(0);
            QSqlRecord chromosomeRecord = selectQuery.record();
            chromosome = ChromosomePtr(new Chromosome);
            chromosome->length = chromosomeRecord.field("lengthh").value().toUInt();
            chromosome->name = name;
            chromosome->id = chromosomeRecord.field("id").value().toInt();
        }
        else if (0 == selectQuery.size()) {
            // Insert into table new one
            chromosome = ChromosomePtr(new Chromosome);
            chromosome->name = name;

            QSqlQuery insertQuery("", *_db);
            insertQuery.prepare("INSERT INTO chromosomes(name, id_organisms) VALUES(:name,:org_id)");
            insertQuery.bindValue(":name", name);
            insertQuery.bindValue(":org_id", organism->id);
            if (!insertQuery.exec()) {
                qWarning() << insertQuery.lastError();
                qWarning() << insertQuery.lastError().text();
                qWarning() << insertQuery.lastQuery();
            }
            else {
                chromosome->id = insertQuery.lastInsertId().toInt();
            }
            _db->commit();
            if (!name.toLower().startsWith("unk") && !name.toLower().startsWith("mit")) {
                organism->dbChromosomeCount ++;
            }
        }
    }
    _chromosomes[key] = chromosome;
    return chromosome;
}

void Database::updateOrganism(OrganismPtr organism)
{
    QMutexLocker lock(&organism->mutex);

    if (0 == organism->id) {
        return;
    }

    // Name might be changed, so update search key
    _organismsMutex.lock();
    Q_FOREACH(const QString &key, _organisms.keys()) {
        OrganismPtr oldCandidate = _organisms[key];
        if (oldCandidate == organism && key != organism->name) {
            _organisms[organism->name] = organism;
            _organisms.remove(key);
            break;
        }
    }
    _organismsMutex.unlock();

    QSqlQuery query("", *_db);
    // TODO tax groups id
    query.prepare("UPDATE organisms SET "
                        "name=:name, "
                        "ref_seq_assembly_id=:ref_seq_assembly_id, "
                        "annotation_release=:annotation_release, "
                        "annotation_date=:annotation_date, "
                        "taxonomy_xref=:taxonomy_xref, "
                        "taxonomy_list=:taxonomy_list, "
                        "real_chromosome_count=:real_chromosome_count, "
                        "db_chromosome_count=:db_chromosome_count, "
                        "real_mitochondria=:real_mitochondria, "
                        "db_mitochondria=:db_mitochondria, "
                        "unknown_sequences_count=:unknown_sequences_count, "
                        "total_sequences_length=:total_sequences_length, "
                        "b_genes_count=:b_genes_count, "
                        "r_genes_count=:r_genes_count, "
                        "cds_count=:cds_count, "
                        "rna_count=:rna_count, "
                        "unknown_prot_genes_count=:unknown_prot_genes_count, "
                        "unknown_prot_cds_count=:unknown_prot_cds_count, "
                        "exons_count=:exons_count, "
                        "introns_count=:introns_count "
                        "WHERE id=:id"
                        );
    query.bindValue(":id", organism->id);
    query.bindValue(":name", organism->name);
    query.bindValue(":ref_seq_assembly_id", organism->refSeqAssemblyId);
    query.bindValue(":annotation_release", organism->annotationRelease);
    query.bindValue(":annotation_date", organism->annotationDate);
    query.bindValue(":taxonomy_xref", organism->taxonomyXref);
    query.bindValue(":taxonomy_list", organism->taxonomyList.join("; "));
    query.bindValue(":real_chromosome_count", organism->realChromosomeCount);
    query.bindValue(":db_chromosome_count", organism->dbChromosomeCount);
    query.bindValue(":real_mitochondria", organism->realMitochondria);
    query.bindValue(":db_mitochondria", organism->dbMitochondria);
    query.bindValue(":unknown_sequences_count", organism->unknownSequencesCount);
    query.bindValue(":total_sequences_length", organism->totalSequencesLength);
    query.bindValue(":b_genes_count", organism->bGenesCount);
    query.bindValue(":r_genes_count", organism->rGenesCount);
    query.bindValue(":cds_count", organism->cdsCount);
    query.bindValue(":rna_count", organism->rnaCount);
    query.bindValue(":unknown_prot_genes_count", organism->unknownProtGenesCount);
    query.bindValue(":unknown_prot_cds_count", organism->unknownProtCdsCount);
    query.bindValue(":exons_count", organism->exonsCount);
    query.bindValue(":introns_count", organism->intronsCount);

    if (!query.exec()) {
        qWarning() << query.lastError();
        qWarning() << query.lastError().text();
        qWarning() << query.lastQuery();
    }

    if (organism->taxGroup2) {
        query.clear();
        query.prepare("UPDATE organisms SET id_tax_groups2=:tid WHERE id=:id");
        query.bindValue(":tid", organism->taxGroup2.toStrongRef()->id);
        query.bindValue(":id", organism->id);
        if (!query.exec()) {
            qWarning() << query.lastError();
            qWarning() << query.lastError().text();
            qWarning() << query.lastQuery();
        }
    }
}

void Database::updateChromosome(ChromosomePtr chromosome)
{
    if (!chromosome) {
        return;
    }
    QMutexLocker locker(&chromosome->mutex);
    if (0==chromosome->id) {
        return;
    }
    QSqlQuery query("", *_db);
    query.prepare("UPDATE chromosomes SET lengthh=:l WHERE id=:id");
    query.bindValue(":l", chromosome->length);
    query.bindValue(":id", chromosome->id);
    if (!query.exec()) {
        qWarning() << query.lastError();
        qWarning() << query.lastError().text();
        qWarning() << query.lastQuery();
    }
}

TaxKingdomPtr Database::findOrCreateTaxKingdom(const QString &name)
{
    TaxKingdomPtr kingdom;
    _taxMutex.lock();
    if (_kingdoms.contains(name)) {
        kingdom = _kingdoms[name];
    }
    _taxMutex.unlock();
    if (kingdom) {
        return kingdom;
    }

    QSqlQuery selectQuery("", *_db);
    selectQuery.prepare("SELECT * FROM tax_kingdoms WHERE name=:name");
    selectQuery.bindValue(":name", name);
    if (!selectQuery.exec()) {
        qWarning() << selectQuery.lastError();
        qWarning() << selectQuery.lastError().text();
        qWarning() << selectQuery.lastQuery();
    }
    else {
        Q_ASSERT(selectQuery.size() == 1 || selectQuery.size() == 0);  // Unique values in table
        if (1 == selectQuery.size()) {
            selectQuery.seek(0);
            QSqlRecord kingdomRecord = selectQuery.record();
            kingdom = TaxKingdomPtr(new TaxKingdom);
            kingdom->name = name;
            kingdom->id = kingdomRecord.field("id").value().toInt();
        }
        else if (0 == selectQuery.size()) {
            kingdom = TaxKingdomPtr(new TaxKingdom);
            kingdom->name = name;
            QSqlQuery insertQuery("", *_db);
            insertQuery.prepare("INSERT INTO tax_kingdoms(name) VALUES(:name)");
            insertQuery.bindValue(":name", name);
            if (!insertQuery.exec()) {
                qWarning() << insertQuery.lastError();
                qWarning() << insertQuery.lastError().text();
                qWarning() << insertQuery.lastQuery();
            }
            else {
                kingdom->id = insertQuery.lastInsertId().toInt();
            }
        }

    }

    _taxMutex.lock();
    _kingdoms[name] = kingdom;
    _taxMutex.unlock();
    return kingdom;
}

TaxGroup1Ptr Database::findOrCreateTaxGroup1(const QString &name, const QString &type, TaxKingdomPtr kingdom)
{
    TaxGroup1Ptr group;
    const auto key = QPair<QString,QString>(name, type);
    _taxMutex.lock();
    if (_taxGroups1.contains(key)) {
        group = _taxGroups1[key];
    }
    _taxMutex.unlock();
    if (group) {
        return group;
    }

    QSqlQuery selectQuery("", *_db);
    selectQuery.prepare("SELECT * FROM tax_groups1 WHERE name=:name AND typee=:typee");
    selectQuery.bindValue(":name", name);
    selectQuery.bindValue(":typee", type);
    if (!selectQuery.exec()) {
        qWarning() << selectQuery.lastError();
        qWarning() << selectQuery.lastError().text();
        qWarning() << selectQuery.lastQuery();
    }
    else {
        Q_ASSERT(selectQuery.size() == 1 || selectQuery.size() == 0);  // Unique values in table
        if (1 == selectQuery.size()) {
            selectQuery.seek(0);
            QSqlRecord kingdomRecord = selectQuery.record();
            group = TaxGroup1Ptr(new TaxGroup1);
            group->name = name;
            group->type = type;
            group->id = kingdomRecord.field("id").value().toInt();
            group->kingdomPtr = kingdom;
        }
        else if (0 == selectQuery.size()) {
            group = TaxGroup1Ptr(new TaxGroup1);
            group->name = name;
            group->type = type;
            group->kingdomPtr = kingdom;
            QSqlQuery insertQuery("", *_db);
            insertQuery.prepare("INSERT INTO tax_groups1(name,typee,id_tax_kingdoms) VALUES(:name,:typee,:id_tax_kingdoms)");
            insertQuery.bindValue(":name", name);
            insertQuery.bindValue(":typee", type);
            insertQuery.bindValue(":id_tax_kingdoms", kingdom->id);
            if (!insertQuery.exec()) {
                qWarning() << insertQuery.lastError();
                qWarning() << insertQuery.lastError().text();
                qWarning() << insertQuery.lastQuery();
            }
            else {
                group->id = insertQuery.lastInsertId().toInt();
            }
        }

    }

    _taxMutex.lock();
    _taxGroups1[key] = group;
    _taxMutex.unlock();
    return group;
}

TaxGroup2Ptr Database::findOrCreateTaxGroup2(const QString &name, const QString &type, TaxGroup1Ptr group1)
{
    TaxGroup2Ptr group;
    const auto key = QPair<QString,QString>(name, type);
    _taxMutex.lock();
    if (_taxGroups2.contains(key)) {
        group = _taxGroups2[key];
    }
    _taxMutex.unlock();
    if (group) {
        return group;
    }

    QSqlQuery selectQuery("", *_db);
    selectQuery.prepare("SELECT * FROM tax_groups1 WHERE name=:name AND typee=:typee");
    selectQuery.bindValue(":name", name);
    selectQuery.bindValue(":typee", type);
    if (!selectQuery.exec()) {
        qWarning() << selectQuery.lastError();
        qWarning() << selectQuery.lastError().text();
        qWarning() << selectQuery.lastQuery();
    }
    else {
        Q_ASSERT(selectQuery.size() == 1 || selectQuery.size() == 0);  // Unique values in table
        if (1 == selectQuery.size()) {
            selectQuery.seek(0);
            QSqlRecord kingdomRecord = selectQuery.record();
            group = TaxGroup2Ptr(new TaxGroup2);
            group->name = name;
            group->type = type;
            group->id = kingdomRecord.field("id").value().toInt();
            group->kingdomPtr = group1->kingdomPtr;
            group->taxGroup1Ptr = group1;
        }
        else if (0 == selectQuery.size()) {
            group = TaxGroup2Ptr(new TaxGroup2);
            group->name = name;
            group->type = type;
            group->kingdomPtr = group1->kingdomPtr;
            group->taxGroup1Ptr = group1;
            QSqlQuery insertQuery("", *_db);
            insertQuery.prepare("INSERT INTO tax_groups2(name,typee,id_tax_groups1) VALUES(:name,:typee,:id_tax_groups1)");
            insertQuery.bindValue(":name", name);
            insertQuery.bindValue(":typee", type);
            insertQuery.bindValue(":id_tax_groups1", group1->id);

            if (!insertQuery.exec()) {
                qWarning() << insertQuery.lastError();
                qWarning() << insertQuery.lastError().text();
                qWarning() << insertQuery.lastQuery();
            }
            else {
                group->id = insertQuery.lastInsertId().toInt();
            }
        }

    }

    _taxMutex.lock();
    _taxGroups2[key] = group;
    _taxMutex.unlock();
    return group;
}

void Database::dropSequenceIfExists(SequencePtr sequence)
{
    OrganismPtr organism = sequence->organism.toStrongRef();
    organism->mutex.lock();
    qint32 organismId = organism->id;
    organism->mutex.unlock();
    const QString refSeqId = sequence->refSeqId;

    QSqlQuery query("", *_db);

    query.prepare("SELECT id FROM sequences WHERE id_organisms=:id_organisms AND ref_seq_id=:ref_seq_id");
    query.bindValue(":id_organisms", organismId);
    query.bindValue(":ref_seq_id", refSeqId);

    if (!query.exec()) {
        qWarning() << query.lastError();
        qWarning() << query.lastError().text();
        qWarning() << query.lastQuery();
        return;
    }

    for (int i=0; i<query.size(); ++i) {
        query.seek(i);
        const qint32 seqId = query.record().field("id").value().toInt();

        query.prepare("DELETE FROM introns WHERE id_sequences=:seq_id");
        query.bindValue(":seq_id", seqId);

        if (!query.exec()) {
            qWarning() << query.lastError();
            qWarning() << query.lastError().text();
            qWarning() << query.lastQuery();
        }

        query.prepare("DELETE FROM coding_exons WHERE id_sequences=:seq_id");
        query.bindValue(":seq_id", seqId);

        if (!query.exec()) {
            qWarning() << query.lastError();
            qWarning() << query.lastError().text();
            qWarning() << query.lastQuery();
        }

        query.prepare("DELETE FROM isoforms WHERE id_sequences=:seq_id");
        query.bindValue(":seq_id", seqId);

        if (!query.exec()) {
            qWarning() << query.lastError();
            qWarning() << query.lastError().text();
            qWarning() << query.lastQuery();
        }

        query.prepare("DELETE FROM genes WHERE id_sequences=:seq_id");
        query.bindValue(":seq_id", seqId);

        if (!query.exec()) {
            qWarning() << query.lastError();
            qWarning() << query.lastError().text();
            qWarning() << query.lastQuery();
        }

        query.prepare("DELETE FROM sequences WHERE id=:seq_id");
        query.bindValue(":seq_id", seqId);

        if (!query.exec()) {
            qWarning() << query.lastError();
            qWarning() << query.lastError().text();
            qWarning() << query.lastQuery();
        }
    }
}

void Database::addSequence(SequencePtr sequence)
{
    OrganismPtr organism = sequence->organism.toStrongRef();
    organism->mutex.lock();
    qint32 organismId = organism->id;
    organism->mutex.unlock();

    dropSequenceIfExists(sequence);

    QSqlQuery query("", *_db);
    query.prepare("INSERT INTO sequences("
                          "source_file_name"
                          ", ref_seq_id"
                          ", description"
                          ", lengthh"
                          ", id_organisms"
                          ", id_chromosomes"
                          ", origin_file_name"
                          ") VALUES("
                          ":file_name"
                          ", :ref_seq_id"
                          ", :description"
                          ", :lengthh"
                          ", :id_organisms"
                          ", :id_chromosomes"
                          ", :origin_file_name"
                          ")");
    query.bindValue(":source_file_name", sequence->sourceFileName);
    query.bindValue(":ref_seq_id", sequence->refSeqId);
    query.bindValue(":description", sequence->description);
    query.bindValue(":lengthh", sequence->length);
    query.bindValue(":id_organisms", organismId);
    qint32 chromosomeId = 0;
    if (sequence->chromosome) {
        ChromosomePtr chr = sequence->chromosome.toStrongRef();
        chr->mutex.lock();
        chromosomeId = chr->id;
        chr->mutex.unlock();
    }
    query.bindValue(":id_chromosomes", chromosomeId);
    query.bindValue(":origin_file_name", sequence->originFileName);

    if (!query.exec()) {
        qWarning() << query.lastError();
        qWarning() << query.lastError().text();
        qWarning() << query.lastQuery();
        return;
    }
    else {
        sequence->id = query.lastInsertId().toInt();
    }

    Q_FOREACH(GenePtr gene, sequence->genes) {
        addGene(gene);
    }

    if (sequence->chromosome) {
        ChromosomePtr chromosome = sequence->chromosome;
        chromosome->mutex.lock();
        chromosome->length += sequence->length;
        const QString chromosomeName = chromosome->name;
        chromosome->mutex.unlock();
        updateChromosome(chromosome);
        if (chromosomeName.toLower().startsWith("unk")) {
            organism->mutex.lock();
            organism->unknownSequencesCount ++;
            organism->mutex.unlock();
        }
    }

    organism->mutex.lock();
    organism->totalSequencesLength += sequence->length;
    Q_FOREACH(GenePtr gene, sequence->genes) {
        if (gene->hasCDS) {
            organism->bGenesCount ++;
        }
        if (gene->hasRNA && !gene->hasCDS) {
            organism->rGenesCount ++;
        }
        Q_FOREACH(IsoformPtr iso, gene->isoforms) {
            organism->exonsCount += iso->exons.size();
            organism->intronsCount += iso->introns.size();
        }
    }
    organism->mutex.unlock();
}

void Database::storeOrigin(SequencePtr sequence)
{
    if (_sequencesStoreDir == QDir::root()) {
        return;
    }
    OrganismPtr organism = sequence->organism.toStrongRef();
    organism->mutex.lock();
    QString organismName = organism->name;
    organism->mutex.unlock();
    organismName.replace(QRegExp("\\s+"), "_");
    organismName.replace(QRegExp("[(),./\\]"), "");
    organismName = organismName.toLower();

    QString chromosomeName;
    ChromosomePtr chromosome = sequence->chromosome.toStrongRef();
    if (chromosome) {
        chromosome->mutex.lock();
        chromosomeName = chromosome->name;
        chromosome->mutex.unlock();
        chromosomeName.replace(QRegExp("\\s+"), "_");
        chromosomeName.replace(".", "_");
        chromosomeName.replace(QRegExp("[(),/\\]"), "");
        chromosomeName = chromosomeName.toLower();
    }

    QString refName = sequence->refSeqId;
    refName.replace(QRegExp("\\s+"), "_");
    refName.replace(".", "_");
    refName.replace(QRegExp("[(),/\\]"), "");
    refName = refName.toLower();

    QString dirName = chromosomeName.isEmpty()
            ? organismName : organismName + "/" + chromosomeName;
    QString fileName = dirName + "/" + refName + ".raw.txt";

    if (! _sequencesStoreDir.mkpath(dirName)) {
        qWarning() << "Can't create dir '" << _sequencesStoreDir.filePath(dirName) <<
                      "'. Sequence '" << fileName << "' will not be stored!";
        return;
    }

    QFile originFile(_sequencesStoreDir.absoluteFilePath(fileName));
    if (!originFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Can't open '" << originFile.fileName() <<
                      "'. Sequence '" << fileName << "' will not be stored!";
        return;
    }

    if (originFile.write(sequence->origin) != sequence->origin.size()) {
        qWarning() << "Can't write '" << originFile.fileName() <<
                      "' (possible out of space). Sequence '" << fileName << "' will not be stored!";
    }
    else {
        sequence->originFileName = fileName;
    }
    originFile.close();
}


void Database::addGene(GenePtr gene)
{
    const qint32 sequenceId = gene->sequence.toStrongRef()->id;
    QSqlQuery query("", *_db);
    query.prepare("INSERT INTO genes("
                  "id_sequences"
                  ", name"
                  ", backward_chain"
                  ", protein_but_not_rna"
                  ", pseudo_gene"
                  ", startt"
                  ", endd"
                  ", start_code"
                  ", end_code"
                  ", max_introns_count"
                  ") VALUES("
                  ":id_sequences"
                  ", :name"
                  ", :backward_chain"
                  ", :protein_but_not_rna"
                  ", :pseudo_gene"
                  ", :startt"
                  ", :endd"
                  ", :start_code"
                  ", :end_code"
                  ", :max_introns_count"
                  ")");
    query.bindValue(":id_sequences", sequenceId);
    query.bindValue(":name", gene->name);
    query.bindValue(":backward_chain", gene->backwardChain);
    query.bindValue(":protein_but_not_rna", gene->isProteinButNotRna);
    query.bindValue(":pseudo_gene", gene->isPseudoGene);
    query.bindValue(":startt", UINT32_MAX == gene->start ? 0 : gene->start);
    query.bindValue(":endd", gene->end);
    query.bindValue(":start_code", UINT32_MAX == gene->startCode ? 0 : gene->startCode);
    query.bindValue(":end_code", gene->endCode);
    query.bindValue(":max_introns_count", gene->maxIntronsCount);


    if (!query.exec()) {
        qWarning() << query.lastError();
        qWarning() << query.lastError().text();
        qWarning() << query.lastQuery();
        return;
    }
    else {
        gene->id = query.lastInsertId().toInt();
    }

    Q_FOREACH(IsoformPtr isoform, gene->isoforms) {
        addIsoform(isoform);
    }

}

void Database::addIsoform(IsoformPtr isoform)
{
    const qint32 geneId = isoform->gene.toStrongRef()->id;
    QSqlQuery query("", *_db);
    query.prepare("INSERT INTO isoforms("
                  "id_genes"
                  ", id_sequences"
                  ", protein_xref"
                  ", protein_name"
                  ", product"
                  ", cds_start"
                  ", cds_end"
                  ", mrna_start"
                  ", mrna_end"
                  ", mrna_length"
                  ", exons_cds_count"
                  ", exons_mrna_count"
                  ", exons_length"
                  ", start_codon"
                  ", end_codon"
                  ", maximum_by_introns"
                  ", error_in_length"
                  ", error_in_start_codon"
                  ", error_in_end_codon"
                  ", error_in_intron"
                  ", error_in_coding_exon"
                  ", error_main"
                  ") VALUES("
                  ":id_genes"
                  ", :id_sequences"
                  ", :protein_xref"
                  ", :protein_name"
                  ", :product"
                  ", :cds_start"
                  ", :cds_end"
                  ", :mrna_start"
                  ", :mrna_end"
                  ", :mrna_length"
                  ", :exons_cds_count"
                  ", :exons_mrna_count"
                  ", :exons_length"
                  ", :start_codon"
                  ", :end_codon"
                  ", :maximum_by_introns"
                  ", :error_in_length"
                  ", :error_in_start_codon"
                  ", :error_in_end_codon"
                  ", :error_in_intron"
                  ", :error_in_coding_exon"
                  ", :error_main"
                  ")");
    query.bindValue(":id_genes", geneId);
    query.bindValue(":id_sequences", isoform->sequence.toStrongRef()->id);
    query.bindValue(":protein_xref", isoform->proteinXref);
    query.bindValue(":protein_name", isoform->proteinName);
    query.bindValue(":product", isoform->product);
    query.bindValue(":cds_start", UINT32_MAX == isoform->cdsStart ? 0 : isoform->cdsStart);
    query.bindValue(":cds_end", isoform->cdsEnd);
    query.bindValue(":mrna_start", UINT32_MAX == isoform->mrnaStart ? 0 : isoform->mrnaStart);
    query.bindValue(":mrna_end", isoform->mrnaEnd);
    query.bindValue(":mrna_length",
                    isoform->mrnaEnd == 0 || UINT32_MAX == isoform->mrnaStart
                    ? 0 : qint32(isoform->mrnaEnd) - qint32(isoform->mrnaStart) + 1);
    query.bindValue(":exons_cds_count", isoform->exonsCdsCount);
    query.bindValue(":exons_mrna_count", isoform->exonsMrnaCount);
    query.bindValue(":exons_length", isoform->exonsLength);
    query.bindValue(":start_codon", isoform->startCodon);
    query.bindValue(":end_codon", isoform->endCodon);
    query.bindValue(":maximum_by_introns", isoform->isMaximumByIntrons);

    query.bindValue(":error_in_length", isoform->errorInLength);
    query.bindValue(":error_in_start_codon", isoform->errorInStartCodon);
    query.bindValue(":error_in_end_codon", isoform->errorInEndCodon);
    query.bindValue(":error_in_intron", isoform->errorInIntron);
    query.bindValue(":error_in_coding_exon", isoform->errorInCodingExon);
    query.bindValue(":error_main", isoform->errorMain);


    if (!query.exec()) {
        qWarning() << query.lastError();
        qWarning() << query.lastError().text();
        qWarning() << query.lastQuery();
        return;
    }
    else {
        isoform->id = query.lastInsertId().toInt();
    }

    Q_FOREACH(CodingExonPtr exon, isoform->exons) {
        addCodingExon(exon);
    }

    Q_FOREACH(IntronPtr intron, isoform->introns) {
        addIntron(intron);
    }

    Q_FOREACH(CodingExonPtr exon, isoform->exons) {
        updateNeigbourIntronsIds(exon);
    }

}

void Database::addCodingExon(CodingExonPtr exon)
{
    const qint32 seqId = exon->isoform.toStrongRef()->gene.toStrongRef()->sequence.toStrongRef()->id;
    const qint32 geneId = exon->isoform.toStrongRef()->gene.toStrongRef()->id;
    const qint32 isoformId = exon->isoform.toStrongRef()->id;
    QSqlQuery query("", *_db);
    query.prepare("INSERT INTO coding_exons("
                  "id_isoforms"
                  ", id_genes"
                  ", id_sequences"
                  ", startt"
                  ", endd"
                  ", lengthh"
                  ", typee"
                  ", start_phase"
                  ", end_phase"
                  ", length_phase"
                  ", indexx"
                  ", rev_index"
                  ", start_codon"
                  ", end_codon"
                  ", error_in_pseudo_flag"
                  ", error_n_in_sequence"
                  ") VALUES("
                  ":id_isoforms"
                  ", :id_genes"
                  ", :id_sequences"
                  ", :startt"
                  ", :endd"
                  ", :lengthh"
                  ", :typee"
                  ", :start_phase"
                  ", :end_phase"
                  ", :length_phase"
                  ", :indexx"
                  ", :rev_index"
                  ", :start_codon"
                  ", :end_codon"
                  ", :error_in_pseudo_flag"
                  ", :error_n_in_sequence"
                  ")");
    query.bindValue(":id_isoforms", isoformId);
    query.bindValue(":id_genes", geneId);
    query.bindValue(":id_sequences", seqId);
    query.bindValue(":startt", exon->start);
    query.bindValue(":endd", exon->end);
    query.bindValue(":lengthh", exon->end - exon->start + 1);
    query.bindValue(":typee", qint16(exon->type));
    query.bindValue(":start_phase", exon->startPhase);
    query.bindValue(":end_phase", exon->endPhase);
    query.bindValue(":length_phase", exon->lengthPhase);
    query.bindValue(":indexx", exon->index);
    query.bindValue(":rev_index", exon->revIndex);
    query.bindValue(":start_codon", exon->startCodon);
    query.bindValue(":end_codon", exon->endCodon);
    query.bindValue(":error_in_pseudo_flag", exon->errorInPseudoFlag);
    query.bindValue(":error_n_in_sequence", exon->errorNInSequence);


    if (!query.exec()) {
        qWarning() << query.lastError();
        qWarning() << query.lastError().text();
        qWarning() << query.lastQuery();
        return;
    }
    else {
        exon->id = query.lastInsertId().toInt();
    }
}

void Database::addIntron(IntronPtr intron)
{
    const qint32 seqId = intron->isoform.toStrongRef()->gene.toStrongRef()->sequence.toStrongRef()->id;
    const qint32 geneId = intron->isoform.toStrongRef()->gene.toStrongRef()->id;
    const qint32 isoformId = intron->isoform.toStrongRef()->id;
    QSqlQuery query("", *_db);
    query.prepare("INSERT INTO introns("
                  "id_isoforms"
                  ", id_genes"
                  ", id_sequences"
                  ", prev_exon"
                  ", next_exon"
                  ", startt"
                  ", endd"
                  ", id_intron_types"
                  ", start_dinucleotide"
                  ", end_dinucleotide"
                  ", lengthh"
                  ", indexx"
                  ", rev_index"
                  ", length_phase"
                  ", phase"
                  ", error_start_dinucleotide"
                  ", error_end_dinucleotide"
                  ", error_main"
                  ", warning_n_in_sequence"
                  ") VALUES("
                  ":id_isoforms"
                  ", :id_genes"
                  ", :id_sequences"
                  ", :prev_exon"
                  ", :next_exon"
                  ", :startt"
                  ", :endd"
                  ", :id_intron_types"
                  ", :start_dinucleotide"
                  ", :end_dinucleotide"
                  ", :lengthh"
                  ", :indexx"
                  ", :rev_index"
                  ", :length_phase"
                  ", :phase"
                  ", :error_start_dinucleotide"
                  ", :error_end_dinucleotide"
                  ", :error_main"
                  ", :warning_n_in_sequence"
                  ")");
    query.bindValue(":id_isoforms", isoformId);
    query.bindValue(":id_genes", geneId);
    query.bindValue(":id_sequences", seqId);

    query.bindValue(":prev_exon", intron->prevExon.toStrongRef()->id);
    query.bindValue(":next_exon", intron->nextExon.toStrongRef()->id);
    query.bindValue(":startt", intron->start);
    query.bindValue(":endd", intron->end);
    query.bindValue(":id_intron_types", intron->intronTypeId);
    query.bindValue(":start_dinucleotide", intron->startDinucleotide);
    query.bindValue(":end_dinucleotide", intron->endDinucleotide);

    query.bindValue(":lengthh", qint32(intron->end) - qint32(intron->start) + 1);
    query.bindValue(":indexx", intron->index);
    query.bindValue(":rev_index", UINT32_MAX == intron->revIndex ? 0 : intron->revIndex);
    query.bindValue(":length_phase", intron->lengthPhase);
    query.bindValue(":phase", intron->phase);
    query.bindValue(":error_start_dinucleotide", intron->errorInStartDinucleotide);
    query.bindValue(":error_end_dinucleotide", intron->errorInEndDinucleotide);
    query.bindValue(":error_main", intron->errorMain);
    query.bindValue(":warning_n_in_sequence", intron->warningNInSequence);


    if (!query.exec()) {
        qWarning() << query.lastError();
        qWarning() << query.lastError().text();
        qWarning() << query.lastQuery();
        return;
    }
    else {
        intron->id = query.lastInsertId().toInt();
    }
}

void Database::updateNeigbourIntronsIds(CodingExonPtr exon)
{
    const qint32 exonId = exon->id;
    QSqlQuery query("", *_db);
    if (exon->prevIntron) {
        const qint32 prevId = exon->prevIntron.toStrongRef()->id;
        query.prepare("UPDATE coding_exons SET prev_intron=:prev_id WHERE id=:exon_id");
        query.bindValue(":prev_id", prevId);
        query.bindValue(":exon_id", exonId);
        if (!query.exec()) {
            qWarning() << query.lastError();
            qWarning() << query.lastError().text();
            qWarning() << query.lastQuery();
        }
    }
    if (exon->nextIntron) {
        const qint32 nextId = exon->nextIntron.toStrongRef()->id;
        query.prepare("UPDATE coding_exons SET next_intron=:next_id WHERE id=:exon_id");
        query.bindValue(":next_id", nextId);
        query.bindValue(":exon_id", exonId);
        if (!query.exec()) {
            qWarning() << query.lastError();
            qWarning() << query.lastError().text();
            qWarning() << query.lastQuery();
        }
    }
}

Database::~Database()
{
    if (_db->isOpen()) {
        _db->close();
    }
}


