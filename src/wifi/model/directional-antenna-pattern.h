/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef DIRECTIONAL_ANTENNA_PATTERN_H
#define DIRECTIONAL_ANTENNA_PATTERN_H

#include "directional-antenna.h"
#include "map"

namespace ns3 {

/**
 * \brief Directional Antenna functionality for 60 GHz based on IEEE 802.15.3c Antenna Model.
 */
class DirectionalAntennaPattern : public DirectionalAntenna
{
public:
  static TypeId GetTypeId (void);
  DirectionalAntennaPattern (void);
  virtual ~DirectionalAntennaPattern (void);

  /* Virtual Functions */
  double GetTxGainDbi (double angle) const;
  double GetRxGainDbi (double angle) const;

  virtual bool IsPeerNodeInTheCurrentSector (double angle) const;

protected:
  double GetGainDbi (double angle, uint8_t sectorId, uint8_t antennaId) const;
  void SetAntennaRadiationPattern (std::string filename);
  void LoadPattern (void);

private:
  std::string m_fileName;                   //!< The name of the antenna radiation pattern file.
  typedef uint8_t PatternIndex;             //!< Typedef for the index of the antenna radiation pattern.
  typedef std::vector<double> GainVector;   //!< The gain vector which correponds to antenna radiation pattern.
  typedef std::map<PatternIndex, GainVector> AntennaPatterns;
  typedef AntennaPatterns::const_iterator AntennaPatternsCI;
  AntennaPatterns m_antennaPatterns;


};

} // namespace ns3

#endif /* DIRECTIONAL_ANTENNA_PATTERN_H */
