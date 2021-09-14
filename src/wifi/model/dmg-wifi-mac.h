/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2021 IMDEA Networks Institute
 * Authors: Hany Assasa <hany.assasa@gmail.com>
 *          Nina Grosheva <nina.grosheva@gmail.com>
 */
#ifndef DMG_WIFI_MAC_H
#define DMG_WIFI_MAC_H

#include "regular-wifi-mac.h"
#include "codebook.h"
#include "dmg-ati-txop.h"
#include "dmg-sls-txop.h"
#include "dmg-capabilities.h"
#include "edmg-capabilities.h"
#include "wigig-data-types.h"
#include <queue>


namespace ns3 {

#define dot11MaxBFTime                   1    /* Maximum Beamforming Time (in units of beacon interval). */
#define dot11BFTXSSTime                 10    /* Timeout until the initiator restarts ISS (in Milliseconds)." */
#define dot11MaximalSectorScan          10
#define dot11ABFTRTXSSSwitch            10
#define dot11BFRetryLimit               10    /* Beamforming Retry Limit. */
/**
  * SSW Frame Size = 26 Bytes
  * Ncw = 2, Ldpcw = 160, Ldplcw = 160, dencodedSymbols  = 328
  * Frames duration precalculated using DMG MCS-0 and reported in microseconds
  */
#define sswTxTime                   NanoSeconds (14909) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (5964).
#define sswFbckTxTime               NanoSeconds (18255) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310).
#define sswAckTxTime                NanoSeconds (18255) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310).
//#define sswTxTime                   MicroSeconds (15) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (5964).
//#define sswFbckTxTime               MicroSeconds (19) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310).
//#define sswAckTxTime                MicroSeconds (19) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310).
#define edmgSswTxTime               NanoSeconds (19273) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (5964).
#define edmgSswFbckTxTime           NanoSeconds (24945) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310).
#define edmgSswAckTxTime            NanoSeconds (24945) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (9310).
#define edmgShortSswTxTime          NanoSeconds (13411) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (4466).
#define edmgBrpPollFrame            NanoSeconds (25091) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (16146).
#define maxEdmgCtrlFrame            NanoSeconds (307855) // NanoSeconds (4291) + NanoSeconds (4654) + NanoSeconds (298910).
#define aAirPropagationTime         NanoSeconds (100)
#define aTSFResolution              MicroSeconds (1)
#define GUARD_TIME                  MicroSeconds (10)
#define MBIFS                       MicroSeconds (9)
#define SLS_FEEDBACK_PHASE_DURATION (sswFbckTxTime + MBIFS + sswAckTxTime + 2 * aAirPropagationTime)
#define EDMG_SLS_FEEDBACK_PHASE_DURATION (edmgSswFbckTxTime + MBIFS + edmgSswAckTxTime + 2 * aAirPropagationTime)
// Timeout Values
#define SSW_ACK_TIMEOUT             2 * sswAckTxTime + 2 * (aAirPropagationTime + MBIFS)
#define EDMG_SSW_ACK_TIMEOUT        2 * edmgSswAckTxTime + 2 * (aAirPropagationTime + MBIFS)
// Size of different DMG Control Frames in Bytes 802.11ad
#define POLL_FRAME_SIZE             22
#define SPR_FRAME_SIZE              27
#define GRANT_FRAME_SIZE            27
// Global Association Identifiers.
#define AID_AP                      0
#define AID_BROADCAST               255
// Antenna Configuration.
#define NO_ANTENNA_CONFIG           255
#define NO_AWV_ID                   255
// Allocation duration for both SPs and CBAPs.
#define BROADCAST_CBAP              0
#define MAX_SP_BLOCK_DURATION       32767
#define MAX_CBAP_BLOCK_DURATION     65535
#define MAX_NUM_BLOCKS              255
#define aMinTXSSSectorFBCnt         16

enum ChannelAccessPeriod {
  CHANNEL_ACCESS_BTI = 0,
  CHANNEL_ACCESS_ABFT,
  CHANNEL_ACCESS_ATI,
  CHANNEL_ACCESS_BHI,
  CHANNEL_ACCESS_DTI
};

/** Type definitions **/
typedef std::pair<DynamicAllocationInfoField, BF_Control_Field> AllocationData; /* Typedef for dynamic allocation of SPs. */
typedef std::list<AllocationData> AllocationDataList;
typedef AllocationDataList::const_iterator AllocationDataListCI;
/**
 * Typedef for antenna configuration for storing SNR valeus during an SLS phase.
 * \param MyAntennaID The ID of the antenna array used for measuring the signal.
 * \param PeerAntennaID The ID of the peer antenna array transmitting the frame.
 * \param PeerSectorID The ID of the peer sector transmitting the frame.
 */
typedef std::tuple<AntennaID, AntennaID, SectorID> ANTENNA_CONFIGURATION_COMBINATION;

//// NINA ////
/* SNR */
typedef double SNR;   /* Typedef for SNR value. */
typedef std::vector<SNR> SNR_LIST;
typedef SNR_LIST::const_iterator SNR_LIST_ITERATOR;
//// NINA ////

/* Typedefs for Recording SNR Value for MIMO Beamforming training */
typedef std::tuple<BRP_CDOWN, RX_ANTENNA_ID, TX_ANTENNA_ID>                 MIMO_CONFIGURATION; /* Typedef to save the MIMO configuration associated with a given SNR measurement */
typedef std::map<MIMO_CONFIGURATION, SNR_LIST>                              SU_MIMO_SNR_MAP;    /* Map to save all SNR measurements done during SU-MIMO BFT in the SISO Phase */
typedef std::map<MIMO_CONFIGURATION, SNR>                                   MU_MIMO_SNR_MAP;    /* Map to save all SNR measurements done during MU-MIMO BFT in the SISO Phase */

typedef std::pair<BRP_CDOWN, SNR_LIST>                                      MIMO_SNR_MEASUREMENT; /* Map between the list of SNR Measurements and the BRP CDOWN value of the packet they were received in */
typedef std::vector<MIMO_SNR_MEASUREMENT>                                   MIMO_SNR_LIST;        /* Typedef to save all SNR measurements done during the MIMO phase of SU and MU MIMO BFT */
typedef MIMO_SNR_LIST::iterator                                             MIMO_SNR_LIST_I;

typedef std::tuple<TX_ANTENNA_ID, uint8_t, SectorID>                        MIMO_FEEDBACK_CONFIGURATION; /* Typedef to save the MIMO configuration associated with a given SNR measurement received as feedback */
typedef std::map<MIMO_FEEDBACK_CONFIGURATION, SNR>                          MIMO_FEEDBACK_MAP;           /* Map to save all SNR measurements done during MIMO BFT. */

typedef std::multimap<SNR, MIMO_FEEDBACK_CONFIGURATION, std::greater<SNR>>  MIMO_FEEDBACK_SORTED_MAP;     /* A MIMO Feedback Map which reverses the Key-value mapping and is sorted according to descending SNR order. */
typedef MIMO_FEEDBACK_SORTED_MAP::iterator                                  MIMO_FEEDBACK_SORTED_MAP_I;
typedef std::vector<MIMO_FEEDBACK_SORTED_MAP>                               MIMO_FEEDBACK_SORTED_MAPS;    /* A vector of MIMO Feedback sorted Maps */
typedef MIMO_FEEDBACK_SORTED_MAPS::iterator                                 MIMO_FEEDBACK_SORTED_MAPS_I;
typedef std::vector<MIMO_FEEDBACK_CONFIGURATION>                            MIMO_FEEDBACK_COMBINATION;    /* A vector of MIMO Feedback configurations that compose a MIMO combination */
typedef MIMO_FEEDBACK_COMBINATION::iterator                                 MIMO_FEEDBACK_COMBINATION_I;
typedef std::multimap<SNR, MIMO_FEEDBACK_COMBINATION, std::greater<SNR>>    MIMO_CANDIDATE_MAP;           /* A map between MIMO Feedback combinations and their joint SNR, sorted in descending order of the SNR */
typedef MIMO_CANDIDATE_MAP::iterator                                        MIMO_CANDIDATE_MAP_I;

typedef std::vector<ANTENNA_CONFIGURATION>                                  MIMO_ANTENNA_COMBINATION;       /* A combination of Antenna Configurations to be used in MIMO mode */
typedef std::vector<MIMO_ANTENNA_COMBINATION>                               MIMO_ANTENNA_COMBINATIONS_LIST; /* A list of combinations of Antenna Configurations to be used in MIMO mode */
typedef MIMO_ANTENNA_COMBINATIONS_LIST::iterator                            MIMO_ANTENNA_COMBINATIONS_LIST_I;

typedef std::map<MIMO_FEEDBACK_CONFIGURATION, uint16_t>                     SISO_ID_SUBSET_INDEX_MAP;    /* A Map between a MIMO Feedback Configuration and its SISO ID Subset Index. */
typedef std::pair<uint32_t, uint8_t>                                        SNR_MEASUREMENT_INDEX;       /* Typedef to store the index of a given SNR measurement in a MIMO_SNR_LIST. */
typedef std::map<uint16_t, SNR_MEASUREMENT_INDEX>                           SISO_ID_SUBSET_INDEX_RX_MAP; /* A Map between the the SNR Measurement index and the SISO Id Subset Index of a given SNR measurement. */

/* A typedef to hold the Tx and Rx AWV IDs of a measurement done during the MIMO phase of MIMO BFT.
 * The pair holds the Tx AWV Id and a map between the Rx Antenna ID and the Rx AWV ID for all Rx Antennas.
 * This is because we test all possible Tx combinations, but we test only all possible Rx candidates and
 * then create all possible combinations in post-processing, so the measurement can be a combination of
 * Rx candidates tested at different times */
typedef std::pair<uint16_t, std::map<RX_ANTENNA_ID, uint16_t>> MEASUREMENT_AWV_IDs;
typedef std::priority_queue<std::pair<double, MEASUREMENT_AWV_IDs>> SNR_MEASUREMENT_AWV_IDs_QUEUE;
typedef std::vector<MEASUREMENT_AWV_IDs> BEST_TX_COMBINATIONS_AWV_IDS; /* A typedef to store the Tx and Rx AWV IDs of the top Tx Measurements from the MIMO phase of MIMO BFT.*/
typedef std::pair<bool, uint8_t> userMaskConfig; /* Typedef to store the result of the reading of the Group User Mask field. */
typedef std::map<AntennaID, Mac48Address> MU_MIMO_ANTENNA2RESPONDER;         /* Typedef for a map between an Antenna ID and the responder it will transmit to in MU-MIMO*/
typedef std::map<AntennaID, AntennaID> SU_MIMO_ANTENNA2ANTENNA;              /* Typedef for a map between a TX Antenna ID and the RX Antenna ID it will transmit to in SU-MIMO*/

/**
 * The SLS completion callback structure.
 */
struct SlsCompletionAttrbitutes {
  SlsCompletionAttrbitutes ();
  SlsCompletionAttrbitutes (Mac48Address _peerStation, ChannelAccessPeriod _accessPeriod,
                            BeamformingDirection _beamformingDirection,
                            bool _isInitiatorTXSS, bool _isResponderTXSS, uint16_t _bftID,
                            AntennaID _antennaID, SectorID _sectorID, double _maxSnr)
    : peerStation (_peerStation),
      accessPeriod (_accessPeriod),
      beamformingDirection (_beamformingDirection),
      isInitiatorTXSS (_isInitiatorTXSS),
      isResponderTXSS (_isResponderTXSS),
      bftID (_bftID),
      antennaID (_antennaID),
      sectorID (_sectorID),
      maxSnr (_maxSnr)
  {}

