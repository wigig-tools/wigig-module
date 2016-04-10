/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Hany Assasa <Hany.assasa@gmail.com>
 */
#ifndef DMG_INFORMATION_ELEMENTS_H
#define DMG_INFORMATION_ELEMENTS_H

#include <stdint.h>
#include "ns3/attribute-helper.h"
#include "ns3/buffer.h"
#include "ns3/mac48-address.h"
#include "ns3/wifi-information-element.h"
#include "fields-headers.h"
#include "map"

namespace ns3 {

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 Request Element 8.4.2.13
 */
class RequestElement : public WifiInformationElement
{
public:
  RequestElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * Add Element ID to the list of request elements.
   * \param id The ID of the requested element.
   */
  void AddRequestElementID (WifiInformationElementId id);
  /**
   * \return The list of elements that are requested to be included in the frame.
   * The Requested Element IDs are listed in order of increasing element ID
   */
  WifiInformationElementIdList GetWifiInformationElementIdList (void) const;
  /**
   * \return The number of reqested IEs.
   */
  size_t GetNumberOfRequestedIEs (void) const;

private:
  WifiInformationElementIdList m_list;

};

std::ostream &operator << (std::ostream &os, const RequestElement &element);
std::istream &operator >> (std::istream &is, RequestElement &element);

ATTRIBUTE_HELPER_HEADER (RequestElement)

enum TimeoutIntervalType
{
  ReassociationDeadlineInterval = 1,
  KeyLifetimeInterval = 2,
  AssociationComebackTime = 3,
};

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 Timeout Interval Element (TIE) 8.4.2.51
 */
class TimeoutIntervalElement : public WifiInformationElement
{
public:
  TimeoutIntervalElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * Set the Timeout Interval type field.
   * \param type
   */
  void SetTimeoutIntervalType (enum TimeoutIntervalType type);
  /**
   * The Timeout Interval Value field contains an unsigned 32-bit integer.
   * \param value
   */
  void SetTimeoutIntervalValue (uint32_t value);

  enum TimeoutIntervalType GetTimeoutIntervalType (void) const;
  uint32_t GetTimeoutIntervalValue (void) const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;

private:
  uint8_t m_timeoutIntervalType;
  uint32_t m_timeoutIntervalValue;

};

std::ostream &operator << (std::ostream &os, const TimeoutIntervalElement &element);
std::istream &operator >> (std::istream &is, TimeoutIntervalElement &element);

ATTRIBUTE_HELPER_HEADER (TimeoutIntervalElement)

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad DMG Opereation Element 8.4.2.131
 */
class DmgOperationElement : public WifiInformationElement
{
public:
  DmgOperationElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /* DMG Operation Information */
  /**
   * The TDDTI (time division data transfer interval) field is set to 1 if the STA, while operating as a
   * PCP/AP, is capable of providing channel access as defined in 9.33.6 and 10.4 and is set to 0 otherwise.
   * \param tddti
   */
  void SetTDDTI (bool tddti);
  /**
   * The Pseudo-static Allocations field is set to 1 if the STA, while operating as a PCP/AP, is capable of
   * providing pseudo-static allocations as defined in 9.33.6.4 and is set to 0 otherwise. The Pseudo-static
   * Allocations field is set to 1 only if the TDDTI field in the DMG PCP/AP Capability Information field is set
   * to 1. The Pseudo-static Allocations field is reserved if the TDDTI field in the DMG PCP/AP Capability
   * Information field is set to 0.
   * \param pseudoStatic
   */
  void SetPseudoStaticAllocations (bool pseudoStatic);
  /**
   * The PCP Handover field is set to 1 if the PCP provides PCP Handover as defined in 10.28.2 and is set
   * to 0 ifthe PCP does not provide PCP Handover.
   * \param handover
   */
  void SetPcpHandover (bool handover);

  bool GetTDDTI () const;
  bool GetPseudoStaticAllocations () const;
  bool GetPcpHandover () const;

  void SetDmgOperationInformation (uint16_t info);
  uint16_t GetDmgOperationInformation () const;

  /* DMG BSS Parameter Configuration */
  void SetPSRequestSuspensionInterval (uint8_t interval);
  void SetMinBHIDuration (uint16_t duration);
  void SetBroadcastSTAInfoDuration (uint8_t duration);
  void SetAssocRespConfirmTime (uint8_t time);
  void SetMinPPDuration (uint8_t duration);
  void SetSPIdleTimeout (uint8_t timeout);
  void SetMaxLostBeacons (uint8_t max);

  uint8_t GetPSRequestSuspensionInterval () const;
  uint16_t GetMinBHIDuration () const;
  uint8_t GetBroadcastSTAInfoDuration () const;
  uint8_t GetAssocRespConfirmTime () const;
  uint8_t GetMinPPDuration () const;
  uint8_t GetSPIdleTimeout () const;
  uint8_t GetMaxLostBeacons () const;

  void SetDmgBssParameterConfiguration (uint64_t config);
  uint64_t GetDmgBssParameterConfiguration () const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;

private:
  /* DMG Operation Information */
  bool m_tddti;
  bool m_pseudo;
  bool m_handover;
  /* DMG BSS Parameter Configuration */
  uint8_t m_psRequestSuspensionInterval;
  uint16_t m_minBHIDuration;
  uint8_t m_broadcastSTAInfoDuration;
  uint8_t m_assocRespConfirmTime;
  uint8_t m_minPPDuration;
  uint8_t m_spIdleTimeout;
  uint8_t m_maxLostBeacons;

};

std::ostream &operator << (std::ostream &os, const DmgOperationElement &element);
std::istream &operator >> (std::istream &is, DmgOperationElement &element);

ATTRIBUTE_HELPER_HEADER (DmgOperationElement)

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad DMG Beam Refinement Element 8.4.2.132
 */
class BeamRefinementElement : public WifiInformationElement
{
public:
  BeamRefinementElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * \param initiator A value of 1 in the Initiator field indicates that the sender
   * is the beam refinement initiator. Otherwise, this field is set to 0.
   */
  void SetAsBeamRefinementInitiator (bool initiator);
  /**
   * \param response A value of 1 in the TX-train-response field indicates that this
   * packet is the response to a TX training request. Otherwise, this field is set to 0.
   */
  void SetTxTrainResponse (bool response);
  /**
   * \param response A value of 1 in the RX-train-response field indicates that the
   * packet serves as  an acknowledgment for a RX training request.
   * Otherwise, this field is set to 0.
   */
  void SetRxTrainResponse (bool response);
  /**
   * \param value A value of 1 in the TX-TRN-OK field confirms a previous training
   * request received by a STA. Otherwise, this field is set to 0.
   */
  void SetTxTrnOk (bool value);
  /**
   * \param feedback A value of 1 in the TXSS-FBCK-REQ field indicates a request for
   * transmit sector sweep feedback.
   */
  void SetTxssFbckReq (bool feedback);
  /**
   * \param The BS-FBCK field specifies the sector that resulted in the best receive
   * quality in the last received BRP-TX packet. If the last received packet was not
   * a BRP-TX packet, this field is set to 0. The determination of the best sector
   * is implementation dependent.
   */
  void SetBsFbck (uint8_t feedback);
  /**
   * \param The BS-FBCK Antenna ID field specifies the antenna ID corresponding to the
   *  sector indicated by the BF-FBCK field.
   */
  void SetBsFbckAntennaID (uint8_t id);

