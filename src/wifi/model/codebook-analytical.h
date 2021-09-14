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

/**
 * Interface for generating radiation pattern using analytical expression.
 */
struct AnalyticalPatternConfig : virtual public PatternConfig {
  double steeringAngle;                       //!< Steering angle of the sector with respect to the X axis.
  double mainLobeBeamWidth;                   //!< Main-lobe beam width of the sector.

protected:
  friend class CodebookAnalytical;

  double halfPowerBeamWidth;                  //!< Half power beam width (-3 dB) of the sector.
  double maxGain;                             //!< Maximum gain of the sector in dBi.
  double sideLobeGain;                        //!< Side lobe gain of the sector in dBi.
};

/**
 * Analytical AWV configuration.
 */
struct Analytical_AWV_Config : virtual public AWV_Config, virtual public AnalyticalPatternConfig {
};

/**
 * Analytical Sector configuration.
 */
struct AnalyticalSectorConfig : virtual public SectorConfig, virtual public AnalyticalPatternConfig {
};

struct AnalyticalAntennaConfig : public PhasedAntennaArrayConfig {
  double quasiOmniGain;         //!< Quasi omni gain of this antenna.
};

typedef enum {
  SIMPLE_CODEBOOK = 0,
  CUSTOM_CODEBOOK = 1,
  EMPTY_CODEBOOK = 2,
} AnalyticalCodebookType;

/**
 * \brief Codebook for Analytical Representation of Phased Antenna Array Patterns.
 */
class CodebookAnalytical : public Codebook
{
public:
  static TypeId GetTypeId (void);

  CodebookAnalytical (void);
  virtual ~CodebookAnalytical (void);

  /**
   * Append RF Chain.
   * \param rfchainID The ID of the RF Chain.
   */
  void AppendRFChain (RFChainID rfchainID);
  /**
   * Append new phased antenna array.
   * \param antennaID The ID of the phased antenna array.
   * \param orientation The orientation of the antenna array in radians.
   * \param quasiOmniGain The gain of the quasi-omni mode in dBi.
   */
  void AppendAntenna (RFChainID rfchainID, AntennaID antennaID, double orientation, double quasiOmniGain);
  /**
   * Add a new virtual sector to the codebook.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the new sector wthin the antenna.
   * \param sectorConfig analytical sector configuration with all the angles in radians.
   */
  void AppendSector (AntennaID antennaID, SectorID sectorID, Ptr<AnalyticalSectorConfig> sectorConfig);
  /**
   * Add new sector to the codebook.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the new sector wthin the antenna.
   * \param steeringAngle The steering angle of the sector (the center) in degrees.
   * \param mainLobeBeamWidth The main-lobe beamwidth in degrees.
   * \param sectorType The type of the sector.
   * \param sectorUsage The usage of the sector.
   */
  void AppendSector (AntennaID antennaID, SectorID sectorID,
                     double steeringAngle, double mainLobeBeamWidth,
                     SectorType sectorType, SectorUsage sectorUsage);
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
   * Set the type of the codebook to use (Simple or Custom).
   * \param type the type of the codebook to use.
   */
  void SetCodeBookType (AnalyticalCodebookType type);
  /**
   * Get the total number of sectors for a specific phased antenna array.
   * \param antennaID The ID of the phased antenna array.
   * \return The number of sectors for the specified phased antenna array.
   */
  uint8_t GetNumberSectorsPerAntenna (AntennaID antennaID) const;
  /**
   * Append list of AWVs to a specific sector by dividing the beamwidth of the sector into equally size
   * number of AWVs.
   * \param antennaID The ID of the antenna array.
   * \param sectorID The ID of the sector.
   * \param numberOfAWVs The number of AWVs.
   */
  void AppendListOfAWV (AntennaID antennaID, SectorID sectorID, uint8_t numberOfAWVs);

protected:
  /**
   * Load code book from a text file.
   */
  virtual void LoadCodebook (std::string filename);

private:
  void DoInitialize (void);

  /**
   * Create a certain number of phased antenna arrays with certain number of sectors.
   * We divide the azimuth plane into equally sized sectors.
   * \param numberOfAntennas The total number of antenna arrays.
   * \param numberOfSectors The total number of sectors per antenna array.
   * \param numberOfAwvs The number of custom AWVs per sector.
   */
  void CreateEquallySizedSectors (uint8_t numberOfAntennas, uint8_t numberOfSectors, uint8_t numberOfAwvs);
  /**
   * Get transmission gain in dBi based on the selected antenna and sector.
   * \param angle The azimuth angle towards the peer device.
   * \param antennaID The ID of the antenna.
   */
  double GetGainDbi (double angle, Ptr<AnalyticalPatternConfig> patternConfig);
  /**
   * Get half power beam width for specific main lobe Width.
   * \param mainLobeWidth The main lobe width of the sector.
   * \return The half power beam width in radians.
   */
  double GetHalfPowerBeamWidth (double mainLobeWidth) const;
  /**
   * Get maximum gain in dBi for specific half power beam width.
   * \param halfPowerBeamWidth The half power beam width of the sector.
   * \return The maximum gain in dBi for specific Thetha -3dB.
   */
  double GetMaxGainDbi (double halfPowerBeamWidth) const;
  /**
   * Get side lobe gain in dBi for specific main lobe Width.
   * \param halfPowerBeamWidth The half power beam width of the sector.
   * \return The side lobe gain in dBi for specific Main Lobe Width.
   */
  double GetSideLobeGain (double halfPowerBeamWidth) const;
  /**
   * Set Codebook FileName.
   * \param fileName The name of the codebook file to load.
   */
  void SetCodebookFileName (std::string fileName);
  /**
   * Get transmission gain in dBi based on the selected antenna and sector.
   * \param antennaID The ID of the antenna.
   * \param sectorID The ID of the sector.
   * \param sectorConfig The configuration structure of the sector.
   */
  void AddSectorToBeamformingLists (AntennaID antennaID, SectorID sectorID, Ptr<SectorConfig> sectorConfig);
  /**
   * Set analytical sector/awv pattern configuration including HPBW, Side Lobe Gain, and Max Gain.
   * \param patternConfig The configuration structure of the sector/awv.
   */
  void SetPatternConfiguration (Ptr<AnalyticalPatternConfig> patternConfig);

private:
  uint8_t m_antennas;           //!< The number of antenna arrays for the simple analytical codebook.
  uint8_t m_sectors;            //!< The number of sectors per antenna for the simple analytical codebook.
  uint8_t m_awvs;               //!< The number of AWVs per virtual sector.
  double m_overlapPercentage;   //!< The percentage of overlap between AWVs of the same sector.

};

} // namespace ns3

#endif /* CODEBOOK_ANALYTICAL_H */
