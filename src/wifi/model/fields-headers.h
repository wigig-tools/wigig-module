/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef FIELDS_HEADERS_H
#define FIELDS_HEADERS_H

#include "ns3/header.h"
#include "ns3/mac48-address.h"

namespace ns3 {

/***********************************
 *    Sector Sweep Field (8.4a.1)
 ***********************************/

typedef enum {
  BeamformingInitiator = 0,
  BeamformingResponder = 1
} BeamformingDirection;

/**
 * \ingroup wifi
 * \brief Sector Sweep (SSW) Field.
 */
class DMG_SSW_Field : public ObjectBase
{
public:
  DMG_SSW_Field ();
  ~DMG_SSW_Field ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  void Print(std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
  * Set the direction field in the SSW field.
  * \param dir 0 = the frame is transmiited by the beamforming initiator.
  *		 1 = the frame is transmiited by the beamforming responder.
  */
  void SetDirection (BeamformingDirection dir);
  /**
  * Set the Count Down (CDOWN) field. the number of remaining DMG beacon frame transmissions to the end
  * of TXSS, or the number of the remaining SSW frame transmissions to the end of the TXSS/RXSS.
  * \param cdown This field is set to 0 in the last frame DMG Beacon and SSW frame transmission.
  * Possible values range from 0 to 511.
  */
  void SetCountDown (uint16_t cdown);
  /**
  * Set Sector ID (SID).
  * \param sid indicates the sector number through which the frame containing this SSW field is transmitted.
  */
  void SetSectorID (uint8_t sid);
  /**
  * Set the DMG Antenna ID.
  * \param antenna_id the DMG antenna the transmitter is currently using for this transmission.
  */
  void SetDMGAntennaID (uint8_t antenna_id);
  /**
  * Set the Receive Sector Sweep (RXSS) Length.
  * \param length the length of a receive sector sweep as required by the transmitting STA.
  * It is defined in units of a SSW frame
  */
  void SetRXSSLength (uint8_t length);

  /**
  * Get the direction field in the SSW field.
  *
  * \return Whether the frame is transmitted by Beamforming Initiator or Responder.
  */
  BeamformingDirection GetDirection (void) const;
  /**
  * Get the Count Down (CDOWN) field value.
  * \return the number of remaining DMG beacon frame transmissions to the end of TXSS,
  * or the number of the remaining SSW frame transmissions to the end of the TXSS/RXSS.
  */
  uint16_t GetCountDown (void) const;
  /**
  * Get Sector ID (SID) value.
  * \return the sector number through which the frame containing this SSW field is transmitted.
  */
  uint8_t GetSectorID (void) const;
  /**
  * Get the DMG Antenna ID value.
  * \return the DMG antenna the transmitter is currently using for this transmission.
  */
  uint8_t GetDMGAntennaID (void) const;
  /**
  * Get the RXSS Length.
  * \return the length of a receive sector sweep as required by the transmitting STA.
  * It is defined in units of a SSW frame
  */
  uint8_t GetRXSSLength (void) const;

private:
  BeamformingDirection m_dir;
  uint16_t m_cdown;
  uint8_t m_sid;
  uint8_t m_antenna_id;
  uint8_t m_length;

};

/****************************************
 * Dynamic Allocation Info Field (8.4a.2)
 ****************************************/

typedef enum  {
  SERVICE_PERIOD_ALLOCATION = 0,
  CBAP_ALLOCATION = 1
} AllocationType;

typedef uint8_t AllocationID;

/**
 * \ingroup wifi
 * \brief Dynamic Allocation Information Field.
 */
class DynamicAllocationInfoField : public ObjectBase
{
public:
  DynamicAllocationInfoField ();
  ~DynamicAllocationInfoField ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
  * Set the TID field that identifies the TC or TS for the allocation request or grant.
  * \param tid The value of the Traffic Identifier field.
  */
  void SetTID (uint8_t tid);
  /**
  * Set the allocation type field.
  * \param
  */
  void SetAllocationType (AllocationType value);
  /**
  * Set source AID.
  * \param
  */
  void SetSourceAID (uint8_t aid);
  /**
  * Setd destination AID.
  * \param
  */
  void SetDestinationAID (uint8_t aid);
  /**
  * Set the furation of the current allocation .
  * \param
  */
  void SetAllocationDuration (uint16_t duration);
  /**
  * Set the value of the reserved field .
  * \param
  */
  void SetReserved (uint8_t reserved);

