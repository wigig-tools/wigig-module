/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
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
   * Add list of request elements.
   * \param list A list of the IDs of the requested elements.
   */
  void AddListOfRequestedElemetID (WifiInformationElementIdList &list);
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

ATTRIBUTE_HELPER_HEADER (RequestElement);

enum MeasurementType {
  BASIC_REQUEST = 0,
  CCA_REQEUST,
  RPI_REQUEST,
  CHANNEL_LOAD_REQUEST,
  NOISE_HISTOGRAM_LOAD_REQUEST,
  BEACON_REQUEST,
  FRAME_REQUEST,
  STA_STATISTICS_REQUEST,
  LCI_REQUEST,
  TRANSMIT_STREAM_REQUEST,
  MULTICAST_DIAGNOSTIC_REQUEST,
  LOCATION_CIVIC_REQUEST,
  LOCATION_IDENTIFIER_REQUEST,
  DIRECTIONAL_CHANNEL_QUALITY_REQUEST,
  DIRECTIONAL_MEASUREMENT_REQUEST,
  DIRECTIONAL_STATISTICS_REQUEST,
  MEASUREMENT_PAUSE_REQUEST = 255,
};

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 Measurement Request Information Element (8.4.2.23)
 */
class MeasurementRequestElement : public WifiInformationElement
{
public:
  MeasurementRequestElement ();

  WifiInformationElementId ElementId () const;
  virtual uint8_t GetInformationFieldSize () const;
  virtual void SerializeInformationField (Buffer::Iterator start) const = 0;
  virtual uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length) = 0;

  void SetMeasurementToken (uint8_t token);
  void SetMeasurementRequestMode (bool parallel, bool enable, bool request, bool report, bool durationManadatory);
  void SetMeasurementType (MeasurementType type);

  uint8_t GetMeasurementToken (void) const;
  bool IsParallelMode (void) const;
  bool IsEnableMode (void) const;
  bool IsRequestMode (void) const;
  bool IsReportMode (void) const;
  bool IsDurationMandatory (void) const;
  MeasurementType GetMeasurementType (void) const;

protected:
  uint8_t m_measurementToken;
  uint8_t m_measurementRequestMode;
  MeasurementType m_measurementType;

};

enum MeasurementMethod {
  ANIPI = 0,
  RSNI = 1,
} ;

/**
 * \ingroup wifi
 *
 * Directional Channel Quality Request (8.4.2.23.16)
 */
class DirectionalChannelQualityRequestElement : public MeasurementRequestElement
{
public:
  DirectionalChannelQualityRequestElement ();

  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void SetOperatingClass (uint8_t oclass);
  void SetChannelNumber (uint8_t number);
  void SetAid (uint8_t aid);
  void SetReservedField (uint8_t field);
  void SetMeasurementMethod (MeasurementMethod method);
  void SetMeasurementStartTime (uint64_t startTime);
  void SetMeasurementDuration (uint16_t duration);
  void SetNumberOfTimeBlocks (uint8_t blocks);

  uint8_t GetOperatingClass (void) const;
  uint8_t GetChannelNumber (void) const;
  uint8_t GetAid (void) const;
  uint8_t GetReservedField (void) const;
  MeasurementMethod GetMeasurementMethod (void) const;
  uint64_t GetMeasurementStartTime (void) const;
  uint16_t GetMeasurementDuration (void) const;
  uint8_t GetNumberOfTimeBlocks (void) const;

private:
  uint8_t m_operatingClass;
  uint8_t m_channelNumber;
  uint8_t m_aid;
  uint8_t m_reserved;
  MeasurementMethod m_measurementMethod;
  uint64_t m_measurementStartTime;
  uint16_t m_measurementDuration;
  uint8_t m_numberOfTimeBlocks;

};

std::ostream &operator << (std::ostream &os, const DirectionalChannelQualityRequestElement &element);
std::istream &operator >> (std::istream &is, DirectionalChannelQualityRequestElement &element);

ATTRIBUTE_HELPER_HEADER (DirectionalChannelQualityRequestElement);

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 Measurement Report Information Element (8.4.2.24)
 */
class MeasurementReportElement : public WifiInformationElement
{
public:
  MeasurementReportElement ();

  WifiInformationElementId ElementId () const;
  virtual uint8_t GetInformationFieldSize () const;

  void SetMeasurementToken (uint8_t token);
  void SetMeasurementReportMode (bool late, bool incapable, bool refused);
  void SetMeasurementType (MeasurementType type);

  uint8_t GetMeasurementToken (void) const;
  bool IsLateMode (void) const;
  bool IsIncapableMode (void) const;
  bool IsRefusedMode (void) const;
  MeasurementType GetMeasurementType (void) const;

protected:
  uint8_t m_measurementToken;
  uint8_t m_measurementReportMode;
  MeasurementType m_measurementType;

};

typedef uint8_t TimeBlockMeasurement;
typedef std::list<TimeBlockMeasurement> TimeBlockMeasurementList;
typedef TimeBlockMeasurementList::const_iterator TimeBlockMeasurementListCI;

/**
 * \ingroup wifi
 *
 * Directional Channel Quality Request (8.4.2.24.15)
 */
class DirectionalChannelQualityReportElement : public MeasurementReportElement
{
public:
  DirectionalChannelQualityReportElement ();

  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void SetOperatingClass (uint8_t oclass);
  void SetChannelNumber (uint8_t number);
  void SetAid (uint8_t aid);
  void SetReservedField (uint8_t field);
  void SetMeasurementMethod (uint8_t method);
  void SetMeasurementStartTime (uint64_t startTime);
  void SetMeasurementDuration (uint16_t duration);
  void SetNumberOfTimeBlocks (uint8_t blocks);
  void AddTimeBlockMeasurement (TimeBlockMeasurement measurement);

  uint8_t GetOperatingClass (void) const;
  uint8_t GetChannelNumber (void) const;
  uint8_t GetAid (void) const;
  uint8_t GetReservedField (void) const;
  uint8_t GetMeasurementMethod (void) const;
  uint64_t GetMeasurementStartTime (void) const;
  uint16_t GetMeasurementDuration (void) const;
  uint8_t GetNumberOfTimeBlocks (void) const;
  TimeBlockMeasurementList GetTimeBlockMeasurementList (void) const;

private:
  uint8_t m_operatingClass;
  uint8_t m_channelNumber;
  uint8_t m_aid;
  uint8_t m_reserved;
  uint8_t m_measurementMethod;
  uint64_t m_measurementStartTime;
  uint16_t m_measurementDuration;
  uint8_t m_numberOfTimeBlocks;
  TimeBlockMeasurementList m_measurementList;

};

std::ostream &operator << (std::ostream &os, const DirectionalChannelQualityReportElement &element);
std::istream &operator >> (std::istream &is, DirectionalChannelQualityReportElement &element);

ATTRIBUTE_HELPER_HEADER (DirectionalChannelQualityReportElement);

enum TrafficType {
  TRAFFIC_TYPE_PERIODIC_TRAFFIC_PATTERN = 0,
  TRAFFIC_TYPE_APERIODIC_TRAFFIC_PATTERN = 1
};

enum DataDirection {
  DATA_DIRECTION_UPLINK = 0,
  DATA_DIRECTION_DOWNLINK = 1,
  DATA_DIRECTION_DIRECT_LINK = 2,
  DATA_DIRECTION_BIDIRECTIONAL_LINK = 3
};

enum AccessPolicy {
  ACCESS_POLICY_RESERVED = 0,
  ACCESS_POLICY_CONTENTION_BASED_CHANNEL_ACCESS = 1,
  ACCESS_POLICY_CONTROLLED_CHANNEL_ACCESS = 2,
  ACCESS_POLICY_MIXED_MODE = 3
};

enum TSInfoAckPolicy {
  ACK_POLICY_NORMAL_ACK = 0,
  ACK_POLICY_NO_ACK = 1,
  ACK_POLICY_RESERVED = 2,
  ACK_POLICY_BLOCK_ACK = 3
};

enum AllocationDirection {
  ALLOCATION_DIRECTION_SOURCE = 0,
  ALLOCATION_DIRECTION_DESTINATION = 1
};

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 Traffic Specifications Element (TSPEC) 8.4.2.32
 */