  /* FBCK-REQ field */
  void SetSnrRequested (bool requested);
  void SetChannelMeasurementRequested (bool requested);
  void SetNumberOfTapsRequested (uint8_t number);
  void SetSectorIdOrderRequested (bool present);

  /* FBCK-TYPE field */
  void SetSnrPresent (bool present);
  void SetChannelMeasurementPresent (bool present);
  void SetTapDelayPresent (bool present);
  void SetNumberOfTapsPresent (uint8_t number);
  void SetNumberOfMeasurements (uint8_t number);
  void SetSectorIdOrderPresent (bool present);
  void SetNumberOfBeams (uint8_t number);

  /**
   * \param mid A value of 1 in the MID Extension field indicates the intention to continue
   *  transmitting BRP-RX packets during the MID subphases. Otherwise, this field is set to 0.
   */
  void SetMidExtension (bool mid);
  /**
   * \param mid A value of 1 in the Capability Request field indicates that the transmitter
   * of the frame requests the intended receiver to respond with a BRP frame that includes
   * the BRP Request field. Otherwise, this field is set to 0.
   */
  void SetCapabilityRequest (bool mid);

  bool IsBeamRefinementInitiator (void) const;
  bool IsTxTrainResponse (void) const;
  bool IsRxTrainResponse (void) const;
  bool IsTxTrnOk (void) const;
  bool IsTxssFbckReq (void) const;
  uint8_t GetBsFbck (void) const;
  uint8_t GetBsFbckAntennaID (void) const;

  /* FBCK-REQ field */
  bool IsSnrRequested (void) const;
  bool IsChannelMeasurementRequested (void) const;
  uint8_t GetNumberOfTapsRequested (void) const;
  bool IsSectorIdOrderRequested (void) const;

  /* FBCK-TYPE field */
  bool IsSnrPresent (void) const;
  bool IsChannelMeasurementPresent (void) const;
  bool IsTapDelayPresent (void) const;
  uint8_t GetNumberOfTapsPresent (void) const;
  uint8_t GetNumberOfMeasurements (void) const;
  bool IsSectorIdOrderPresent (void) const;
  uint8_t GetNumberOfBeams (void) const;

  bool IsMidExtension (void) const;
  bool IsCapabilityRequest (void) const;

private:
  bool m_initiator;
  bool m_txTrainResponse;
  bool m_rxTrainResponse;
  bool m_txTrnOk;
  bool m_txssFbckReq;
  uint8_t m_bsFbck;
  uint8_t m_bsFbckAntennaId;
  /* FBCK-REQ field format */
  bool m_snrRequested;
  bool m_channelMeasurementRequested;
  uint8_t m_numberOfTapsRequested;
  bool m_sectorIdOrderRequested;
  /* FBCK-TYPE field */
  bool m_snrPresent;
  bool m_channelMeasurementPresent;
  bool m_tapDelayPresent;
  uint8_t m_numberOfTapsPresent;
  uint8_t m_numberOfMeasurements;
  bool m_sectorIdOrderPresent;
  uint8_t m_numberOfBeams;

  bool m_midExtension;
  bool m_capabilityRequest;

};

std::ostream &operator << (std::ostream &os, const BeamRefinementElement &element);
std::istream &operator >> (std::istream &is, BeamRefinementElement &element);

ATTRIBUTE_HELPER_HEADER (BeamRefinementElement)

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Wakeup Schedule Element 8.4.2.133
 */
class WakeupScheduleElement : public WifiInformationElement
{
public:
  WakeupScheduleElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * Set the lower order 4 octets of the TSF timer at the start of the next Awake BI.
   * \param time
   */
  void SetBiStartTime (uint32_t time);
  /**
   * Set the sleep cycle duration for the non-PCP STA in beacon intervals, i.e., the sum of Awake BIs and
   * Doze BIs that make up the sleep cycle. The Sleep Cycle field value can only be a power of two.
   * Other values are reserved.
   * \param cycle
   */
  void SetSleepCycle (uint16_t cycle);
  /**
   * Set the Number of Awake/Doze BIs at the beginning of each sleep cycle.
   * \param number
   */
  void SetNumberOfAwakeDozeBIs (uint16_t number);

  uint32_t GetBiStartTime () const;
  uint16_t GetSleepCycle () const;
  uint16_t GetNumberOfAwakeDozeBIs () const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;

private:
  uint32_t m_biStarTime;
  uint16_t m_sleepCycle;
  uint16_t m_numberBIs;

};

std::ostream &operator << (std::ostream &os, const WakeupScheduleElement &element);
std::istream &operator >> (std::istream &is, WakeupScheduleElement &element);

ATTRIBUTE_HELPER_HEADER (WakeupScheduleElement)

/******************************************
*    Allocation Field Format (8-401aa)
*******************************************/

/**
 * \ingroup wifi
 * Implement the header for Relay Capable STA Info Field.
 */
class AllocationField
{
public:
  AllocationField ();

  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  void SetAllocationID (uint8_t id);
  void SetAllocationType (AllocationType type);
  void SetAsPseudoStatic (bool value);
  void SetAsTruncatable (bool value);
  void SetAsExtendable (bool value);
  void SetPcpActive (bool value);
  void SetLpScUsed (bool value);