  uint8_t GetTID (void) const;
  AllocationType GetAllocationType (void) const;
  uint8_t GetSourceAID (void) const;
  uint8_t GetDestinationAID (void) const;
  uint16_t GetAllocationDuration (void) const;
  uint8_t GetReserved (void);

private:
  uint8_t m_tid;
  AllocationType m_allocationType;
  uint8_t m_sourceAID;
  uint8_t m_destinationAID;
  uint16_t m_allocationDuration;
  uint8_t m_reserved;

};

typedef std::vector<DynamicAllocationInfoField> DynamicAllocationInfoList;
typedef DynamicAllocationInfoList::const_iterator DynamicAllocationInfoListCI;

/*******************************************
 *    Sector Sweep Feedback Field (8.4a.3)
 *******************************************/

/**
 * \ingroup wifi
 * \brief Sector Sweep Feedback Field.
 */
class DMG_SSW_FBCK_Field : public ObjectBase
{
public:
  static TypeId GetTypeId (void);
  DMG_SSW_FBCK_Field ();
  virtual ~DMG_SSW_FBCK_Field ();

  virtual TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
  * Set the total number of sectors the initiator uses in the ISS or the ID of
  * the frame that was received with best quality in the preceding sector sweep.
  * \param value the total number of sectors the initiator uses in the ISS or the ID of the best sector.
  */
  void SetSector (uint16_t value);
  /**
  * Set the number of RX DMG Antennas in ISS or Select DMG Antenna in case
  * \param antennas the number of RX DMG antennas the initiator uses during the following RSS.
  */
  void SetDMGAntenna (uint8_t antennas);
  /**
  * Set the SNR Report in case not ISS or the reserved value in case ISS.
  * \param antennas
  */
  void SetSNRReport (uint8_t antennas);
  /**
  * Set whether a non-PCP/non-AP STA requires the PCP/AP to initiate the communication.
  * \param value
  */
  void SetPollRequired (bool value);
  /**
  * Set the value of the reserved subfield in the SSW Feedback field.
  * \param value
  */
  void SetReserved (uint8_t value);
  /**
  * Set whether the SSW Feedback Field is transmitted as part of ISS.
  * \param value True if the SSW Feedback Field is transmitted as part of ISS, otherwise false.
  */
  void IsPartOfISS (bool value);
  uint16_t GetSector (void) const;
  uint8_t GetDMGAntenna (void) const;
  bool GetPollRequired (void) const;
  uint8_t GetReserved (void) const;

private:
  uint16_t m_sectors;
  uint8_t m_antennas;
  uint8_t m_snr_report;
  bool m_poll_required;
  uint8_t m_reserved;
  bool m_iss;

};

/***********************************
 *    BRP Request Field (8.4a.4).
 ***********************************/

/**
 * \ingroup wifi
 * \brief Beam Refinement Protocol Request Field.
 */
class BRP_Request_Field : public ObjectBase
{
public:
  BRP_Request_Field ();
  ~BRP_Request_Field ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  void SetL_RX (uint8_t value);
  /**
   * \param value The TX-TRN-REQ field is set to 1 to indicate that the STA needs transmit training as part
   * of beam refinement. Otherwise, it is set to 0.
   */
  void SetTX_TRN_REQ (bool value);
  /**
   * If the MID-REQ field is set to 0, the L-RX field indicates the compressed number of TRN-R subfields
   * requested by the transmitting STA as part of beam refinement. To obtain the desired number of TRN-R
   * subfields, the value of the L-RX field is multiplied by 4. Possible values range from 0 to 16, corresponding
   * to 0 to 64 TRN-R fields. Other values are reserved. If the field is set to 0, the transmitting STA does not need
   * receiver training as part of beam refinement. If the MID-REQ field is set to 1, the L-RX field indicates the
   * compressed number of AWV settings that the STA uses during the MID phase. To obtain the number of
   * AWVs that is used, the value of the L-RX field is multiplied by 4.
   */
  void SetMID_REQ (bool value);
  /**
   * A STA sets the BC-REQ field to 1 in SSW-Feedback or BRP frames to indicate a request for an I/R-BC
   * subphase; otherwise, the STA sets the field to 0 to indicate it is not requesting an I/R-BC subphase. In case
   * an R-BC subphase is requested, the STA can include information on the TX sector IDs to be used by the
   * STA receiving this request. The STA receiving this request sets the BC-grant field in SSW-ACK or BRP
   * frames to 1 to grant this request; otherwise, the STA sets it to 0 to reject the request.
   */
  void SetBC_REQ (bool value);
  void SetMID_Grant (bool value);
  void SetBC_Grant (bool value);
  void SetChannel_FBCK_CAP (bool value);
  void SetTXSectorID (uint8_t value);
  void SetOtherAID (uint8_t value);
  void SetTXAntennaID (uint8_t value);
  void SetReserved (uint8_t value);

