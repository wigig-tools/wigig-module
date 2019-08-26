/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef CODEBOOK_ANALYTICAL_H
#define CODEBOOK_ANALYTICAL_H

#include "ns3/object.h"
#include "map"
#include "vector"
#include "codebook.h"

namespace ns3 {

struct AnalyticalPatternConfig : virtual public PatternConfig {
  double steeringAngle;
  double mainLobeBeamWidth;

protected:
  friend class CodebookAnalytical;

  double halfPowerBeamWidth;
  double maxGain;
  double sideLobeGain;
};

struct Analytical_AWV_Config : virtual public AWV_Config, virtual public AnalyticalPatternConfig {
};

struct AnalyticalSectorConfig : virtual public SectorConfig, virtual public AnalyticalPatternConfig {
};

struct AnalyticalAntennaConfig : public PhasedAntennaArrayConfig {
  double quasiOmniGain;
};

typedef enum {
  SIMPLE_CODEBOOK = 0,
  CUSTOM_CODEBOOK = 1,
  EMPTY_CODEBOOK = 2,
} AnalyticalCodebookType;

class CodebookAnalytical : public Codebook
{
public:
  static TypeId GetTypeId (void);

  CodebookAnalytical (void);
  virtual ~CodebookAnalytical (void);

  void AppendAntenna (AntennaID antennaID, double orientation, double quasiOmniGain);
  void AppendSector (AntennaID antennaID, SectorID sectorID, Ptr<AnalyticalSectorConfig> sectorConfig);
  void AppendSector (AntennaID antennaID, SectorID sectorID,
                     double steeringAngle, double mainLobeBeamWidth,
                     SectorType sectorType, SectorUsage sectorUsage);
  double GetTxGainDbi (double angle);
  double GetRxGainDbi (double angle);
  double GetTxGainDbi (double azimuth, double elevation);
  double GetRxGainDbi (double azimuth, double elevation);
  void SetCodeBookType (AnalyticalCodebookType type);
  uint8_t GetNumberSectorsPerAntenna (AntennaID antennaID) const;
  void AppendListOfAWV (AntennaID antennaID, SectorID sectorID, uint8_t numberOfAWVs);

protected:
  virtual void LoadCodebook (std::string filename);

private:
  void CreateEquallySizedSectors (uint8_t numberOfAntennas, uint8_t numberOfSectors, uint8_t numberOfAwvs);
  double GetGainDbi (double angle, Ptr<AnalyticalPatternConfig> patternConfig);
  double GetHalfPowerBeamWidth (double mainLobeWidth) const;
  double GetMaxGainDbi (double halfPowerBeamWidth) const;
  double GetSideLobeGain (double halfPowerBeamWidth) const;
  void SetCodebookFileName (std::string fileName);
  void AddSectorToBeamformingLists (AntennaID antennaID, SectorID sectorID, Ptr<SectorConfig> sectorConfig);
  void SetPatternConfiguration (Ptr<AnalyticalPatternConfig> patternConfig);

private:
  uint8_t m_antennas;
  uint8_t m_sectors;
  uint8_t m_awvs;
  double m_overlapPercentage;
  AnalyticalCodebookType m_analyticalCodebookType;

};

} // namespace ns3

#endif /* CODEBOOK_ANALYTICAL_H */