  uint8_t GetAllocationID (void) const;
  AllocationType GetAllocationType (void) const;
  bool IsPseudoStatic (void) const;
  bool IsTruncatable (void) const;
  bool IsExtendable (void) const;
  bool IsPcpActive (void) const;
  bool IsLpScUsed (void) const;

  void SetAllocationControl (uint16_t ctrl);
  uint16_t GetAllocationControl (void) const;

  void SetBfControl (BF_Control_Field &field);
  /**
   * The Source AID field is set to the AID of the STA that initiates channel access during the SP or CBAP
   * allocation or, in the case of a CBAP allocation, can also be set to the broadcast AID if all STAs are allowed
   * to initiate transmissions during the CBAP allocation.
   * \param aid
   */
  void SetSourceAid (uint8_t aid);
  /**
   * The Destination AID field indicates the AID of a STA that is expected to communicate with the source
   * DMG STA during the allocation or broadcast AID if all STAs are expected to communicate with the source
   * DMG STA during the allocation. The broadcast AID asserted in the Source AID and the Destination AID
   * fields for an SP allocation indicates that during the SP a non-PCP/non-AP STA does not transmit unless it
   * receives a Poll or Grant frame from the PCP/AP.
   * \param aid
   */
  void SetDestinationAid (uint8_t aid);
  /**
   * The Allocation Start field contains the lower 4 octets of the TSF at the time the SP or CBAP starts. The
   * Allocation Start field can be specified at a future beacon interval when the pseudo-static field is set to 1.
   * \param start
   */
  void SetAllocationStart (uint32_t start);
  /**
   * The Allocation Block Duration field indicates the duration, in microseconds, of a contiguous time block for
   * which the SP or CBAP allocation is made and cannot cross beacon interval boundaries. Possible values
   * range from 1 to 32 767 for an SP allocation and 1 to 65535 for a CBAP allocation.
   * \param start
   */
  void SetAllocationBlockDuration (uint16_t duration);
  /**
   * The Number of Blocks field contains the number of time blocks making up the allocation.
   * \param number
   */
  void SetNumberOfBlocks (uint8_t number);
  /**
   * The Allocation Block Period field contains the time, in microseconds, between the start of two consecutive
   * time blocks belonging to the same allocation. The Allocation Block Period field is reserved when the
   * Number of Blocks field is set to 1.
   * \param period
   */
  void SetAllocationBlockPeriod (uint16_t period);

  BF_Control_Field GetBfControl (void) const;
  uint8_t GetSourceAid (void) const;
  uint8_t GetDestinationAid (void) const;
  uint32_t GetAllocationStart (void) const;
  uint16_t GetAllocationBlockDuration (void) const;
  uint8_t GetNumberOfBlocks (void) const;
  uint16_t GetAllocationBlockPeriod (void) const;

private:
  /* Allocation Control Subfield*/
  uint8_t m_allocationID;                 //!< Allocation ID
  uint8_t m_allocationType;               //!< Allocation Type
  bool m_pseudoStatic;                    //!< Pseudostatic
  bool m_truncatable;                     //!< Truncatable
  bool m_extendable;                      //!< Extendable
  bool m_pcpActive;                       //!< PCP Active
  bool m_lpScUsed;                        //!< Low Power SC.

  BF_Control_Field m_bfControl;           //!< BF Control.
  uint8_t m_SourceAid;                   //!< Source STA AID.
  uint8_t m_destinationAid;              //!< Destination STA AID.
  uint16_t m_allocationStart;             //!< Allocation Start.
  uint16_t m_allocationBlockDuration;     //!< Allocation Block Duration.
  uint8_t m_numberOfBlocks;               //!< Number of Blocks.
  uint16_t m_allocationBlockPeriod;       //!< Allocation Block Period.

};

typedef std::vector<AllocationField> AllocationFieldList;

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Extended Schedule Element 8.4.2.134
 *
 * Because the length parameter supports only 255 octets of payload in an element, the PCP/AP can
 * split the Allocation fields into more than one Extended Schedule element entry in the same DMG
 * Beacon or Announce frame. Despite this splitting, the set of Extended Schedule element entries
 * conveyed within a DMG Beacon and Announce frame is considered to be a single schedule for the
 * beacon interval, and in this standard referred to simply as Extended Schedule element unless
 * otherwise noted. The Allocation fields are ordered by increasing allocation start time with
 * allocations beginning at the same time arbitrarily ordered.
 */
class ExtendedScheduleElement : public WifiInformationElement
{
public:
  ExtendedScheduleElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void AddAllocationField (AllocationField &field);
  void SetAllocationFieldList (const AllocationFieldList &list);

  AllocationFieldList GetAllocationFieldList (void) const;

private:
  AllocationFieldList m_list;

};

std::ostream &operator << (std::ostream &os, const ExtendedScheduleElement &element);
std::istream &operator >> (std::istream &is, ExtendedScheduleElement &element);

ATTRIBUTE_HELPER_HEADER (ExtendedScheduleElement)

/**
 * \ingroup wifi
 * Implement the header for Relay Capable STA Info Field.
 */
class StaInfoField
{
public:
  StaInfoField ();

  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
   * Set the AID of the STA for which availability is indicated.
   * \param aid
   */
  void SetAid (uint8_t aid);
  /**
   * Set CBAP field to 1 to indicate that the STA is available to receive transmissions during CBAPs or not.
   * \param value
   */
  void SetCbap (bool value);
  /**
   * Set Polling Phase to 1 to indicate that the STA is available during PPs and is set to 0 otherwise.
   * \param value
   */
  void SetPollingPhase (bool value);
  void SetReserved (uint8_t value);

