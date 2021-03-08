/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/fatal-error.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include "common-header.h"
#include "dmg-information-elements.h"

#include "supported-rates.h"
#include "ht-capabilities.h"
#include "ht-operation.h"
#include "erp-information.h"
#include "vht-capabilities.h"
#include "vht-operation.h"
#include "dmg-capabilities.h"
#include "edmg-capabilities.h"
#include "dsss-parameter-set.h"
#include "edca-parameter-set.h"
#include "cf-parameter-set.h"
#include "extended-capabilities.h"
#include "he-capabilities.h"
#include "he-operation.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CommonHeader");

/***********************************************************
 *          Generic Wifi Management Frame
 ***********************************************************/
void
MgtFrame::AddWifiInformationElement (Ptr<WifiInformationElement> element)
{
  if (element->ElementId () != IE_EXTENSION)
    {
      m_map[std::make_pair (element->ElementId (), 0)] = element;
    }
  else
    {
      m_map[std::make_pair (element->ElementId (), element->ElementIdExt ())] = element;
    }
}

Ptr<WifiInformationElement>
MgtFrame::GetInformationElement (WifiInfoElementId id)
{
  WifiInformationElementMap::const_iterator it = m_map.find (id);
  if (it != m_map.end ())
    {
      return it->second;
    }
  else
    {
      return 0;
    }
}

uint32_t
MgtFrame::GetInformationElementsSerializedSize (void) const
{
  Ptr<WifiInformationElement> element;
  uint32_t size = 0;
  for (WifiInformationElementMap::const_iterator elem = m_map.begin (); elem != m_map.end (); elem++)
    {
      element = elem->second;
      size += element->GetSerializedSize ();
    }
  return size;
}

WifiInformationElementMap
MgtFrame::GetListOfInformationElement (void) const
{
  return m_map;
}

void
MgtFrame::PrintInformationElements (std::ostream &os) const
{

}

Buffer::Iterator
MgtFrame::SerializeInformationElements (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  Ptr<WifiInformationElement> element;
  for (WifiInformationElementMap::const_iterator elem = m_map.begin (); elem != m_map.end (); elem++)
    {
      element = elem->second;
      i = element->Serialize (i);
    }
  return i;
}

Buffer::Iterator
MgtFrame::DeserializeInformationElements (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  Ptr<WifiInformationElement> element;
  uint8_t id, extensionId, length;
  while (!i.IsEnd ())
    {
      i = DeserializeExtensionElementID (i, id, length, extensionId);
      if (id != IE_EXTENSION)
        {
          i = DeserializeElementID (i, id , length);
        }
      switch (id)
        {
          case IE_SUPPORTED_RATES:
            {
              element = Create<SupportedRates> ();
              break;
            }
          case IE_EXTENDED_SUPPORTED_RATES:
            {
              element = Create<ExtendedSupportedRatesIE> ();
              break;
            }
          case IE_HT_CAPABILITIES:
            {
              element = Create<HtCapabilities> ();
              break;
            }
          case IE_VHT_CAPABILITIES:
            {
              element = Create<VhtCapabilities> ();
              break;
            }
          case IE_HT_OPERATION:
            {
              element = Create<HtOperation> ();
              break;
            }
          case IE_VHT_OPERATION:
            {
              element = Create<VhtOperation> ();
              break;
            }
          case IE_ERP_INFORMATION:
            {
              element = Create<ErpInformation> ();
              break;
            }
          case IE_EDCA_PARAMETER_SET:
            {
              element = Create<EdcaParameterSet> ();
              break;
            }
          case IE_DSSS_PARAMETER_SET:
            {
              element = Create<DsssParameterSet> ();
              break;
            }
          case IE_DMG_CAPABILITIES:
            {
              element = Create<DmgCapabilities> ();
              break;
            }
          case IE_MULTI_BAND:
            {
              element = Create<MultiBandElement> ();
              break;
            }
          case IE_DMG_OPERATION:
            {
              element = Create<DmgOperationElement> ();
              break;
            }
          case IE_NEXT_DMG_ATI:
            {
              element = Create<NextDmgAti> ();
              break;
            }
          case IE_RELAY_CAPABILITIES:
            {
              element = Create<RelayCapabilitiesElement> ();
              break;
            }
          case IE_EXTENDED_SCHEDULE:
            {
              element = Create<ExtendedScheduleElement> ();
              break;
            }
          case IE_EXTENDED_CAPABILITIES:
            {
              element = Create<ExtendedCapabilities> ();
              break;
            }
          case IE_STA_AVAILABILITY:
            {
              element = Create<StaAvailabilityElement> ();
              break;
            }
          case IE_DMG_BEAM_REFINEMENT:
            {
              element = Create<BeamRefinementElement> ();
              break;
            }
          case IE_CHANNEL_MEASUREMENT_FEEDBACK:
            {
              element = Create<ChannelMeasurementFeedbackElement> ();
              break;
            }
          case IE_CF_PARAMETER_SET:
            {
              element = Create<CfParameterSet> ();
              break;
            }
          case IE_EXTENSION:
            {
              switch (extensionId)
                {
                  case IE_EXT_HE_CAPABILITIES:
                    {
                      element = Create<HeCapabilities> ();
                      break;
                    }
                  case IE_EXT_HE_OPERATION:
                    {
                      element = Create<HeOperation> ();
                      break;
                    }
                  case IE_EXTENSION_EDMG_TRAINING_FIELD_SCHEDULE:
                    {
                      element = Create<EdmgTrainingFieldScheduleElement> ();
                      break;
                    }
                  case IE_EXTENSION_EDMG_CAPABILITIES:
                    {
                      element = Create<EdmgCapabilities> ();
                      break;
                    }
                  case IE_EXTENSION_EDMG_OPERATION:
                    {
                      element = Create<EdmgOperationElement> ();
                      break;
                    }
                  case IE_EXTENSION_EDMG_CHANNEL_MEASUREMENT_FEEDBACK:
                    {
                      element = Create<EDMGChannelMeasurementFeedbackElement> ();
                      break;
                    }
                  case IE_EXTENSION_EDMG_BRP_REQUEST:
                    {
                      element = Create<EdmgBrpRequestElement> ();
                      break;
                    }
                  case IE_EXTENSION_EDMG_GROUP_ID_SET:
                    {
                      element = Create<EDMGGroupIDSetElement> ();
                      break;
                    }
                  case IE_EXTENSION_EDMG_PARTIAL_SECTOR_SWEEP:
                    {
                      element = Create<EdmgPartialSectorLevelSweep> ();
                      break;
                    }
                  default:
                    NS_FATAL_ERROR ("Extension Information Element="  << +extensionId << " is not supported");
                }
              break;
            }
          default:
            NS_FATAL_ERROR ("Information Element="  << +id << " is not supported");
        }

      if (id != IE_EXTENSION)
        {
          i = element->DeserializeElementBody (i, length);
          m_map[std::make_pair (id, 0)] = element;
        }
      else
        {
          i = element->DeserializeElementBody (i, length - 1);
          m_map[std::make_pair (id, extensionId)] = element;
        }
    }

  return i;
}

}