  Mac48Address peerStation;                   //!< The MAC address of the peer station.
  ChannelAccessPeriod accessPeriod;           //!< The current Channel access period BTI or DTI.
  BeamformingDirection beamformingDirection;  //!< Initiator or Responder.
  bool isInitiatorTXSS;                       //!< Flag to indicate if the initiator is TXSS or RXSS.
  bool isResponderTXSS;                       //!< Flag to indicate if the responder is TXSS or RXSS.
  uint16_t bftID;                             //!< The ID of the current BFT.
  AntennaID antennaID;                        //!< The ID of the selected antenna.
  SectorID sectorID;                          //!< The ID of the selected sector.
  double maxSnr;                              //!< The maximum snr value as a result fo the SLS procedure.
};

/**
 * The IEEE 802.11ad SLS BFT initiator side state machine.
 */
enum SLS_INITIATOR_STATE_MACHINE {
  SLS_INITIATOR_IDLE = 0,
  SLS_INITIATOR_SECTOR_SELECTOR = 1,
  SLS_INITIATOR_SSW_ACK = 2,
  SLS_INITIATOR_TXSS_PHASE_COMPELTED = 3,
};

/**
 * \ingroup wifi
 * TracedValue Callback signature for SLS_INITIATOR_STATE_MACHINE
 *
 * \param [in] oldState original state of the traced variable
 * \param [in] newState new state of the traced variable
 */
typedef void (* SlsInitiatorTracedValueCallback)(const SLS_INITIATOR_STATE_MACHINE oldState,
                                                 const SLS_INITIATOR_STATE_MACHINE newState);
/**
 * The IEEE 802.11ad SLS BFT responder side state machine.
 */
enum SLS_RESPONDER_STATE_MACHINE {
  SLS_RESPONDER_IDLE = 0,
  SLS_RESPONDER_SECTOR_SELECTOR = 1,
  SLS_RESPONDER_SSW_FBCK = 2,
  SLS_RESPONDER_TXSS_PHASE_PRECOMPLETE = 3,
  SLS_RESPONDER_TXSS_PHASE_COMPELTED = 4,
};

/**
 * \ingroup wifi
 * TracedValue Callback signature for SLS_RESPONDER_STATE_MACHINE
 *
 * \param [in] oldState original state of the traced variable
 * \param [in] newState new state of the traced variable
 */
typedef void (* SlsResponderTracedValueCallback)(const SLS_RESPONDER_STATE_MACHINE oldState,
                                                 const SLS_RESPONDER_STATE_MACHINE newState);

enum BRP_TRAINING_TYPE {
  BRP_TRN_T = 0,
  BRP_TRN_R = 1,
  BRP_TRN_T_R = 2,
};

enum BEAM_LINK_MAINTENANCE_TIMER_STATE {
  BEAM_LINK_MAINTENANCE_TIMER_RELEASE,
  BEAM_LINK_MAINTENANCE_TIMER_RESET,
  BEAM_LINK_MAINTENANCE_TIMER_SETUP_RELEASE,
  BEAM_LINK_MAINTENANCE_TIMER_HALT,
  BEAM_LINK_MAINTENANCE_TIMER_EXPIRES
};

/**
 * The Group Beamforming completion callback structure.
 */
struct GroupBfCompletionAttrbitutes {
  GroupBfCompletionAttrbitutes ();
  GroupBfCompletionAttrbitutes (Mac48Address _peerStation, BeamformingDirection _beamformingDirection, uint16_t _bftID,
                            AntennaID _antennaID, SectorID _sectorID, AWV_ID _awvID, double _maxSnr)
    : peerStation (_peerStation),
      beamformingDirection (_beamformingDirection),
      bftID (_bftID),
      antennaID (_antennaID),
      sectorID (_sectorID),
      awvID (_awvID),
      maxSnr (_maxSnr)
  {}

  Mac48Address peerStation;                   //!< The MAC address of the peer station.
  BeamformingDirection beamformingDirection;  //!< Initiator or Responder.
  uint16_t bftID;                             //!< The ID of the current BFT.
  AntennaID antennaID;                        //!< The ID of the selected antenna.
  SectorID sectorID;                          //!< The ID of the selected sector.
  AWV_ID awvID;                               //!< The ID of the select AWV.
  double maxSnr;                              //!< The maximum snr value as a result fo the SLS procedure.
};

/**
 * The MIMO phase measurements callback structure.
 */
struct MimoPhaseMeasurementsAttributes {
  MimoPhaseMeasurementsAttributes ();
  MimoPhaseMeasurementsAttributes (Mac48Address _peerStation, MIMO_SNR_LIST _mimoSnrList, SNR_MEASUREMENT_AWV_IDs_QUEUE _queue,
                            bool _differentRxCombinations, uint8_t _nTxAntennas, uint8_t _nRxAntennas, uint8_t _rxCombinationsTested, uint16_t _bftID )
    : peerStation (_peerStation),
      mimoSnrList (_mimoSnrList),
      queue (_queue),
      differentRxCombinations (_differentRxCombinations),
      nTxAntennas (_nTxAntennas),
      nRxAntennas (_nRxAntennas),
      rxCombinationsTested (_rxCombinationsTested),
      bftID (_bftID)
  {}

  Mac48Address peerStation;                   //!< The MAC address of the peer station.
  MIMO_SNR_LIST mimoSnrList;                  //!< The list of measurements taken during the MIMO phase.
  SNR_MEASUREMENT_AWV_IDs_QUEUE queue;        //!< A priority queue based on the minumum per-stream SNR for every possible AWV ID combination.
  bool differentRxCombinations;               //!< Whether or not we want to feedback different Rx combinations.
  uint8_t nTxAntennas;                         //!< The number of Tx Antennas.
  uint8_t nRxAntennas;                         //!< The number of Rx Antennas.
  uint8_t rxCombinationsTested;               //!< Number of Rx Combinations tested.
  uint16_t bftID;                             //!< The ID of the current BFT.
};

/**
 * States for SU-MIMO BFT state machine.
 */
enum SU_MIMO_BF_TRAINING_PHASES {
  SU_WAIT_SU_MIMO_BF_TRAINING = 0,
  SU_SISO_SETUP_PHASE,
  SU_SISO_INITIATOR_TXSS,
  SU_SISO_RESPONDER_FBCK,
  SU_SISO_RESPONDER_TXSS,
  SU_SISO_INITIATOR_FBCK,
  SU_MIMO_SETUP_PHASE,
  SU_MIMO_INITIATOR_SMBT,
  SU_MIMO_RESPONDER_SMBT,
  SU_MIMO_FBCK_PHASE,
};

/**
 * \ingroup wifi
 * TracedValue Callback signature for SU_MIMO_BF_TRAINING_PHASES
 *
 * \param [in] oldState original state of the traced variable
 * \param [in] newState new state of the traced variable
 */
typedef void (* SU_MIMO_BFT_TracedValueCallback)(const SU_MIMO_BF_TRAINING_PHASES oldState,
                                                 const SU_MIMO_BF_TRAINING_PHASES newState);

/**
 * Phases for MU-MIMO BFT state machine.
 */
enum MU_MIMO_BF_TRAINING_PHASES {
  MU_WAIT_MU_MIMO_BF_TRAINING = 0,
  MU_SISO_TXSS,
  MU_SISO_FBCK,
  MU_MIMO_BF_SETUP,
  MU_MIMO_BF_TRAINING,
  MU_MIMO_BF_FBCK,
  MU_MIMO_BF_SELECTION,
};

/**
 * \ingroup wifi
 * TracedValue Callback signature for MU_MIMO_BF_TRAINING_PHASES
 *
 * \param [in] oldState original state of the traced variable
 * \param [in] newState new state of the traced variable
 */
typedef void (* MU_MIMO_BFT_TracedValueCallback)(const MU_MIMO_BF_TRAINING_PHASES oldState,
                                                 const MU_MIMO_BF_TRAINING_PHASES newState);

/**
 * The current data communication mode for IEEE 802.11ay node.
 */
enum DATA_COMMUNICATION_MODE {
  DATA_MODE_SISO = 0,
  DATA_MODE_SU_MIMO,
  DATA_MODE_MU_MIMO,
};

class BeamRefinementElement;
class DmgWifiPhy;

/**
 * \brief IEEE 802.11ad/ay Upper MAC.
 * \ingroup wifi
 *
 * Abstract class for Directional Multi Gigabit (DMG) support.
 */
class DmgWifiMac : public RegularWifiMac
{
public:
  static TypeId GetTypeId (void);

  DmgWifiMac ();
  virtual ~DmgWifiMac ();

  /**
   * Get Association Identifier (AID).
   * \return The AID of the station.
   */
  virtual uint16_t GetAssociationID (void) = 0;
  /**
   * \param address the current address of this MAC layer.
   */
  virtual void SetAddress (Mac48Address address);
  /**
  * \param packet the packet to send.
  * \param to the address to which the packet should be sent.
  *
  * The packet should be enqueued in a tx queue, and should be
  * dequeued as soon as the channel access function determines that
  * access is granted to this MAC.
  */
  virtual void Enqueue (Ptr<Packet> packet, Mac48Address to) = 0;
  /**
   * \param stationManager the station manager attached to this MAC.
   */
  virtual void SetWifiRemoteStationManager(Ptr<WifiRemoteStationManager> stationManager);
  /**
   * Steer the directional antenna towards specific station for transmission.
   * \param address The MAC address of the peer station.
   */
  void SteerTxAntennaToward (Mac48Address address, bool isData = false);
  /**
   * Steer the directional antenna towards specific station for transmission and reception.
   * \param address The MAC address of the peer station.
   */
  void SteerAntennaToward (Mac48Address address, bool isData = false);
  /**
   * Steer the directional antenna towards specific station for transmission in SISO mode and set the reception in omni-mode.
   * \param address The MAC address of the peer station.
   */
  void SteerSisoTxAntennaToward (Mac48Address address);
  /**
   * Steer the directional antenna towards specific station for transmission and reception in SISO mode.
   * \param address The MAC address of the peer station.
   */
  void SteerSisoAntennaToward (Mac48Address address);
  /**
   * Steer the directional antennas towards specific station for reception in MIMO mode.
   * \param address The MAC address of the peer station.
   */
  void SteerMimoRxAntennaToward (Mac48Address address);
  /**
   * Steer the directional antennas towards specific station for transmission in MIMO mode.
   * \param address The MAC address of the peer station.
   */
  void SteerMimoTxAntennaToward (Mac48Address address);
  /**
   * Steer the directional antennas towards specific station for transmission and reception in MIMO mode.
   * \param address The MAC address of the peer station.
   */
  void SteerMimoAntennaToward (Mac48Address address);

  /**
   * Print SNR Table for each station that we did sectcor level sweeping with.
   */
  void PrintSnrTable (void);
  /**
   * Print Beam Refinement Measurements for each device.
   */
  void PrintBeamRefinementMeasurements (void);
  /**
   * Print Beam Refinement Measurements for the AP measured from TRN fields appended to beacons.
   */
  void PrintGroupBeamformingMeasurements (void);
  /**
   * Calculate the duration of a single sweep based on the number of sectors.
   * \param sectors The number of sectors.
   * \return The total duration required for completing a single sweep.
   */
  Time CalculateSectorSweepDuration (uint8_t sectors);
  /**
   * This function is used to calculate the amount of time to stay in a specific
   * Quasi-omni pattern while the peer side is sweeping across all its antennas.
   * \param peerAntennas The total number of antennas in the peer device.
   * \param peerSectors The total number of sectors over all DMG antennas in the peer device.
   * \return The total duration required for completing a single sweep.
   */
  Time CalculateSingleAntennaSweepDuration (uint8_t peerAntennas, uint8_t peerSectors);
  /**
   * Calculate the duration of a sector sweep based on the number of sectors and antennas.
   * \param peerAntennas The total number of antennas in the peer device.
   * \param myAntennas The total number of antennas in the current device.
   * \param mySectors The total number of sectors over all DMG antennas in the current device.
   * \return The total duration required for completing a single sweep.
   */
  Time CalculateSectorSweepDuration (uint8_t peerAntennas, uint8_t myAntennas, uint8_t mySectors);
  /**
   * Calculate the duration of the sector sweep of the current station.
   * \return The total duration required for completing a single sweep.
   */
  Time CalculateMySectorSweepDuration (void);
  /**
   * This function is used to calculate the duration of the I-TXSS using Short SSW frames
   * \param antennas The total number of antennas in the device.
   * \param sectors The total number of sectors over all DMG antennas in the device.
   * \return The total duration required for completing a single sweep.
   */
  Time CalculateShortSectorSweepDuration (uint8_t antennas, uint8_t sectors);
  /**
   * This function is used to calculate the duration of the SISO Feedback Phase during MU-MIMO BFT.
   * We assume that the duration of the BRP Feedback packets will the maximum allowed (payload of 1023 bytes)
   * to ensure that there is enough time to complete the feedback from all STAs.
   * \return The total duration required for completing a single sweep.
   */
  Time CalculateSisoFeedbackDuration ();
  /**
   * Calculate the duration of the Beamforming Training (SLS Only).
   * \param initiatorSectors The number of Tx/Rx sectors in the initiator station.
   * \param responderSectors The number of Tx/Rx sectors in the responder station.
   * \return The total duration required for completing beamforming training.
   */
  Time CalculateBeamformingTrainingDuration (uint8_t initiatorSectors, uint8_t responderSectors);
  /**
   * Calculate the total duration for Beamforming Training (SLS Only).
   * \param initiatorAntennas The number of antenna arrays in the initiator station.
   * \param initiatorSectors The number of Tx/Rx sectors in the initiator station.
   * \param initiatorAntennas The number of antenna arrays in the responde stationr.
   * \param responderSectors The number of Tx/Rx sectors in the responder station.
   * \return The total duration required for completing beamforming training.
   */
  Time CalculateTotalBeamformingTrainingDuration (uint8_t initiatorAntennas, uint8_t initiatorSectors,
                                                  uint8_t responderAntennas, uint8_t responderSectors);
  /**
   * Store the DMG capabilities of a DMG STA.
   * \param wifiMac Pointer to the DMG STA.
   */
  void StorePeerDmgCapabilities (Ptr<DmgWifiMac> wifiMac);
  /**
   * Retreive the DMG Capbilities of a peer DMG station.
   * \param stationAddress The MAC address of the peer station.
   * \return DMG Capbilities of the Peer station if exists otherwise null.
   */
  Ptr<DmgCapabilities> GetPeerStationDmgCapabilities (Mac48Address stationAddress) const;
  /**
   * Store the EDMG capabilities of a EDMG STA.
   * \param wifiMac Pointer to the EDMG STA.
   */
  void StorePeerEdmgCapabilities (Ptr<DmgWifiMac> wifiMac);
  /**
   * Retreive the EDMG Capbilities of a peer EDMG station.
   * \param stationAddress The MAC address of the peer station.
   * \return EDMG Capbilities of the Peer station if exists otherwise null.
   */
  Ptr<EdmgCapabilities> GetPeerStationEdmgCapabilities (Mac48Address stationAddress) const;
  /**
   * Calculate the duration of the Beamforming Training (SLS Only) based on the capabilities of the responder.
   * \param responderAddress The MAC address of the responder.
   * \param isInitiatorTXSS Indicate whether the initiator is TXSS or RXSS.
   * \param isResponderTXSS Indicate whether the respodner is TXSS or RXSS.
   * \return The total duration required for completing beamforming training.
   */
  Time ComputeBeamformingAllocationSize (Mac48Address responderAddress, bool isInitiatorTXSS, bool isResponderTXSS);
  /**
   * Initiate Beamforming with a particular DMG STA.
   * \param peerAid The AID of the peer DMG STA.
   * \param peerAddress The address of the DMG STA to perform beamforming training with.
   * \param isInitiator Indicate whether we are the Beamforming Initiator.
   * \param isInitiatorTXSS Indicate whether the initiator is TXSS or RXSS.
   * \param isResponderTXSS Indicate whether the respodner is TXSS or RXSS.
   * \param length The length of the Beamforming Service Period.
   */
  void StartBeamformingTraining (uint8_t peerAid, Mac48Address peerAddress,
                                 bool isInitiator, bool isInitiatorTXSS, bool isResponderTXSS,
                                 Time length);

