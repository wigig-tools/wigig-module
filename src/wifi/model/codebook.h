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

#include "rf-chain.h"
#include "wigig-data-types.h"

#include <cmath>
#include <map>
#include <vector>

namespace ns3 {

#define MAXIMUM_NUMBER_OF_RF_CHAINS     8       //!< The maximum number of RF Chains for 802.11ay.
#define MAXIMUM_NUMBER_OF_ANTENNAS_11AY 8       //!< The maximum number of antennas is limited to 8 in 802.11ay.
#define MAXIMUM_NUMBER_OF_ANTENNAS      4       //!< The maximum number of antennas is limited to 4 in 802.11ad.
#define MAXIMUM_SECTORS_PER_ANTENNA     64      //!< The maximum number of sectors per antenna is limited to 64 sectors.
#define MAXIMUM_NUMBER_OF_SECTORS       128     //!< The maximum total number of sectors is limited to 1028 sectors.
#define AZIMUTH_CARDINALITY     361             //!< Number of the azimuth angles (1 Degree granularity from 0 -> 360).
#define ELEVATION_CARDINALITY   181             //!< Number of the elevation angles (1 Degree granularity from -90 -> 90).

typedef std::map<Mac48Address, Antenna2SectorList> BeamformingSectorList;     //!< Typedef for beamforming sector list (Custom sector set for specific station).
typedef BeamformingSectorList::iterator BeamformingSectorListI;               //!< Typedef for beafmorming sector Iterator.
typedef BeamformingSectorList::const_iterator BeamformingSectorListCI;        //!< Typedef for beafmorming sector Iterator.

enum CurrentBeamformingPhase {
  BHI_PHASE = 0,
  SLS_PHASE = 1
}; //!< Current beamforming Phase in the Beacon Interval.

enum CommunicationMode {
  SISO_MODE = 0,
  MIMO_MODE = 1
};

class WifiNetDevice;

/**
 * \brief Codebook for defining phased antenna arrays configurations in IEEE 802.11ad/ay devices.
 */
class Codebook : public Object
{
public:
  static TypeId GetTypeId (void);

  Codebook (void);
  virtual ~Codebook (void);

  /**
   * Load Codebook from a text file.
   */
  virtual void LoadCodebook (std::string filename) = 0;
  /**
   * Get Codebook FileName
   * \return The current loaded codebook.
   */
  std::string GetCodebookFileName (void) const;

  /**
   * Get transmit antenna array gain in dBi.
   * \param angle The angle towards the intended receiver.
   * \return Transmit antenna array gain in dBi based on the steering angle.
   */
  virtual double GetTxGainDbi (double angle) = 0;
  /**
   * Get receive antenna array array gain in dBi.
   * \param angle The angle towards the intended transmitter.
   * \return Receive antenna gain in dBi based on the steering angle.
   */
  virtual double GetRxGainDbi (double angle) = 0;
  /**
   * Get transmit antenna array gain in dBi.
   * \param azimuth The azimuth angle towards the intedned receiver.
   * \param elevation The elevation angle towards the intedned receiver.
   * \return Transmit antenna gain in dBi based on the steering angle.
   */
  virtual double GetTxGainDbi (double azimuth, double elevation) = 0;
  /**
   * Get receive antenna array gain in dBi.
   * \param azimuth The azimuth angle towards the intedned receiver.
   * \param elevation The elevation angle towards the intedned receiver.
   * \return Receive antenna gain in dBi based on the steering angle.
   */
  virtual double GetRxGainDbi (double azimuth, double elevation) = 0;
  /**
   * Get the total number of transmit sectors in the codebook.
   * \return The total number of transmit sectors in the codebook.
   */
  uint8_t GetTotalNumberOfTransmitSectors (void) const;
  /**
   * Get the total number of receive sectors in the codebook.
   * \return The total number of receive sectors in the codebook.
   */
  uint8_t GetTotalNumberOfReceiveSectors (void) const;
  /**
   * Get the total number of sectors in the codebook.
   * \return The total number of sectors in the codebook.
   */
  uint8_t GetTotalNumberOfSectors (void) const;
  /**
   * Get the total number of sectors for a specific phased antenna array.
   * \param antennaID The ID of the phased antenna array.
   * \return The number of sectors for the specified phased antenna array.
   */
  virtual uint8_t GetNumberSectorsPerAntenna (AntennaID antennaID) const = 0;
  /**
   * Get the total number of antennas in the codebook.
   * \return The total number of antennas in the codebook.
   */
  uint8_t GetTotalNumberOfAntennas (void) const;

