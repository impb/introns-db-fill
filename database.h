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
                        const QString &dbName, const QString &sequencesStoreDir,
                                       const QString &translationsStoreDir);

  OrganismPtr findOrCreateOrganism(const QString & name);
  ChromosomePtr findOrCreateChromosome(const QString &name, OrganismPtr organism);

  void updateOrganism(OrganismPtr organism);
  void updateChromosome(ChromosomePtr chromosome);

  TaxKingdomPtr findOrCreateTaxKingdom(const QString & name);
  TaxGroup1Ptr findOrCreateTaxGroup1(const QString & name, const QString & type, TaxKingdomPtr kingdom);
  TaxGroup2Ptr findOrCreateTaxGroup2(const QString & name, const QString & type, TaxGroup1Ptr group1);

  void dropSequenceIfExists(SequencePtr sequence);

  void addSequence(SequencePtr sequence);
  void addOrphanedCDS(const QString & fileName, const quint32 lineStart, const quint32 lineEnd,
                      const QString &refSeqId,
                      const QString &dbXref,
                      const QString &product);
  void storeOrigin(SequencePtr sequence);
  void storeTranslation(IsoformPtr isoform);
  static QString format60(const QString &s);

  void addGene(GenePtr gene);
  void addIsoform(IsoformPtr isoform);
  void addCodingExon(ExonPtr exon);
  void addIntron(IntronPtr intron);
  void updateNeigbourIntronsIds(ExonPtr exon);

  ~Database();

private:

  static QMutex _connectionsMutex;
  static QMap<Qt::HANDLE, QSqlDatabase> _connections;

  static QMutex _organismsMutex;
  static QMap<QString, OrganismPtr> _organisms;

  static QMutex _chromosomesMutex;
  static QMap<QPair<OrganismPtr,QString>, ChromosomePtr> _chromosomes;

  static QMutex _taxMutex;
  static QMap<QString, TaxKingdomPtr> _kingdoms;
  static QMap<QPair<QString,QString>, TaxGroup1Ptr> _taxGroups1;
  static QMap<QPair<QString,QString>, TaxGroup2Ptr> _taxGroups2;


  QDir _sequencesStoreDir;
  QDir _translationsStoreDir;
  QSqlDatabase * _db = nullptr;

};

#endif // DATABASE_H