class TspecElement : public WifiInformationElement
{
public:
  TspecElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The Traffic Type subfield is a single bit and is set to 1 for a periodic traffic pattern (e.g., isochronous
   * TS of MSDUs or A-MSDUs, with constant or variable sizes, that are originated at fixed rate) or set
   * to 0 for an aperiodic, or unspecified, traffic pattern (e.g., asynchronous TS of low-duty cycles).
   * \param type The type of the traffic.
   */
  void SetTrafficType (TrafficType type);
  /**
   * The TSID subfield is 4 bits in length and contains a value that is a TSID. The MSB (bit 4 in TS Info
   * field) of the TSID subfield is always set to 1 when the TSPEC element is included within an ADDTS Response frame.
   * \param id
   */
  void SetTSID (uint8_t id);
  /**
   * Set the direction of data carried by the TS.
   * \param direction The direction of data carried by the TS.
   */
  void SetDataDirection (DataDirection direction);
  /**
   * Set the access method to be used for the TS.
   * \param policy The access policy to be used for the TS.
   */
  void SetAccessPolicy (AccessPolicy policy);
  /**
   * Set the Aggregation subfield is valid only when the access method is HCCA or SPCA or when the access
   * method is EDCA and the Schedule subfield is equal to 1. It is set to 1 by a non-AP STA to indicate
   * that an aggregate schedule is required. It is set to 1 by the AP if an aggregate schedule is being
   * provided to the STA. It is set to 0 otherwise. In all other cases, the Aggregation subfield is reserved.
   * \param enable
   */
  void SetAggregation (bool enable);
  /**
   * Set the APSD subfield to indicate that automatic PS delivery is to be used for the traffic associated
   * with the TSPEC and set to 0 otherwise.
   * \param enable
   */
  void SetAPSD (bool enable);
  /**
   * The UP indicates the actual value of the UP to be used for the transport of MSDUs or A-MSDUs belonging
   * to this TS when relative prioritization is required. When the TCLAS element is present in the request,
   * the UP subfield in TS Info field of the TSPEC element is reserved.
   * \param priority
   */
  void SetUserPriority (uint8_t priority);
  /**
   * Set TS Info Ack Policy to indicate whether MAC acknowledgments are required for MPDUs or A-MSDUs belonging
   * to this TSID and the form of those acknowledgments.
   * \param policy
   */
  void SetTSInfoAckPolicy (TSInfoAckPolicy policy);
  /**
   * The Schedule subfield is 1 bit in length and specifies the requested type of schedule. The setting of
   * the subfield when the access policy is EDCA is shown in Table 9-142. When the Access Policy
   * subfield is equal to any value other than EDCA, the Schedule subfield is reserved. When the
   * Schedule and APSD subfields are equal to 1, the AP sets the aggregation bit to 1, indicating that an
   * aggregate schedule is being provided to the STA.
   * \param schedule
   */
  void SetSchedule (bool schedule);

  void SetDmgAttributesField (uint16_t infieldfo);
  uint16_t GetDmgAttributesField (void) const;

  /**
   * The Nominal MSDU Size field is 2 octets long and contains an unsigned integer that specifies the nominal
   * size, in octets, of MSDUs or (where A-MSDU aggregation is employed) A-MSDUs belonging to the TS
   * under this TSPEC. If the Fixed subfield is equal to 1, then the size of the MSDU or A-MSDU is fixed and
   * is indicated by the Size subfield. If the Fixed subfield is equal to 0, then the size of the MSDU or
   * A-MSDU might not be fixed and the Size subfield indicates the nominal MSDU size. If both the Fixed and
   * Size subfields are equal to 0, then the nominal MSDU or A-MSDU size is unspecified.
   * \param size
   */
  void SetNominalMsduSize (uint16_t size, bool fixed);
  /**
   * Set the maximum size, in octets, of MSDUs or A-MSDUs belonging to the TS under this TSPEC.
   * \param size
   */
  void SetMaximumMsduSize (uint16_t size);
  /**
   * Set minimum interval, in microseconds, between the start of two successive SPs.
   * \param interval
   */
  void SetMinimumServiceInterval (uint32_t interval);
  /**
   * SetMaximumServiceInterval
   * \param interval
   */
  void SetMaximumServiceInterval (uint32_t interval);
  void SetInactivityInterval (uint32_t interval);
  void SetSuspensionInterval (uint32_t interval);
  void SetServiceStartTime (uint32_t time);
  void SetMinimumDataRate (uint32_t rate);
  void SetMeanDataRate (uint32_t rate);
  void SetPeakDataRate (uint32_t rate);
  void SetBurstSize (uint32_t size);
  void SetDelayBound (uint32_t bound);
  void SetMinimumPhyRate (uint32_t rate);
  void SetSurplusBandwidthAllowance (uint16_t allowance);
  void SetMediumTime (uint16_t time);

  void SetAllocationID (uint8_t id);
  void SetAllocationDirection (AllocationDirection dircetion);
  void SetAmsduSubframe (uint8_t amsdu);
  void SetReliabilityIndex (uint8_t index);

  TrafficType GetTrafficType (void) const;
  uint8_t GetTSID (void) const;
  DataDirection GetDataDirection (void) const;
  AccessPolicy GetAccessPolicy (void) const;
  bool GetAggregation (void) const;
  bool GetAPSD (void) const;
  uint8_t GetUserPriority (void) const;
  TSInfoAckPolicy GetTSInfoAckPolicy (void) const;
  bool GetSchedule (void) const;

  uint16_t GetNominalMsduSize (void) const;
  bool IsMsduSizeFixed (void) const;
  uint16_t GetMaximumMsduSize (void) const;
  uint32_t GetMinimumServiceInterval (void) const;
  uint32_t GetMaximumServiceInterval (void) const;
  uint32_t GetInactivityInterval (void) const;
  uint32_t GetSuspensionInterval (void) const;
  uint32_t GetServiceStartTime (void) const;
  uint32_t GetMinimumDataRate (void) const;
  uint32_t GetMeanDataRate (void) const;
  uint32_t GetPeakDataRate (void) const;
  uint32_t GetBurstSize (void) const;
  uint32_t GetDelayBound (void) const;
  uint32_t GetMinimumPhyRate (void) const;
  uint16_t GetSurplusBandwidthAllowance (void) const;
  uint16_t GetMediumTime (void) const;

  uint8_t GetAllocationID (void) const;
  AllocationDirection GetAllocationDirection (void) const;
  uint8_t GetAmsduSubframe (void) const;
  uint8_t GetReliabilityIndex (void) const;

private:
  uint8_t m_trafficType;
  uint8_t m_tsid;
  uint8_t m_dataDirection;
  uint8_t m_accessPolicy;
  uint8_t m_aggregation;
  uint8_t m_apsd;
  uint8_t m_userPriority;
  uint8_t m_tsInfoAckPolicy;
  uint8_t m_schedule;

  uint16_t m_nominalMsduSize;
  uint16_t m_maximumMsduSize;
  uint32_t m_minimumServiceInterval;
  uint32_t m_maximumServiceInterval;
  uint32_t m_inactivityInterval;
  uint32_t m_suspensionInterval;
  uint32_t m_serviceStartTime;
  uint32_t m_minimumDateRate;
  uint32_t m_meanDateRate;
  uint32_t m_peakDateRate;
  uint32_t m_burstSize;
  uint32_t m_delayBound;
  uint32_t m_minimumPhyRate;
  uint16_t m_surplusBandwidthAllowance;
  uint16_t m_mediumTime;

  uint8_t m_allocationID;
  uint8_t m_allocationDirection;
  uint8_t m_amsduSubframe;
  uint8_t m_reliabilityIndex;

};

std::ostream &operator << (std::ostream &os, const TspecElement &element);
std::istream &operator >> (std::istream &is, TspecElement &element);

ATTRIBUTE_HELPER_HEADER (TspecElement);

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 Traffic Stream (TS) Delay 8.4.2.34
 */
class TsDelayElement : public WifiInformationElement
{
public:
  TsDelayElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * Set the amount of delay time, in TUs, a STA should wait before it  reinitiates setup of a TS.
   * The Delay field is set to 0 when an AP does not expect to serve any TSPECs for  an indeterminate
   * time and does not know this time a priori.
   * \param type
   */
  void SetDelay (uint32_t delay);
  uint32_t GetDelay (void) const;

private:
  uint32_t m_delay;

};

std::ostream &operator << (std::ostream &os, const TsDelayElement &element);
std::istream &operator >> (std::istream &is, TsDelayElement &element);

ATTRIBUTE_HELPER_HEADER (TsDelayElement);

typedef enum {
  ReassociationDeadlineInterval = 1,
  KeyLifetimeInterval = 2,
  AssociationComebackTime = 3,
} TimeoutIntervalType;

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
  void SetTimeoutIntervalType (TimeoutIntervalType type);
  /**
   * The Timeout Interval Value field contains an unsigned 32-bit integer.
   * \param value
   */
  void SetTimeoutIntervalValue (uint32_t value);

  TimeoutIntervalType GetTimeoutIntervalType (void) const;
  uint32_t GetTimeoutIntervalValue (void) const;

private:
  uint8_t m_timeoutIntervalType;
  uint32_t m_timeoutIntervalValue;

};

std::ostream &operator << (std::ostream &os, const TimeoutIntervalElement &element);
std::istream &operator >> (std::istream &is, TimeoutIntervalElement &element);

ATTRIBUTE_HELPER_HEADER (TimeoutIntervalElement);

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

ATTRIBUTE_HELPER_HEADER (DmgOperationElement);