  /**
   * Set the list of sectors utilized during beacon transmission interval (BTI).
   * \param sectors the list of the sectors utilized during BTI access period.
   */
  void SetBeaconingSectors (Antenna2SectorList sectors);
  /**
   * Append new sector to the list of Beaconing sectors.
   * \param antennaID The ID of the antenna array.
   * \param sectorID The ID of the sector.
   */
  void AppendBeaconingSector (AntennaID antennaID, SectorID sectorID);
  /**
   * Remove sector from the list of Beaconing sectors.
   * \param antennaID The ID of the antenna array.
   * \param sectorID The ID of the sector.
   */
  void RemoveBeaconingSector (AntennaID antennaID, SectorID sectorID);
  /**
   * Get the number of sectors utilized during the current beacon header interval (BHI).
   * \return The number of sectors utilized for Beaconing during the current beacon header interval (BHI).
   */
  uint8_t GetNumberOfSectorsInBHI (void);

  /**
   * Set the global list of transmit/receive sectors utilized during beamforming training (SLS Phase).
   * \param type The type of sector sweep.
   * \param sectors the list of sectors utilized during an SLS Phase.
   */
  void SetGlobalBeamformingSectorList (SectorSweepType type, Antenna2SectorList &sectors);
  /**
   * Append new sector transmit/receive to the list of SLS sectors.
   * \param address the MAC address of the peer station to have custom beamforming sectors with.
   * \param type The type of sector sweep (Tx/Rx).
   * \param antennaID The ID of the Antenna.
   * \param sectorID The ID of the sector.
   */
  void AppendBeamformingSector (Mac48Address address, SectorSweepType type, AntennaID antennaID, SectorID sectorID);
  /**
   * Get the number of sectors utilized during beamforming training.
   * \param type The type of sector sweep.
   * \return the number of sectors utilized during beamforming training.
   */
  uint8_t GetNumberOfSectorsForBeamforming (SectorSweepType type);
  /**
   * Append new sector transmit/receive to the general list of SLS sectors.
   * \param type The type of sector sweep (Tx/Rx).
   * \param antennaID The ID of the antenna array.
   * \param sectorID The ID of the sector.
   */
  void AppendBeamformingSector (SectorSweepType type, AntennaID antennaID, SectorID sectorID);
  /**
   * Set the list of transmit/receive sectors utilized during beamforming training (SLS Phase) with specific station.
   * \param type The type of sector sweep.
   * \param address The MAC address of the peer station.
   * \param sectors the list of sectors utilized during an SLS Phase.
   */
  void SetBeamformingSectorList (SectorSweepType type, Mac48Address address, Antenna2SectorList &sectorList);
  /**
   * Set the list of transmit/receive sectors utilized during MIMO beamforming training with specific station.
   * \param type The type of sector sweep.
   * \param address The MAC address of the peer station.
   * \param sectors the list of sectors utilized during MIMO BFT.
   */
  void SetMimoBeamformingSectorList (SectorSweepType type, Mac48Address address, Antenna2SectorList &sectorList);
  /**
   * Remove sector from the list of Beaconing sectors.
   * \param antennaID The ID of the antenna array.
   * \param sectorID The ID of the sector.
   */
  void RemoveBeamformingSector (AntennaID antennaID, SectorID sectorID);
  /**
   * Get the number of sectors utilized during beamforming training.
   * \param address the MAC address of the peer station.
   * \param type The type of sector sweep.
   * \return the number of sectors utilized during beamforming training.
   */
  uint8_t GetNumberOfSectorsForBeamforming (Mac48Address address, SectorSweepType type);
  /**
   * Copy the content of an existing codebook to the current codebook.
   * \param codebook A pointer to the codebook that we want to copy its content.
   */
  virtual void CopyCodebook (const Ptr<Codebook> codebook);
  /**
   * Change phased antenna array orientation using Euler transformation.
   * \param antennaID The ID of the antenna array.
   * \param psi The rotation around the z-axis in degrees.
   * \param theta The rotation around the y-axis in degrees.
   * \param phi The rotation around the x-axis in degrees.
   */
  virtual void ChangeAntennaOrientation (AntennaID antennaID, double psi, double theta, double phi);
  /**
   * Append a new custom AWV for under a specific sector.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the sector.
   * \param awvConfig A pointer to the custom AWV configuration.
   */
  void AppendAWV (AntennaID antennaID, SectorID sectorID, Ptr<AWV_Config> awvConfig);
  /**
   * Delete all the custom AWVs under a specific sector
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the sector.
   */
  void DeleteAWVs (AntennaID antennaID, SectorID sectorID);
  /**
   * Get a list of active antenna arrays across all the RF chains.
   * \return The list of active antenna arrays.
   */
  AntennaArrayList GetActiveAntennaList (void) const;
  /**
   * Get list of active tx beam patterns (Sector/AWV) across all the active RF chains.
   * \return A list of active tx beam patterns (Sector/AWV).
   */
  ActivePatternList GetActiveTxPatternList (void);
  /**
   * Get list of active rx beam patterns (Sector/AWV) across all the active RF chains.
   * \return A list of active rx beam patterns (Sector/AWV).
   */
  ActivePatternList GetActiveRxPatternList (void);
  /**
   * Set the current communication mode (SISO or MIMO).
   * \param mode The current communication mode (SISO or MIMO).
   */
  void SetCommunicationMode (CommunicationMode mode);
  /**
   * Get the current communication mode (SISO or MIMO).
   * \return The current communication mode (SISO or MIMO).
   */
  CommunicationMode GetCommunicationMode (void) const;
  /**
   * Set the IDs of the active RF chains for MIMO transmission.
   * \param chainList The list of active RF Chains with their corresponding active antenna IDs.
   */
  void SetActiveRFChainIDs (ActiveRFChainList &chainList);
  /**
   * This function returns a list of the antenna IDs of all antennas present in the codebook.
   * \return A list of the antenna IDs of all antennas in the codebook
   */
  std::vector<AntennaID> GetTotalAntennaIdList (void);
  /**
   * A function that returns the MIMO Tx antenna configuration combination associated with a given TX AWV ID based on the list of
   * sectors that were used in the MIMO phase of SU-MIMO BF training.
   * \param index The Tx AWV ID
   * \param peer The Address of the peer station (in MU MIMO we use the station's own address)
   * \return The MIMO AWV Configuration that corresponds to the Tx AWV ID
   */
  MIMO_AWV_CONFIGURATION GetMimoConfigFromTxAwvId (uint32_t index, Mac48Address peer);
  /**
   * A function that returns the Rx antenna configuration combination associated with a given index based on the list of
   * sectors that were used in the MIMO phase of SU-MIMO BF training.
   * \param indices A map between the Rx Antenna Id and the Rx AWV Id of the configuration
   * \param peer The address of the peer station
   * \return The MIMO AWV Configuration that corresponds to the Rx AWV I
   */
  MIMO_AWV_CONFIGURATION GetMimoConfigFromRxAwvId (std::map<RX_ANTENNA_ID, uint16_t> indices, Mac48Address peer);
  /**
   * A function that returns the Tx Configuration associated with a given short SSW CDOWN based on the list of sectors that
   * were used in the SISO phase of MU-MIMO BF training.
   */
  ANTENNA_CONFIGURATION GetAntennaConfigurationShortSSW (uint16_t cdown);
  /**
   * A function that returns the correct sector ID from a given antenna configuration reported by the peer station based on the list of sectors that
   * were used in the MIMO BRP phase of SU-MIMO BF training. This is necessary in case the number of sectors does not exactly match the
   * number of TRN Subfields used (since all units have to have the same number of subfields there might be extra subfields in the last unit)
   * which leads to some repetition of the sectors trained at the end.
   */
  SectorID GetSectorIdMimoBrpTxss (AntennaID antenna, SectorID sector);

protected:
  friend class DmgWifiHelper;
  friend class DmgWifiMac;
  friend class DmgApWifiMac;
  friend class DmgStaWifiMac;
  friend class DmgAdhocWifiMac;
  friend class DmgWifiPhy;
  friend class WifiPhy;
  friend class SpectrumDmgWifiPhy;
  friend class QdPropagationEngine;

