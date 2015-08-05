#ifndef DATABASE_H
#define DATABASE_H

#include "structures.h"

#include <QDir>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QPair>
#include <QSqlDatabase>
#include <QSharedPointer>

class Database {
public:
  static QSharedPointer<Database> open(const QString &host,
                        const QString &userName, const QString &password,
                        const QString &dbName, const QString &sequencesStoreDir);

  OrganismPtr findOrCreateOrganism(const QString & name);
  void updateOrganism(OrganismPtr organism);

  TaxKingdomPtr findOrCreateTaxKingdom(const QString & name);
  TaxGroup1Ptr findOrCreateTaxGroup1(const QString & name, const QString & type, TaxKingdomPtr kingdom);
  TaxGroup2Ptr findOrCreateTaxGroup2(const QString & name, const QString & type, TaxGroup1Ptr group1);

  void dropSequenceIfExists(SequencePtr sequence);

  void addSequence(SequencePtr sequence);
  void storeOrigin(SequencePtr sequence);

  void addGene(GenePtr gene);
  void addIsoform(IsoformPtr isoform);
  void addCodingExon(CodingExonPtr exon);
  void addIntron(IntronPtr intron);
  void updateNeigbourIntronsIds(CodingExonPtr exon);

  ~Database();

private:

  static QMutex _organismsMutex;
  static QMap<QString, OrganismPtr> _organisms;

  static QMutex _taxMutex;
  static QMap<QString, TaxKingdomPtr> _kingdoms;
  static QMap<QPair<QString,QString>, TaxGroup1Ptr> _taxGroups1;
  static QMap<QPair<QString,QString>, TaxGroup2Ptr> _taxGroups2;


  QDir _sequencesStoreDir;
  QSqlDatabase _db;

};

#endif // DATABASE_H