enum NUMBER_OF_TAPS {
  TAPS_1 = 0,
  TAPS_5 = 1,
  TAPS_15 = 2,
  TAPS_63 = 3,
};

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
   * \param index The BS-FBCK field indicates the index of the TRN-T field that was received
   * with the best quality in the last received BRP-TX PPDU, where the first TRN-T field in
   * the PPDU is defined as having an index equal to 1. If the last received PPDU was not a
   * BRP-TX PPDU, this field is set to 0. The determination of best quality is implementation dependent.
   */
  void SetBsFbck (uint8_t index);
  /**
   * \param The BS-FBCK Antenna ID field specifies the antenna ID corresponding to the
   *  sector indicated by the BF-FBCK field.
   */
  void SetBsFbckAntennaID (uint8_t id);

  /* FBCK-REQ field */

  /**
   * \param requested If set to 1, the SNR subfield is requested as part of the channel measurement
   * feedback. Otherwise, set to 0.
   */
  void SetSnrRequested (bool requested);
  /**
   * \param requested If set to 1, the Channel Measurement subfield is requested as part of the channel
   * measurement feedback. Otherwise, set to 0.
   */
  void SetChannelMeasurementRequested (bool requested);
  /**
   * \param number The number of taps in each channel measurement.
   */
  void SetNumberOfTapsRequested (NUMBER_OF_TAPS number);
  /**
   * \param present If set to 1, the Sector ID Order subfield is requested as part of the channel
   * measurement feedback. Otherwise, set to 0.
   */
  void SetSectorIdOrderRequested (bool present);

  /* FBCK-TYPE field */

  /**
   * \param present Set to 1 to indicate that the SNR subfield is present as part of the channel measurement
   * feedback. Set to 0 otherwise.
   */
  void SetSnrPresent (bool present);
  /**
   * \param present Set to 1 to indicate that the Channel Measurement subfield is present as part of the
   * channel measurement feedback. Set to 0 otherwise.
   */
  void SetChannelMeasurementPresent (bool present);
  /**
   * \param present Set to 1 to indicate that the Tap Delay subfield is present as part of the channel
   * measurement feedback. Set to 0 otherwise.
   */
  void SetTapDelayPresent (bool present);
  /**
   * \param number Number of taps in each channel measurement.
   */
  void SetNumberOfTapsPresent (NUMBER_OF_TAPS number);
  /**
   * \param number Number of measurements in the SNR subfield and the Channel Measurement subfield.
   * It is equal to the number of TRN-T subfields in the BRP-TX packet on which the measurement is based,
   * or the number of received sectors if TXSS result is reported by setting the TXSS-FBCK-REQ subfield to 1.
   */
  void SetNumberOfMeasurements (uint8_t number);
  /**
   * \param present Set to 1 to indicate that the Sector ID Order subfield is present as part of the channel
   * measurement feedback. Set to 0 otherwise.
   */
  void SetSectorIdOrderPresent (bool present);
  /**
   * \param linkType SetLinkType Set to 0 for the initiator link and to 1 for the responder link.
   */
  void SetLinkType (bool linkType);
  /**
   * \param type Set to 0 for the transmitter antenna and to 1 for the receiver antenna.
   */
  void SetAntennaType (bool type);
  /**
   * \param number Indicates the number of beams in the Sector ID Order subfield for the MIDC subphase.
   */
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
  NUMBER_OF_TAPS GetNumberOfTapsRequested (void) const;
  bool IsSectorIdOrderRequested (void) const;

  /* FBCK-TYPE field */
  bool IsSnrPresent (void) const;
  bool IsChannelMeasurementPresent (void) const;
  bool IsTapDelayPresent (void) const;
  NUMBER_OF_TAPS GetNumberOfTapsPresent (void) const;
  uint8_t GetNumberOfMeasurements (void) const;
  bool IsSectorIdOrderPresent (void) const;
  bool GetLinkType (void) const;
  bool GetAntennaType (void) const;
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
  NUMBER_OF_TAPS m_numberOfTapsRequested;
  bool m_sectorIdOrderRequested;
  /* FBCK-TYPE field */
  bool m_snrPresent;
  bool m_channelMeasurementPresent;
  bool m_tapDelayPresent;
  NUMBER_OF_TAPS m_numberOfTapsPresent;
  uint8_t m_numberOfMeasurements;
  bool m_sectorIdOrderPresent;
  bool m_linkType;
  bool m_antennaType;
  uint8_t m_numberOfBeams;

  bool m_midExtension;
  bool m_capabilityRequest;

};

std::ostream &operator << (std::ostream &os, const BeamRefinementElement &element);
std::istream &operator >> (std::istream &is, BeamRefinementElement &element);

ATTRIBUTE_HELPER_HEADER (BeamRefinementElement);

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

ATTRIBUTE_HELPER_HEADER (WakeupScheduleElement);

/******************************************
*    Allocation Field Format (8-401aa)
*******************************************/

/**
 * \ingroup wifi
 * Implement the Allocation Field Format for the Extended Schedule Element.
 */
class AllocationField
{
public:
  AllocationField ();

  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
   * The Allocation ID field, when set to a nonzero value, identifies an airtime allocation from Source AID to
   * Destination AID. Except for CBAP allocations with broadcast Source AID and broadcast Destination AID,
   * the tuple (Source AID, Destination AID, Allocation ID) uniquely identifies the allocation. For CBAP
   * allocations with broadcast Source AID and broadcast Destination AID, the Allocation ID is zero.
   * \param id The ID of the current allocation.
   */
  void SetAllocationID (AllocationID id);
  /**
   * The AllocationType field defines the channel access mechanism during the allocation.
   * \param type either SP allocation or CBAP allocation.
   */
  void SetAllocationType (AllocationType type);
  /**
   * The Pseudo-static field is set to 1 to indicate that this allocation is pseudo-static and set to 0 otherwise.
   * \param value
   */
  void SetAsPseudoStatic (bool value);
  /**
   * For an SP allocation, the Truncatable field is set to 1 to indicate that the source DMG STA and destination
   * DMG STA can request SP truncation and set to 0 otherwise. For a CBAP allocation, the Truncatable field is reserved.
   * \param value
   */
  void SetAsTruncatable (bool value);
  /**
   * For an SP allocation, the Extendable field is set to 1 to indicate that the source DMG STA and destination
   * DMG STA can request SP extension and set to 0 otherwise. For a CBAP allocation, the Extendable field is reserved.
   * \param value
   */
  void SetAsExtendable (bool value);
  /**
   * The PCP Active field is set to 1 if the PCP is available to receive transmissions during the CBAP or SP
   * allocation and set to 0 otherwise. The PCP Active field is set to 1 if at least one of the Truncatable or
   * Extendable fields are set to 1, or when transmitted by an AP.
   * \param value
   */
  void SetPcpActive (bool value);
  /**
   * The LP SC Used subfield is set to 1 to indicate that the low-power SC PHY is used in this SP.
   * Otherwise, it is set to 0.
   * \param value True if LP SC is used in this SP otherwise is false.
   */
  void SetLpScUsed (bool value);

  AllocationID GetAllocationID (void) const;
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

  /* Private Information */
  void SetAllocationAnnounced (void);
  bool IsAllocationAnnounced (void) const;

private:
  /* Allocation Control Subfield*/
  AllocationID m_allocationID;            //!< Allocation ID.
  uint8_t m_allocationType;               //!< Allocation Type.
  bool m_pseudoStatic;                    //!< Pseudostatic Allocation.
  bool m_truncatable;                     //!< Truncatable service period.
  bool m_extendable;                      //!< Extendable service period.
  bool m_pcpActive;                       //!< PCP Active.
  bool m_lpScUsed;                        //!< Low Power SC Operation.

  BF_Control_Field m_bfControl;           //!< BF Control.
  uint8_t m_SourceAid;                    //!< Source STA AID.
  uint8_t m_destinationAid;               //!< Destination STA AID.
  uint32_t m_allocationStart;             //!< Allocation Start.
  uint16_t m_allocationBlockDuration;     //!< Allocation Block Duration.
  uint8_t m_numberOfBlocks;               //!< Number of Blocks.
  uint16_t m_allocationBlockPeriod;       //!< Allocation Block Period.

  bool m_allocationAnnounced;             //!< Flag to indicate if allocation has been announced in BTI or ATI.

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

ATTRIBUTE_HELPER_HEADER (ExtendedScheduleElement);

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

  /**
   * Add STA Info Field.
   * \param field The STA Information Field.
   */
  void AddStaInfo (StaInfoField &field);
  /**
   * Set the STA Info list to be sent by the PCP/AP.
   * \param list The STA Info list.
   */
  void SetStaInfoList (const StaInfoFieldList &list);
  /**
   * \return The list of STA Info sent by the PCP/AP.
   */
  StaInfoFieldList GetStaInfoList (void) const;
  /**
   * This function returns the first STA Info Field in the list (Used by the PCP/AP to obtain information about the STA).
   * \return
   */
  StaInfoField GetStaInfoField (void) const;

private:
  StaInfoFieldList m_list;

};

std::ostream &operator << (std::ostream &os, const StaAvailabilityElement &element);
std::istream &operator >> (std::istream &is, StaAvailabilityElement &element);

