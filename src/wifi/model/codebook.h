/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef CODEBOOK_H
#define CODEBOOK_H

#include "ns3/antenna-model.h"
#include "ns3/mac48-address.h"
#include "ns3/object.h"
#include "ns3/traced-value.h"

#include <map>
#include <vector>
#include <cmath>

namespace ns3 {

#define MAXIMUM_NUMBER_OF_RF_CHAINS     8
#define MAXIMUM_NUMBER_OF_ANTENNAS_11AY 8
#define MAXIMUM_NUMBER_OF_ANTENNAS      4
#define MAXIMUM_SECTORS_PER_ANTENNA     64
#define MAXIMUM_NUMBER_OF_SECTORS       128
#define AZIMUTH_CARDINALITY     361
#define ELEVATION_CARDINALITY   181

typedef uint8_t RFChainID;
typedef uint8_t AntennaID;
typedef uint8_t SectorID;
typedef uint8_t AWV_ID;
typedef double Directivity;

enum CurrentBeamformingPhase {
  BHI_PHASE = 0,
  SLS_PHASE = 1
};

enum SectorType {
  TX_SECTOR = 0,
  RX_SECTOR = 1,
  TX_RX_SECTOR = 2,
};

enum SectorUsage {
  BHI_SECTOR = 0,
  SLS_SECTOR = 1,
  BHI_SLS_SECTOR = 2
};

enum SectorSweepType {
  TransmitSectorSweep = 0,
  ReceiveSectorSweep = 1,
};

enum BeamRefinementType {
  RefineTransmitSector = 0,
  RefineReceiveSector = 1,
};

struct PatternConfig : public SimpleRefCount<PatternConfig> {
  virtual ~PatternConfig ();
};

struct AWV_Config : virtual public PatternConfig {
};

typedef std::vector<Ptr<AWV_Config> > AWV_LIST;
typedef AWV_LIST::const_iterator AWV_LIST_CI;
typedef AWV_LIST::iterator AWV_LIST_I;

struct SectorConfig : virtual public PatternConfig {
  SectorType sectorType;
  SectorUsage sectorUsage;
  AWV_LIST awvList;
};

typedef std::map<SectorID, Ptr<SectorConfig> > SectorList;
typedef SectorList::iterator SectorListI;
typedef SectorList::const_iterator SectorListCI;

struct Orientation {
  double x;
  double y;
  double z;
};

struct PhasedAntennaArrayConfig : public SimpleRefCount<PhasedAntennaArrayConfig> {
  double azimuthOrientationDegree;
  double elevationOrientationDegree;
  Orientation orientation;
  SectorList sectorList;
};

typedef std::vector<SectorID> SectorIDList;
typedef SectorIDList::iterator SectorIDListI;
typedef std::map<AntennaID, SectorIDList> Antenna2SectorList;
typedef Antenna2SectorList::iterator Antenna2SectorListI;
typedef Antenna2SectorList::const_iterator Antenna2SectorListCI;
typedef std::map<AntennaID, Ptr<PhasedAntennaArrayConfig> > AntennaArrayList;
typedef AntennaArrayList::iterator AntennaArrayListI;
typedef AntennaArrayList::const_iterator AntennaArrayListCI;
typedef std::map<Mac48Address, Antenna2SectorList> BeamformingSectorList;
typedef BeamformingSectorList::iterator BeamformingSectorListI;
typedef BeamformingSectorList::const_iterator BeamformingSectorListCI;

struct RFChain : public SimpleRefCount<RFChain> {
  AntennaArrayList antennaArrayList;
};

typedef std::map<RFChainID, Ptr<RFChain> > RFChainList;
typedef RFChainList::iterator RFChainListI;
typedef RFChainList::const_iterator RFChainListCI;

class Codebook : public Object
{
public:
  static TypeId GetTypeId (void);

  Codebook (void);
  virtual ~Codebook (void);

  virtual void LoadCodebook (std::string filename) = 0;
  std::string GetCodebookFileName (void) const;

  virtual double GetTxGainDbi (double angle) = 0;
  virtual double GetRxGainDbi (double angle) = 0;
  virtual double GetTxGainDbi (double azimuth, double elevation) = 0;
  virtual double GetRxGainDbi (double azimuth, double elevation) = 0;
  uint8_t GetTotalNumberOfTransmitSectors (void) const;
  uint8_t GetTotalNumberOfReceiveSectors (void) const;
  uint8_t GetTotalNumberOfSectors (void) const;
  virtual uint8_t GetNumberSectorsPerAntenna (AntennaID antennaID) const = 0;
  uint8_t GetTotalNumberOfAntennas (void) const;
  void SetBeaconingSectors (Antenna2SectorList sectors);
  void AppendBeaconingSector (AntennaID antennaID, SectorID sectorID);
  void RemoveBeaconingSector (AntennaID antennaID, SectorID sectorID);
  uint8_t GetNumberOfSectorsInBHI (void);
  void SetGlobalBeamformingSectorList (SectorSweepType type, Antenna2SectorList &sectors);
  void AppendBeamformingSector (Mac48Address address, SectorSweepType type, AntennaID antennaID, SectorID sectorID);
  uint8_t GetNumberOfSectorsForBeamforming (SectorSweepType type);
  void AppendBeamformingSector (SectorSweepType type, AntennaID antennaID, SectorID sectorID);
  void SetBeamformingSectorList (SectorSweepType type, Mac48Address address, Antenna2SectorList &sectorList);
  void RemoveBeamformingSector (AntennaID antennaID, SectorID sectorID);
  uint8_t GetNumberOfSectorsForBeamforming (Mac48Address address, SectorSweepType type);
  void CopyCodebook (const Ptr<Codebook> codebook);
  virtual void ChangeAntennaOrientation (AntennaID antennaID, double azimuthOrientation, double elevationOrientation);
  void AppendAWV (AntennaID antennaID, SectorID sectorID, Ptr<AWV_Config> awvConfig);

protected:
  friend class DmgWifiMac;
  friend class DmgApWifiMac;
  friend class DmgStaWifiMac;
  friend class DmgAdhocWifiMac;
  friend class DmgWifiPhy;
  friend class WifiPhy;
  friend class SpectrumDmgWifiPhy;
  friend class QdPropagationLossModel;