  virtual void DoDispose ();
  virtual void DoInitialize (void);

  /**
   * Set the WifiNetDevice we are associated with.
   * \param device A pointer to the WifiNetDevice we are associated with.
   */
  void SetDevice (Ptr<WifiNetDevice> device);
  /**
   * Get a pointer to the WifiNetDevice we are associated with.
   * \return device A pointer to the WifiNetDevice we are associated with.
   */
  Ptr<WifiNetDevice> GetDevice (void) const;

  /**
   * Set the ID of the active AWV within the current transmit sector.
   * \param awvID The ID of the AWV within the transmit sector.
   */
  void SetActiveTxAwvID (AWV_ID awvID);
  /**
   * Set the ID of the active AWV ID within the current receive sector.
   * \param awvID The ID of the AWV within the receive sector.
   */
  void SetActiveRxAwvID (AWV_ID awvID);
  /**
   * Set the ID of the active transmit sector.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the new sector wthin the antenna array.
   */
  void SetActiveTxSectorID (AntennaID antennaID, SectorID sectorID); 
  /**
   * Set the ID of the active receive sector.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the new sector wthin the antenna array.
   */
  void SetActiveRxSectorID (AntennaID antennaID, SectorID sectorID);
  /**
   * Set the ID of the active RF chain for SISO transmission.
   * \param chainID The ID of the RF Chain to be activated.
   */
  void SetActiveRFChainID (RFChainID chainID);
  /**
   * Get active Tx pattern ID (Sector ID or AWV ID).
   * \return Active Tx pattern ID
   */
  uint8_t GetActiveTxPatternID (void) const;
  /**
   * Get active Rx pattern ID (Sector ID or AWV ID).
   * \return Active Rx pattern ID.
   */
  uint8_t GetActiveRxPatternID (void) const;
  /**
   * Get active Tx pattern IDs (Sector ID or AWV ID) for all active antennas.
   * \return Active Tx pattern ID
   */
  MIMO_AWV_CONFIGURATION GetActiveTxPatternIDs (void);
  /**
   * Get active Rx pattern ID (Sector ID or AWV ID) for all active antennas.
   * \return Active Rx pattern ID.
   */
  MIMO_AWV_CONFIGURATION GetActiveRxPatternIDs (void) ;
  /**
   * Get current active transmit antenna array pattern config.
   * \return A pointer to the pattern conifg of the transmit phased antenna array.
   */
  Ptr<PatternConfig> GetTxPatternConfig (void) const;
  /**
   * Get current active receive antenna array pattern config.
   * \return A pointer to the pattern conifg of the receive phased antenna array.
   */
  Ptr<PatternConfig> GetRxPatternConfig (void) const;
  /**
   * Get current active antenna array config.
   * \return A pointer to the current active antenna array config.
   */
  Ptr<PhasedAntennaArrayConfig> GetAntennaArrayConfig (void) const;
  /**
   * Get the ID of the current active Tx Antenna.
   * \return the ID of the current active Tx Antenna.
   */
  AntennaID GetActiveAntennaID (void) const;
  /**
   * Get the ID of the current active Tx Sector.
   * \return the ID of the current active Tx Sector.
   */
  SectorID GetActiveTxSectorID (void) const;
  /**
   * Get the ID of the current active Rx Sector.
   * \return the ID of the current active Rx Sector.
   */
  SectorID GetActiveRxSectorID (void) const;
  /**
   * Set the ID of the active RF Chain for SISO transmission.
   * \return chainID The RF Chain ID.
   */
  RFChainID GetActiveRFChainID (void) const;
  /**
   * Get the IDs of the active RF chains for MIMO transmission.
   * \return chainList The list of active RF chains with their corresponding active antenna IDs.
   */
  ActiveRFChainList GetActiveRFChainIDs (void) const;
  /**
   * Return the number of active RF Chains for transmission.
   * \return The number of active RF Chains used for transmission.
   */
  uint8_t GetNumberOfActiveRFChains (void) const;
  /**
   * Return the number of total RF Chains for transmission.
   * \return The number of total RF Chains that the STA can use for transmission.
   */
  uint8_t GetTotalNumberOfRFChains (void) const;
  /**
   * Get list of active antennas IDs across all the active RF chains.
   * \return The list of active antenna IDs for all the active RF chains/.
   */
  AntennaList GetActiveAntennasIDs (void) const;