ATTRIBUTE_HELPER_HEADER (StaAvailabilityElement);

/*********************************************
*  DMG Allocation Info Field Format (8-401af)
**********************************************/

typedef enum {
  ISOCHRONOUS = 0,
  ASYNCHRONOUS = 1
} AllocationFormat;

/**
 * \ingroup wifi
 * Implement the header for DMG Allocation Info Field.
 */
class DmgAllocationInfo
{
public:
  DmgAllocationInfo ();

  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
   * The Allocation ID field identifies an allocation if set to a nonzero value, and is used as follows:
   * — The STA that transmits an ADDTS Request containing the DMG TSPEC element sets the Allocation ID to
   * a nonzero value to create a new allocation or modify an existing allocation.
   * — The STA that transmits an ADDTS Response containing the DMG TSPEC element sets theAllocation ID to
   * a nonzero value to identify a created or modified allocation or sets it to zero if creating or
   * modifying the allocation failed.
   * \param id The unique ID of the allocation.
   */
  void SetAllocationID (AllocationID id);
  /**
   * The AllocationType field defines the channel access mechanism used during the allocation.
   * \param type
   */
  void SetAllocationType (AllocationType type);
  /**
   * Set the allocation format.
   * \param format Either Isochronous allocation or Asynchronous allocation.
   */
  void SetAllocationFormat (AllocationFormat format);
  /**
   * The Pseudo-static field is set to 1 for a pseudo-static allocation and set to 0 otherwise.
   * \param value
   */
  void SetAsPseudoStatic (bool value);
  void SetAsTruncatable (bool value);
  void SetAsExtendable (bool value);
  void SetLpScUsed (bool value);
  /**
   * The UP field indicates the lowest priority UP to be used for possible transport of MSDUs belonging to TCs
   * with the same source and destination of the allocation.
   * \param value
   */
  void SetUp (uint8_t value);
  void SetDestinationAid (uint8_t aid);

  AllocationID GetAllocationID (void) const;
  AllocationType GetAllocationType (void) const;
  AllocationFormat GetAllocationFormat (void) const;
  bool IsPseudoStatic (void) const;
  bool IsTruncatable (void) const;
  bool IsExtendable (void) const;
  bool IsLpScUsed (void) const;
  uint8_t GetUp (void) const;
  uint8_t GetDestinationAid (void) const;

private:
  AllocationID m_allocationID;            //!< Allocation ID.
  uint8_t m_allocationType;               //!< Allocation Type.
  uint8_t m_allocationFormat;             //!< Allocation Format.
  bool m_pseudoStatic;                    //!< Pseudostatic Allocation.
  bool m_truncatable;                     //!< Truncatable service period.
  bool m_extendable;                      //!< Extendable service period.
  bool m_lpScUsed;                        //!< Low Power SC Operation.
  uint8_t m_up;                           //!< User Priority.
  uint8_t m_destAid;                      //!< Destination Association Identifier.

};

/**
 * \ingroup wifi
 * Implement the header for Constraint subfield format (Figure 8-401ah).
 */
class ConstraintSubfield
{
public:
  ConstraintSubfield ();

  uint32_t GetSerializedSize (void) const;
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  Buffer::Iterator Deserialize (Buffer::Iterator start);

  /**
   * The TSCONST Start Time field contains the lower 4 octets of the TSF at the time the scheduling constraint starts.
   * \param time The start time of the traffic scheduling contraint.
   */
  void SetStartStartTime (uint32_t time);
  /**
   * The TSCONST Duration field indicates the time, in microseconds, for which the scheduling constraint is specified.
   * \param duration The duration of the traffic scheduling contraint in microseconds.
   */
  void SetDuration (uint16_t duration);

  void SetPeriod (uint16_t period);
  /**
   * The Interferer MAC Address field is set to the value of the TA field within a frame received during the
   * interval of time indicated by this TSCONST field. If the value is unknown, the Interferer MAC Address field
   * is set to the broadcast MAC address.
   * \param address The MAC address of the Interferer.
   */
  void SetInterfererAddress (Mac48Address address);

  uint32_t GetStartStartTime (void) const;
  uint16_t GetDuration (void) const;
  uint16_t GetPeriod (void) const;
  Mac48Address GetInterfererAddress (void) const;

private:
  uint32_t m_startTime;           //!< Start Time.
  uint16_t m_duration;            //!< Duration in MicroSeconds.
  uint16_t m_period;              //!< Period in MicroSeconds.
  Mac48Address m_address;         //!< Interferer MAC address.

};

typedef std::vector<ConstraintSubfield> ConstraintList;
typedef ConstraintList::const_iterator ConstraintListCI;

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad DMG TSPEC element 8.4.2.136
 *
 * The DMG TSPEC element is present in the ADDTS Request frame sent by a non-PCP/non-AP DMG STA
 * and contains the set of parameters needed to create or modify an airtime allocation. The DMG TSPEC
 * element is also present in the ADDTS Response frame sent by a DMG PCP/AP and reflects the parameters,
 * possibly modified, by which the allocation was created.
 */
class DmgTspecElement : public WifiInformationElement
{
public:
  DmgTspecElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void SetDmgAllocationInfo (DmgAllocationInfo &info);
  void SetBfControl (BF_Control_Field &ctrl);
  /**
   * Set the allocation period as a fraction or multiple of the beacon interval (BI).
   * \param period The allocation period check table (Table 8-183k) in the 802.11ad standard.
   * \param multiple Flag to indicate if the allocation period is multiple or fraction of BI.
   */
  void SetAllocationPeriod (uint16_t period, bool multiple);
  void SetMinimumAllocation (uint16_t min);
  void SetMaximumAllocation (uint16_t max);
  void SetMinimumDuration (uint16_t duration);
  void AddTrafficSchedulingConstraint (ConstraintSubfield &constraint);

  DmgAllocationInfo GetDmgAllocationInfo (void) const;
  BF_Control_Field GetBfControl (void) const;
  uint16_t GetAllocationPeriod (void) const;
  bool IsAllocationPeriodMultipleBI (void) const;
  uint16_t GetMinimumAllocation (void) const;
  uint16_t GetMaximumAllocation (void) const;
  uint16_t GetMinimumDuration (void) const;
  uint8_t GetNumberOfConstraints (void) const;
  ConstraintList GetConstraintList (void) const;

private:
  DmgAllocationInfo m_dmgAllocationInfo;
  BF_Control_Field m_bfControlField;
  uint16_t m_allocationPeriod;
  uint16_t m_minAllocation;
  uint16_t m_maxAllocation;
  uint16_t m_minDuration;
  ConstraintList m_constraintList;

};

std::ostream &operator << (std::ostream &os, const DmgTspecElement &element);
std::istream &operator >> (std::istream &is, DmgTspecElement &element);

ATTRIBUTE_HELPER_HEADER (DmgTspecElement);

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

ATTRIBUTE_HELPER_HEADER (NextDmgAti);

enum SessionType {
  SessionType_InfrastructureBSS = 0,
  SessionType_IBSS = 1,
  SessionType_DLS = 2,
  SessionType_TDLS = 3,
  SessionType_PBSS = 4
};

enum BandID {
  Band_TV_white_Spaces = 0, /* TV White Spaces */
  Band_Sub_1_GHz,           /* Sub-1 GHz (excluding TV white spaces) */
  Band_2_4GHz,              /* 2.4 GHz */
  Band_3_6GHz,              /* 3.6 GHz */
  Band_4_9GHz,              /* 4.9 and 5 GHz */
  Band_60GHz                /* 60 GHz */
};

struct Band {
  uint8_t Band_ID;
  uint8_t Setup;
  uint8_t Operation;
};

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

ATTRIBUTE_HELPER_HEADER (ChannelMeasurementFeedbackElement);

/* Typedef for Channel Measurement Feedback Element List */
typedef std::vector<Ptr<ChannelMeasurementFeedbackElement> > ChannelMeasurementFeedbackElementList;
typedef ChannelMeasurementFeedbackElementList::const_iterator ChannelMeasurementFeedbackElementListCI;
typedef ChannelMeasurementFeedbackElementList::iterator ChannelMeasurementFeedbackElementListI;

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

ATTRIBUTE_HELPER_HEADER (AwakeWindowElement);

enum StaRole {
  ROLE_AP = 0,
  ROLE_TDLS_STA = 1,
  ROLE_IBSS_STA = 2,
  ROLE_PCP = 3,
  ROLE_NON_PCP_NON_AP = 4
} ;

enum ConnectionCapability {
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
  void SetStaRole (StaRole role);
  void SetStaMacAddressPresent (bool present);
  void SetPairwiseCipherSuitePresent (bool present);