  uint8_t GetAid (void) const;
  bool GetCbap (void) const;
  bool GetPollingPhase (void) const;
  uint8_t GetReserved (void) const;

private:
  uint8_t m_aid;
  bool m_cbap;
  bool m_pp;
  uint8_t m_reserved;

};

typedef std::vector<StaInfoField> StaInfoFieldList;

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Extended Schedule Element 8.4.2.135
 *
 * The STA Availability element is used by a non-PCP/non-AP STA to inform a PCP/AP about the STA
 * availability during the subsequent CBAPs (9.33.5) and to indicate participation in the Dynamic allocation of
 * service periods (9.33.7). The PCP/AP uses the STA Availability element to inform the non-PCP/non-AP
 * STAs of other STAs availability during subsequent CBAPs and participation of other STAs in the Dynamic
 * allocation of service periods.
 */
class StaAvailabilityElement : public WifiInformationElement
{
public:
  StaAvailabilityElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void AddStaAvailabilityElement (StaInfoField &field);
  void SetAllocationFieldList (const StaInfoFieldList &list);

  StaInfoFieldList GetAllocationFieldList (void) const;

private:
  StaInfoFieldList m_list;

};

std::ostream &operator << (std::ostream &os, const StaAvailabilityElement &element);
std::istream &operator >> (std::istream &is, StaAvailabilityElement &element);

ATTRIBUTE_HELPER_HEADER (StaAvailabilityElement)

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Next DMG ATI Element 8.4.2.137
 */
class NextDmgAti : public WifiInformationElement
{
public:
  NextDmgAti ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void SetStartTime (uint32_t time);
  void SetAtiDuration (uint16_t duration);

  uint32_t GetStartTime () const;
  uint16_t GetAtiDuration () const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;

private:
  uint32_t m_startTime;
  uint16_t m_atiDuration;

};

std::ostream &operator << (std::ostream &os, const NextDmgAti &element);
std::istream &operator >> (std::istream &is, NextDmgAti &element);

ATTRIBUTE_HELPER_HEADER (NextDmgAti)

typedef enum {
  SessionType_InfrastructureBSS = 0,
  SessionType_IBSS = 1,
  SessionType_DLS = 2,
  SessionType_TDLS = 3,
  SessionType_PBSS = 4
} SessionType;

typedef enum {
  Band_TV_white_Spaces = 0, /* TV White Spaces */
  Band_Sub_1_GHz,           /* Sub-1 GHz (excluding TV white spaces) */
  Band_2_4GHz,              /* 2.4 GHz */
  Band_3_6GHz,              /* 3.6 GHz */
  Band_4_9GHz,              /* 4.9 and 5 GHz */
  Band_60GHz                /* 60 GHz */
} BandID;

typedef struct {
  uint8_t Band_ID;
  uint8_t Setup;
  uint8_t Operation;
} Band;

/* SNR */
typedef uint8_t SNR;
typedef std::vector<SNR> SNR_LIST;
typedef SNR_LIST::const_iterator SNR_LIST_ITERATOR;

/* Channel Measurement */
typedef uint8_t I_COMPONENT;
typedef uint8_t Q_COMPONENT;
typedef std::pair<I_COMPONENT, Q_COMPONENT> TAP_COMPONENTS;
typedef std::vector<TAP_COMPONENTS> TAP_COMPONENTS_LIST;
typedef TAP_COMPONENTS_LIST::iterator TAP_COMPONENTS_LIST_ITERATOR;
typedef std::vector<TAP_COMPONENTS_LIST> CHANNEL_MEASUREMENT_LIST;
typedef CHANNEL_MEASUREMENT_LIST::const_iterator CHANNEL_MEASUREMENT_LIST_ITERATOR;

/* Tap Delay */
typedef uint8_t TAP_DELAY;
typedef std::vector<TAP_DELAY> TAP_DELAY_LIST;
typedef TAP_DELAY_LIST::const_iterator TAP_DELAY_LIST_ITERATOR;

/* Sector ID Order */
typedef uint8_t SECTOR_ID;
typedef uint8_t ANTENNA_ID;
typedef std::pair<SECTOR_ID, ANTENNA_ID> SECTOR_ID_ORDER;
typedef std::vector<SECTOR_ID_ORDER> SECTOR_ID_ORDER_LIST;
typedef SECTOR_ID_ORDER_LIST::const_iterator SECTOR_ID_ORDER_LIST_ITERATOR;

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Channel Measurement Feedback element 8.4.2.138
 * The Channel Measurement Feedback element is used to carry the channel measurement feedback data that
 * the STA has measured on the TRN-T fields of the BRP packet that contained the Channel Measurement
 * request, to provide a list of sectors identified during a sector sweep, or during beam combination (9.35.6.3).
 * The format and size of the Channel Measurement Feedback element are defined by the parameter values
 * specified in the accompanying DMG Beam Refinement element.
 */
class ChannelMeasurementFeedbackElement : public WifiInformationElement
{
public:
  ChannelMeasurementFeedbackElement ();
  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void AddSnrItem (SNR snr);
  void AddChannelMeasurementItem (TAP_COMPONENTS_LIST taps);
  void AddTapDelayItem (TAP_DELAY item);
  void AddSectorIdOrder (SECTOR_ID_ORDER order);

  SNR_LIST GetSnrList (void) const;
  CHANNEL_MEASUREMENT_LIST GetChannelMeasurementList (void) const;
  TAP_DELAY_LIST GetTapDelayList (void) const;
  SECTOR_ID_ORDER_LIST GetSectorIdOrderList (void) const;

private:
  SNR_LIST m_snrList;
  CHANNEL_MEASUREMENT_LIST m_channelMeasurementList;
  TAP_DELAY_LIST m_tapDelayList;
  SECTOR_ID_ORDER_LIST m_sectorIdOrderList;

};

std::ostream &operator << (std::ostream &os, const ChannelMeasurementFeedbackElement &element);
std::istream &operator >> (std::istream &is, ChannelMeasurementFeedbackElement &element);

ATTRIBUTE_HELPER_HEADER (ChannelMeasurementFeedbackElement)

/* Typedef for Channel Measurement Feedback Element List */
typedef std::vector<ChannelMeasurementFeedbackElement *> ChannelMeasurementFeedbackElementList;

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Awake Window Element 8.4.2.139
 */
class AwakeWindowElement : public WifiInformationElement
{
public:
  AwakeWindowElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * Set the length of the Awake Window measured in microseconds.
   * \param window
   */
  void SetAwakeWindow (uint16_t window);