  virtual void DoDispose ();

  void SetActiveTxSectorID (SectorID sectorID, AntennaID antennaID);
  void SetActiveRxSectorID (SectorID sectorID, AntennaID antennaID);
  AntennaID GetActiveAntennaID (void) const;
  SectorID GetActiveTxSectorID (void) const;
  SectorID GetActiveRxSectorID (void) const;
  void SetReceivingInQuasiOmniMode (void);
  void SetReceivingInQuasiOmniMode (AntennaID antennaID);
  void StartReceivingInQuasiOmniMode (void);
  bool SwitchToNextQuasiPattern (void);
  void SetReceivingInDirectionalMode (void);
  void RandomizeBeacon (bool beaconRandomization);

  void InitializeCodebook (void);
  void StartBTIAccessPeriod (void);
  bool GetNextSectorInBTI (void);
  uint8_t GetNumberOfBIs (void) const;
  uint8_t GetRemaingSectorCount (void) const;
  void InitiateABFT (Mac48Address address);
  bool GetNextSectorInABFT (void);
  void StartSectorSweeping (Mac48Address address, SectorSweepType type, uint8_t peerAntennas);
  bool GetNextSector (bool &changeAntenna);
  bool GetReceivingMode (void) const;
  uint8_t GetTotalNumberOfElements (void) const;
  Orientation GetOrientation (uint8_t antennaId);
  void InitiateBRP (AntennaID antennaID, SectorID sectorID, BeamRefinementType type);
  void InitiateBeamTracking (AntennaID antennaID, SectorID sectorID, BeamRefinementType type);
  bool GetNextAWV (void);
  void UseLastTxSector (void);
  void UseCustomAWV (void);
  bool IsCustomAWVUsed (void) const;
  uint8_t GetNumberOfAWVs (AntennaID antennaID, SectorID sectorID) const;
  uint8_t GetActiveTxPatternID (void) const;
  uint8_t GetActiveRxPatternID (void) const;

private:
  uint8_t GetNumberOfSectors (Mac48Address address, BeamformingSectorList &list);
  uint8_t CountNumberOfSectors (Antenna2SectorList *sectorList);
  void AppendToSectorList (Antenna2SectorList &globalList, AntennaID antennaID, SectorID sectorID);
  void SetActiveTxSectorID (SectorID sectorID);
  void SetActiveRxSectorID (SectorID sectorID);
  void SetActiveAntennaID (AntennaID antennaID);

protected:
  std::string m_fileName;
  RFChainList m_rfChainList;
  AntennaArrayList m_antennaArrayList;

  Antenna2SectorList m_txBeamformingSectors;
  Antenna2SectorList m_rxBeamformingSectors;
  uint8_t m_totalTxSectors;
  uint8_t m_totalRxSectors;
  uint8_t m_totalSectors;
  uint8_t m_totalAntennas;
  BeamformingSectorList m_txCustomSectors;
  BeamformingSectorList m_rxCustomSectors;

  Antenna2SectorList m_bhiAntennasList;
  bool m_beaconRandomization;
  uint8_t m_btiSectorOffset;
  Antenna2SectorListI m_bhiAntennaI;

  CurrentBeamformingPhase m_currentBFPhase;
  SectorSweepType m_sectorSweepType;
  SectorID m_currentSectorIndex;
  uint8_t m_remainingSectors;
  Antenna2SectorListI m_beamformingAntenna;
  SectorIDList m_beamformingSectorList;
  Antenna2SectorList *m_currentSectorList;
  Mac48Address m_peerStation;

  TracedValue<AntennaID> m_antennaID;
  TracedValue<SectorID> m_txSectorID;
  TracedValue<SectorID> m_rxSectorID;
  Ptr<PhasedAntennaArrayConfig> m_antennaConfig;
  Ptr<PatternConfig> m_txPattern;
  Ptr<PatternConfig> m_rxPattern;

  bool m_quasiOmniMode;
  Antenna2SectorListI m_quasiAntennaIter;

  bool m_useAWV;
  AWV_LIST *m_currentAwvList;
  AWV_LIST_I m_currentAwvI;

};

} // namespace ns3

#endif /* CODEBOOK_H */