  uint8_t GetL_RX (void);
  bool GetTX_TRN_REQ (void);
  bool GetMID_REQ (void);
  bool GetBC_REQ (void);
  bool GetMID_Grant (void);
  bool GetBC_Grant (void);
  bool GetChannel_FBCK_CAP (void);
  uint8_t GetTXSectorID (void);
  uint8_t GetOtherAID (void);
  uint8_t GetTXAntennaID (void);
  uint8_t GetReserved (void);

private:
  uint8_t m_L_RX;
  bool m_TX_TRN_REQ;
  bool m_MID_REQ;
  bool m_BC_REQ;
  bool m_MID_Grant;
  bool m_BC_Grant;
  bool m_Channel_FBCK_CAP;
  uint8_t m_TXSectorID;
  uint8_t m_OtherAID;
  uint8_t m_TXAntennaID;
  uint8_t m_Reserved;

};

/***************************************
 * Beamforming Control Field (8.4a.5)
 ***************************************/

/**
 * \ingroup wifi
 * \brief The Beamforming Control Field.
 */
class BF_Control_Field : public ObjectBase
{
public:
  BF_Control_Field (void);
  virtual ~BF_Control_Field (void);

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  void SetBeamformTraining (bool value);
  void SetAsInitiatorTxss (bool value);
  void SetAsResponderTxss (bool value);

  void SetTotalNumberOfSectors (uint8_t sectors);
  void SetNumberOfRxDmgAntennas (uint8_t antennas);

  void SetRxssLength (uint8_t length);
  void SetRxssTxRate (bool rate);

  bool IsBeamformTraining (void) const;
  bool IsInitiatorTxss (void) const;
  bool IsResponderTxss (void) const;

  uint8_t GetTotalNumberOfSectors (void) const;
  uint8_t GetNumberOfRxDmgAntennas (void) const;

  uint8_t GetRxssLength (void) const;
  bool GetRxssTxRate (void) const;

private:
  bool m_beamformTraining;
  bool m_isInitiatorTxss;
  bool m_isResponderTxss;

  /* BF Control Fields when both IsInitiatorTXSS and IsResponderTXSS subfields are
   * equal to 1 and the BF Control field is transmitted in Grant or Grant ACK frames */
  uint8_t m_sectors;
  uint8_t m_antennas;

  /* BF Control field format in all other cases */
  uint8_t m_rxssLength;
  bool m_rxssTXRate;

  uint8_t m_reserved;

};

/***************************************
 * Beamformed Link Maintenance (8.4a.6)
 ***************************************/

typedef enum {
  UNIT_32US = 0,
  UNIT_2000US = 1
} BeamLinkMaintenanceUnitIndex;

/**
 * \ingroup wifi
 * \brief The Beamformed Link Maintenance provides the DMG STA with the value of dot11BeamLinkMaintenanceTime.
 */
class BF_Link_Maintenance_Field : public ObjectBase
{
public:
  BF_Link_Maintenance_Field ();
  ~BF_Link_Maintenance_Field ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
  * Set the encoding of the BeamLink Maintenance Unit Index.
  * \param index The encoding of the BeamLink Maintenance Unit Index.
  */
  void SetUnitIndex (BeamLinkMaintenanceUnitIndex index);
  /**
  * Set the number of RX DMG Antennas in ISS or Select DMG Antenna in case
  * \param value The number of RX DMG antennas the initiator uses during the following RSS.
  */
  void SetMaintenanceValue (uint8_t value);
  /**
  * Set the BeamLink isMaster field is set to 1 to indicate that the DMG STA is the
  * master of the data transfer and set to 0 if the DMG STA is a slave of the data transfer.
  * \param value Inidicate whether the DMG STA is the master or not.
  */
  void SetAsMaster (bool value);