  StaRole GetStaRole (void) const;
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
   * \param capability
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

ATTRIBUTE_HELPER_HEADER (MultiBandElement);

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

ATTRIBUTE_HELPER_HEADER (NextPcpListElement);

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

enum Activity {
  NO_CHANGE_PREFFERED = 0,
  CHANGED_MCS,
  DECREASED_TRANSMIT_POWER,
  INCREASED_TRANSMIT_POWER,
  FAST_SESSION_TRANSFER,
  POWER_CONSERVE,
  PERFORM_SLS
};

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad DMG Link Margin element 8.4.2.144
 */
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

ATTRIBUTE_HELPER_HEADER (SwitchingStreamElement);

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

private:
  uint32_t m_fstsID;
  SessionType m_sessionType;
  bool m_switchIntent;
  Band m_newBand;
  Band m_oldBand;

};

std::ostream &operator << (std::ostream &os, const SessionTransitionElement &element);
std::istream &operator >> (std::istream &is, SessionTransitionElement &element);

ATTRIBUTE_HELPER_HEADER (SessionTransitionElement);

enum RelayDuplexMode {
  RELAY_FD_AF = 1,  /* Full Duplex, Amplify and Forward */
  RELAY_HD_DF = 2,  /* Half Duplex, Decode and Forward */
  RELAY_BOTH = 3
};

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad Cluster Report Element 8.4.2.149
 */
class ClusterReportElement : public WifiInformationElement
{
public:
  ClusterReportElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /** Cluster Report Control **/

  /**
   * The Cluster Request subfield is set to 1 to indicate that the STA is requesting the PCP/AP to start PCP/AP
   * clustering (9.34). Otherwise, it is set to 0.
   * \param request
   */
  void SetClusterRequest (bool request);
  /**
   * The Cluster Report subfield is set to 1 to indicate that this element contains a cluster report. If this subfield is
   * set to 1, the Reported BSSID, Reference Timestamp and Clustering Control fields are present in this
   * element. Otherwise, if the Cluster Report subfield is set to 0, none of the Reported BSSID, Reference
   * Timestamp, Clustering Control, Extended Schedule Element, and TSCONST fields is present in this
   * element.
   * \param report
   */
  void SetClusterReport (bool report);
  /**
   * The Schedule Present subfield is valid only if the Cluster Report subfield is set to 1; otherwise, it is reserved.
   * The Schedule present subfield is set to 1 to indicate that the Extended Schedule Element field is present in
   * this element. Otherwise, the Extended Schedule Element field is not present in this element.
   * \param present
   */
  void SetSchedulePresent (bool present);
  void SetTsConstPresent (bool present);
  void SetEcpacPolicyEnforced (bool enforced);
  void SetEcpacPolicyPresent (bool present);

  /**
   * Set the BSSID of the DMG Beacon frame that triggered this report.
   * \param bssid The BSSID of the DMG Beacon frame that triggered this report.
   */
  void SetReportedBssID (Mac48Address bssid);
  /**
   * The Reference Timestamp field contains the lower 4 octets of the TSF timer value sampled at the instant that
   * the STA’s MAC received a DMG Beacon frame that triggered this report.
   * \param timestamp
   */
  void SetReferenceTimestamp (uint32_t timestamp);
  /**
   * Set Clustering Control
   * \param field Contains the Clustering Control received in the DMG Beacon that triggered this report.
   */
  void SetClusteringControl (ExtDMGClusteringControlField &field);

  void SetEcpacPolicyElement ();
  void SetExtendedScheduleElement (ExtendedScheduleElement &element);
  /**
   * \param constraint The Traffic Scheduling Constraint (TSCONST) field is defined in 8.4.2.136 and specifies
   * periods of time with respect to the TBTT of the beacon interval of the BSS the STA participates where the STA
   * experiences poor channel conditions, such as due to interference.
   */
  void AddTrafficSchedulingConstraint (ConstraintSubfield &constraint);

  bool GetClusterRequest (void) const;
  bool GetClusterReport (void) const;
  bool GetSchedulePresent (void) const;
  bool GetTsConstPresent (void) const;
  bool GetEcpacPolicyEnforced (void) const;
  bool GetEcpacPolicyPresent (void) const;

  Mac48Address GetReportedBssID (void) const;
  uint32_t GetReferenceTimestamp (void) const;
  ExtDMGClusteringControlField GetClusteringControl (void) const;
  void GetEcpacPolicyElement (void) const;
  ExtendedScheduleElement GetExtendedScheduleElement (void) const;
  uint8_t GetNumberOfContraints (void) const;
  ConstraintList GetTrafficSchedulingConstraintList (void) const;

private:
  /** Cluster Report Control **/
  bool m_clusterRequest;
  bool m_clusterReport;
  bool m_schedulePresent;
  bool m_tsconstPresent;
  bool m_ecpacPolicyEnforced;
  bool m_ecpacPolicyPresent;

  Mac48Address m_bssID;
  uint32_t m_timestamp;
  ExtDMGClusteringControlField m_clusteringControl;
  ExtendedScheduleElement m_scheduleElement;
  ConstraintList m_constraintList;

};

std::ostream &operator << (std::ostream &os, const ClusterReportElement &element);
std::istream &operator >> (std::istream &is, ClusterReportElement &element);

ATTRIBUTE_HELPER_HEADER (ClusterReportElement);

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

ATTRIBUTE_HELPER_HEADER (RelayCapabilitiesElement);

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

ATTRIBUTE_HELPER_HEADER (RelayTransferParameterSetElement);

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

ATTRIBUTE_HELPER_HEADER (QuietPeriodRequestElement);

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

ATTRIBUTE_HELPER_HEADER (QuietPeriodResponseElement);

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ad ECPAC Policy Element 8.4.2.157
 */
class EcpacPolicyElement : public WifiInformationElement
{
public:
  EcpacPolicyElement ();

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /** ECPAC Policy Detail **/

  void SetBhiEnforced (bool enforced);
  void SetTxssCbapEnforced (bool enforced);
  void SetProtectedPeriodEnforced (bool enforced);

  void SetCCSRID (Mac48Address ccsrID);
  void SetTimestampOffsetBitmap (uint32_t bitmap);
  void SetTxssCbapOffset (uint16_t offset);
  void SetTxssCbapDuration (uint8_t duration);
  void SetTxssCbapMaxMem (uint8_t max);

  bool GetBhiEnforced (void) const;
  bool GetTxssCbapEnforced (void) const;
  bool GetProtectedPeriodEnforced (void) const;

  Mac48Address GetCCSRID (void) const;
  uint32_t GetTimestampOffsetBitmap (void) const;
  uint16_t GetTxssCbapOffset (void) const;
  uint8_t GetTxssCbapDuration (void) const;
  uint8_t GetTxssCbapMaxMem (void) const;

private:
  /** ECPAC Policy Detail **/
  bool m_bhiEnforced;
  bool m_txssCbapEnforced;
  bool m_protectedPeriodEnforced;

  Mac48Address m_ccsrID;
  uint32_t m_timestampOffsetBitmap;
  uint16_t m_txssCbapOffset ;
  uint8_t m_txssCbapDuration;
  uint8_t m_txssCbapMaxMem;

};

std::ostream &operator << (std::ostream &os, const EcpacPolicyElement &element);
std::istream &operator >> (std::istream &is, EcpacPolicyElement &element);

ATTRIBUTE_HELPER_HEADER (EcpacPolicyElement);

struct EDMGSectorIDOrder {
  SECTOR_ID SectorID;       /* Sector ID/CDOWN/AWV Feedback ID*/
  ANTENNA_ID TXAntennaID;
  ANTENNA_ID RXAntennaID;
};

typedef std::vector<EDMGSectorIDOrder> EDMGSectorIDOrder_List;
typedef EDMGSectorIDOrder_List::iterator EDMGSectorIDOrder_ListI;
typedef EDMGSectorIDOrder_List::const_iterator EDMGSectorIDOrder_ListCI;

typedef uint8_t BRP_CDOWN;
typedef std::vector<BRP_CDOWN> BRP_CDOWN_LIST;
typedef BRP_CDOWN_LIST::iterator BRP_CDOWN_LIST_I;
typedef BRP_CDOWN_LIST::const_iterator BRP_CDOWN_LIST_CI;

typedef uint16_t Tap_Delay;
typedef std::vector<Tap_Delay> Tap_Delay_List;
typedef Tap_Delay_List::iterator Tap_Delay_List_I;
typedef Tap_Delay_List::const_iterator Tap_Delay_List_CI;

/**
 * \ingroup wifi
 *
 * The EDMG Channel Measurement Feedback element is used to carry channel measurement feedback data
 * that an EDMG STA has measured on DMG Beacon frames, SSW frames, Short SSW packets or TRN
 * fields of EDMG BRP packets. This channel measurement feedback data is provided in addition to what is
 * provided in the Channel Measurement Feedback element. The EDMG Channel Measurement Feedback
 * element provides a list of sectors per transmit DMG antenna identified during sector sweep or during beam
 * combination, or a list of TX sector combinations identified during SU-MIMO or MU-MIMO beamforming
 * training and that can be used to establish a beamformed SISO, SU-MIMO or MU-MIMO link, as well as
 * AWV information obtained during beam tracking.
 */