  /**
   * Initiate TXSS in CBAP allocation using TxOP.
   * \param peerAddress The address of the DMG STA to perform beamforming training with.
   */
  virtual void Perform_TXSS_TXOP (Mac48Address peerAddress);
  /**
   * Set Codebook
   * \param codebook The codebook for antennas and sectors patterns.
   */
  void SetCodebook (Ptr<Codebook> codebook);
  /**
   * Get Codebook
   * \return A pointer to the current codebook.
   */
  Ptr<Codebook> GetCodebook (void) const;
  /**
   * Initiate BRP transaction with a peer sation in ATI access period.
   * \param receiver The address of the peer station.
   * \param L_RX The compressed number of TRN-R subfields requested by the transmitting STA as part of beam refinement.
   * \param TX_TRN_REQ To indicate that the STA needs transmit training as part of beam refinement.
   */
  void InitiateBrpTransaction (Mac48Address receiver, uint8_t L_RX, bool TX_TRN_REQ);
  /**
   * Initiate BRP transaction with a peer sation after an SLS phase.
   * \param receiver The address of the peer station.
   * \param requestField The BRP Request Field from the SLS phase.
   */
//  void InitiateBrpTransaction (Mac48Address receiver, BRP_Request_Field &requestField);

  /* Temporary Function to store AID mapping */
  void MapAidToMacAddress (uint16_t aid, Mac48Address address);
  /**
   * Get antenna configuration for communication with a particular DMG STA.
   * \param stationAddress The MAC address of the DMG STA.
   * \param isTxConfiguration Flag to indicate whether we need transmit or receive antenna configuration.
   * \return The best antenna configuration for communication with the specified DMG STA.
   */
  ANTENNA_CONFIGURATION GetAntennaConfiguration (const Mac48Address stationAddress, bool isTxConfiguration) const;
  /* Set functions for EDMG parameters */
  /**
   * Sets the TRN Sequence Length when transmitting EDMG TRN fields.
   * \param trnSeqLen TRN Sequence Length
   */
   void SetTrnSeqLength (TRN_SEQ_LENGTH trnSeqLen);
  /**
   * Sets the number of TRN subfields at the beginning of a TRN-Unit
   * which are transmitted with the same AWV as defined in 29.9.2.2.3
   * \param edmgTrnP Edmg TRN P value
   */
   void Set_EDMG_TRN_P (uint8_t edmgTrnP);
  /**
   * Sets the number of TRN Subfields in a unit used for transmit training.
   * \return edmgTrnM Edmg TRN M value
   */
   void Set_EDMG_TRN_M (uint8_t edmgTrnM);
  /**
   * Sets the number of consecutive TRN subfields within the EDMG-Unit M
   * which are transmitted using the same AWV.
   * \param edmgTrnN Edmg TRN N value
   */
   void Set_EDMG_TRN_N (uint8_t edmgTrnN);
  /**
   * Sets the number of TRN units repeated with the same AWV in order
   * to perform RX training at the responder.
   * \param rxPerTxUnits Rx Per Tx Units
   */
   void Set_RxPerTxUnits (uint8_t rxPerTxUnits);

  /* Get functions for EDMG parameters */
  /**
   * Returns the TRN Sequence Length when transmitting EDMG TRN fields.
   * \return TRN Sequence Length
   */
  TRN_SEQ_LENGTH GetTrnSeqLength (void) const;
  /**
   * Returns the number of TRN subfields at the beginning of a TRN-Unit
   * which are transmitted with the same AWV as defined in 29.9.2.2.3
   * \return Edmg TRN P value
   */
  uint8_t Get_EDMG_TRN_P (void) const;
  /**
   * Returns the number of TRN Subfields used for transmit training.
   * \return Edmg TRN M value
   */
  uint8_t Get_EDMG_TRN_M (void) const;
  /**
   * Returns the number of consecutive TRN subfields within the EDMG-Unit M
   * which are transmitted using the same AWV.
   * \param Edmg TRN N value
   */
  uint8_t Get_EDMG_TRN_N (void) const;
  /**
   * Returns the number of TRN units repeated with the same AWV in order
   * to perform RX training at the responder.
   */
  uint8_t Get_RxPerTxUnits (void) const;
  /**
   * True if Channel Aggregation is used and false otherwise.
   * \return if channel aggregation is used.
   */
  bool GetChannelAggregation (void) const;
  /**
   * Returns BRP CDOWN value in EDMG BRP Packets
   */
  uint8_t GetBrpCdown (void) const;

  /* EDMG SU-MIMO Beamforming functions */
  //// NINA ////
  /**
   * Initiates the SU-MIMO beamforming protocol with a given responder.
   * \param responder The MAC address of the STA that we want to initiate the beamforming with.
   * \param isBrpTxssNeeded Indicates whether the SISO phase is comprised of a MIMO BRP TXSS procedure or a SISO feedback procedure.
   * \param antennaIds A vector if IDs that indicates which DMG antennas will be used for the SU-MIMO training and transmissions.
   */
  void StartSuMimoBeamforming (Mac48Address responder, bool isBrpTxssNeeded, std::vector<AntennaID> antennaIds , bool useAwvs = false);
  /**
   * Starts the MIMO BRP TXSS Setup Procedure as part of the SISO SU-MIMO phase.
   * \param responder The MAC address of the STA that we are beamforming training with.
   * \param antennaIds A vector if IDs that indicates which DMG antennas will be used for the SU-MIMO training and transmissions.
   */
  void StartMimoBrpTxssSetup (Mac48Address responder, std::vector<AntennaID> antennaIds );
  /**
   * Starts the SISO Feedback Procedure as part of the SISO SU-MIMO phase.
   * \param responder The MAC address of the STA that we are beamforming training with.
   */
  void StartSuSisoFeedback (Mac48Address responder, std::vector<AntennaID> antennaIds );
  /**
   * Starts the MIMO BRP TXSS after the setup procedure has been completed, as part of the SISO SU-MIMO phase.
   */
  void StartMimoBrpTxss ();
  /**
   * Send a MIMO BRP Frame as part of a BRP TXSS.
   * \param address The MAC address of the peer station.
   */
  void SendMimoBrpTxssFrame (Mac48Address address);
  /**
   * End the receive of a BRP packet with TRN subfields. According to the BRP CDOWN value of the
   * packet change the antenna configurations in the codebook
   */
  void EndMimoTrnField (void);
  /**
   * Send a MIMO BRP Feedback frame as part of a BRP TXSS.
   * \param address The MAC address of the peer station.
   */
  void SendSuMimoTxssFeedback (void);
  /**
  * \brief StartSuMimoMimoPhase
  * Starts the MIMO phase of SU-MIMO BFT. Receives a list of Tx candidates to try out which was generated based on the feedback
  * from the SISO phase. Creates a list of Rx candidates to try out based on the received list.
  * \param from MAC address of the peer station
  * \param candidates list of candidates to try out in the MIMO phase
  * \param txCombinationsRequested number of Tx combinations that we want to receive feedback for in the feedback phase.
  **/
  void StartSuMimoMimoPhase (Mac48Address from, MIMO_ANTENNA_COMBINATIONS_LIST candidates,
                             uint8_t txCombinationsRequested, bool useAwvs = true);
  /**
   * \brief SendSuMimoSetupFrame
   * Sends a MIMO BF Setup Frame to the peer station, specifying the requested parameters for the MIMO
   * training to follow.
   */
  void SendSuMimoSetupFrame (void);
  /**
  * Starts the SMBT subphase of the MIMO phase of SU-MIMO BFT.
  **/
  void StartSuMimoBfTrainingSubphase (void);
  /**
   * Send a MIMO BRP Frame as part of a MIMO BF training (during MIMO phase of SU or MU MIMO BFT).
   * \param address The MAC address of the peer station.
   */
  void SendMimoBfTrainingBrpFrame (Mac48Address address);
  /**
   * Send a MIMO BF Feedback Frame as part of a SU-MIMO BF training giving feedback for the training that
   * happened in the previous SMBT subphase.
   */
  void SendSuMimoBfFeedbackFrame (void);
  /**
   * Find all possible combinations of Tx-Rx pairs that we should check. The combination includes a Tx-Rx pair
   * for each stream, making sure that no two Tx-Rx pairs have the same Tx or Rx antenna.
   * This ensures that we are training the correct combinations to establish independent streams.
   * \param offset The offset from the start of txRxCombinations.
   * \param nStreams The number of streams that we want to establish
   * \param txRxCombinations The feedback Map split according to the Tx-Rx pair.
   * \param validCombinations A vector that contains all valid Tx-Rx Pair combinations
   * \param indexes A vector that contains all indexes of txRxCombinations (goes 0 1 2... size(txRxCombinations) - 1)
   */
  void FindAllValidCombinations (uint16_t offset, uint16_t nStreams, MIMO_FEEDBACK_SORTED_MAPS &txRxCombinations,
                                 std::vector<std::vector<uint16_t>> &validCombinations, std::vector<uint16_t> &currentCombination ,
                                 std::vector<uint16_t> indexes);
  /**
   * Find all possible combinations of Tx-Rx pairs that we should check - when establishing nStreams we need
   * to match each Tx antenna to an Rx antenna, making sure that no Tx or Rx Antennad appears twice in different
   * streams. This function returns all possible combinations that can be made taking into account the number of
   * Tx and Rx antennas in the form of a list of indexes from 1 - nStreams * nRx (each Idx can be matched to a Tx Idx
   * and a Rx Idx knowing that there are two for loops - the outside one is for the Tx antenna and the inside one
   * for the Rx antennas.)
   * \param offset The offset from the start of indexes.
   * \param nStreams The number of streams that we want to establish
   * \param nRx The number of STAs that are being trained.
   * \param validTxRxPairs A vector that contains all valid Tx-Rx Pair combinations
   * \param currentCombination The current combination of Tx and Rx pair that is being considered
   * \param indexes A vector of indexes from 1 - nStreams * nSTAs
   */
  void FindAllValidTxRxPairs (uint16_t offset, uint8_t nStreams, uint8_t nRx,
                             std::vector<std::vector<uint16_t> > &validTxRxPairs, std::vector<uint16_t> &currentCombination ,
                             std::vector<uint16_t> indexes);
  /**
   * From a given feedback with measurements from the SISO phase of MIMO Beamforming training
   * find the K best candidates to test in the MIMO phase ranking the candidates according to the joint SINR.
   * \param k The number of candidates to test.
   * \param numberOfStreams The number of concurrent streams that we want to establish (number of Tx antennas transmitting at the same time).
   * \param numberOfRxAntennas The number of Receive antennas of the peer station (Or members of the MU group in MU-MIMO BFT).
   * \param feedback The feedback Map that contains all the measurements done in the SISO phase.
   * \return A list of K best combinations of antenna configurations to test in the MIMO phase.
   */
  MIMO_ANTENNA_COMBINATIONS_LIST FindKBestCombinations (uint16_t k, uint8_t numberOfStreams, uint8_t numberOfRxAntennas,
                                                        MIMO_FEEDBACK_MAP feedback);
  /**
   * From a given measurement list from the MIMO phase of MIMO Beamforming training find the best combinations (according to the
   * number of requested combinations) to feedback to the peer station.
   * \param nBestCombinations The number of combinations the peer station has requested to feedback.
   * \param rxCombinationsTested The number of Rx configurations that were tested.
   * \param nTxAntennas The number of Tx Antennas used in the training.
   * \param nRxAntennas The number of Rx Antennas used in the training.
   * \param measurements The measurement list that contains all the measurements done in the MIMO phase.
   * \param differentRxCombinations Whether we should report combinations with the same TX AWV ID and
   * different RX AWV IDs, or only the top combination per TX AWV ID.
   * \return A list of the TX and RX AWV IDs of the top combinations measured during the MIMO phase of MIMO BFT.
   */
  BEST_TX_COMBINATIONS_AWV_IDS FindBestTxCombinations (uint8_t nBestCombinations, uint16_t rxCombinationsTested,
                                                       uint8_t nTxAntennas, uint8_t nRxAntennas, MIMO_SNR_LIST measurements,
                                                       bool differentRxCombinations);
  /**
   * From a given measurement list from the feedback from the MIMO phase of MU-MIMO BFT, find the optimal
   * MU-MIMO configuration which should be used.
   * \param nTx The number of Tx antennas that are being tested
   * \param nRx The number of STAs which are being trained
   * \param feedback The feedback list that contains all the feedback fiven by stations done in the MIMO phase.
   * \return The Tx ID associated with the optimal antenna configuration.
   */
  MIMO_FEEDBACK_COMBINATION FindOptimalMuMimoConfig (uint8_t nTx, uint8_t nRx, MIMO_FEEDBACK_MAP feedback, std::vector<uint16_t> txAwvIds);
  /**
   * Get the current communication mode with the station (SISO, SU-MIMO or MU-MIMO) from the Data Communication
   * Mode table. In case there is no entry for the station the default mode is SISO.
   * \param station The Mac address of the station that we are communicating with.
   * \return The communication mode that needs to be used to communicate with this station.
   */
  DATA_COMMUNICATION_MODE GetStationDataCommunicationMode (Mac48Address station);
  /**
   * Get the number of streams that can be used for communication with this station.
   * Note: For now we assume that the number of streams is equal to the number of antennas that we use for MIMO
   * \param station The Mac address of the station that we are communicating with.
   * \return The number of streams that can be used for communication with the station.
   */
  uint8_t GetStationNStreams (Mac48Address station);
  //// NINA ////
  /* EDMG MU-MIMO Beamforming functions */
  //// NINA ////
  /**
   * Initiates the MU-MIMO beamforming protocol with a given MU group.
   * \param isInitiatorTxssNeeded Indicates whether the SISO phase contains an Initiator TXSS or not.
   * \param edmgGroupId The group ID of the MU group that we want to perform BFT with.
   */
  void StartMuMimoBeamforming (bool isInitiatorTxssNeeded, uint8_t edmgGroupId );
  /**
   * Starts an Initiator TXSS as part of the SISO phase of MU-MIMO BFT. During the Initiator TXSS the initiator
   * sends Short SSW frames sweeping through all Tx sectors across all DMG antennas while the MU group receipients
   * measure the received SNR.
   */
  void StartMuMimoInitiatorTXSS (void);
  /**
   * Send an EDMG Short SSW Frame as part of Initiator TXSS.
   */
  void SendMuMimoInitiatorTxssFrame (void);
  /**
   * Starts the SISO Feedback phase of MU-MIMO BFT. During the SISO Feedback phase the initiator polls all the members of the
   * MU group being trained for feedback regarding the SNR measured during the last initiator TXSS.
   */
  void StartMuMimoSisoFeedback (void);
  /**
   * Sends a BRP poll frame to a member from the MU group being trained in the current MU-MIMO BFT to ask for feedback.
   */
  void SendBrpFbckPollFrame (void);
  /**
   * \param station The MAC address of the initiator
   * \param useAwvsInMimoPhase Whether we will train only sectors, or additionally AWVs
   * in the following MIMO phase of the training (used to calculate the number of TRN units
   * the STA needs for receive training).
   * Sends feedback from the SISO Initiator TXSS if it existed or from the latest BFT if it didn´t.
   * Sends a BRP frame with feedback as a response to a BRP frame send by an initiator for MU-MIMO BFT.
   */
  void SendBrpFbckFrame (Mac48Address station, bool useAwvsinMimoPhase = false);
  /**
   * Function to call if MU-MIMO BFT failed.
   */
  void MuMimoBFTFailed (void);
  /**
   * Starts the MIMO phase of MU-MIMO BFT. Receives a list of candidates to try out which was generated based on the feedback
   * from the SISO phase. Sets up the codebook for the future training subphase and starts the setup phase.
   */
  void StartMuMimoMimoPhase (MIMO_ANTENNA_COMBINATIONS_LIST candidates, bool useAwvs = false);
  /**
   * Sends a MIMO setup frame to inform the responders for MIMO phase that follows. Specifies if the MIMO phase will be reciprocal/Non reciprocal
   * (currently only Non Reciprocal is supported) and which users from the group should participate in the following MIMO BF training.
   */
  void SendMuMimoSetupFrame (void);
  /**
   * Starts the MU BF Training subphase of the MIMO phase of MU-MIMO BFT. During this phase the initiator sends BRP frames with TRN-RX/TX subfields
   * for simultanous transmit and receive training. Different combinations of Tx sectors are tested (chosen based on the feedback from the SISO phase).
   */
  void StartMuMimoBfTrainingSubphase (void);
  /**
   * Starts the MU BF Feedback subphase of the MIMO phase of MU-MIMO BFT. During this phase the initiator polls each responder that participated in the
   * MIMO phase training for feedback.
   */
  void StartMuMimoBfFeedbackSubphase (void);
  /**
   * Sends a MIMO BF Poll frame to a responder from the MU Group asking for feedback from the MIMO phase training that was done.
   */
  void SendMimoBfPollFrame (void);
  /**
   * Sends a MIMO BF Feedback frame in response to a MIMO BF Poll frame send by the initiator of the MU-MIMO BFT. The frame contains
   * feedback for the top Tx Configurations measured in the MIMO phase training.
   */
  void SendMuMimoBfFeedbackFrame (Mac48Address station);
  /**
   * Start the MU BF Selection Subphase of the MIMO phase of MU-MIMO BFT. From all the feedback received in the previous phase it
   * chooses the optimal MU MIMO configurations and saves it for future use. Afterwards it starts sending the first Selection frame to inform
   * the responders of the MU MIMO Rx configs they should use in the future.
   */
  void StartMuMimoSelectionSubphase ();
  /**
   * Sends a MIMO BF Selection frame to inform the responders in the MU group about the Rx configuration they should use in MU-MIMO data transmissions.
   */
  void SendMuMimoBfSelectionFrame (void);
  /**
   * Creates the Group User Mask Field which is a bitmap that indicates whether an EDMG STA in the target MU group is
   * requested to engage in the subsequent MU-MIMO BF training. The order of EDMG STAs in the Group User
   * Mask field follows the order in which they appear in the corresponding EDMG Group field of the EDMG
   * Group ID Set element defining the target MU group. The first bit (i.e., the least significant bit) of the Group
   * User Mask field corresponds to the first EDMG STA in the MU group, the second bit corresponds to the
   * second EDMG STA, and so on. A bit is set to 1 to indicate that the corresponding EDMG STA is requested
   * to engage in the subsequent MU-MIMO BF training; otherwise the bit is set to 0. If the number of EDMG STAs in the target MU group is
   * smaller than 32, the corresponding bits in the Group User Mask field are set to 0.
   */
  uint32_t GenerateEdmgMuGroupMask (void);
  /**
   * Based on the Group User Mask Field checks if the given user is included in the user group.
   * Checks if the bit that corresponds to this station is set to 0 or 1 in the Group User Mask Field and how many users are included in the user group before the user.
   */
  userMaskConfig IsIncludedInUserGroup (uint32_t groupUserMaskField);
  /**
   * Returns the last EDMG Group ID set element that was send.
   */
  Ptr<EDMGGroupIDSetElement> GetEdmgGroupIdSetElement (void) const;
  //// NINA ////

protected:
  friend class MacLow;
  friend class DmgStaWifiMac;

  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual void Configure80211ad (void);
  virtual void Configure80211ay (void);