  BeamLinkMaintenanceUnitIndex GetUnitIndex (void) const;
  uint8_t GetMaintenanceValue (void) const;
  bool IsMaster (void) const;

private:
  BeamLinkMaintenanceUnitIndex m_unitIndex;
  uint8_t m_value;
  bool m_isMaster;

};

/******************************************
* Beacon Clustering Control Field (8-34c&d)
*******************************************/

typedef enum {
  NOT_PARTICIPATING = 0,
  SYNC_PCP_AP = 1,
  PARTICIPATING = 2
} ClusterMemberRole;

/**
 * \ingroup wifi
 * Implement the header for Clustering Control Field.
 */
class ExtDMGClusteringControlField : public ObjectBase
{
public:
  ExtDMGClusteringControlField ();
  ~ExtDMGClusteringControlField ();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  void SetDiscoveryMode (bool value);
  bool GetDiscoveryMode (void) const;

  /** Clustering Control field format if the Discovery Mode is False **/

  /**
   * If ECPAC Policy Enforced field is set to 0, the Beacon SP Duration subfield indicates the duration, in units
   * of 8 μs, of the Beacon SPs in the cluster. If ECPAC Policy Enforced field is set to 1, the Beacon SP Duration
   * subfield indicates the maximum duration, in units of 8 μs, of the beacon header interval (BHI) of the BSS,
   * and the minimum duration of Beacon SPs in the cluster (see 9.34.2.2).
   * \param duration
   */
  void SetBeaconSpDuration (uint8_t duration);
  /**
   * The cluster to which the transmitter of the Clustering Control field belongs is identified by the Cluster ID
   * subfield. The MAC address of the synchronization PCP (S-PCP)/S-AP is the Cluster ID of the cluster.
   * \param clusterID
   */
  void SetClusterID (Mac48Address clusterID);
  /**
   * The Cluster Member Role subfield identifies the role that the transmitting STA assumes within the cluster.
   * A value of 0 means that the STA is currently not participating in clustering. A value of 1 means that the STA
   * acts as the S-PCP/S-AP of the cluster. A value of 2 means that the STA participates in the cluster, but not as
   * the S-PCP/S-AP. The value 3 is reserved.
   * \param role
   */
  void SetClusterMemberRole (ClusterMemberRole role);
  /**
   * The ClusterMaxMem subfield defines the maximum number of PCPs and/or APs, including the S-PCP/S-AP, that can
   * participate in the cluster. The value of the ClusterMaxMem subfield is computed in relation to the beacon interval
   * value (9.34.2). The value 0 is reserved. Values 8 and above are reserved if the ECPAC Policy Enforced field is set
   * to 0. The value 1 is assigned to the S-PCP/S-AP.
   * \param max
   */
  void SetClusterMaxMem (uint8_t max);
  void SetReserved (uint16_t value);

  uint8_t GetBeaconSpDuration (void) const;
  Mac48Address GetClusterID (void) const;
  ClusterMemberRole GetClusterMemberRole (void) const;
  uint8_t GetClusterMaxMem (void) const;
  uint16_t GetReserved (void) const;

  /** Clustering Control field format if the Discovery Mode is True **/

  /**
   * The A-BFT Responder Address subfield contains the MAC address of the STA that is allowed to transmit
   * during the A-BFT, if present, that follows the BTI.
   * \param address The MAC address of the A-BFT Responder Address.
   */
  void SetABFT_ResponderAddress (Mac48Address address);

  Mac48Address GetABFT_ResponderAddress (void) const;

private:
  bool m_discoveryMode;
  /* With Discovery Mode Disabled */
  uint8_t m_beaconSpDuration;
  Mac48Address m_clusterID;
  uint8_t m_clusterMemberRole;
  uint8_t m_clusterMaxMem;

  /* With Discovery Mode Enabled */
  Mac48Address m_responderAddress;
  uint16_t m_reserved;

};

} // namespace ns3

#endif /* FIELDS_HEADERS_H */