class EDMGChannelMeasurementFeedbackElement : public WifiInformationElement
{
public:
  EDMGChannelMeasurementFeedbackElement ();

  WifiInformationElementId ElementId () const;
  WifiInformationElementId ElementIdExtension () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  void Add_EDMG_SectorIDOrder (EDMGSectorIDOrder &order);
  void Add_BRP_CDOWN (BRP_CDOWN brpCdown);
  void Add_Tap_Delay (Tap_Delay tapDelay);

  EDMGSectorIDOrder_List Get_EDMG_SectorIDOrder_List (void) const;
  BRP_CDOWN_LIST Get_BRP_CDOWN_List (void) const;
  Tap_Delay_List Get_Tap_Delay_List (void) const;

private:
  EDMGSectorIDOrder_List m_edmgSectorIDOrder_List;
  BRP_CDOWN_LIST m_brp_CDOWN_List;
  Tap_Delay_List m_tap_Delay_List;

};

std::ostream &operator << (std::ostream &os, const EDMGChannelMeasurementFeedbackElement &element);
std::istream &operator >> (std::istream &is, EDMGChannelMeasurementFeedbackElement &element);

ATTRIBUTE_HELPER_HEADER (EDMGChannelMeasurementFeedbackElement);

typedef std::vector<Ptr<EDMGChannelMeasurementFeedbackElement> > EDMGChannelMeasurementFeedbackElementList;
typedef EDMGChannelMeasurementFeedbackElementList::iterator EDMGChannelMeasurementFeedbackElementListI;
typedef EDMGChannelMeasurementFeedbackElementList::const_iterator EDMGChannelMeasurementFeedbackElementListCI;

typedef std::vector<uint8_t> AID_LIST;
typedef AID_LIST::const_iterator AID_LIST_CI;
typedef AID_LIST::iterator AID_LIST_I;

struct EDMGGroupTuple {
  uint8_t groupID;    /* The EDMG Group ID subfield is a unique, nonzero value that identifies the group. */
  AID_LIST aidList;   /* Each AID subfield contains the AID of an EDMG STA that belongs to the group. */
};

typedef std::vector<EDMGGroupTuple> EDMGGroupTuples;
typedef EDMGGroupTuples::const_iterator EDMGGroupTuplesCI;
typedef EDMGGroupTuples::iterator EDMGGroupTuplesI;

/**
 * \ingroup wifi
 *
 * The EDMG Group ID Set element allows an AP or PCP to define groups of MU capable EDMG STAs to
 * perform DL MU-MIMO beamforming training and transmissions. The EDMG Group ID Set element is
 * transmitted in DMG Beacon or Announce frames.
 */
class EDMGGroupIDSetElement : public WifiInformationElement
{
public:
  EDMGGroupIDSetElement ();

  WifiInformationElementId ElementId () const;
  WifiInformationElementId ElementIdExtension () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * The Number of EDMG Groups field indicates the number of EDMG Group fields.
   * \param number
   */
  void SetNumberofEDMGGroups (uint8_t number);
  void AddEDMGGroupTuple (EDMGGroupTuple &tuple);

  uint8_t GetNumberofEDMGGroups (void) const;
  EDMGGroupTuples GetEDMGGroupTuples (void) const;

private:
  uint8_t m_numEDMGGroups;
  EDMGGroupTuples m_edmgGroupTuples;

};

std::ostream &operator << (std::ostream &os, const EDMGGroupIDSetElement &element);
std::istream &operator >> (std::istream &is, EDMGGroupIDSetElement &element);

ATTRIBUTE_HELPER_HEADER (EDMGGroupIDSetElement);

enum MimoBeamformingType {
  SU_MIMO_BEAMFORMING = 0,
  MU_MIMO_BEAMFORMING = 1,
};

enum MimoPhaseType {
  MIMO_PHASE_NON_RECPIROCAL = 0,
  MIMO_PHASE_RECPIROCAL = 1,
};

/**
 * \ingroup wifi
 *
 * The IEEE 802.11ay MIMO Setup Control element is used to carry configuration information
 * required for the SU-MIMO BF training and feedback subphases or the MU-MIMO BF training
 * and feedback subphases.
 */
class MimoSetupControlElement : public WifiInformationElement
{
public:
  MimoSetupControlElement ();

  WifiInformationElementId ElementId () const;
  WifiInformationElementId ElementIdExtension () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * This field is set to 0 to indicate SU-MIMO beamforming and is set to 1 to indicate
   * MU-MIMO beamforming.
   * \param type
   */
  void SetMimoBeamformingType (MimoBeamformingType type);
  /**
   * This field is set to 0 to indicate the non-reciprocal MIMO phase and is set to 1
   * to indicate the reciprocal MIMO phase.
   * \param type
   */
  void SetMimoPhaseType (MimoPhaseType type);
  /**
   * Indicates the EDMG Group ID of target MU group. This field is reserved when the
   * SU/MU field is set to 0.
   * \param groupID
   */
  void SetEDMGGroupID (uint8_t groupID);
  /**
   * SetGroupUserMask
   * \param mask
   */
  void SetGroupUserMask (uint32_t mask);
  /**
   * Indicates the requested number of consecutive TRN-Units in which the same
   * AWV is used in the transmission of the last M TRN subfields of each TRN-Unit.
   * This field is reserved when the SU/MU field is set to 0.
   * \param number
   */
  void SetLTxRx (uint8_t number);
  /**
   * The value of this field plus one indicates the requested number of TRN
   * subfields in a TRN-Unit that can be used for transmit training.
   * This field is reserved when the SU/MU field is set to 1.
   * \param unit
   */
  void SetRequestedEDMGTRNUnitM (uint8_t unit);
  /**
   * This field is set to 1 to indicate the sender is the initiator and is set to 0
   * otherwise. This field is set to 1 when the SU/MU field is set to 1.
   * \param isInitiator
   */
  void SetAsInitiator (bool isInitiator);
  /**
   * If the Channel Measurement Requested subfield is set to 1, the Channel Measurement
   * subfield is requested as part of MIMO BF feedback. Otherwise, set to 0
   * \param value
   */
  void SetChannelMeasurementRequested (bool value);
  /**
   * The Number of Taps Requested subfield indicates the number of taps requested in each
   * channel measurement. The encoding for this subfield is specified in Table 9-234.
   * \param taps
   */
  void SetNumberOfTapsRequested (uint8_t taps);
  /**
   * The value of the Number of TX Sector Combinations Requested subfield plus one indicates
   * the number of TX sector combinations requested as part of MIMO BF feedback.
   * \param requested
   */
  void SetNumberOfTXSectorCombinationsRequested (uint8_t requested);
  /**
   * The Channel Aggregation Requested subfield is set to 1 to indicate that the TRN field is transmitted over a
   * 2.16+2.16 GHz or 4.32+4.32 GHz channel and to request the channel measurement feedback per channel,
   * in case channel aggregation is used as part of MIMO BF feedback. Otherwise, this subfield is set to 0.
   * \param requested
   */
  void SetChannelAggregationRequested (bool requested);

  MimoBeamformingType GetMimoBeamformingType (void) const;
  MimoPhaseType GetMimoPhaseType (void) const;
  uint8_t GetEDMGGroupID (void) const;
  uint32_t GetGroupUserMask (void) const;
  uint8_t GetLTxRx (void) const;
  uint8_t GetRequestedEDMGTRNUnitM (void) const;
  bool IsInitiator (void) const;
  bool IsChannelMeasurementRequested (void) const;
  uint8_t GetNumberOfTapsRequested (void) const;
  uint8_t GetNumberOfTXSectorCombinationsRequested (void) const;
  bool IsChannelAggregationRequested (void) const;

private:
  MimoBeamformingType m_mimoBeamformingType;
  MimoPhaseType m_mimoPhaseType;
  uint8_t m_edmgGroupID;
  uint32_t m_groupUserMask;
  uint8_t m_LTxRx;
  uint8_t m_requestedEDMGTRNUnitM;
  bool m_isInitiator;
  bool m_channelMeasurementRequested;
  uint8_t m_numberOfTapsRequested;
  uint8_t m_numberOfTXSectorCombinationsRequested;
  bool m_channelAggregationRequested;

};

std::ostream &operator << (std::ostream &os, const MimoSetupControlElement &element);
std::istream &operator >> (std::istream &is, MimoSetupControlElement &element);

ATTRIBUTE_HELPER_HEADER (MimoSetupControlElement);

enum PollType {
  POLL_TRAINING_PACKET = 0,
  POLL_MIMO_BF_FEEDBACK = 1,
};

/**
 * \ingroup wifi
 */
class MimoPollControlElement : public WifiInformationElement
{
public:
  MimoPollControlElement ();

