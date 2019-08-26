/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef CODEBOOK_NUMERICAL_H
#define CODEBOOK_NUMERICAL_H

#include "ns3/object.h"
#include "map"
#include "vector"
#include "codebook.h"

namespace ns3 {

typedef double* DirectivityTable;

struct NumericalPatternConfig : virtual public PatternConfig {
protected:
  friend class CodebookNumerical;
  DirectivityTable directivity;

};

struct Numerical_AWV_Config : virtual public AWV_Config, virtual public NumericalPatternConfig {
};

struct NumericalSectorConfig : virtual public SectorConfig, virtual public NumericalPatternConfig {
};

struct NumericalAntennaConfig : public PhasedAntennaArrayConfig {
  DirectivityTable quasiOmniDirectivity;
};

class CodebookNumerical : public Codebook
{
public:
  static TypeId GetTypeId (void);

  CodebookNumerical (void);
  virtual ~CodebookNumerical (void);

  void LoadCodebook (std::string filename);
  double GetTxGainDbi (double angle);
  double GetRxGainDbi (double angle);
  double GetTxGainDbi (double azimuth, double elevation);
  double GetRxGainDbi (double azimuth, double elevation);
  uint8_t GetNumberSectorsPerAntenna (AntennaID antennaID) const;
  void ChangeAntennaOrientation (AntennaID antennaID, double azimuthOrientation, double elevationOrientation);

private:
  void DoDispose (void);
  double GetGainDbi (double angle, DirectivityTable sectorDirectivity) const;
  void SetCodebookFileName (std::string fileName);

};

} // namespace ns3

#endif /* CODEBOOK_NUMERICAL_H */