  //// NINA ////
  /**
   * This function calculates and returns the number of MIMO BRP-TXSS packets to transmit based on vector that
   * contains the IDs on all the antennas that need to be trained. The number of MIMO BRP-TXSS packets
   * is determined by the number of antennas connected to the same RF Chain ID that need to be trained -
   * if more than 1 one antenna connected to the same RF Chain needs to be trained, it's not possible
   * to train all of the antennas at the same time.
   * The function also creates a vector of RFChainIdList which stores the antennas and their corresponding
   * RF Chains that need to be trained in each MIMO BRP TXSS packet.
   * \param antennaMask The antenna mask which defines the ID of the active antennas.
   * \return The total number of BRP packets to transmit.
   */
  uint8_t SetUpMimoBrpTxss (std::vector<AntennaID> antennaIds, Mac48Address peerStation);
  /**
   * This function returns a list of the antenna IDs that will be trained in the current packet during MIMO BFT.
   * \return A list of the antenna IDs of the antennas to be tested in the current MIMO BFT packet.
   */
  std::vector<AntennaID> GetCurrentMimoAntennaIdList (void);
  /**
   * This function calculates the number of TRN subfields needed in the current MIMO BRP TXSS packet.
   * The number of subfields is equal to the maximum of the number of sectors in each antenna that is being trained.
   * \return The number of TRN subfields needed in the current MIMO BRP TXSS packet.
   */
  uint8_t GetNumberOfTrnSubfieldsForMimoBrpTxss (void);
  /**
   * This function activates the correct RF chains and antennas for the transmission of the
   * TRN field of the current MIMO BFT packet
   */
  void StartMimoTx (void);
  /**
   * This function activates the correct RF chains and antennas for the reception of the
   * TRN field of the current MIMO BFT packet
   * \param combination The position of the combination of antennas to be used when receiving this packet.
   */
  void StartMimoRx (uint8_t combination);
  /**
   * Get the total number of receive sectors or receive AWVs in the codebook, depending of if AWVs are trained in the MIMO BFT.
   * \return The total number of receive sectors/AWVs in the codebook.
   */
  uint8_t GetTotalNumberOfReceiveSectorsOrAWVs (void);
  /**
   * This function returns the number of TRN subfields needed to do receive training in SMBT based on the
   * candidates chosen for testing in the MIMO phase. The number is determined by the number of candidates and
   * also the number of AWVs in each sector - if AWVs are used.
   * The function also creates a vector of RFChainIdList and which stores the antennas and their corresponding
   * RF Chains that need to tested and stores the sector candidates for sweeping later
   * \param from MAC address of the peer station that we are doing training with
   * \param antennaCandidates a list of antennas that we are training simultanously
   * \param sectorCandidates a list of sectors per antennna that we are training
   * \return The total number of TRN subfields needed for receive training
   */
  uint16_t GetSMBT_Subfields (Mac48Address from, std::vector<AntennaID> antennaCandidates,
                              Antenna2SectorList txSectorCandidates, Antenna2SectorList rxSectorCandidates);
  //// NINA ////
  /**
   * Select the quasi-omni receive mode for the current phased antenna array.
   */
  void SetReceivingInQuasiOmniMode (void);
  /**
   * Set specific antenna pattern to quasi-omni mode.
   * \param antennaID The ID of the phased antenna array.
   */
  void SetReceivingInQuasiOmniMode (AntennaID antennaID);
  /**
   * Start receiving in Quasi-omni Mode using the antenna array with the lowest ID.
   */
  void StartReceivingInQuasiOmniMode (void);
  /**
   * Switch to the next quasi-omni mode.
   * \return True, if switch has been done, false otherwise.
   */
  bool SwitchToNextQuasiPattern (void);
  /**
   * Set specific antenna pattern to quasi-omni mode when we have multiple antennas
   * but only one of them is active at a time.
   * \param antennaID The ID of the phased antenna array.
   */
  void SetReceivingInDirectionalMode (void);
  /**
   * Randomize beacon transmission in BTI access period.
   * \param beaconRandomization True if we want to randmoize beacon transmission otherwise false.
   */
  void RandomizeBeacon (bool beaconRandomization);
  /**
   * Initiate BTI access period. Calling this function lets the codebook chooses from the list
   * of sectors to be used in BTI.
   */
  void StartBTIAccessPeriod (void);
  /**
   * Get next sector in BTI access period.
   * \return True, if we still have sectors to cover otherwise false.
   */
  bool GetNextSectorInBTI (void);
  /**
   * Get the total number of beacon intervals it takes the PCP/AP to cover all sectors in all DMG antennas.
   * \return The total number of beacon intervals.
   */
  uint8_t GetNumberOfBIs (void) const;
  /**
   * Get number of remaing sector in BTI or SLS.
   * \return The total number of remaining sectors in BTI or SLS.
   */
  uint16_t GetRemaingSectorCount(void) const;
  /**
   * Initiate A-BFT access period as TXSS
   * \param address The MAC address of the DMG PCP/AP
   */
  void InitiateABFT (Mac48Address address);
  /**
   * Get next sector in A-BFT access period.
   * \return True, if we still have sectors to cover otherwise false.
   */
  bool GetNextSectorInABFT (void);
  /**
   * Start Sector Sweeping phase. Calling this function lets the code book chooses
   * from the list of sectors to be used in the SLS phase.
   * \param address the MAC address of the peer station.
   * \param type Sector Sweep Type (Transmit/Receive).
   */
  void StartSectorSweeping (Mac48Address address, SectorSweepType type, uint8_t peerAntennas);
  /**
   * Get the next Sector ID in the Beamforming Sector List.
   * \param changeAntenna True if we are changing the antenna otherwise false.
   * \return True if the antenna has been switched.
   */
  bool GetNextSector (bool &changeAntenna);
  /**
   * Check the current receiving mode (Quasi = True, Dierctional = False) in the current active RF chain.
   * \return True if the antenna is in quasi-omni reception mode; otherwise false.
   */
  bool IsQuasiOmniMode (void) const;
  /**
   * Get the 3d orientation of an antenna array.
   * \param antennaID The ID of the phased antenna array.
   * \return The antenna 3D orientation vector.
   */
  Orientation GetOrientation (AntennaID antennaId);
  /**
   * Initiate Beam Refinement Protocol after completing SLS phase.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the sector to refine wthin the antenna array.
   * \param type Beam Refinement Type (Transmit/Receive).
   */
  void InitiateBRP (AntennaID antennaID, SectorID sectorID, BeamRefinementType type);
  /**
   * Initiate Beam Refinement Protocol after completing SLS phase.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the sector to refine wthin the antenna array.
   * \param type Beam Refinement Type (Transmit/Receive).
   */
  void InitiateBeamTracking (AntennaID antennaID, SectorID sectorID, BeamRefinementType type);
  /**
   * Get the next AWV in the AWV list of the selected transmit/receive sector for all the active RF chains.
   */
  void GetNextAWV (void);
  /**
   * Reuse the last used transmit sector for each active antenna array in the list of active RF chains.
   * This function is called after Beam Tracking or initiating Beam Refinement.
   */
  void UseLastTxSector (void);
  /**
   * Reuse custom AWV for communication/Beam refinement.
   * This function is called in DmgWifiPhy during beam refinement.
   * \param type Type of Beam refinement - Transmit or Receive.
   */
  void UseCustomAWV (BeamRefinementType type);
  /**
   * Check whether we are using custom AWV for Tx or Rx.
   * \return True if we are using custom AWV, otherwise False if we are using sector.
   */
  bool IsCustomAWVUsed (void) const;
  /**
   * Return the number of AWVs used by the current active Tx/Rx sector.
   * \return The total number of AWVs used by the current active Tx/Rx sector.
   */
  uint8_t GetNumberOfAWVs (AntennaID antennaID, SectorID sectorID) const;
  /**
   * Remember the first used AWV at the start of the TRN field. Used in EDMG TRN-RX/TX fields.
   * \return Active Rx pattern ID.
   */
  void SetFirstAWV (void);
  /**
   * Go back to the first used AWV. Used in EDMG TRN-RX/TX fields.
   * \return Active Rx pattern ID.
   */
  void UseFirstAWV (void) ;
  /**
   * Begin receive training using TRN-R fields appended to a beacon.
   */
  void StartBeaconTraining (void);
  /**
   * Get number of remaing AWVs in current sector.
   * \return The total number of remaining AWVs in the current sector.
   */
  uint8_t GetRemaingAWVCount (void) const;
  /**
   * For the current sector start using different AWVs if possible.
   * Used in when we are training using TRN-R fields appended to DMG Beacons.
   */
  void StartSectorRefinement (void);
  //// NINA ////
  /**
   * Set up the antenna configurations to use when sector sweeping during SU-MIMO BFT. Calling this function sets up the list of antennas
   * that need to be tested in the current packet and the list of sectors that need to be tested for each antenna.
   * Note: The function does not actually activate any antennas, as this needs to be done at the start of the MIMO field.
   * \param address the MAC address of the peer station.
   * \param type Sector Sweep Type (Transmit/Receive).
   * \param firstCombination If we should use the first MIMO combination or switch to the next one.
   */
  void InitializeMimoSectorSweeping (Mac48Address address, SectorSweepType type, bool firstCombination);
  /**
   * Get the next Sector ID/ AWV ID in the Beamforming Sector Lists of all the antennas that are simultaneously being trained.
   * \param firstSector whether this is the first sector that is being trained.
   */
  void GetMimoNextSector(bool firstSector);
  /**
   * Reuse the sector used before starting the sweeping for each active antenna array in the list of active RF chains.
   * This function is called during BRP TXSS for transmitting the P subfields.
   */
  void UseOldTxSector (void);
  /**
   * Get the next Sector ID/ AWV ID in the Beamforming Sector Lists of all the antennas that are simultaneously being trained by testing all
   * possible combinations of AWVs.
   * \param firstSector whether this is the first sector that is being trained.
   */
  void GetMimoNextSectorWithCombinations (bool firstSector);
  /**
   * Reset the antenna config of all active antennas to the first one in the beamforming training list.
   */
  void SetMimoFirstCombination (void);
  /**
   * Start Initiator TXSS as part of SISO phase of MU-MIMO BFT. Calling this function lets the codebook sweep through
   * all transmit sectors across all DMG antennas.
   */
  void StartMuMimoInitiatorTXSS (void);