  /**
   * Get a pointer to the current DMG Wifi Phy.
   * \return
   */
  Ptr<DmgWifiPhy> GetDmgWifiPhy (void) const;
  /**
   * Initiator SLS TXSS TxOP is granted to the station.
   * \param peerAddress The address of the DMG STA to perform beamforming training with.
   */
  void TXSS_TXOP_Access_Granted (Mac48Address peerAddress, SLS_ROLE slsRole, bool isFeedback);
  /**
   * Reset SLS initiator related variables.
   */
  void Reset_SLS_Initiator_Variables (void);
  /**
   * Reset SLS responder related variables.
   */
  void Reset_SLS_Responder_Variables (void);
  /**
   * Reset SLS BFT related variables.
   */
  void Reset_SLS_State_Machine_Variables (void);
  /**
   * Initialize Sector Sweep Parameters for TXSS.
   * \param peerAddress The address of the DMG STA to perform beamforming training with.
   */
  bool Initialize_Sector_Sweep_Parameters (Mac48Address peerAddress);
  /**
   * Start Beamforming Initiator Phase.
   */
  void StartBeamformingInitiatorPhase (void);
  /**
   * Start Beamforming Responder Phase.
   * \address The MAC address of the peer station.
   */
  void StartBeamformingResponderPhase (Mac48Address address);
  /**
   * Terminate service period.
   */
  void EndServicePeriod (void);
  /**
   * Set whether PCP Handover is supported or not.
   * \param value Set to true if PCP handover is enabled otherwise it is set to false.
   */
  void SetPcpHandoverSupport (bool value);
  /**
   * \return Whether the PCP handover is supported or not.
   */
  bool GetPcpHandoverSupport (void) const;

  /**
   * Send Information Request frame.
   * \param to The MAC address of the receiving station.
   * \param request Pointer to the Request Element.
   */
  void SendInformationRequest (Mac48Address to, ExtInformationRequest &requestHdr);
  /**
   * Send Information Response to the station that requested the information.
   * \param to The MAC address of the station requested the information.
   * \param responseHdr Pointer to the Response Element.
   */
  void SendInformationResponse (Mac48Address to, ExtInformationResponse &responseHdr);
  /**
   * Send Unsolicited Information Response Frame when there is a change in the best observed TX Sector and Antenna ID.
   * Used when beamforming training is done using TRN-R fields appended to a beacon.
   * \param receiver The MAC address of the responding station.
   */
  void SendUnsolicitedTrainingResponse (Mac48Address receiver);
  /**
   * Return the DMG capability of the current STA.
   * \return the DMG capability that we support
   */
  virtual Ptr<DmgCapabilities> GetDmgCapabilities (void) const = 0;
  /**
   * Return the EDMG capability of the current STA.
   * \return the EDMG capability that we support
   */
  Ptr<EdmgCapabilities> GetEdmgCapabilities (void) const;