  WifiInformationElementId ElementId () const;
  WifiInformationElementId ElementIdExtension () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * This field is set to 0 to indicate SU-MIMO beamforming and is set to 1 to indicate
   * MU-MIMO beamforming.
   * \param type
   */
  void SetMimoBeamformingType (MimoBeamformingType type);
  /**
   * This field is set to 1 to indicate training packet poll used in the reciprocal MU-MIMO
   * beamforming and is set to 0 to indicate MIMO BF feedback poll used in the SU-MIMO
   * beamforming or the non-reciprocal MU-MIMO beamforming.
   * \param type
   */
  void SetPollType (PollType type);
  /**
   * Indicates the requested number of consecutive TRN-Units in which the same
   * AWV is used in the transmission of the last M TRN subfields of each TRN-Unit.
   * This field is reserved when the Poll Type field is set to 0.
   * \param number
   */
  void SetLTxRx (uint8_t number);
  /**
   * The value of this field plus one indicates the requested number of TRN
   * subfields in a TRN-Unit that can be used for transmit training.
   * This field is reserved when the Poll Type field is set to 0.
   * \param unit
   */
  void SetRequestedEDMGTRNUnitM (uint8_t unit);
  /**
   * Indicates the requested number of TRN subfields at the start of a TRN-Unit that use the
   * same AWV. A value of zero indicates zero requested TRN subfields, a value of one
   * indicates one requested TRN subfield, a value of two indicates two requested TRN
   * subfields and a value of three indicates four requested TRN subfields. This field is reserved
   * when the Poll Type field is set to 0.
   * \param unit
   */
  void SetRequestedEDMGTRNUnitP (uint8_t unit);
  /**
   * Indicates the maximum duration, in microseconds, during which EDMG BRP-RX/TX
   * packets can be transmitted by the polled responder following the transmission of the MIMO
   * BF Poll frame containing this element. Possible values range from 1 to 16383. This field is
   * reserved when the Poll Type field is set to 0.
   * \param duration
   */
  void SetTrainingDuration (uint16_t duration);

  MimoBeamformingType GetMimoBeamformingType (void) const;
  PollType GetPollType (void) const;
  uint8_t GetLTxRx (void) const;
  uint8_t GetRequestedEDMGTRNUnitM (void) const;
  uint8_t GetRequestedEDMGTRNUnitP (void) const;
  uint16_t GetTrainingDuration (void) const;

private:
  MimoBeamformingType m_mimoBeamformingType;
  PollType m_pollType;
  uint8_t m_LTxRx;
  uint8_t m_requestedEDMGTRNUnitM;
  uint8_t m_requestedEDMGTRNUnitP;
  uint16_t m_duration;

};

std::ostream &operator << (std::ostream &os, const MimoPollControlElement &element);
std::istream &operator >> (std::istream &is, MimoPollControlElement &element);

ATTRIBUTE_HELPER_HEADER (MimoPollControlElement);

/**
 * \ingroup wifi
 *
 * The MIMO Feedback Control element is used to carry configuration information for
 * accompanying Channel Measurement Feedback element, EDMG Channel Measurement Feedback
 * element and/or Digital BF Feedback element.
 *
 */
class MIMOFeedbackControl : public WifiInformationElement
{
public:
  MIMOFeedbackControl ();

  WifiInformationElementId ElementId () const;
  WifiInformationElementId ElementIdExtension () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * This field is set to 0 to indicate SU-MIMO beamforming and is set to 1 to indicate
   * MU-MIMO beamforming.
   * \param type
   */
  void SetMimoBeamformingType (MimoBeamformingType type);
  /**
   * This field is set to 0 to indicate initiator link and is set to 1 otherwise.
   * This field is set to 0 when the SU/MU field is set to 1.
   * \param isInitiator
   */
  void SetLinkTypeAsInitiator (bool isInitiator);
  /**
   * This field indicates whether MIMO BF feedback is included in the MIMO BF Feedback
   * frame containing the MIMO Feedback Control element or when the EDMG STA
   * transmitting the MIMO Feedback Control element will be ready with MIMO BF feedback.
   * \param delay
   */
  void SetComebackDelay (uint8_t delay);

  /** The following fields are reserved when the Comeback Delay field is set to a nonzero value. **/

  /**
   * If the Channel Measurement Present subfield is set to 1, the Channel Measurement
   * subfield is present as part of the MIMO BF feedback. Otherwise, set to 0.
   * \param present
   */
  void SetChannelMeasurementPresent (bool present);
  /**
   * If the Tap Delay Present subfield is set to 1, the Tap Delay subfield is present as part of
   * the MIMO BF feedback. Otherwise, set to 0.
   * \param present
   */
  void SetTapDelayPresent (bool present);
  /**
   * The Number of Taps Present subfield indicates the number of taps present in each channel measurement.
   * This subfield has the same encoding as the Number of Taps Requested subfield.
   * \param taps
   */
  void SetNumberOfTapsPresent (uint8_t taps);
  /**
   * The value of the Number of TX Sector Combinations Present subfield plus one indicates the number of TX
   * sector combinations, N tsc , for the MIMO BF feedback. The number of measurements, Nmeas, is NTX × NRX
   * multiples of the number of TX sector combinations, Ntsc.
   * \param present
   */
  void SetNumberOfTXSectorCombinationsPresent (uint8_t present);
  /**
   * The Precoder Information Present subfield is set to 1 to indicate that precoding information is present as
   * part of the MIMO BF feedback. Otherwise, it is set to 0.
   * \param present
   */
  void SetPrecodingInformationPresent (bool present);
  /**
   * The Channel Aggregation Present subfield is set to 1 to indicate that, in case of channel aggregation,
   * channel measurement feedback per channel is present. Otherwise, it is set to 0.
   * \param present
   */
  void SetChannelAggregationPresent (bool present);

  /** The following subfields are part of the Digital Fbck Control Field that fefines the requirements for
   *  the digital feedback type. **/

  /**
   * Indicates the number of columns, Nc, in the beamforming feedback matrix minus one
   * \param nc The number of columns.
   */
  void SetNumberOfColumns (uint8_t nc);
  /**
   * Indicates the number of rows, Nr, in a beamforming feedback matrix minus one:
   * \param nr The number of rows.
   */
  void SetNumberOfRows (uint8_t nr);
  /**
   * Indicates the Tx Antennas reported in the accompanying Digital BF Feedback element. If the
   * CSI for the i th Tx Antenna is included in the accompanying Digital BF feedback element, the
   * i th bit in Tx Antenna Mask is set to 1. Otherwise, the i th bit in Tx Antenna Mask is set to 0.
   * \param mask
   */
  void SetTxAntennaMask (uint8_t mask);
  /**
   * Indicates the number of contiguous 2.16 GHz channels the measurement was made for minus one:
   * Set to 0 for 2.16 GHz
   * Set to 1 for 4.32 GHz
   * Set to 2 for 6.48 GHz
   * Set to 3 for 8.64 GHz
   * \param ncb The number of contiguous 2.16 GHz channels the measurement was made for.
   */
  void SetNumberOfContiguousChannels (uint8_t ncb);
  /**
   * Indicates the subcarrier grouping, Ng, used for beamforming feedback matrix
   * Set to 0 for N g = 2
   * Set to 1 for N g = 4
   * Set to 2 for N g = 8
   * Set to 3 for dynamic grouping; reserved if dynamic grouping is not supported
   * If the Feedback Type subfield is 0, the Grouping subfield is reserved.
   * \param ng
   */
  void SetGrouping (uint8_t ng);
  /**
   * Indicates the size of codebook entries.
   * If the SU/MU field in the MIMO Feedback Control element is 1:
   *  - Set to 0 for 6 bits for Ψ, 4 bits for φ.
   *  - Value 1 is reserved.
   * If the SU/MU field in the MIMO Feedback Control element is 0:
   *  - Set to 0 for 9 bits for Ψ , 7 bits for φ.
   *  - Value 1 is reserved.
   * \param type
   */
  void SetCodebookInformationType (bool type);
  /**
   * Indicates which type of feedback is provided. Set to 0 for uncompressed beamforming
   * feedback in time domain (EDMG SC mode) and set to 1 for compressed using Givens-
   * Rotation in frequency domain (EDMG OFDM mode).
   * \param type
   */
  void SetFeedbackType (bool type);
  /**
   * This field is represented by the variable Nsc.
   * If the Feedback Type subfield is 0, Nsc is the number of feedback taps per element of the SC
   * feedback matrix.
   * If the Feedback Type subfield is 1 and the Grouping subfield is less than 3, Nsc is determined
   * by Table 29.
   * If the Feedback Type subfield is 1 and the Grouping subfield is 3, Nsc specifies the number of
   * subcarriers present in the Digital Beamforming Feedback Information field of the Digital BF
   * Feedback element minus one.
   * \param number
   */
  void SetNumberOfFeedbackMatricesOrTaps (uint16_t number);

  MimoBeamformingType SetMimoBeamformingType (void) const;
  bool IsLinkTypeAsInitiator (void) const;
  uint8_t SetComebackDelay (void) const;

  /** The following fields are reserved when the Comeback Delay field is set to a nonzero value. **/

  bool IsChannelMeasurementPresent (void) const;
  bool IsTapDelayPresent (void) const;
  uint8_t GetNumberOfTapsPresent (void) const;
  uint8_t GetNumberOfTXSectorCombinationsPresent (void) const;
  bool IsPrecodingInformationPresent(void) const;
  bool IsChannelAggregationPresent (void) const;