  /**
   * Set up the antenna configurations to use when sector sweeping during MU-MIMO BFT in the MIMO phase.
   * Calling this function sets up the list of antennas that need to be tested in the current packet and the
   * list of Tx sectors and/or AWVs that need to be tested for each antenna.
   * Note: The function does not actually activate any antennas, as this needs to be done at the start of the MIMO field.
   * \param own the MAC address of the station performing the training.
   * \param antennaCandidates a list of antennas that we are training simultanously.
   * \param txSectorCandidates a list of Tx sectors per antennna that we are training
   */
  void SetUpMuMimoSectorSweeping (Mac48Address own, std::vector<AntennaID> antennaCandidates,
                             Antenna2SectorList txSectorCandidates);
  /**
   * Indicate whether to use AWVs for MIMO beamforming training
   * \param useAwvs
   */
  void SetUseAWVsMimoBft (bool useAwvs);
  /**
   * Returns the list of the general receive sectors of the antenna.
   * \return the list of general receive sectors
   */
  Antenna2SectorList GetRxSectorsList (void);
  //// NINA ////

private:
  /**
   * Get the number Of sectors with specific station.
   * \param address The MAC address of the peer station.
   * \param list The beamforming Tx/Rx sector list.
   * \return The total number of Tx/Rx sectors.
   */
  uint8_t GetNumberOfSectors (Mac48Address address, BeamformingSectorList &list);
  /**
   * Get the total number of sector for specific beamforming list.
   * \param sectorList The list of antennas with their sectors.
   * \return The total number of sector for specific beamforming list.
   */
  uint8_t CountNumberOfSectors (Antenna2SectorList *sectorList);
  //// NINA ////
  /**
   * Get the number of sectors for simultaneous training of multiple antennas
   * \param sectorList The list of antennas with their sectors.
   * \param useAwv Whether we train the AWVs withing each sector
   * \return The total number of sector for specific beamforming list.
   */
  uint16_t CountMimoNumberOfSectors (Antenna2SectorList sectorList, bool useAwv);
  /**
   * Get the number of Tx subfields for simultaneous training of multiple antennas, assuming that
   * that all combinations of AWVs in the different antennas are tested
   * \return The total number of subfields for the specific beamforming list.
   */
  uint16_t CountMimoNumberOfTxSubfields (Mac48Address peer);
  //// NINA ////
  /**
   * Append new sector to a sector list.
   * \param globalList
   * \param antennaID The ID of the Antenna.
   * \param sectorID The ID of the sector.
   */
  void AppendToSectorList (Antenna2SectorList &globalList, AntennaID antennaID, SectorID sectorID);
  /**
   * Set the ID of the active transmit sector.
   * \param sectorID The ID of the transmit sector wthin the antenna array.
   */
  void SetActiveTxSectorID (SectorID sectorID);
  /**
   * Set the ID of the active receive sector.
   * \param sectorID The ID of the receive sector wthin the antenna array.
   */
  void SetActiveRxSectorID (SectorID sectorID);
  /**
   * Activate an RF chain and add it to the last of active RF chains.
   * \param chainID The ID of the RF Chain to be activated.
   */
  void ActivateRFChain (RFChainID chainID);

protected:
  /* Codebook Variables */
  std::string m_fileName;                     //!< The name of the file describing the transmit and receive patterns.
  Ptr<WifiNetDevice> m_device;                //!< Pointer to the WifiNetDevice we are associated with.

