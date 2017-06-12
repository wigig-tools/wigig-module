/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef DIRECTIONAL_ANTENNA_H
#define DIRECTIONAL_ANTENNA_H

#include "ns3/object.h"
#include <stdlib.h>
#include <cmath>

namespace ns3 {

/**
 * \brief Directional Antenna Model for Millimeterwave Communications.
 */
class DirectionalAntenna : public Object {
public:
  static TypeId GetTypeId (void);
  /**
   * Set number of sectors supported by the station.
   * \param sectors Number of sectors.
   */
  void SetNumberOfSectors (uint8_t sectors);
  /**
   * Set number of antenna arrays supported by the station.
   * \param antennas Number of antennas.
   */
  void SetNumberOfAntennas (uint8_t antennas);
  /**
   * Get number of sectors in each antenna array.
   * \return
   */
  uint8_t GetNumberOfSectors (void) const;
  /**
   * Get number of antenna arrays.
   * \return
   */
  uint8_t GetNumberOfAntennas (void) const;

  /**
   * \param sector The ID of the current sector in the Tx antenna array.
   */
  void SetCurrentTxSectorID (uint8_t sectorId);

  /**
   * Set current transmit antenna.
   * \param antenna The ID of the current Tx antenna array.
   */
  void SetCurrentTxAntennaID (uint8_t antennaId);
  /**
   * \param sector The ID of the current sector in the Rx antenna array.
   */
  void SetCurrentRxSectorID (uint8_t sectorId);
  /**
   * Set current receive antenna array ID.
   * \param antenna The ID of the current Rx antenna array.
   */
  void SetCurrentRxAntennaID (uint8_t antennaId);

  void SetInitialSectorAngleOffset (double offset);

  void SetBoresight (double awv);
  /**
   * Get the ID of the next Tx sector.
   * \return the ID of the next Tx sector.
   */
  uint8_t GetNextTxSectorID (void) const;
  /**
   * Get the ID of the next Rx sector.
   * \return the ID of the next Rx sector.
   */
  uint8_t GetNextRxSectorID (void) const;

  /**
   * Get the ID of the current Tx sector in the antenna array.
   * \return
   */
  uint8_t GetCurrentTxSectorID (void) const;
  /**
   * Get the ID of the current Tx antenna array.
   * \return
   */
  uint8_t GetCurrentTxAntennaID (void) const;
  /**
   * Get Current Rx Sector ID
   * \return
   */
  uint8_t GetCurrentRxSectorID (void) const;
  /**
   * Get Current Rx Antenna ID
   * \return
   */
  uint8_t GetCurrentRxAntennaID (void) const;
  /**
   * Return the aperature that a single antenna can cover.
   * \return the aperature of a single antenna
   */
  double GetAntennaAperature (void) const;
  /**
   * Return the mainlobe width of a single sector.
   * \return the mainlobe width of a single sector
   */
  double GetMainLobeWidth (void) const;
  /**
   * Se receive antenna pattern to be Omni.
   */
  void SetInOmniReceivingMode (void);
  /**
   * Se receive antenna pattern to be directional.
   */
  void SetInDirectionalReceivingMode (void);
  /**
   * Obtain antenna gain at the specified angle.
   * \param angle The angle between the transmitter and the receiver.
   * \return the antenna gain at the specified angle.
   */
  virtual double GetTxGainDbi (double angle) const = 0;
  /**
   * Obtain antenna gain at the specified angle.
   * \param angle The angle between the transmitter and the receiver.
   * \return the antenna gain at the specified angle.
   */
  virtual double GetRxGainDbi (double angle) const = 0;

  virtual bool IsPeerNodeInTheCurrentSector (double angle) const = 0;

protected:
  /**
   * Obtain antenna gain at the specified angle.
   * \param angle The angle between the transmitter and the receiver.
   * \return the antenna gain at the specified angle.
   */
  virtual double GetGainDbi (double angle, uint8_t sectorId, uint8_t antennaId) const = 0;

  double  m_antennaAperature;         /* Main Lobe Function (First Zero). */
  double  m_mainLobeWidth;            /* Main Lobe Function (First Zero). */

  uint8_t m_txSectorId;               /* Current Tx Sector ID (Index). */
  uint8_t m_txAntennaId;              /* Current Tx Antenna ID (Index). */
  uint8_t m_rxSectorId;               /* Current Tx Sector ID (Index). */
  uint8_t m_rxAntennaId;              /* Current Tx Antenna ID (Index). */

  bool    m_omniAntenna;              /* Is the antenna behaves as Omni Antenna */
  uint8_t m_antennas;                 /* Number of antennas. */
  uint8_t m_sectors;                  /* Number of sectors per antenna. */

};

} // namespace ns3

#endif /* DIRECTIONAL_ANTENNA_H */