  uint16_t GetAwakeWindow () const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;

private:
  uint16_t m_awakeWindow;

};

std::ostream &operator << (std::ostream &os, const AwakeWindowElement &element);
std::istream &operator >> (std::istream &is, AwakeWindowElement &element);

ATTRIBUTE_HELPER_HEADER (AwakeWindowElement)

enum StaRole
{
  ROLE_AP = 0,
  ROLE_TDLS_STA = 1,
  ROLE_IBSS_STA = 2,
  ROLE_PCP = 3,
  ROLE_NON_PCP_NON_AP = 4
};

enum ConnectionCapability
{
  CAPABILITY_AP = 1,
  CAPABILITY_PCP = 2,
  CAPABILITY_DLS = 4,
  CAPABILITY_TDLS = 8,
  CAPABILITY_IBSS = 12
};

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Multi-band 8.4.2.140
 */
class MultiBandElement : public WifiInformationElement
{
public:
  MultiBandElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /* Multi-band Control field */
  void SetStaRole (enum StaRole role);
  void SetStaMacAddressPresent (bool present);
  void SetPairwiseCipherSuitePresent (bool present);

  enum StaRole GetStaRole (void) const;
  bool IsStaMacAddressPresent (void) const;
  bool IsPairwiseCipherSuitePresent (void) const;

  void SetMultiBandControl (uint8_t control);
  uint8_t GetMultiBandControl () const;

  /* Multi-band element Fields */
  void SetBandID (BandID id);
  void SetOperatingClass (uint8_t operating);
  void SetChannelNumber (uint8_t number);
  void SetBssID (Mac48Address bss);
  void SetBeaconInterval (uint16_t interval);
  void SetTsfOffset (uint64_t offset);
  /**
   * Set the Multi-band Connection Capability. The Multi-band Connection Capability field indicates the
   * connection capabilities supported by the STA on the channel and band indicated in this element.
   * @param capability
   */
  void SetConnectionCapability (uint8_t capability);
  void SetFstSessionTimeout (uint8_t timeout);
  void SetStaMacAddress (Mac48Address address);
  void SetPairwiseCipherSuiteCount (uint16_t count);
  void SetPairwiseCipherSuiteList (uint32_t count);

  BandID GetBandID (void) const;
  uint8_t GetOperatingClass (void) const;
  uint8_t GetChannelNumber (void) const;
  Mac48Address GetBssID (void) const;
  uint16_t GetBeaconInterval (void) const;
  uint64_t GetTsfOffset (void) const;
  uint8_t GetConnectionCapability () const;
  uint8_t GetFstSessionTimeout (void) const;
  Mac48Address GetStaMacAddress (void) const;
  uint16_t GetPairwiseCipherSuiteCount (void) const;  /* Not Implemented Yet */
  uint32_t GetPairwiseCipherSuiteList (void) const;   /* Not Implemented Yet */

private:
  uint8_t m_staRole;
  bool m_staMacAddressPresent;
  bool m_pairWiseCipher;
  uint8_t m_bandID;
  uint8_t m_operatingClass;
  uint8_t m_channelNumber;
  Mac48Address m_bssID;
  uint16_t m_beaconInterval;
  uint64_t m_tsfOffset;
  uint8_t m_connectionCapability;
  uint8_t m_fstSessionTimeout;
  Mac48Address m_staMacAddress;

};

std::ostream &operator << (std::ostream &os, const MultiBandElement &element);
std::istream &operator >> (std::istream &is, MultiBandElement &element);

ATTRIBUTE_HELPER_HEADER (MultiBandElement)

typedef std::vector<uint8_t> NextPcpAidList;

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Next PCP List Element 8.4.2.142
 */
class NextPcpListElement : public WifiInformationElement
{
public:
  NextPcpListElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The Token field is set to 0 when the PBSS is initialized and incremented each time the NextPCP List is updated.
   * \param token
   */
  void SetToken (uint8_t token);
  /**
   * Each AID of NextPCP i field contains the AID value of a STA. The AID values are listed in the order
   * described in 10.28.2.
   * \param aid
   */
  void AddNextPcpAid (uint8_t aid);

  uint8_t GetToken () const;
  NextPcpAidList GetListOfNextPcpAid (void) const;

private:
  uint8_t m_token;
  NextPcpAidList m_list;

};

std::ostream &operator << (std::ostream &os, const NextPcpListElement &element);
std::istream &operator >> (std::istream &is, NextPcpListElement &element);

ATTRIBUTE_HELPER_HEADER (NextPcpListElement)

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Next PCP Handover Element 8.4.2.143
 */
class PcpHandoverElement : public WifiInformationElement
{
public:
  PcpHandoverElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The Old BSSID field contains the BSSID of the PBSS from which control is being handed over.
   * \param id
   */
  void SetOldBssID (Mac48Address id);
  /**
   * The New PCP Address field indicates the MAC address of the new PCP following a handover.
   * \param address
   */
  void SetNewPcpAddress (Mac48Address address);
  /**
   * The Remaining BIs field indicates the number of beacon intervals, from the beacon interval in which this
   * element is transmitted, remaining until the handover takes effect.
   * \param number
   */
  void SetRemainingBIs (uint8_t number);