  /* Front End Architecture Variables */
  RFChainList m_rfChainList;                  //!< List of RF Chains associated with this codebook.
  ActiveRFChainList m_activeRFChainList;      //!< List of active RF Chain list with their corresponding active antenna array for MIMO.
  RFChainID m_activeRFChainID;                //!< Active RF Chain for SISO transmission.
  Ptr<RFChain> m_activeRFChain;               //!< Pointer to the current active RF chain.
  AntennaArrayList m_antennaArrayList;        //!< List of antenna arrays associated with this codebook.
  AntennaArrayList m_activeAntennaArrayList;  //!< List of active antenna arrays across all the RF chains.(Note:Not yet used anywhere)
  CommunicationMode m_communicationMode;

  /* Beamforming Variables */
  CurrentBeamformingPhase m_currentBFPhase;   //!< The current beamforming phase (BTI/A-BFT or SLS).
  SectorSweepType m_sectorSweepType;          //!< Sector Sweep Type during SLS Phase (Transmit/Receive).
  SectorID m_currentSectorIndex;              //!< The index of the current sector for BTI or SLS.
  uint16_t m_remainingSectors;                 //!< The total number of sectors remaining to cover in SLS phase.
  Antenna2SectorListI m_beamformingAntenna;   //!< Iterator to the curent utilized antenna in either BTI or SLS.
  SectorIDList m_beamformingSectorList;       //!< The list of the sectors witin the current used antenna.
  Antenna2SectorList *m_currentSectorList;    //!< Current list of TXSS sectors.
  Mac48Address m_peerStation;                 //!< The MAC address of the peer station in the SLS phase.

