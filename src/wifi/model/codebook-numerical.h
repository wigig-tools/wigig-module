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

typedef double* DirectivityTable;                     //!< Typedef for a vector of directivities in the azimuth plane.

/**
 * Interface for generating radiation pattern using measurement pattern.
 */
struct NumericalPatternConfig : virtual public PatternConfig {
protected:
  friend class CodebookNumerical;
  DirectivityTable directivity;

};

/**
 * Numerical AWV Configuration (Only directivity).
 */
struct Numerical_AWV_Config : virtual public AWV_Config, virtual public NumericalPatternConfig {
};

/**
 * Numerical Sector Configuration (Only directivity).
 */
struct NumericalSectorConfig : virtual public SectorConfig, virtual public NumericalPatternConfig {
};

struct NumericalAntennaConfig : public PhasedAntennaArrayConfig {
  /**
   * Get a pointer to the quasi-omni pattern associated with this array.
   * \return A pointer to the quasi-omni pattern.
   */
  Ptr<NumericalPatternConfig> GetQuasiOmniConfig (void) const;
};

/**
 * \brief Codebook for Sectors generated either by Phased Antenna Array Toolbox in Matlab or by loading measured
 * radiation patterns e.g. the TALON router radiation patterns for AP/STA operation modes.
 * The codebook always assumes that the provided radiation patterns correspond to antenna orientation set to zero degree.
 * This simplifies simulation as the user does not have to re-generate the codebook file in Matlab, each time the user changes
 * the antenna orientation. The ChangeAntennaOrientation function takes care of rotating the radiation pattern  tablebased on the
 * the provided orientation degree.
 */
class CodebookNumerical : public Codebook
{
public:
  static TypeId GetTypeId (void);

  CodebookNumerical (void);
  virtual ~CodebookNumerical (void);
  /**
   * Load code book from a text file.
   */
  void LoadCodebook (std::string filename);
  /**
   * Get transmit antenna gain dBi.
   * \param angle The angle towards the intended receiver.
   * \return Transmit antenna gain in dBi based on the steering angle.
   */
  double GetTxGainDbi (double angle);
  /**
   * Get transmit antenna gain in dBi.
   * \param angle The angle towards the intended transmitter.
   * \return Receive antenna gain in dBi based on the steering angle.
   */
  double GetRxGainDbi (double angle);
  /**
   * Get transmit antenna gain in dBi.
   * \param azimuth The azimuth angle towards the intedned receiver.
   * \param elevation The elevation angle towards the intedned receiver.
   * \return Transmit antenna gain in dBi based on the steering angle.
   */
  double GetTxGainDbi (double azimuth, double elevation);
  /**
   * Get receive antenna gain in dBi.
   * \param azimuth The azimuth angle towards the intedned receiver.
   * \param elevation The elevation angle towards the intedned receiver.
   * \return Receive antenna gain in dBi based on the steering angle.
   */
  double GetRxGainDbi (double azimuth, double elevation);
  /**
   * Get the total number of sectors for a specific phased antenna array.
   * \param antennaID The ID of the phased antenna array.
   * \return The number of sectors for the specified phased antenna array.
   */
  uint8_t GetNumberSectorsPerAntenna (AntennaID antennaID) const;
  /**
   * Change phased antenna array orientation.
   * \param antennaID The ID of the antenna array.
   * \param azimuthOrientation The new azimuth orientation of the antenna array in degrees.
   * \param elevationOrientation The new elevation orientation of the antenna array in degrees.
   */
  void ChangeAntennaOrientation (AntennaID antennaID, double azimuthOrientation, double elevationOrientation);

private:
  void DoDispose (void);
  /**
   * Get transmission gain in dBi based on the selected antenna and sector.
   * \param angle The azimuth angle towards the peer device.
   * \param sectorDirectivity Pointer to the directivity table of the selected sectors.
   * \return Transmit/Receive antenna gain in dBi based on the azimuth angle and the selected sector.
   */
  double GetGainDbi (double angle, DirectivityTable sectorDirectivity) const;
  /**
   * Set Codebook FileName.
   * \param fileName The name of the codebook file to load.
   */
  void SetCodebookFileName (std::string fileName);

};

} // namespace ns3

#endif /* CODEBOOK_NUMERICAL_H */