  Mac48Address GetOldBssID (void) const;
  Mac48Address GetNewPcpAddress (void) const;
  uint8_t GetRemainingBIs (void) const;

private:
  Mac48Address m_oldBssID;
  Mac48Address m_newPcpAddress;
  uint8_t m_remaining;

};

std::ostream &operator << (std::ostream &os, const PcpHandoverElement &element);
std::istream &operator >> (std::istream &is, PcpHandoverElement &element);

ATTRIBUTE_HELPER_HEADER (PcpHandoverElement);

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad DMG Link Margin element 8.4.2.144
 */

typedef enum {
  NO_CHANGE_PREFFERED = 0,
  CHANGED_MCS,
  DECREASED_TRANSMIT_POWER,
  INCREASED_TRANSMIT_POWER,
  FAST_SESSION_TRANSFER,
  POWER_CONSERVE,
  PERFORM_SLS
} Activity;

class LinkMarginElement : public WifiInformationElement
{
public:
  LinkMarginElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The Activity field is set to a preferred action that the STA sending this element recommends that the peer
   * STA indicated in the RA field of the Link Measurement Report frame execute. The method by which the
   * sending STA determines a suitable action for the peer STA is implementation specific.
   * \param activity
   */
  void SetActivity (Activity activity);
  /**
   * The MCS field is set to an MCS value that the STA sending this element recommends that the peer STA
   * indicated in the RA field of the Link Measurement Report frame use to transmit frames to this STA. The
   * reference PER for selection of the MCS is 10^(-2) for an MPDU length of 4096 octets. The method by which
   * the sending STA determines a suitable MCS for the peer STA is implementation specific.
   * \param mcs
   */
  void SetMcs (uint8_t mcs);
  /**
   * The Link Margin field contains the measured link margin of data frames received from the peer STA
   * indicated in the RA field of the Link Measurement Report frame and is coded as a twos complement signed
   * integer in units of decibels. A value of –128 indicates that no link margin is provided. The measurement
   * method of link margin is beyond the scope of this standard.
   * \param margin
   */
  void SetLinkMargin (uint8_t margin);
  /**
   * Set the value of SNR. The SNR field indicates the SNR measured during the reception of a PHY packet.
   * Values are from –13 dB to 50.75 dB in 0.25 dB steps.
   * \param snr
   */
  void SetSnr (uint8_t snr);
  /**
   * The Reference Timestamp field contains the lower 4 octets of the TSF timer value sampled at the instant that
   * the MAC received the PHY-CCA.indication(IDLE) signal that corresponds to the end of the reception of the
   * PPDU that was used to generate the feedback information contained in the Link Measurement Report frame.
   * \param timestamp
   */
  void SetReferenceTimestamp (uint32_t timestamp);

  Activity GetActivity (void) const;
  uint8_t GetMcs (void) const;
  uint8_t GetLinkMargin (void) const;
  uint8_t GetSnr (void) const;
  uint32_t GetReferenceTimestamp (void) const;

private:
  Activity m_activity;
  uint8_t m_mcs;
  uint8_t m_linkMargin;
  uint8_t m_snr;
  uint8_t m_timestamp;

};

std::ostream &operator << (std::ostream &os, const LinkMarginElement &element);
std::istream &operator >> (std::istream &is, LinkMarginElement &element);

ATTRIBUTE_HELPER_HEADER (LinkMarginElement);

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad DMG Link Adaptation Acknowledgment element 8.4.2.145
 *
 */
class LinkAdaptationAcknowledgment : public WifiInformationElement
{
public:
  LinkAdaptationAcknowledgment ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The Activity field is set to the action that the STA sending this element has executed following the reception
   * of the recommended activity in a Link Measurement Report frame. The method by which the sending STA
   * determines the action is described in 9.37 and the Activity field is defined in 8.4.2.144.2.
   * \param activity
   */
  void SetActivity (Activity activity);
  /**
   * The Reference Timestamp field contains the lower 4 octets of the TSF timer value sampled at the instant that
   * the MAC received the PHY-CCA.indication(IDLE) signal that corresponds to the end of the reception of the
   * PPDU that was used to generate the feedback information contained in the Link Measurement Report frame.
   * \param timestamp
   */
  void SetReferenceTimestamp (uint32_t timestamp);

  Activity GetActivity (void) const;
  uint32_t GetReferenceTimestamp (void) const;

private:
  Activity m_activity;
  uint8_t m_timestamp;

};

std::ostream &operator << (std::ostream &os, const LinkAdaptationAcknowledgment &element);
std::istream &operator >> (std::istream &is, LinkAdaptationAcknowledgment &element);

ATTRIBUTE_HELPER_HEADER (LinkAdaptationAcknowledgment);

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Switching Stream Element 8.4.2.146
 */

struct StreamID
{
  uint8_t TID;
  bool direction;
};

struct SwitchingParameters
{
  struct StreamID OldBandStreamID;
  struct StreamID NewBandStreamID;
  bool IsNewBandValid;
  bool LltType;
  uint8_t Reserved;
};

typedef std::vector<struct SwitchingParameters> SwitchingParametersList;

class SwitchingStreamElement : public WifiInformationElement
{
public:
  SwitchingStreamElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * Set the frequency band to which the information carried in this element is related.
   * \param id
   */
  void SetOldBandID (BandID id);
  /**
   * Set the frequency band to which the information contained in the Stream ID in New Band
   * subfield of this element is related.
   * \param id
   */
  void SetNewBandID (BandID id);
  /**
   * Set the Non-QoS Data Frames field specifies whether non-QoS data frames can be transmitted in the frequency
   * band indicated in the New Band ID field. If the Non-QoS Data Frames field is set to 0, non-QoS data frames
   * cannot be transmitted in the frequency band indicated in the New Band ID field. If the Non-QoS Data
   * Frames field is set to 1, non-QoS data frames can be transmitted in the frequency band indicated in the New
   * Band ID field.
   * \param number
   */
  void SetNonQosDataFrames (uint8_t frames);
  /**
   * Set the number of streams to be switched
   * \param number
   */
  void SetNumberOfStreamsSwitching (uint8_t number);

  BandID GetOldBandID (void) const;
  BandID GetNewBandID (void) const;
  uint8_t GetNonQosDataFrames (void) const;
  uint8_t GetNumberOfStreamsSwitching (void) const;

  void AddSwitchingParametersField (struct SwitchingParameters &parameters);
  SwitchingParametersList GetSwitchingParametersList (void) const;

private:
  uint8_t m_oldBandID;
  uint8_t m_newBandID;
  uint8_t m_nonQosDataFrames;
  uint8_t m_numberOfStreamsSwitching;

  /* Switching Parameters */
  SwitchingParametersList m_switchingParametersList;

};

std::ostream &operator << (std::ostream &os, const SwitchingStreamElement &element);
std::istream &operator >> (std::istream &is, SwitchingStreamElement &element);

ATTRIBUTE_HELPER_HEADER (SwitchingStreamElement)

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Session Transition Element 8.4.2.147
 */
class SessionTransitionElement : public WifiInformationElement
{
public:
  SessionTransitionElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void SetFstsID (uint32_t id);
  void SetSessionControl (uint8_t ctrl);
  void SetSessionControl (SessionType sessionType, bool switchIntent);
  void SetNewBand (Band &newBand);
  void SetOldBand (Band &oldBand);