  /* Beamforming Variables */  
  Antenna2SectorList m_txBeamformingSectors;  //!< List of the general transmit sectors utilized during SLS.
  Antenna2SectorList m_rxBeamformingSectors;  //!< List of the general receive sectors utilized during SLS.
  BeamformingSectorList m_txCustomSectors;    //!< List of transmit sectors utilized during SLS per Station.
  BeamformingSectorList m_rxCustomSectors;    //!< List of receive sectors utilized during SLS per Station.
  uint8_t m_totalTxSectors;                   //!< The total number of transmit sectors within the Codebook.
  uint8_t m_totalRxSectors;                   //!< The total number of receive sectors within the Codebook.
  uint8_t m_totalSectors;                     //!< The total number of sectors within the Codebook.
  uint8_t m_totalAntennas;                    //!< The total number of antennas within the Codebook.

  /* BHI Access Period Variables */
  Antenna2SectorList m_bhiAntennaList;        //!< List of antenna arrays utilized during the BHI access period.
  bool m_beaconRandomization;                 //!< Flag to indicate whether we want to randomize the selection of DMG Beacon for each BI.
  uint8_t m_btiSectorOffset;                  //!< The first antenna sector to start the BTI with.
  Antenna2SectorListI m_bhiAntennaI;          //!< Iterator to the current utilized antenna array and its corresponding sectors in BHI access period.

  /* Beam Refinement and Tracking Variables */
  AWV_LIST_I m_currentAwvI;                   //!< An iterator for the current AWV list.

  /* Data Transmission Variables */
  AWV_LIST_I m_firstAwvI;                     //!< First AWV used at the start of receiving an EDMG TRN-RX/TX field.
  //// NINA ////
  /* MIMO Beamforming variables */
  std::vector<ActiveRFChainList> m_mimoCombinations; //!< A vector that contains the combinations of Antennas (and their RF Chains) that need to be used to transmit each packet during MIMO BFT.
  std::vector<ActiveRFChainList>::iterator m_currentMimoCombination; //!< Iterator to the combination of antennas used in the current packet during MIMO BFT.
  BeamformingSectorList m_txMimoCustomSectors;    //!< List of transmit sectors utilized during MIMO BFT per Station.
  BeamformingSectorList m_rxMimoCustomSectors;    //!< List of receive sectors utilized during MIMO BFT per Station.
  //// NINA ////
  bool m_useAWVsMimoBft;                      //!< Flag that indicates if we´re using AWVs during the MIMO BFT.

};

} // namespace ns3

#endif /* CODEBOOK_H */
