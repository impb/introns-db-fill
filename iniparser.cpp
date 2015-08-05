#include "iniparser.h"

#include <QFile>
#include <QMutexLocker>



void IniParser::setSourceFileName(const QString &fileName)
{
    QMutexLocker lock(&_mutex);
    if (QFile(fileName).exists(fileName)) {
        _impl = new QSettings(fileName, QSettings::IniFormat);
    }
}

void IniParser::updateOrganism(OrganismPtr organism)
{
    QMutexLocker lock(&organism->mutex);
    const QVariant name = value("organisms", "name");
    const QVariant ref_seq_assembly_id = value("organisms", "ref_seq_assembly_id");
    const QVariant real_chromosome_count = value("organisms", "real_chromosome_count");
    const QVariant real_mitochondria = value("organisms", "real_mitochondria");
    const QVariant annotation_release = value("organisms", "annotation_release");
    const QVariant annotation_date = value("organisms", "annotation_date");

    if (name.isValid()) {
        organism->name = name.toString();
    }
    if (ref_seq_assembly_id.isValid()) {
        organism->refSeqAssemblyId = ref_seq_assembly_id.toString();
    }
    if (real_chromosome_count.isValid()) {
        organism->realChromosomeCount = real_chromosome_count.toUInt();
    }
    if (real_mitochondria.isValid()) {
        organism->realMitochondria = bool(real_mitochondria.toUInt());
    }
    if (annotation_release.isValid()) {
        organism->annotationRelease = annotation_release.toString();
    }
    if (annotation_date.isValid()) {
        static QRegExp rxDate("(\\d+)\\s+(\\S+)\\s+(\\d\\d\\d\\d)");
        if (-1 != rxDate.indexIn(annotation_date.toString())) {
            int day = rxDate.cap(1).toInt();
            int year = rxDate.cap(3).toInt();
            const QString monthString = rxDate.cap(2).toLower().left(3);
            static const QStringList Months = QStringList()
                    << "jan" << "feb" << "mar"
                    << "apr" << "may" << "jun"
                    << "jul" << "aug" << "sep"
                    << "oct" << "nov" << "dec";
            int month = 1 + Months.indexOf(monthString);
            const bool dayValid = 1 <= day && day <= 31;
            const bool monthValid = 1 <= month && month <= 12;
            const bool yearValid = 1970 <= year && year <= 2039;
            if (dayValid && monthValid && yearValid) {
                organism->annotationDate.setDate(year, month, day);
            }
        }
    }

}

void IniParser::updateOrganismTaxonomy(OrganismPtr organism)
{
    const QVariant king_name = value("tax_kingdoms", "name");
    const QVariant group1_name = value("tax_groups1", "name");
    const QVariant group1_type = value("tax_groups1", "typee");
    const QVariant group2_name = value("tax_groups2", "name");
    const QVariant group2_type = value("tax_groups2", "typee");

    QMutexLocker lock(&organism->mutex);

    if (king_name.isValid()) {
        organism->kingdom = _db->findOrCreateTaxKingdom(king_name.toString());
    }
    if (group1_name.isValid() && group1_type.isValid()) {
        organism->taxGroup1 = _db->findOrCreateTaxGroup1(
                    group1_name.toString(), group1_type.toString(),
                    organism->kingdom
                    );
    }
    if (group2_name.isValid() && group2_type.isValid()) {
        organism->taxGroup2 = _db->findOrCreateTaxGroup2(
                    group2_name.toString(), group2_type.toString(),
                    organism->taxGroup1
                    );
    }
    if (organism->kingdom && organism->taxGroup1) {
        organism->taxGroup1.toStrongRef()->kingdomPtr =
                organism->kingdom;
    }
    if (organism->taxGroup1 && organism->taxGroup2) {
        organism->taxGroup2.toStrongRef()->taxGroup1Ptr =
                organism->taxGroup1;
    }
    if (organism->kingdom && organism->taxGroup2) {
        organism->taxGroup2.toStrongRef()->kingdomPtr =
                organism->kingdom;
    }
}

void IniParser::setDatabase(QSharedPointer<Database> database)
{
    _db = database;
}


QVariant IniParser::value(const QString &tableName, const QString &fieldName) const
{
    QMutexLocker lock(&_mutex);
    if (! _impl) {
        return QVariant::Invalid;
    }
    else {
        _impl->beginGroup(tableName);
        const QVariant result = _impl->value(fieldName);
        _impl->endGroup();
        return result;
    }
}

IniParser::~IniParser()
{
    if (_impl)
        delete _impl;
}
