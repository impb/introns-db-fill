#ifndef INIPARSER_H
#define INIPARSER_H

#include "structures.h"
#include "database.h"

#include <QMutex>
#include <QSettings>
#include <QString>

class IniParser
{
public:
    void setSourceFileName(const QString &fileName);

    void updateOrganism(OrganismPtr organism);
    void updateOrganismTaxonomy(OrganismPtr organism);

    void setDatabase(QSharedPointer<Database> database);

    QVariant value(const QString &tableName, const QString &fieldName) const;
    ~IniParser();
private:
    QSettings * _impl = nullptr;
    mutable QMutex _mutex;
    QSharedPointer<Database> _db;
};

#endif // INIPARSER_H
