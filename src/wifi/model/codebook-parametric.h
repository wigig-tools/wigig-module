/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef CODEBOOK_PARAMETRIC_H
#define CODEBOOK_PARAMETRIC_H

#include "ns3/object.h"
#include "codebook.h"
#include <complex>
#include <iostream>

namespace ns3 {

typedef std::complex<double> Complex;
typedef std::vector<Complex> WeightsVector;
typedef WeightsVector::const_iterator WeightsVectorCI;
typedef Complex** ArrayPattern;
typedef Directivity** DirectivityMatrix;
typedef Complex*** SteeringVector;

struct ParametricPatternConfig : virtual public PatternConfig {
public:
  ArrayPattern GetArrayPattern (void) const;

public:
  WeightsVector elementsWeights;

protected:
  friend class CodebookParametric;
  ArrayPattern arrayPattern;
  DirectivityMatrix directivity;

};

struct Parametric_AWV_Config : virtual public AWV_Config, virtual public ParametricPatternConfig {
};

struct ParametricSectorConfig : virtual public SectorConfig, virtual public ParametricPatternConfig {
};

struct ParametricAntennaConfig : public PhasedAntennaArrayConfig {
public:
  uint16_t elements;
  SteeringVector steeringVector;

  DirectivityMatrix singleElementDirectivity;
  WeightsVector quasiOmniWeights;
  uint8_t amplitudeQuantizationBits;
  uint8_t phaseQuantizationBits;

  double CalculateDirectivity (double azimuth, double elevation, WeightsVector &weightsVector);
  double CalculateDirectivityForDirection (double azimuth, double elevation);
  void CalculateDirectivity (WeightsVector *weights, ArrayPattern &arrayPattern, DirectivityMatrix &directivity);
  ArrayPattern GetQuasiOmniArrayPattern (void) const;

private:
  friend class CodebookParametric;
  ArrayPattern quasiOmniArrayPattern;
  DirectivityMatrix quasiOmniDirectivity;
  double phaseQuantizationStepSize;

};

class CodebookParametric : public Codebook
{
public:
  static TypeId GetTypeId (void);

  CodebookParametric (void);
  virtual ~CodebookParametric (void);

  void LoadCodebook (std::string filename);
  double GetTxGainDbi (double angle);
  double GetRxGainDbi (double angle);
  double GetTxGainDbi (double azimuth, double elevation);
  double GetRxGainDbi (double azimuth, double elevation);
  uint8_t GetNumberSectorsPerAntenna (AntennaID antennaID) const;
  void UpdateSectorWeights (AntennaID antennaID, SectorID sectorID, WeightsVector &weightsVector);
  void UpdateQuasiOmniWeights (AntennaID antennaID, WeightsVector &weightsVector);
  void AppendSector (AntennaID antennaID, SectorID sectorID, SectorUsage sectorUsage,
                     SectorType sectorType, WeightsVector &weightsVector);
  void PrintCodebookContent (bool printAWVs);
  void PrintAWVs (AntennaID antennaID, SectorID sectorID);
  void ChangeAntennaOrientation (AntennaID antennaID, double azimuthOrientation, double elevationOrientation);
  void AppendBeamRefinementAwv (AntennaID antennaID, SectorID sectorID, WeightsVector &weightsVector);
  void AppendBeamRefinementAwv (AntennaID antennaID, SectorID sectorID,
                                double steeringAngleAzimuth, double steeringAngleElevation);
  uint16_t GetNumberOfElements (AntennaID antennaID) const;
  ArrayPattern GetTxAntennaArrayPattern (void);
  ArrayPattern GetRxAntennaArrayPattern (void);

private:
  void DoDispose (void);

  void PrintDirectivity (DirectivityMatrix directivity) const;
  double GetGainDbi (double azimuth, double elevation, DirectivityMatrix directivity) const;
  void SetCodebookFileName (std::string fileName);
  WeightsVector ReadAntennaWeightsVector (std::ifstream &file, double elements);

};

} // namespace ns3

#endif /* CODEBOOK_PARAMETRIC_H */