  /** The following subfields are part of the Digital Fbck Control Field that fefines the requirements for
   *  the digital feedback type. **/

  uint8_t GetNumberOfColumns (void) const;
  uint8_t GetNumberOfRows (void) const;
  uint8_t GetTxAntennaMask (void) const;
  uint8_t GetNumberOfContiguousChannels (void) const;
  uint8_t GetGrouping (void) const;
  bool GetCodebookInformationType (void) const;
  bool GetFeedbackType (void) const;
  uint16_t GetNumberOfFeedbackMatricesOrTaps (void) const;

private:
  MimoBeamformingType m_mimoBeamformingType;
  bool m_linkType;
  uint8_t m_comebackDelay;

  bool m_isChannelMeasurementPresent;
  bool m_isTapDelayPresent;
  uint8_t m_numberOfTapsPresent;
  uint8_t m_nummberOfTXSectorCombinationsPresent;
  bool m_isPrecodingInformationPresent;
  bool m_ssChannelAggregationPresent;

  uint8_t m_numberOfColumns;
  uint8_t m_numberOfRows;
  uint8_t m_txAntennaMask;
  uint8_t m_numberOfContiguousChannels;
  uint8_t m_grouping;
  bool m_codebookInformationType;
  bool m_feedbackType;
  uint16_t m_numberOfFeedbackMatricesOrTaps;

};

std::ostream &operator << (std::ostream &os, const MIMOFeedbackControl &element);
std::istream &operator >> (std::istream &is, MIMOFeedbackControl &element);

ATTRIBUTE_HELPER_HEADER (MIMOFeedbackControl);

struct ReciprocalConfigData {
  uint16_t ConfigurationAWVFeedbackID;  /* 11 Bits */
  uint8_t ConfigurationBRPCDOWN;        /*  6 Bits */
  uint8_t ConfigurationRXAntennaID;     /*  3 Bits */
};

typedef std::vector<ReciprocalConfigData> ReciprocalConfigDataList;
typedef ReciprocalConfigDataList::const_iterator ReciprocalConfigDataListCI;
typedef ReciprocalConfigDataList::iterator ReciprocalConfigDataListI;

struct ReciprocalTransmissionConfig {
  ANTENNA_ID antennaID;
  uint32_t reciprocalConfigGroupUserMask;
  ReciprocalConfigDataList configList;
};

typedef std::vector<ReciprocalTransmissionConfig> ReciprocalTransmissionConfigList;
typedef ReciprocalTransmissionConfigList::const_iterator ReciprocalTransmissionConfigListCI;
typedef ReciprocalTransmissionConfigList::iterator ReciprocalTransmissionConfigListI;

enum MultiUserTransmissionConfigType {
  MU_NonReciprocal = 0,
  MU_Reciprocal = 1
};

/**
 * \ingroup wifi
 *
 * The MIMO Feedback Control element is used to carry configuration information for
 * accompanying Channel Measurement Feedback element, EDMG Channel Measurement Feedback
 * element and/or Digital BF Feedback element.
 *
 */
class MIMOSelectionControlElement : public WifiInformationElement
{
public:
  MIMOSelectionControlElement ();

  WifiInformationElementId ElementId () const;
  WifiInformationElementId ElementIdExtension () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * Indicates the EDMG group ID of target MU group.
   * \param id
   */
  void SetEDMGGroupID (uint8_t id);
  /**
   * Indicates the number of MU-MIMO transmission configurations, N conf .
   * \param number
   */
  void SetNumberOfMultiUserConfigurations (uint8_t number);
  /**
   * This field is set to 0 to indicate the MU-MIMO transmission configurations obtained from the non-
   * reciprocal MU-MIMO BF training. This field is set to 1 to indicate the MU-MIMO transmission
   * configurations obtained from the reciprocal MU-MIMO BF training.
   * \param type
   */
  void SetMultiUserTransmissionConfigurationType (MultiUserTransmissionConfigType type);
  void AddReciprocalMUBFTrainingBasedTransmissionConfig (ReciprocalTransmissionConfig &config);

  uint8_t GetEDMGGroupID (void) const;
  uint8_t GetNumberOfMultiUserConfigurations (void) const;
  MultiUserTransmissionConfigType GetMultiUserTransmissionConfigurationType (void) const;
  ReciprocalTransmissionConfigList GetReciprocalTransmissionConfigList (void) const;

private:
  uint8_t m_edmgGroupID;
  uint8_t m_numMUConfigurations;
  MultiUserTransmissionConfigType m_muType;
  ReciprocalTransmissionConfigList m_reciprocalConfigList;

};

std::ostream &operator << (std::ostream &os, const MIMOSelectionControlElement &element);
std::istream &operator >> (std::istream &is, MIMOSelectionControlElement &element);

ATTRIBUTE_HELPER_HEADER (MIMOSelectionControlElement);

struct DigitalFeedbackComponent {
  uint8_t realPart;
  uint8_t imagPart;
};

typedef std::vector<DigitalFeedbackComponent> DigitalBeamformingFeedbackMatrix;
typedef DigitalBeamformingFeedbackMatrix::iterator DigitalBeamformingFeedbackMatrixI;
typedef DigitalBeamformingFeedbackMatrix::const_iterator DigitalBeamformingFeedbackMatrixCI;

typedef std::vector<DigitalBeamformingFeedbackMatrix> DigitalBeamformingFeedbackInformation;
typedef DigitalBeamformingFeedbackInformation::iterator DigitalBeamformingFeedbackInformationI;
typedef DigitalBeamformingFeedbackInformation::const_iterator DigitalBeamformingFeedbackInformationCI;

typedef uint8_t DifferentialSubcarrierIndex;
typedef std::vector<DifferentialSubcarrierIndex> DifferentialSubcarrierIndexList;
typedef DifferentialSubcarrierIndexList::iterator DifferentialSubcarrierIndexListI;
typedef DifferentialSubcarrierIndexList::const_iterator DifferentialSubcarrierIndexListCI;

/**
 * \ingroup wifi
 *
 * The Digital BF Feedback element is transmitted in the MIMO BF Feedback frame and carries feedback
 * information in the form of beamforming feedback matrices and differential SNRs. The feedback
 * information can be used by a transmit beamformer to determine digital beamforming steering matrices, Q.
 * This process is described in 10.43.9.2.4. When the Digital BF Feedback element is transmitted in the
 * MIMO BF Feedback frame, the SNR fields within the Channel Measurement Feedback element are
 * interpreted as average SNR per stream, as defined in Table 9-240 (see 9.4.2.136).
 */
class DigitalBFFeedbackElement : public WifiInformationElement
{
public:
  DigitalBFFeedbackElement ();

  WifiInformationElementId ElementId () const;
  WifiInformationElementId ElementIdExtension () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * If Feedback Type subfield is 0, represents the beamforming matrix in time domain for the 1 st tap as
   * described above. If Feedback Type subfield is 1, represents the beamforming matrix for the 1st
   * subcarrier, indexed by matrix angles in the order shown in Table 27
   */
  void AddDigitalBeamformingFeedbackMatrix (DigitalBeamformingFeedbackMatrix &matrix);
  /**
   * Add differential subcarrier Index
   * \param index
   */
  void AddDifferentialSubcarrierIndex (DifferentialSubcarrierIndex index);
  /**
   * Add relative tap delay
   * \param tapDelay
   */
  void AddRelativeTapDelay (Tap_Delay tapDelay);
  /**
   * Add differential SNR for Space-time stream.
   * \param differentialSNR
   */
  void AddDifferentialSNRforSpaceTimeStream (SNR differentialSNR);

  DigitalBeamformingFeedbackInformation GetDigitalBeamformingFeedbackInformation (void) const;
  DifferentialSubcarrierIndexList GetDifferentialSubcarrierIndex (void) const;
  Tap_Delay_List GetTapDelay (void) const;
  SNR_LIST GetMUExclusiveBeamformingReport (void) const;

private:
  DigitalBeamformingFeedbackInformation m_digitalFBInfo;
  DifferentialSubcarrierIndexList m_differentialSubcarrierIndexList;
  Tap_Delay_List m_tapDelay;
  SNR_LIST m_muExclusiveBeamformingReport;

};

std::ostream &operator << (std::ostream &os, const DigitalBFFeedbackElement &element);
std::istream &operator >> (std::istream &is, DigitalBFFeedbackElement &element);

ATTRIBUTE_HELPER_HEADER (DigitalBFFeedbackElement);

typedef std::vector<Ptr<DigitalBFFeedbackElement> > DigitalBFFeedbackElementList;
typedef DigitalBFFeedbackElementList::iterator DigitalBFFeedbackElementListI;
typedef DigitalBFFeedbackElementList::const_iterator DigitalBFFeedbackElementListCI;

} //namespace ns3

#endif /* DMG_INFORMATION_ELEMENTS_H */