  /**
   * \param sbifs the value of Short Beamforming Interframe Space (SBIFS).
   */
  void SetSbifs (Time sbifs);
  /**
   * \param mbifs the value of Medium Beamforming Interframe Space (MBIFS).
   */
  void SetMbifs (Time mbifs);
  /**
   * \param lbifs the value of Large Beamforming Interframe Space (LBIFS).
   */
  void SetLbifs (Time lbifs);
  /**
   * \param brpifs the value of Beam Refinement Protocol Interframe Spacing (BRPIFS).
   */
  void SetBrpifs (Time brpifs);
  /**
   * \return The value of of Short Beamforming Interframe Space (SBIFS)
   */
  Time GetSbifs (void) const;
  /**
   * \return The the value of Medium Beamforming Interframe Space (MBIFS).
   */
  Time GetMbifs (void) const;
  /**
   * \return The value of Large Beamforming Interframe Space (LBIFS).
   */
  Time GetLbifs (void) const;
  /**
   * \return The value of Beam Refinement Protocol Interframe Spacing (BRPIFS).
   */
  Time GetBrpifs (void) const;
  /**
   * Get sector sweep duration either at the initiator or at the responder.
   * \param sectors The number of sectors in the duration.
   * \return The total duration required to complete sector sweep.
   */
  Time GetSectorSweepDuration (uint8_t sectors);
  /**
   * Get sector sweep slot time in A-BFT access period.
   * \param fss The number of sectors in a slot.
   * \return The duration of a single slot in A-BFT access period.
   */
  Time GetSectorSweepSlotTime (uint8_t fss);
  /**
   * Map received SNR value to a specific address and TX antenna configuration (The Tx of the peer station).
   * \param address The MAC address of the receiver.
   * \param antennaID The ID of the transmitting antenna.
   * \param sectorID The ID of the transmitting sector.
   * \param snr The received Signal to Noise Ration in dB.
   */
  void MapTxSnr (Mac48Address address, AntennaID antennaID, SectorID sectorID, double snr);
  /**
     * Map received SNR value to a specific address and TX antenna configuration (The Tx of the peer station).
     * \param address The MAC address of the receiver.
     * \param antennaID The ID of the receiving antenna
     * \param antennaID The ID of the transmitting antenna.
     * \param sectorID The ID of the transmitting sector.
     * \param snr The received Signal to Noise Ration in dB.
     */
    void MapTxSnr (Mac48Address address, AntennaID RxAntennaID, AntennaID TxAntennaID, SectorID sectorID, double snr);
  /**
   * Map received SNR value to a specific address and RX antenna configuration (The Rx of the calling station).
   * \param address The address of the receiver.
   * \param antennaID The ID of the receiving antenna.
   * \param sectorID The ID of the receiving sector.
   * \param snr The received Signal to Noise Ration in dB.
   */
  void MapRxSnr (Mac48Address address, AntennaID antennaID, SectorID sectorID, double snr);
  /**
   * Get the remaining time for the current allocation period.
   * \return The remaining time for the current allocation period.
   */
  Time GetRemainingAllocationTime (void) const;
  /**
   * Get the remaining time for the current sector sweep.
   * \return The remaining time for the current sector sweep.
   */
  Time GetRemainingSectorSweepTime (void) const;
  /**
   * Transmit control frame using either CBAP or SP access period.
   * \param packet
   * \param hdr
   * \param duration The duration in the SSW field.
   */
  void TransmitControlFrame (Ptr<const Packet> packet, WifiMacHeader &hdr, Time duration);
  /**
   * Transmit control frame using SP access period.
   * \param packet
   * \param hdr
   * \param duration The duration in the SSW field.
   */
  void TransmitControlFrameImmediately (Ptr<const Packet> packet, WifiMacHeader &hdr, Time duration);
  /**
   * Transmit Short SSW frame using either CBAP or SP access period.
   * \param packet
   */
  void TransmitShortSswFrame (Ptr<const Packet> packet);
  /**
   * Receive a Short SSW frame.
   */
  void ReceiveShortSswFrame(Ptr<Packet> packet, double rxSnr);
  /**
   * \brief Receive Sector Sweep Frame.
   * \param packet Pointer to the sector sweep packet.
   * \param hdr The MAC header of the packet.
   */
  void ReceiveSectorSweepFrame (Ptr<Packet> packet, const WifiMacHeader *hdr);
  /**
   * Record Beamformed Link Maintenance Value
   * \param field The BF Link Maintenance Field in SSW-FBCK/ACK.
   */
  void RecordBeamformedLinkMaintenanceValue (BF_Link_Maintenance_Field field);
  /**
   * Switch to the next Quasi-omni pattern in the codebook
   * \param switchTime The time when to switch to the next Quasi-Omni Pattern.
   */
  void SwitchQuasiOmniPattern (Time switchTime);
  /**
   * Restart Initiator Sector Sweep (ISS) Phase.
   * \param stationAddress The MAC address of the peer station we want to perform ISS with.
   */
  void RestartInitiatorSectorSweep (Mac48Address stationAddress);
  /**
   * Start Transmit Sector Sweep (TXSS) with specific station.
   * \param address The MAC address of the peer DMG STA.
   * \param direction Indicate whether we are initiator or responder.
   */
  void StartTransmitSectorSweep (Mac48Address address, BeamformingDirection direction);
  /**
   * Start Receive Sector Sweep (RXSS) with specific station.
   * \param address The MAC address of the peer DMG STA.
   * \param direction Indicate whether we are beamforming initiator or responder.
   */
  void StartReceiveSectorSweep (Mac48Address address, BeamformingDirection direction);
  /**
   * Send Receive Sector Sweep Frame as Initiator.
   * \param address The MAC address of the respodner.
   * \param count the remaining number of sectors.
   * \param direction Indicate whether we are beamforming initiator or responder.
   */
  void SendReceiveSectorSweepFrame (Mac48Address address, uint16_t count, BeamformingDirection direction);
  /**
   * Send a Transmit Sector Sweep Frame as an Initiator.
   * \param address The MAC address of the respodner.
   */
  void SendInitiatorTransmitSectorSweepFrame (Mac48Address address);
  /**
   * Send a Tansmit Sector Sweep Frame as a Responder.
   * \param address The MAC address of the initiator.
   */
  void SendRespodnerTransmitSectorSweepFrame (Mac48Address address);
  /**
   * Send SSW FBCK Frame for SLS phase in a scheduled service period or after SSW-Slot in A-BFT.
   * \param receiver The MAC address of the responding station.
   * \param duration The duration in the MAC header.
   */
  void SendSswFbckFrame (Mac48Address receiver, Time duration);
  /**
   * Re-Send SSW FBCK Frame for SLS phase in a scheduled service period.
   */
  void ResendSswFbckFrame (void);
  /**
   * Send SSW ACK Frame for SLS phase in a scheduled service period.
   * \param receiver The MAC address of the peer DMG STA.
   * \param sswFbckDuration The duration in the SSW-FBCK Field.
   */
  void SendSswAckFrame (Mac48Address receiver, Time sswFbckDuration);
  /**
   * The BRP setup subphase is used to set up beam refinement transaction.
   * \param type The type of BRP (Either Transmit, Receive, or both).
   * \param address The MAC address of the receiving station (Responder's MAC Address).
   */
  void InitiateBrpSetupSubphase (BRP_TRAINING_TYPE type, Mac48Address receiver);
  /**
   * Send BRP Frame without TRN Field to a specific station.
   * \param receiver The MAC address of the receiving station.
   * \param requestField The BRP Request Field.
   * \param element The Beam Refinement Element.
   */
  void SendEmptyBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element);
  /**
   * Send BRP Frame with TRN Field to a specific station.
   * \param receiver The MAC address of the receiving station.
   * \param requestField The BRP Request Field.
   * \param element The Beam Refinement Element.
   */
  void SendBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element,
                     bool requestBeamRefinement, PacketType packetType, uint8_t trainingFieldLength);
  /**
   * Send BRP Frame as part of MIMO BF training without TRN Field to a specific station.
   * \param receiver The MAC address of the receiving station.
   * \param requestField The BRP Request Field.
   * \param element The Beam Refinement Element.
   * \param edmgRequest The EDMG BRP Request Element.
   */
  void SendEmptyMimoBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element, EdmgBrpRequestElement &edmgRequest);
  /**
   * Send BRP Frame as part of MIMO BF training with TRN Field to a specific station.
   * \param receiver The MAC address of the receiving station.
   * \param requestField The BRP Request Field.
   * \param element The Beam Refinement Element.
   * \param edmgRequest The EDMG BRP Request Element.
   */
  void SendMimoBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element, EdmgBrpRequestElement &edmgRequest,
                     bool requestBeamRefinement, PacketType packetType, uint8_t trainingFieldLength);
  /**
   * Send BRP Feedback Frame as part of MIMO BF training without TRN Field to a specific station.
   * \param receiver The MAC address of the receiving station.
   * \param requestField The BRP Request Field.
   * \param element The Beam Refinement Element.
   * \param edmgRequest The EDMG BRP Request Element.
   * \param channel The Channel Measurement Feedback Element.
   * \param edmgChannel The EDMG Channel Measurement Feedback Element.
   */
  void SendFeedbackMimoBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element, EdmgBrpRequestElement *edmgRequest,
                                 std::vector<ChannelMeasurementFeedbackElement> channel, std::vector<EDMGChannelMeasurementFeedbackElement> edmgChannel);

  /**
   * This function is called by dervied call to notify that BRP Phase has completed.
   */
  virtual void NotifyBrpPhaseCompleted (void) = 0;
  /**
   * Maps the SNR value to an integer value in order to transmit it in a Channel Measurement Feedback Element.
   * The SNR subfield levels are unsigned integers referenced to a level of –8 dB. Each step is 0.25 dB. SNR
   * values less than or equal to –8 dB are represented as 0. SNR values greater than or equal to 55.75 dB are
   * represented as 0xFF. (From 802.11-2016, section 9.4.2.136)
   * \param snr the SNR value of the subfield
   * \return the corresponding integer value
   */
  uint8_t MapSnrToInt (double snr);
  /**
   * Reverse mapping of the SNR integer value received in the Channel Measurement Feedback Element to a liner value.
   * \param snr the SNR value of the subfield
   * \return the corresponding double value
   */
  double MapIntToSnr (uint8_t snr);

  /* Typedefs for Recording SNR Value per Antenna Configuration */
  typedef std::map<ANTENNA_CONFIGURATION_COMBINATION, SNR>  SNR_MAP;    /* Typedef for Map between Antenna Config and SNR. */
  typedef SNR_MAP                               SNR_MAP_TX;             /* Typedef for SNR TX for each antenna configuration. */
  typedef SNR_MAP                               SNR_MAP_RX;             /* Typedef for SNR RX for each antenna configuration. */
  typedef std::pair<SNR_MAP_TX, SNR_MAP_RX>     SNR_PAIR;               /* Typedef for SNR RX for each antenna configuration. */
  typedef std::map<Mac48Address, SNR_PAIR>      STATION_SNR_PAIR_MAP;   /* Typedef for Map between stations and their SNR Table. */
  typedef STATION_SNR_PAIR_MAP::iterator        STATION_SNR_PAIR_MAP_I; /* Typedef for iterator over SNR MAPPING Table. */
  typedef STATION_SNR_PAIR_MAP::const_iterator  STATION_SNR_PAIR_MAP_CI;/* Typedef for const iterator over SNR MAPPING Table. */

  /* Typedefs for Recording Best Antenna Configuration per Station */
  typedef ANTENNA_CONFIGURATION ANTENNA_CONFIGURATION_TX;               /* Typedef for best TX antenna configuration. */
  typedef ANTENNA_CONFIGURATION ANTENNA_CONFIGURATION_RX;               /* Typedef for best RX antenna configuration. */
  typedef std::tuple<ANTENNA_CONFIGURATION_TX, ANTENNA_CONFIGURATION_RX, double> BEST_ANTENNA_CONFIGURATION;
  typedef std::map<Mac48Address, BEST_ANTENNA_CONFIGURATION>            STATION_ANTENNA_CONFIG_MAP;
  typedef STATION_ANTENNA_CONFIG_MAP::iterator                          STATION_ANTENNA_CONFIG_MAP_I;
  typedef STATION_ANTENNA_CONFIG_MAP::const_iterator                    STATION_ANTENNA_CONFIG_MAP_CI;
  typedef std::pair<AWV_ID_TX, AWV_ID_RX>                               BEST_AWV_ID;
  typedef std::map<Mac48Address, BEST_AWV_ID>                           STATION_AWV_MAP; /* Typedef for mapping the best AWV ID for each STA. */
  typedef std::pair<uint8_t, uint8_t>                                   BEST_MIMO_ANTENNA_CONFIG_INDEX;
  typedef std::map<Mac48Address, BEST_MIMO_ANTENNA_CONFIG_INDEX>        STATION_MIMO_ANTENNA_CONFIG_INDEX_MAP; /* Typedef for mapping the index of the best antenna config in MIMO for each STA */

  //// NINA ////
  /* Typedefs for Recording Best Antenna Configuration Combinations per Station for MIMO transmission */
  typedef std::map<Mac48Address, MIMO_AWV_CONFIGURATIONS> BEST_ANTENNA_SU_MIMO_COMBINATIONS;
  typedef std::map<uint8_t, MIMO_AWV_CONFIGURATIONS>      BEST_ANTENNA_MU_MIMO_COMBINATIONS;
  //// NINA ////

  /* Typedefs for Recording SNR Value per Antenna Pattern Configuration */
  typedef AWV_CONFIGURATION                                     AWV_CONFIGURATION_TX;     /* Typedef for TX antenna pattern configuration. */
  typedef AWV_CONFIGURATION                                     AWV_CONFIGURATION_RX;     /* Typedef for RX antenna pattern configuration. */
  typedef std::pair<AWV_CONFIGURATION_TX, AWV_CONFIGURATION_RX> AWV_CONFIGURATION_TX_RX;
  typedef std::map<AWV_CONFIGURATION_TX_RX, SNR>                SNR_AWV_MAP;              /* Typedef for Map between Antenna Pattern Config and SNR. */
  typedef SNR_AWV_MAP::iterator                                 SNR_AWV_MAP_I;            /* Typedef for Map between Antenna Pattern Config and SNR. */
  typedef std::map<Mac48Address, SNR_AWV_MAP>                   STATION_SNR_AWV_MAP;      /* Typedef for Map between stations and their SNR AWV Table. */
  typedef STATION_SNR_AWV_MAP::iterator                         STATION_SNR_AWV_MAP_I;      /* Typedef for iterator over SNR MAPPING Table. */

  /**
   * Update Best Tx antenna configuration towards specific station.
   * \param stationAddress The MAC address of the peer station.
   * \param antennaConfigTx Best transmit antenna configuration.
   * \param snr the SNR value.
   */
  void UpdateBestTxAntennaConfiguration (const Mac48Address stationAddress, ANTENNA_CONFIGURATION_TX antennaConfigTx, double snr);
  /**
   * Update Best Rx antenna configuration towards specific station.
   * \param stationAddress The MAC address of the peer station.
   * \param antennaConfigRx Best receive antenna configuration.
   * \param snr the SNR value.
   */
  void UpdateBestRxAntennaConfiguration (const Mac48Address stationAddress, ANTENNA_CONFIGURATION_RX antennaConfigRx, double snr);
  /**
   * Update Best Rx antenna configuration towards specific station.
   * \param stationAddress The MAC address of the peer station.
   * \param antennaConfigTx Best transmit antenna configuration.
   * \param antennaConfigRx Best receive antenna configuration.
   * \param snr the SNR value.
   */
  void UpdateBestAntennaConfiguration (const Mac48Address stationAddress,
                                       ANTENNA_CONFIGURATION_TX txConfig, ANTENNA_CONFIGURATION_RX rxConfig, double snr);
  /**
   * Update Best Tx antenna configuration index towards specific station for MIMO communication.
   * \param stationAddress The MAC address of the peer station.
   * \param txIndex Best transmit antenna configuration index.
   */
  void UpdateBestMimoTxAntennaConfigurationIndex (const Mac48Address stationAddress, uint8_t txIndex);
  /**
   * Update Best Rx antenna configuration index towards specific station for MIMO communication.
   * \param stationAddress The MAC address of the peer station.
   * \param rxIndex Best receive antenna configuration index.
   */
  void UpdateBestMimoRxAntennaConfigurationIndex (const Mac48Address stationAddress, uint8_t rxIndex);
  /**
   * Print SNR for either Tx/Rx Antenna Configurations.
   * \param snrMap The SNR Map
   */
  void PrintSnrConfiguration (SNR_MAP &snrMap);
  /**
   * Obtain antenna configuration for the highest received SNR to feed it back
   * \param stationAddress The MAC address of the station.
   * \param isTxConfiguration Is the Antenna Tx Configuration we are searching for or not.
   */
  ANTENNA_CONFIGURATION GetBestAntennaConfiguration (const Mac48Address stationAddress, bool isTxConfiguration);
  /**
   * Obtain antenna configuration for the highest received SNR to feed it back
   * \param stationAddress The MAC address of the station.
   * \param isTxConfiguration Is the Antenna Tx Configuration we are searching for or not.
   * \param maxSnr The SNR value corresponding to the BEst Antenna Configuration.
   */
  ANTENNA_CONFIGURATION GetBestAntennaConfiguration (const Mac48Address stationAddress, bool isTxConfiguration, double &maxSnr);
  /**
   * Update Best Tx AWV ID towards specific station.
   * \param stationAddress The MAC address of the peer station.
   * \param awvIDTx Best transmit AWV ID.
   */
  void UpdateBestTxAWV (const Mac48Address stationAddress, AWV_ID_TX awvIDTx);
  /**
   * Update Best Rx AWV ID towards specific station.
   * \param stationAddress The MAC address of the peer station.
   * \param awvIDRx Best receive antenna configuration.
   */
  void UpdateBestRxAWV (const Mac48Address stationAddress, AWV_ID_RX awvIDRx);
  /**
   * Update Best Rx and Tx AWV ID towards specific station.
   * \param stationAddress The MAC address of the peer station.
   * \param awvIDTx Best transmit AWV ID.
   * \param awvIDRx Best receive AWV ID.
   */
  void UpdateBestAWV (const Mac48Address stationAddress,
                      AWV_ID_TX awvIDTx, AWV_ID_RX awvIDRx);
  /**
   * Obtain antenna pattern configuration for the highest received SNR to feed it back.
   * Used for group beamforming using TRN fields appended to beacons.
   */
  AWV_CONFIGURATION_TX_RX GetBestAntennaPatternConfiguration (Mac48Address peerAp, double &maxSnr);
  /**
   * Get Relay Capabilities Informaion for this DMG STA.
   * \return
   */
  RelayCapabilitiesInfo GetRelayCapabilitiesInfo (void) const;
  /**
   * Get DMG Relay Capabilities Element supported by this DMG STA.
   * \return The DMG Relay Capabilities Information Element.
   */
  Ptr<RelayCapabilitiesElement> GetRelayCapabilitiesElement (void) const;
  /**
   * Send Relay Search Response.
   * \param to
   * \param token The dialog token.
   */
  void SendRelaySearchResponse (Mac48Address to, uint8_t token);
  /**
   * Start Beacon Interval (BI)
   */
  virtual void StartBeaconInterval (void) = 0;
  /**
   * End Beacon Interval (BI)
   */
  virtual void EndBeaconInterval (void) = 0;
  /**
   * Start Beacon Transmission Interval (BTI)
   */
  virtual void StartBeaconTransmissionInterval (void) = 0;
  /**
   * Start Association Beamform Training (A-BFT).
   */
  virtual void StartAssociationBeamformTraining (void) = 0;
  /**
   * Start Announcement Transmission Interval (ATI).
   */
  virtual void StartAnnouncementTransmissionInterval (void) = 0;
  /**
   * Start Data Transmission Interval (DTI).
   */
  virtual void StartDataTransmissionInterval (void) = 0;
  /**
   * Get current access period in the current beacon interval
   * \return The type of the access period (BTI/A-BFT/ATI or DTI).
   */
  ChannelAccessPeriod GetCurrentAccessPeriod (void) const;
  /**
   * Get allocation type for the current period.
   * \return The type of the current allocation (SP or CBAP).
   */
  AllocationType GetCurrentAllocation (void) const;
  /**
   * Resume any Pending TXSS requests in the current CBAP allocation.
   */
  virtual void ResumePendingTXSS (void);
  /**
   * Start contention based access period (CBAP).
   * \param contentionDuration The duration of the contention period.
   */
  void StartContentionPeriod (AllocationID allocationID, Time contentionDuration);
  /**
   * End Contention Period.
   */
  void EndContentionPeriod (void);
  /**
   *BeamLink Maintenance Timeout.
   */
  virtual void BeamLinkMaintenanceTimeout (void);
  /**
   * Schedule the service period (SP) allocation blocks at the start of DTI.
   * \param blocks The number of time blocks making up the allocation.
   * \param spStart
   * \param spLength
   * \param spPeriod
   * \param length The length of the allocation period.
   * \param peerAid The AID of the peer station in this service period.
   * \param peerStation The MAC Address of the peer station in this service period.
   * \param isSource Whether we are the initiator of the service period.
   */
  void ScheduleServicePeriod (uint8_t blocks, Time spStart, Time spLength, Time spPeriod,
                              AllocationID allocationID, uint8_t peerAid, Mac48Address peerAddress, bool isSource);
  /**
   * Start service period (SP) allocation period.
   * \param length The length of the allocation period.
   * \param peerAid The AID of the peer station in this service period.
   * \param peerStation The MAC Address of the peer station in this service period.
   * \param isSource Whether we are the initiator of the service period.
   */
  void StartServicePeriod (AllocationID allocationID, Time length, uint8_t peerAid, Mac48Address peerStation, bool isSource);
  /**
   * Resume transmission for the current service period.
   */
  void ResumeServicePeriodTransmission (void);
  /**
   * Suspend ongoing transmission for the current service period.
   */
  void SuspendServicePeriodTransmission (void);
  /**
   * Add new data forwarding entry.
   * \param nextHopAddress The MAC Address of the next hop.
   */
  void AddForwardingEntry (Mac48Address nextHopAddress);
  /**
   * This function is excuted upon the transmission of frame.
   * \param hdr The header of the transmitted frame.
   */
  virtual void FrameTxOk (const WifiMacHeader &hdr);
  /**
   * This function is excuted upon the transmission of frame during SU-MIMO BFT.
   * \param hdr The header of the transmitted frame.
   */
  void FrameTxOkSuMimoBFT (const WifiMacHeader &hdr);
  /**
   * This function is excuted upon the transmission of frame during MU-MIMO BFT.
   * \param hdr The header of the transmitted frame.
   */
  void FrameTxOkMuMimoBFT (const WifiMacHeader &hdr);
  /**
   * This function is excuted upon the transmission of Short SSW frame.
   */
  void FrameTxOkShortSsw (void);
  /**
   * Handle a received packet.
   *
   * \param mpdu the received MPDU
   */
  virtual void Receive (Ptr<WifiMacQueueItem> mpdu);
  virtual void BrpSetupCompleted (Mac48Address address) = 0;
  /**
   * The packet we sent was successfully received by the receiver
   * (i.e. we received an ACK from the receiver).  If the packet
   * was an association response to the receiver, we record that
   * the receiver is now associated with us.
   *
   * \param hdr the header of the packet that we successfully sent
   */
  virtual void TxOk (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * Get frame Duration in microseconds.
   * \param duration The duration of the frame in nanoseconds.
   * \return The duration of the frame in microseconds.
   */
  Time GetFrameDurationInMicroSeconds (Time duration) const;
  /**
   * Get transmission time duration for SPR frame.
   * \return The transmission time duration for SPR frame.
   */
  Time GetSprFrameDuration (void) const;
  /**
   * Add range of MCSs to support in transmission/reception towards particular station.
   * \param address The MAC address of the peer station
   * \param firstMcs The first MCS index to support.
   * \param lastMcs The last MCS index to support.
   */
  void AddMcsSupport (Mac48Address address, uint32_t initialMcs, uint32_t lastMcs);

protected:
  STATION_SNR_PAIR_MAP m_stationSnrMap;                   //!< Map between peer stations and their SNR Table.
  STATION_ANTENNA_CONFIG_MAP m_bestAntennaConfig;         //!< Map between peer stations and the best antenna configuration.
  STATION_AWV_MAP m_bestAwvConfig;                        //!< Map between peer stations and the best AWV - to be used together with m_bestAntennaConfig.
  ANTENNA_CONFIGURATION m_feedbackAntennaConfig;          //!< Temporary variable to save the best antenna configuration of the peer station.
  double m_feedbackSnr;                                   //!< Temporary variable to save the SNR achieved with the best antenna configuration of the peer station.
  AWV_CONFIGURATION_TX m_currentTxConfig;                 //!< Variable to save the current Tx config of the AP when receiving DMG beacons with TRN-R fields.
  STATION_SNR_AWV_MAP  m_apSnrAwvMap;                     //!< Map between the APs and their SNR Tables (used in Group Beamforming).
  SNR_MAP_TX m_oldSnrTxMap;                               //!< Variable to save the old SNR map when starting a new SLS - to check if the best TX sector has changed.
  double m_maxSnr;                                        //!< Temporary variable to save the SNR corresponding to the best antenna configuration of the peer station.
  STATION_MIMO_ANTENNA_CONFIG_INDEX_MAP m_bestMimoAntennaConfig; //!< Map between peer stations and the index of the optimal MIMO combination

  /** DMG PHY Layer Information **/
  Time m_sbifs;                         //!< Short Beamforming Interframe Space.
  Time m_mbifs;                         //!< Medium Beamforming Interframe Space.
  Time m_lbifs;                         //!< Long Beamfornnig Interframe Space.
  Time m_brpifs;                        //!< Beam Refinement Protocol Interframe Spacing.

  /** EDMG PHY Layer Information **/
  bool m_chAggregation;                 //!< Flag to indicate whether channel aggregation is used or not.

  /** Channel Access Period Variables **/
  ChannelAccessPeriod m_accessPeriod;               //!< The type of the current channel access period.
  Time m_atiDuration;                               //!< The length of the ATI period.
  Time m_biStartTime;                               //!< The start time of the BI Interval.
  Time m_beaconInterval;                            //!< Interval between beacons.
  bool m_atiPresent;                                //!< Flag to indicate if ATI period is present in the current BI.
  TracedCallback<Mac48Address, Time> m_dtiStarted;  //!< Trace Callback for starting new DTI access period.
  /**
   * TracedCallback signature for DTI access period start event.
   *
   * \param address The MAC address of the station.
   * \param duration The duration of the DTI period.
   */
  typedef void (* DtiStartedCallback)(Mac48Address address, Time duration);

  /** BTI Access Period Variables **/
  uint8_t m_nextBeacon;                 //!< The number of BIs following the current beacon interval during which the DMG Beacon is not be present.
  uint8_t m_btiPeriodicity;             //!< The periodicity of the BTI in BI.
  Time m_dtiDuration;                   //!< The duration of the DTI access period.
  Time m_dtiStartTime;                  //!< The start time of the DTI access period.

  /** A-BFT Access Period Variables **/
  Time m_abftDuration;                  //!< The duration of the A-BFT access period.
  uint8_t m_ssSlotsPerABFT;             //!< Number of Sector Sweep Slots Per A-BFT.
  uint8_t m_ssFramesPerSlot;            //!< Number of SSW Frames per Sector Sweep Slot.
  uint8_t m_nextAbft;                   //!< The value of the next A-BFT in DMG Beacon.

  /** EDMG A-BFT Access Period Variables **/
  uint8_t m_abftMultiplier;             //!< Allows for extension of the total length of the A-BFT.
  uint8_t m_abftInSecondaryChannel;     //!< Indicates whether the A-BFT is also allocated on an adjacent secondary channel.

  uint16_t m_totalSectors;              //!< Total number of sectors remaining to cover.
  bool m_pcpHandoverSupport;            //!< Flag to indicate if we support PCP Handover.
  bool m_sectorFeedbackSchedulled;      //!< Flag to indicate if we schedulled SSW Feedback.
  bool m_isUnsolicitedRssEnabled;       //!< Flag to indicate whether the STA can receive unsolicited RSS.

  enum ABFT_STATE {
    WAIT_BEAMFORMING_TRAINING = 0,
    BEAMFORMING_TRAINING_COMPLETED = 1,
    BEAMFORMING_TRAINING_PARTIALLY_COMPLETED = 2,
    FAILED_BEAMFORMING_TRAINING = 3,
    START_RSS_BACKOFF_STATE = 4,
    IN_RSS_BACKOFF_STATE = 5,
  };
  ABFT_STATE m_abftState;                             //!< Beamforming Training State in A-BFT.

  /** ATI Period Variables **/
  Ptr<DmgAtiTxop> m_dmgAtiTxop;                       //!< Dedicated Txop for ATI.

  /** SLS Variables  **/
  Ptr<DmgSlsTxop> m_dmgSlsTxop;                       //!< Dedicated Txop for SLS.

  /* BRP Phase Variables */
  std::map<Mac48Address, bool> m_isBrpResponder;      //!< Map to indicate whether we are BRP Initiator or responder.
  std::map<Mac48Address, bool> m_isBrpSetupCompleted; //!< Map to indicate whether BRP Setup is completed with station.
  std::map<Mac48Address, bool> m_raisedBrpSetupCompleted;
  std::map<Mac48Address, bool> m_isMimoBrpSetupCompleted;
  bool m_recordTrnSnrValues;                          //!< Flag to indicate if we should record reported SNR Values by TRN Fields.
  bool m_requestedBrpTraining;                        //!< Flag to indicate whether BRP Training has been performed.
  bool m_executeBRPinATI;                             //!< Flag to indicate if we execute BRP in ATI phase.

  bool m_executeBRPafterSLS;                          //!< Flag to indicate we execute BRP phase after SLS.
  BRP_TRAINING_TYPE m_brpType;                        //!< The type of the BRP phase to be executed.

  /* DMG Relay Variables */
  RelayDuplexMode m_relayDuplexMode;            //!< The duplex mode of the relay.
  bool m_relayMode;                             //!< Flag to indicate we are in relay mode.
  bool m_redsActivated;                         //!< DMG Station supports REDS.
  bool m_rdsActivated;                          //!< DMG Station supports RDS.
  RelayCapableStaList m_rdsList;                //!< List of Relay STAs (RDS) in the currernt DMG BSS.
  TracedCallback<Mac48Address> m_rlsCompleted;  //!< Flag to indicate we are completed RLS setup.

  /* Information Request/Response Storage */
  typedef std::vector<Ptr<WifiInformationElement> > WifiInformationElementList;
  typedef std::pair<Ptr<DmgCapabilities>, WifiInformationElementMap> StationInformation;
  typedef std::map<Mac48Address, StationInformation> InformationMap;
  typedef InformationMap::iterator InformationMapIterator;
  typedef InformationMap::const_iterator InformationMapCI;
  InformationMap m_informationMap;              //!< List of Information Elements for all the assoicated stations.

  /* EDMG Information Request/Response */
  typedef std::pair<Ptr<EdmgCapabilities>, WifiInformationElementMap> EdmgStationInformation;
  typedef std::map<Mac48Address, EdmgStationInformation> EdmgInformationMap;
  typedef EdmgInformationMap::iterator EdmgInformationMapIterator;
  typedef EdmgInformationMap::const_iterator EdmgInformationMapCI;
  EdmgInformationMap m_edmgInformationMap;     //!< List of EDMG Information Elements for all the assoicated stations.

  /* DMG Parameteres */
  bool m_isCbapOnly;                            //!< Flag to indicate whether the DTI is allocated to CBAP.
  bool m_isCbapSource;                          //!< Flag to indicate that PCP/AP has higher priority for transmission.
  bool m_supportRdp;                            //!< Flag to indicate whether we support RDP.

  /* EDMG Parameters */
  bool m_isEdmgSupported;                       //!< Flag to indicate whether we support IEEE 802.11ay.
  uint8_t m_nextBtiWithTrn;                     //!< Number of BIs until a BTI where TRN-R fields are present in DMG Beacons.
  uint8_t m_trnScheduleInterval;                //!< Periodic interval in number of BIs at which TRN-R fields are present in DMG Beacons.

  /* EDMG Beamforming Parameters */
  TRN_SEQ_LENGTH m_trnSeqLength;                //!< The length of the Golay sequences used in the TRN fields.
  uint8_t m_edmgTrnP;                           //!< Number of TRN Subfields repeated at the start of a unit with the same AWV.
  uint8_t m_edmgTrnM;                           //!< In BRP-TX and BRP-RX/TX packets, the number of TRN Subfields that can be used for training.
  uint8_t m_edmgTrnN;                           //!< In BRP-TX packets, the number of TRN Subfields in a unit transmitted with the same AWV.
  uint8_t m_edmgTrnT;                           //!< In BRP-TX and BRP-RX/TX packets the number of TRN Subfields used for a transitional period at the start.
  uint8_t m_rxPerTxUnits;                       //!< In BRP-RX/TX packets the number of times a TRN unit is repeated for RX training at the responder.

  /* Access Period Allocations */
  AllocationFieldList m_allocationList;         //!< List of access period allocations in DTI.

  /* Service Period Channel Access */
  AllocationID m_currentAllocationID;           //!< The ID of the current allocation.
  AllocationType m_currentAllocation;           //!< The current access period allocation.
  Time m_allocationStarted;                     //!< The time we initiated the allocation.
  Time m_currentAllocationLength;               //!< The length of the current allocation period in MicroSeconds.
  uint8_t m_peerStationAid;                     //!< The AID of the peer DMG STA in the current SP.
  Mac48Address m_peerStationAddress;            //!< The MAC address of the peer DMG STA in the current SP.
  Time m_suspendedPeriodDuration;               //!< The remaining duration of the suspended SP.
  bool m_spSource;                              //!< Flag to indicate if we are the source of the SP.

  /* DMG Beamforming Variables */
  Ptr<Codebook> m_codebook;                     //!< Pointer to the beamforming codebook.
  uint8_t m_bfRetryTimes;                       //!< Counter for Beamforming retry times.
  EventId m_restartISSEvent;                    //!< Event related to restarting ISS.
  EventId m_sswFbckTimeout;                     //!< Timeout Event for receiving SSW FBCK Frame.
  EventId m_sswAckTimeoutEvent;                 //!< Event related to not receiving SSW ACK.
  bool m_isBeamformingInitiator;                //!< Flag to indicate whether we initaited BF.
  bool m_isInitiatorTXSS;                       //!< Flag to indicate whether the initiator is TXSS or RXSS.
  bool m_isResponderTXSS;                       //!< Flag to indicate whether the responder is TXSS or RXSS.
  uint8_t m_peerSectors;                        //!< The total number of the sectors in the peer station.
  uint8_t m_peerAntennas;                       //!< The total number of the antennas in the peer station.
  Time m_sectorSweepStarted;                    //!< Variable to store when the sector sweep phase started.
  Time m_sectorSweepDuration;                   //!< Variable to store when the duration of a sector sweep.
  EventId m_rssEvent;                           //!< Event related to scheduling RSS.
  bool m_antennaPatternReciprocity;             //!< Flag to indicate whether the STA supports antenna pattern reciprocity.
  bool m_performingBFT;                         //!< Flag to indicate whether we are performing BFT.
  bool m_useRxSectors;                          //!< Flag to indicate whether to use Rx beamforming sectors in the station operation.

  //// NINA ////
  /* EDMG Beamforming variables */
  uint8_t m_txssPackets;                        //!< Number of BRP packets necessary for the station to perform transmit training.
  uint8_t m_txssRepeat;                         //!< Number of times that the BRP packets need to be repeated for the station to perform receive training.
  uint8_t m_peerTxssPackets;                    //!< Number of BRP packets necessary for the peer station to perform transmit training.
  uint8_t m_peerTxssRepeat;                     //!< Number of times that the BRP packets need to be repeated for the peer station to perform receive training.
  uint8_t m_remainingTxssPackets;               //!< Number of BRP packets remaining in the current BRP TXSS
  uint8_t m_brpCdown;                           //!< BRP CDOWN of the current BRP packet.
  uint8_t m_lTxRx;                              //!< Number of TRN units requested for receive training.
  uint8_t m_peerLTxRx;                          //!< Number of TRN units requested by the peer station for receive training.
  uint8_t m_edmgTrnMRequested;                  //!< Requested value for EDMG TRN M value when sending EDMG TRN field.
  bool m_timeDomainChannelResponseRequested;    //!< If time domain channel response is requested as part of the feedback.
  NUMBER_OF_TAPS m_numberOfTapsRequested;       //!< the number of channel taps requested on the time domain channel response.

  /* MIMO BFT variables (used in SU-MIMO and MU-MIMO BFT) */
  SNR_LIST m_mimoSisoSnrList;                   //!< List of all the SNR measurements taken during the SISO phase of SU-MIMO or MU-MIMO BF training.
  uint16_t m_rxCombinationsTested;              //!< Number of Rx combinations that need to be tested during the MIMO phase of SU-MIMO BFT.
  std::vector<AntennaID> m_peerAntennaIds;      //!< The vector of antenna Ids of the peer station specifiying the DMG Antennas used to transmit the packet during MIMO BF Training.
  uint16_t m_numberOfUnitsRemaining;            //!< The number of remaining TRN Units to send in the current training.
  MIMO_SNR_LIST m_mimoSnrList;                  //!< A list to hold all the SNR values measured during MIMO phase of SU or MU MIMO BF training.

  /* SU-MIMO BFT variables */
  bool m_suMimoBeamformingTraining;                           //!< Flag to indicate whether the station is performing the SU-MIMO Beamforming training protocol.
  TracedValue<SU_MIMO_BF_TRAINING_PHASES> m_suMimoBfPhase;    //!< SU-MIMO Beamforming Training state machine.
  SU_MIMO_SNR_MAP m_suMimoSisoSnrMap;                         //!< Map to hold all the SNR values measured during the SISO phase of SU-MIMO BF training.
  MIMO_FEEDBACK_MAP m_suMimoFeedbackMap;                      //!< A map to hold all the feedback given from the peer STA for the SISO phase of SU MIMO BF Training.
  uint8_t m_txSectorCombinationsRequested;                    //!< Number of Tx sector combinations that we want to receive feedback for in the MIMO phase.
  uint8_t m_peerTxSectorCombinationsRequested;                //!< Number of Tx sector combinations that the peer station wants to receive feedback for in the MIMO phase.
  BEST_ANTENNA_SU_MIMO_COMBINATIONS m_suMimoTxCombinations;   //!< A map to store the best Tx antenna combination configuration per station for SU-MIMO transmissions;
  BEST_ANTENNA_SU_MIMO_COMBINATIONS m_suMimoRxCombinations;   //!< A map to store the best Rx antenna combination configuration per station for SU-MIMO transmissions;

  /* MU-MIMO BFT variables */
  bool m_muMimoBeamformingTraining;                           //!< Flag to indicate whether the station is performing the MU-MIMO Beamforming training protocol.
  TracedValue<MU_MIMO_BF_TRAINING_PHASES> m_muMimoBfPhase;    //!< MU-MIMO Beamforming Training state machine.
  bool m_isMuMimoInitiator;                                   //!< A flag that specifies if the STA is an initiator in the current MU-MIMO BFT;
  Time m_sisoFbckDuration;                                    //!< Variable to store the duration of the SISO Feedback phase during MU-MIMO BFT.
  EventId m_muMimoFbckTimeout;                                //!< Timeout for receiving a BRP poll frame/ BRP feedback frame/ MIMO Feedback frame during MU-MIMO BFT.
  MU_MIMO_SNR_MAP m_muMimoSisoSnrMap;                         //!< Map to hold all the SNR values measured during the SISO phase of MU-MIMO BF training.
  MIMO_FEEDBACK_MAP m_muMimoFeedbackMap;                      //!< A map to hold all the feedback given from all STAs trained during the SISO phase of MU MIMO BF Training.
  MIMO_AWV_CONFIGURATION m_mimoConfigTraining;                //!< MIMO antenna configuration to be used during MIMO training when we use spatial expansion.
  SISO_ID_SUBSET_INDEX_MAP m_sisoIdSubsetIndexMap;            //!< A map that maps the feedback antenna configuration received to its SISO ID Subset Index.
  SISO_ID_SUBSET_INDEX_RX_MAP m_sisoIdSubsetIndexRxMap;       //!< A map that maps the SISO ID Subset Index of a given feedback antenna configuration to the location of the SNR measurement in the MIMO SNR List.
  std::vector<uint16_t> m_txAwvIdList;                        //!< A list of all the Tx AWV IDs we have received feedback for.
  std::vector<uint16_t> m_sisoIdSubsetIndexList;              //!< A list of the SISO ID Subset Indexes associated with the optimal MU-MIMO configurations that need to be fed back to the responders in the Selection subphase.

  BEST_ANTENNA_MU_MIMO_COMBINATIONS m_muMimoTxCombinations;   //!< A map to store the best Tx antenna combination configuration per MU group for MU-MIMO transmissions;
  BEST_ANTENNA_MU_MIMO_COMBINATIONS m_muMimoRxCombinations;   //!< A map to store the best Rx antenna combination configuration per MU group for MU-MIMO transmissions;
  //// NINA ////

  /**
   * Trace callback for SLS phase completion.
   */
  TracedCallback<SlsCompletionAttrbitutes> m_slsCompleted;
  TracedValue<SLS_INITIATOR_STATE_MACHINE> m_slsInitiatorStateMachine;  //!< IEEE 802.11ad SLS Initiator state machine.
  TracedValue<SLS_RESPONDER_STATE_MACHINE> m_slsResponderStateMachine;  //!< IEEE 802.11ad SLS Responder state machine.
  /**
   * Trace callback for BRP phase completion.
   * \param Mac48Address The MAC address of the peer station.
   * \param BeamRefinementType The type of the beam refinement (BRP-Tx or BRP-Rx).
   * \param AntennaID The ID of the selected antenna.
   * \param SectorID The ID of the selected sector.
   * \param AWV_ID The ID of the selected custom AWV.
   */
  TracedCallback<Mac48Address, BeamRefinementType, AntennaID, SectorID, AWV_ID> m_brpCompleted;
  /**
   * Trace callback for Group Beamforming completion.
   */
  TracedCallback<GroupBfCompletionAttrbitutes> m_groupBeamformingCompleted;
  /**
   * Trace callback for SISO Phase Measurements in SU-MIMO BFT.
   * \param Mac48Address The MAC address of the peer station.
   * \param SU_MIMO_SNE_MAP The map with the measurements from the SISO phase.
   * \param uint8_t The edmg TRN N value which states how many repetitions there are of each AWV.
   * \param uint16_t The BFT ID of the current BFT.
   */
  TracedCallback<Mac48Address, SU_MIMO_SNR_MAP, uint8_t, uint16_t> m_suMimoSisoPhaseMeasurements;
  /**
   * Trace callback for SISO phase of SU-MIMO completion.
   * \param Mac48Address The MAC address of the peer station.
   * \param SU_MIMO_FEEDBACK_MAP The map with feedback from the SISO phase.
   * \param uint8_t The number of Tx antennas that we want to have concurrently active.
   * \param uint8_t The number of Rx Antennas of the peer station.
   * \param uint16_t The ID of the current BFT.
   */
  TracedCallback<Mac48Address, MIMO_FEEDBACK_MAP, uint8_t, uint8_t, uint16_t> m_suMimoSisoPhaseComplete;
  /**
   * Trace callback for MIMO candidates selection for SU-MIMO.
   * \param Mac48Address The MAC address of the peer station.
   * \param Antenna2SectorList The Tx Candidates to be tested in the MIMO phase.
   * \param Antenna2SectorList The Rx Candidates to be tested in the MIMO phase.
   * \param uint16_t The ID of the current BFT.
   */
  TracedCallback<Mac48Address, Antenna2SectorList, Antenna2SectorList, uint16_t> m_suMimomMimoCandidatesSelected;
  /**
   * Trace callback for MIMO Phase measurements in SU-MIMO BFT.
   */
  TracedCallback<MimoPhaseMeasurementsAttributes, SU_MIMO_ANTENNA2ANTENNA> m_suMimoMimoPhaseMeasurements;
  TracedCallback<Mac48Address> m_suMimoMimoPhaseComplete;
  /**
   * Trace callback for SISO Phase Measurements in MU-MIMO BFT.
   * \param Mac48Address The MAC address of the peer station.
   * \param MU_MIMO_SNR_MAP The map with the measurements from the SISO phase.
   * \param uint8_t The MU group ID for the current training.
   * \param uint16_t The BFT ID of the current BFT.
   */
  TracedCallback<Mac48Address, MU_MIMO_SNR_MAP, uint8_t, uint16_t> m_muMimoSisoPhaseMeasurements;
  /**
   * Trace callback for SISO phase of MU-MIMO completion.
   * \param MU_MIMO_FEEDBACK_MAP The map with feedback from the SISO phase.
   * \param uint8_t The number of Tx antennas that we want to have concurrently active.
   * \param uint8_t The number of Rx Stations we trained with.
   * \param uint8_t The MU group ID for the current training.
   * \param uint16_t The BFT ID of the current BFT.
   */
  TracedCallback<MIMO_FEEDBACK_MAP, uint8_t, uint8_t, uint8_t, uint16_t> m_muMimoSisoPhaseComplete;
  /**
   * Trace callback for MIMO candidates selection for MU-MIMO.
   * \param uint8_t The MU Group ID of the group being trained.
   * \param Antenna2SectorList The Tx Candidates to be tested in the MIMO phase.
   * \param uint16_t The BFT ID of the current BFT.
   */
  TracedCallback<uint8_t, Antenna2SectorList, uint16_t> m_muMimomMimoCandidatesSelected;
  /**
   * Trace callback for MIMO Phase measurements in MU-MIMO BFT.
   */
  TracedCallback<MimoPhaseMeasurementsAttributes, uint8_t> m_muMimoMimoPhaseMeasurements;
  /**
   * Trace callback for MIMO candidates selection for MU-MIMO.
   * \param MIMO_AWV_CONFIGURATION The optimal MIMO configuration.
   * \param uint8_t The MU Group ID of the group being trained.
   * \param uint16_t The BFT ID of the current BFT.
   * \param MU_MIMO_ANTENNA2RESPONDER The matching between the antenna IDs and the responders they will transmit to.
   * \param bool Whether the node is the initiator or a responder in the MU-MIMO training
   */
  TracedCallback<MIMO_AWV_CONFIGURATION, uint8_t, uint16_t, MU_MIMO_ANTENNA2RESPONDER, bool> m_muMimoOptimalConfig;
  TracedCallback<> m_muMimoMimoPhaseComplete;
  /**
   * Trace callback for polling from Inititator during SISO Fbck phase of MU-MIMO BFT.
   * \param Mac48Address The MAC address of the Initiator of the MU-MIMO BFT.
   */
  TracedCallback<Mac48Address> m_muMimoSisoFbckPolled;
  /**
   * Register MU MIMO SISO Fbck polled callback.
   * \param callback The SISO Fbck polled callback.
   */
  void RegisterMuMimoSisoFbckPolled (Mac48Address from);

  /**
   * Register MU MIMO SISO Phase complete callback.
   * \param callback The SISO phase complete callback.
   */
  void RegisterMuMimoSisoPhaseComplete (MIMO_FEEDBACK_MAP muMimoFbckMap, uint8_t nRFChains, uint8_t nStas, uint8_t muGroup, uint16_t bftID);

  /** Link Maintenance Variabeles **/
  BeamLinkMaintenanceUnitIndex m_beamlinkMaintenanceUnit;   //!< Link maintenance unit according to 802.11ad-2012.
  uint8_t m_beamlinkMaintenanceValue;                       //!< Link maintenance timer value in MicroSeconds.
  Time dot11BeamLinkMaintenanceTime;                        //!< Link maintenance timer in MicroSeconds.

  /**
   * Typedef for Link Maintenance Information.
   */
  struct BeamLinkMaintenanceInfo {
    void rest (void)
      {
        beamLinkMaintenanceTime = negotiatedValue;
      }
    Time negotiatedValue;                                   //!< Negotiated link maintenance time value in MicroSeconds.
    Time beamLinkMaintenanceTime;                           //!< Remaining link maintenance time value in MicroSeconds.
  };

  typedef std::map<uint8_t, BeamLinkMaintenanceInfo> BeamLinkMaintenanceTable;
  typedef BeamLinkMaintenanceTable::iterator BeamLinkMaintenanceTableI;
  BeamLinkMaintenanceTable m_beamLinkMaintenanceTable;
  EventId m_beamLinkMaintenanceTimeout;                     //!< Event ID related to the timer assoicated to the current beamformed link.
  bool m_currentLinkMaintained;                             //!< Flag to indicate whether the current beamformed link is maintained.
  BeamLinkMaintenanceInfo m_linkMaintenanceInfo;            //!< Current beamformed link maintenance information.

  /**
   * TracedCallback signature for BeamLink Maintenance Timer state change.
   *
   * \param state The state of the beam link maintenance timer.
   * \param aid The AID of the peer station.
   * \param address The MAC address of the peer station.
   * \param timeLeft The amount of remaining time in the current beam link maintenance timer.
   */
  typedef void (* BeamLinkMaintenanceTimerStateChanged)(BEAM_LINK_MAINTENANCE_TIMER_STATE, uint8_t aid,
                                                        Mac48Address address, Time timeLeft);
  TracedCallback<BEAM_LINK_MAINTENANCE_TIMER_STATE, uint8_t, Mac48Address, Time> m_beamLinkMaintenanceTimerStateChanged;

  /* Beam Refinement variables */
  typedef std::vector<double> TRN2SNR;                  //!< Typedef for vector of SNR values for TRN Subfields.
  typedef TRN2SNR::const_iterator TRN2SNR_CI;
  typedef std::map<Mac48Address, TRN2SNR> TRN2SNR_MAP;  //!< Typedef for map of TRN2SNR per station.
  typedef TRN2SNR_MAP::const_iterator TRN2SNR_MAP_CI;
  TRN2SNR m_trn2Snr;                                    //!< Variable to store SNR per TRN subfield for ongoing beam refinement phase or beam tracking.
  TRN2SNR_MAP m_trn2snrMap;                             //!< Variable to store SNR vector for TRN Subfields per device.
  Mac48Address m_peerStation;     /* The address of the station we are waiting BRP Response from */
  typedef std::map<Mac48Address, uint16_t> BFT_ID_MAP;  //!< Typedef for a MAP that holds a BFT ID (identifying the current BFT process) per station.
  BFT_ID_MAP m_bftIdMap;
  typedef std::map<uint8_t, uint16_t> MU_MIMO_BFT_ID_MAP;  //!< Typedef for a MAP that holds a BFT ID (identifying the current BFT process) per MU group.
  MU_MIMO_BFT_ID_MAP m_muMimoBftIdMap;

  /* AID to MAC Address mapping */
  typedef std::map<uint16_t, Mac48Address> AID_MAP;
  typedef std::map<Mac48Address, uint16_t> MAC_MAP;
  typedef AID_MAP::const_iterator AID_MAP_CI;
  typedef MAC_MAP::const_iterator MAC_MAP_CI;
  AID_MAP m_aidMap;                                     //!< Map AID to MAC Address.
  MAC_MAP m_macMap;                                     //!< Map MAC Address to AID.

  /* Data Forwarding Table */
  typedef struct {
    Mac48Address nextHopAddress;
    bool isCbapPeriod;
  } AccessPeriodInformation;

  typedef std::map<Mac48Address, AccessPeriodInformation> DataForwardingTable;
  typedef DataForwardingTable::iterator DataForwardingTableIterator;
  typedef DataForwardingTable::const_iterator DataForwardingTableCI;
  DataForwardingTable m_dataForwardingTable;

  /**
   * TracedCallback signature for service period initiation/termination.
   * \param srcAddress The MAC address of the source station.
   * \param dstAddress The MAC address of the destination station.
   */
  typedef void (* ServicePeriodCallback)(Mac48Address srcAddress, Mac48Address dstAddress);
  TracedCallback<Mac48Address, Mac48Address> m_servicePeriodStartedCallback;
  TracedCallback<Mac48Address, Mac48Address> m_servicePeriodEndedCallback;

  /* Association Traces */
  typedef void (* AssociationCallback)(Mac48Address address, uint16_t);
  TracedCallback<Mac48Address, uint16_t> m_assocLogger; //!< Trace callback when a station associates.
  TracedCallback<Mac48Address> m_deAssocLogger;         //!< Trace callback when a station deassociates.

  /* EMDG MIMO Variables */
  Ptr<EDMGGroupIDSetElement> m_edmgGroupIdSetElement;
  EDMGGroupTuple m_edmgMuGroup; //!< The MU group which is currently being trained in MU-MIMO BFT training.
  typedef std::map <uint16_t, bool> MU_GROUP_MAP;
  MU_GROUP_MAP m_edmgMuGroupMap; //!< A map that specifies if a member from the MU group should be a a part of the MIMO phase or not
  AID_LIST_I m_currentMuGroupMember; //!< Iterator that points to the current member from the MU group being polled for feedback/transmitted to.
  typedef std::map<Mac48Address, DATA_COMMUNICATION_MODE> DataCommunicationModeTable;
  DataCommunicationModeTable m_dataCommunicationModeTable; //!< Table that contains the current communication mode (SISO, SU-MIMO or MU-MIMO) with a given station.
  EventId m_informationUpdateEvent;                  //!< Event for Information Update.
  Time m_informationUpdateTimeout;                   //!< Information Update timeout
private:
  /**
   * This function is called upon transmission of a 802.11 Management frame.
   * \param hdr The header of the management frame.
   */
  void ManagementTxOk (const WifiMacHeader &hdr);
  //// NINA ////
  /**
   * Report SNR Value, this is a callback to be hooked with DmgWifiPhy class.
   * \param patternList A list of the active antenna patterns for each antenna that is active.
   * \param snr A vector of measured SNRs with all with a specific Tx-Rx combination over a specific TRN.
   */
  void ReportMimoSnrValue (AntennaList patternList, std::vector<double> snr);
  //// NINA ////
  /**
   * Report SNR Value in SU-MIMO training, this is a callback to be hooked with DmgWifiPhy class.
   * \param antennaID The ID of the phased antenna array.
   * \param sectorID The ID of the sector within the phased antenna array.
   * \param trnUnitsRemaining The number of remaining TRN Units.
   * \param subfieldsRemaining The number of remaining TRN Subfields within the TRN Unit.
   * \param pSubfieldsRemaining The number of remaining P subfields in the TRN Unit. (Always 0 for DMG TRN Fields).
   * \param snr The measured SNR over specific TRN.
   * \param isTxTrn Flag that indicates whether we are receiving TRN-Tx or TRN-Rx/TX fields (transmit training) or TRN-Rx(no transmit training).
   * \param index Helps us calculate the AWV Feedback for EDMG TRN-Tx and TRN-Rx packets (multiple packets with same AWV).
   */
  void ReportSnrValue (AntennaID antennaID, SectorID sectorID,
                       uint8_t trnUnitsRemaining, uint8_t subfieldsRemaining,
                       uint8_t pSubfieldsRemaining, double snr, bool isTxTrn, uint8_t index);

  uint8_t m_dialogToken;          //!< The token of the current dialog.

};

} // namespace ns3

#endif /* DMG_WIFI_MAC_H */