  uint32_t GetFstsID (void) const;
  uint8_t GetSessionControl (void) const;
  SessionType GetSessionType (void) const;
  bool GetSwitchIntenet (void) const;
  Band GetNewBand (void) const;
  Band GetOldBand (void) const;

  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  uint16_t GetSerializedSize () const;

private:
  uint32_t m_fstsID;
  SessionType m_sessionType;
  bool m_switchIntent;
  Band m_newBand;
  Band m_oldBand;

};

std::ostream &operator << (std::ostream &os, const SessionTransitionElement &element);
std::istream &operator >> (std::istream &is, SessionTransitionElement &element);

ATTRIBUTE_HELPER_HEADER (SessionTransitionElement)

typedef enum
{
  RELAY_FD_AF = 1, /* Full Duplex, Amplify and Forward */
  RELAY_HD_DF = 2, /* Half Duplex, Decode and Forward */
  RELAY_BOTH = 3
} RelayDuplexMode;


/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Relay Capabilities Information Element 8.4.2.150
 *
 * A STA that intends to participate in relay operation (10.35) advertises its capabilities
 * through the Relay Capabilities element.
 *
 */
class RelayCapabilitiesInfo
{
public:
  RelayCapabilitiesInfo ();

  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize () const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
   * The Relay Supportability field indicates whether the STA is capable of relaying by transmitting and
   * receiving frames between a pair of other STAs. A STA capable of relaying is named “relay STA.” This field
   * is set to 1 if the STA supports relaying; otherwise, it is set to 0.
   * \param value
   */
  void SetRelaySupportability (bool value);
  /**
   * The Relay Usability field indicates whether the STA is capable of using frame-relaying through a relay
   * STA. It is set to 1 if the STA supports transmission and reception of frames through a relay STA; otherwise,
   * it set to 0.
   * \param value
   */
  void SetRelayUsability (bool value);
  /**
   * The Relay Permission field indicates whether the PCP/AP allows relay operation (10.35) to be used within
   * the PCP/AP’s BSS. It is set to 0 if relay operation is not allowed in the BSS; otherwise, it is set to 1. This
   * field is reserved when transmitted by a non-PCP/non-AP STA.
   * \param value
   */
  void SetRelayPermission (bool value);
  /**
   * The A/C Power field indicates whether the STA is capable of obtaining A/C power. It is set to 1 if the STA
   * is capable of being supplied by A/C power; otherwise, it is set to 0.
   * \param value
   */
  void SetAcPower (bool value);
  /**
   * The Relay Preference field indicates whether the STA prefers to become RDS rather than REDS. It is set to
   * 1 if a STA prefers to be RDS; otherwise, it is set to 0.
   * \param value
   */
  void SetRelayPreference (bool value);
  /**
   * The Duplex field indicates whether the STA is capable of full-duplex/amplify-and-forward (FD-AF) or half-
   * duplex/decode-and-forward (HD-DF). It is set to 1 if the STA is not capable of HD-DF but is capable of
   * only FD-AF. It is set to 2 if the STA is capable of HD-DF but is not capable of FD-AF. It is set to 3 if the
   * STA is capable of both HD-DF and FD-AF. The value 0 is reserved.
   * \param value
   */
  void SetDuplex (RelayDuplexMode duplex);
  /**
   * The Cooperation field indicates whether a STA is capable of supporting Link cooperating. It is set to 1 if the\
   * STA supports both Link cooperating type and Link switching type. It is set to 0 if a STA supports only Link
   * switching or if the Duplex field is set to 1.
   * \param value
   */
  void SetCooperation (bool value);

  bool GetRelaySupportability (void) const;
  bool GetRelayUsability (void) const;
  bool GetRelayPermission (void) const;
  bool GetAcPower (void) const;
  bool GetRelayPreference (void) const;
  RelayDuplexMode GetDuplex (void) const;
  bool GetCooperation (void) const;

private:
  bool m_supportability;
  bool m_usability;
  bool m_permission;
  bool m_acPower;
  bool m_preference;
  uint8_t m_duplex;
  bool m_cooperation;

};

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Relay Capabilities Information Element 8.4.2.150
 *
 * A STA that intends to participate in relay operation (10.35) advertises its capabilities
 * through the Relay Capabilities element.
 *
 */
class RelayCapabilitiesElement : public WifiInformationElement
{
public:
  RelayCapabilitiesElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void SetRelayCapabilitiesInfo (RelayCapabilitiesInfo &info);
  RelayCapabilitiesInfo GetRelayCapabilitiesInfo (void) const;

private:
  RelayCapabilitiesInfo m_info;

};

std::ostream &operator << (std::ostream &os, const RelayCapabilitiesElement &element);
std::istream &operator >> (std::istream &is, RelayCapabilitiesElement &element);

ATTRIBUTE_HELPER_HEADER (RelayCapabilitiesElement)

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Relay Transfer Parameter Set Information Element 8.4.2.151
 *
 * A source REDS that intends to transfer frames via an RDS advertises the parameters for the relay operation
 * with the transmission of a Relay Transfer Parameter Set element (10.35).
 *
 */
class RelayTransferParameterSetElement : public WifiInformationElement
{
public:
  RelayTransferParameterSetElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The Duplex-Mode subfield indicates that the source REDS set the duplex mode of the RDS involved in
   * RLS. The Duplex-Mode subfield value is set to 0 if the RDS operates in HD-DF mode. It is set to 1 if the
   * RDS operates in FD-AF mode.
   * \param duplex
   */
  void SetDuplexMode (bool mode);
  /**
   * The Cooperation-Mode subfield indicates whether the source REDS sets the link cooperating of the RDS
   * involved in RLS. This subfield is valid only when the RDS is capable of link cooperating type and Duplex-
   * Mode subfield is set to 0. Otherwise, this subfield is reserved. The Cooperation-Mode subfield value is set to
   * 0 if the RDS operates in link switching type. It is set to 1 if the RDS operates in link cooperation type.
   * \param value
   */
  void SetCooperationMode (bool mode);
  /**
   * The Tx-Mode subfield indicates that the source REDS sets the transmission mode of the RDS involved in
   * RLS. This subfield is valid only when the RDS is capable of link–switching type and Duplex-Mode subfield
   * is set to 1. Otherwise, this subfield is reserved. The Tx-Mode subfield value is set to 0 if a group of three
   * STAs involved in the RLS operates in Normal mode and is set to 1 if the group operates in Alternation mode.
   * \param mode
   */
  void SetTxMode (bool mode);
  /**
   * The Link Change Interval subfield indicates when the link of frame transmission between source REDS and
   * destination REDS is changed. From the start position of one reserved contiguous SP, every time instant of
   * Link Change Interval can have an opportunity to change the link. Within one Link Change Interval, only one
   * link is used for frame transfer. The unit of this field is microseconds. This subfield is used only when the
   * group involved in the RLS operates in link switching type.
   * \param value
   */
  void SetLinkChangeInterval (uint8_t interval);
  /**
   * The Data Sensing Time subfield indicates the defer time offset from the time instant of the next Link Change
   * Interval when the link switching occurs. By default, it is set to SIFS plus SBIFS. This subfield is used only
   * when the STAs involved in the RLS operate in link switching with Tx-Mode that is 0.
   * \param time
   */
  void SetDataSensingTime (uint8_t time);
  /**
   * The First Period subfield indicates the period of the source REDS-RDS link in which the source REDS and
   * RDS exchange frames. This subfield is used only when HD-DF RDS operates in link switching type.
   * \param period
   */
  void SetFirstPeriod (uint16_t period);
  /**
   * The Second Period subfield indicates the period of the RDS-destination REDS link in which the RDS and
   * destination REDS exchange frames and the following period of the RDS-source REDS link in which the
   * RDS informs the source REDS of finishing one frame transfer. This subfield is used only when HD-DF RDS
   * operates in link switching type.
   * \param period
   */
  void SetSecondPeriod (uint16_t period);

  bool GetDuplexMode (void) const;
  bool GetCooperationMode (void) const;
  bool GetTxMode (void) const;
  uint8_t GetLinkChangeInterval (void) const;
  uint8_t GetDataSensingTime (void) const;
  uint16_t GetFirstPeriod (void) const;
  uint16_t GetSecondPeriod (void) const;

private:
  void SetRelayTransferParameter (uint64_t value);
  uint64_t GetRelayTransferParameter (void) const;

  bool m_duplex;
  bool m_cooperation;
  bool m_txMode;
  uint8_t m_changeInterval;
  uint8_t m_sensingTime;
  uint16_t m_firstPeriod;
  uint16_t m_secondPeriod;

};

std::ostream &operator << (std::ostream &os, const RelayTransferParameterSetElement &element);
std::istream &operator >> (std::istream &is, RelayTransferParameterSetElement &element);

ATTRIBUTE_HELPER_HEADER (RelayTransferParameterSetElement)

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Quiet Period Request Information Element 8.4.2.152
 *
 * The Quiet Period Request element defines a periodic sequence of quiet intervals that the requester AP
 * requests the responder AP to schedule.
 *
 */
class QuietPeriodRequestElement : public WifiInformationElement
{
public:
  QuietPeriodRequestElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The Request Token field is set to a nonzero value chosen by the requester AP.
   * \param token
   */
  void SetRequestToken (uint16_t token);
  /**
   * The Quiet Period Offset field is set to the offset of the start of the first quiet interval from the QAB Request
   * frame that contains this element, expressed in TUs. The reference time is the start of the preamble of the
   * PPDU that contains this element.
   * \param offset
   */
  void SetQuietPeriodOffset (uint16_t offset);
  /**
   * The Quiet Period field is set to the spacing between the start of two consecutive quiet intervals, expressed in TUs.
   * \param period
   */
  void SetQuietPeriod (uint32_t period);
  /**
   * The Quiet Duration field is set to duration of the quiet time, expressed in TUs.
   * \param duration
   */
  void SetQuietDuration (uint16_t duration);
  /**
   * The Repetition Count field is set to the number of requested quiet intervals.
   * \param count
   */
  void SetRepetitionCount (uint8_t count);
  /**
   * The Target BSSID field is set to the responder AP’s BSSID.
   * \param id
   */
  void SetTargetBssID (Mac48Address id);

  uint16_t GetRequestToken (void) const;
  uint16_t GetQuietPeriodOffset (void) const;
  uint32_t GetQuietPeriod (void) const;
  uint16_t GetQuietDuration (void) const;
  uint8_t GetRepetitionCount (void) const;
  Mac48Address GetTargetBssID (void) const;

private:
  uint16_t m_token;
  uint16_t m_quietPeriodOffset;
  uint32_t m_quietPeriod;
  uint16_t m_quietDuration;
  uint8_t m_repetitionCount;
  Mac48Address m_targetBssID;

};

std::ostream &operator << (std::ostream &os, const QuietPeriodRequestElement &element);
std::istream &operator >> (std::istream &is, QuietPeriodRequestElement &element);

ATTRIBUTE_HELPER_HEADER (QuietPeriodRequestElement)

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Quiet Period Response Information Element 8.4.2.153
 *
 * The Quiet Period Response element defines the feedback information from the AP that received the Quiet
 * Period Request element.
 *
 */
class QuietPeriodResponseElement : public WifiInformationElement
{
public:
  QuietPeriodResponseElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The Request Token field value is copied from the corresponding received Quiet Period Request element.
   * \param token
   */
  void SetRequestToken (uint16_t token);
  /**
   * The BSSID field value is copied of the Target BSSID field of the corresponding received Quiet Period
   * Request element.
   * \param id
   */
  void SetBssID (Mac48Address id);

  void SetStatusCode (uint16_t code);

  uint16_t GetRequestToken (void) const;
  Mac48Address GetBssID (void) const;
  uint16_t GetStatusCode (void) const;

private:
  uint16_t m_token;
  Mac48Address m_bssID;
  uint16_t m_status;

};

std::ostream &operator << (std::ostream &os, const QuietPeriodResponseElement &element);
std::istream &operator >> (std::istream &is, QuietPeriodResponseElement &element);

ATTRIBUTE_HELPER_HEADER (QuietPeriodResponseElement)

} //namespace ns3

#endif /* DMG_INFORMATION_ELEMENTS_H */
