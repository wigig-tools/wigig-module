/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "association-state-machine.h"
#include "wifi-phy.h"
#include "mac-low.h"
#include "mgt-headers.h"
#include "snr-tag.h"
#include "wifi-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AssociationStateMachine");

NS_OBJECT_ENSURE_REGISTERED (AssociationStateMachine);

TypeId
AssociationStateMachine::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::AssociationStateMachine")
    .SetParent<InfrastructureWifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<AssociationStateMachine> ()
    .AddAttribute ("ProbeRequestTimeout", "The duration to actively probe the channel.",
                   TimeValue (Seconds (0.05)),
                   MakeTimeAccessor (&AssociationStateMachine::m_probeRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("WaitBeaconTimeout", "The duration to dwell on a channel while passively scanning for beacon",
                   TimeValue (MilliSeconds (120)),
                   MakeTimeAccessor (&AssociationStateMachine::m_waitBeaconTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("AssocRequestTimeout", "The interval between two consecutive association request attempts.",
                   TimeValue (Seconds (0.5)),
                   MakeTimeAccessor (&AssociationStateMachine::m_assocRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("MaxMissedBeacons",
                   "Number of beacons which much be consecutively missed before "
                   "we attempt to restart association.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&AssociationStateMachine::m_maxMissedBeacons),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ActiveProbing",
                   "If true, we send probe requests. If false, we don't."
                   "NOTE: if more than one STA in your simulation is using active probing, "
                   "you should enable it at a different simulation time for each STA, "
                   "otherwise all the STAs will start sending probes at the same time resulting in collisions. "
                   "See bug 1060 for more info.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&AssociationStateMachine::SetActiveProbing, &AssociationStateMachine::GetActiveProbing),
                   MakeBooleanChecker ())
    .AddTraceSource ("Assoc", "Associated with an access point.",
                     MakeTraceSourceAccessor (&AssociationStateMachine::m_assocLogger),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("DeAssoc", "Association with an access point lost.",
                     MakeTraceSourceAccessor (&AssociationStateMachine::m_deAssocLogger),
                     "ns3::Mac48Address::TracedCallback")
  ;
  return tid;
}

AssociationStateMachine::AssociationStateMachine ()
  : m_state (UNASSOCIATED),
    m_aid (0),
    m_waitBeaconEvent (),
    m_probeRequestEvent (),
    m_assocRequestEvent (),
    m_beaconWatchdogEnd (Seconds (0))
{
  NS_LOG_FUNCTION (this);
}

void
AssociationStateMachine::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
}

AssociationStateMachine::~AssociationStateMachine ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
AssociationStateMachine::GetAssociationId (void) const
{
  NS_ASSERT_MSG (IsAssociated (), "This station is not associated to any AP");
  return m_aid;
}

void
AssociationStateMachine::SetActiveProbing (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_activeProbing = enable;
  if (m_state == WAIT_PROBE_RESP || m_state == WAIT_BEACON)
    {
      NS_LOG_DEBUG ("STA is still scanning, reset scanning process");
      StartScanning ();
    }
}

bool
AssociationStateMachine::GetActiveProbing (void) const
{
  return m_activeProbing;
}

void
AssociationStateMachine::SendAssociationRequest (bool isReassoc)
{
  NS_LOG_FUNCTION (this << GetBssid () << isReassoc);
  if (m_assocRequestEvent.IsRunning ())
    {
      m_assocRequestEvent.Cancel ();
    }
  m_assocRequestEvent = Simulator::Schedule (m_assocRequestTimeout,
                                             &AssociationStateMachine::AssocRequestTimeout, this);
}
void
AssociationStateMachine::TryToEnsureAssociated (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state)
    {
    case ASSOCIATED:
      return;
      break;
    case WAIT_PROBE_RESP:
      /* we have sent a probe request earlier so we
         do not need to re-send a probe request immediately.
         We just need to wait until probe-request-timeout
         or until we get a probe response
       */
      break;
    case WAIT_BEACON:
      /* we have initiated passive scanning, continue to wait
         and gather beacons
       */
      break;
    case UNASSOCIATED:
      /* we were associated but we missed a bunch of beacons
       * so we should assume we are not associated anymore.
       * We try to initiate a scan now.
       */
      m_linkDown ();
      StartScanning ();
      break;
    case WAIT_ASSOC_RESP:
      /* we have sent an association request so we do not need to
         re-send an association request right now. We just need to
         wait until either assoc-request-timeout or until
         we get an association response.
       */
      break;
    case REFUSED:
      /* we have sent an association request and received a negative
         association response. We wait until someone restarts an
         association with a given SSID.
       */
      break;
    }
}

void
AssociationStateMachine::StartScanning (void)
{
  NS_LOG_FUNCTION (this);
  m_candidateAps.clear ();
  if (m_probeRequestEvent.IsRunning ())
    {
      m_probeRequestEvent.Cancel ();
    }
  if (m_waitBeaconEvent.IsRunning ())
    {
      m_waitBeaconEvent.Cancel ();
    }
  if (GetActiveProbing ())
    {
      SetState (WAIT_PROBE_RESP);
      SendProbeRequest ();
      m_probeRequestEvent = Simulator::Schedule (m_probeRequestTimeout,
                                                 &AssociationStateMachine::ScanningTimeout,
                                                 this);
    }
  else
    {
      SetState (WAIT_BEACON);
      m_waitBeaconEvent = Simulator::Schedule (m_waitBeaconTimeout,
                                               &AssociationStateMachine::ScanningTimeout,
                                               this);
    }
}

void
AssociationStateMachine::ScanningTimeout (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_candidateAps.empty ())
    {
      ApInfo bestAp = m_candidateAps.front ();
      m_candidateAps.erase (m_candidateAps.begin ());
      NS_LOG_DEBUG ("Attempting to associate with BSSID " << bestAp.m_bssid);
      Time beaconInterval;
      if (bestAp.m_activeProbing)
        {
          UpdateApInfoFromProbeResp (bestAp.m_probeResp, bestAp.m_apAddr, bestAp.m_bssid);
          beaconInterval = MicroSeconds (bestAp.m_probeResp.GetBeaconIntervalUs ());
        }
      else
        {
          UpdateApInfoFromBeacon (bestAp.m_beacon, bestAp.m_apAddr, bestAp.m_bssid);
          beaconInterval = MicroSeconds (bestAp.m_beacon.GetBeaconIntervalUs ());
        }

      Time delay = beaconInterval * m_maxMissedBeacons;
      RestartBeaconWatchdog (delay);
      SetState (WAIT_ASSOC_RESP);
      SendAssociationRequest (false);
    }
  else
    {
      NS_LOG_DEBUG ("Exhausted list of candidate AP; restart scanning");
      StartScanning ();
    }
}

void
AssociationStateMachine::AssocRequestTimeout (void)
{
  NS_LOG_FUNCTION (this);
  SetState (WAIT_ASSOC_RESP);
  SendAssociationRequest (false);
}

void
AssociationStateMachine::MissedBeacons (void)
{
  NS_LOG_FUNCTION (this);
  if (m_beaconWatchdogEnd > Simulator::Now ())
    {
      if (m_beaconWatchdog.IsRunning ())
        {
          m_beaconWatchdog.Cancel ();
        }
      m_beaconWatchdog = Simulator::Schedule (m_beaconWatchdogEnd - Simulator::Now (),
                                              &AssociationStateMachine::MissedBeacons, this);
      return;
    }
  NS_LOG_DEBUG ("beacon missed");
  SetState (UNASSOCIATED);
  TryToEnsureAssociated ();
}

void
AssociationStateMachine::RestartBeaconWatchdog (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_beaconWatchdogEnd = std::max (Simulator::Now () + delay, m_beaconWatchdogEnd);
  if (Simulator::GetDelayLeft (m_beaconWatchdog) < delay
      && m_beaconWatchdog.IsExpired ())
    {
      NS_LOG_DEBUG ("really restart watchdog.");
      m_beaconWatchdog = Simulator::Schedule (delay, &AssociationStateMachine::MissedBeacons, this);
    }
}

bool
AssociationStateMachine::IsAssociated (void) const
{
  return m_state == ASSOCIATED;
}

bool
AssociationStateMachine::IsWaitAssocResp (void) const
{
  return m_state == WAIT_ASSOC_RESP;
}

void
AssociationStateMachine::Receive (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);
  const WifiMacHeader* hdr = &mpdu->GetHeader ();
  Ptr<const Packet> packet = mpdu->GetPacket ();
  if (hdr->IsBeacon ())
    {
      NS_LOG_DEBUG ("Beacon received");
      MgtBeaconHeader beacon;
      Ptr<Packet> copy = packet->Copy ();
      copy->RemoveHeader (beacon);
      CapabilityInformation capabilities = beacon.GetCapabilities ();
      NS_ASSERT (capabilities.IsEss ());
      bool goodBeacon = false;
      if (GetSsid ().IsBroadcast ()
          || beacon.GetSsid ().IsEqual (GetSsid ()))
        {
          NS_LOG_LOGIC ("Beacon is for our SSID");
          goodBeacon = true;
        }
      SupportedRates rates = beacon.GetSupportedRates ();
      bool bssMembershipSelectorMatch = false;
      for (uint8_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
        {
          uint8_t selector = m_phy->GetBssMembershipSelector (i);
          if (rates.IsBssMembershipSelectorRate (selector))
            {
              NS_LOG_LOGIC ("Beacon is matched to our BSS membership selector");
              bssMembershipSelectorMatch = true;
            }
        }
      if (m_phy->GetNBssMembershipSelectors () > 0 && bssMembershipSelectorMatch == false)
        {
          NS_LOG_LOGIC ("No match for BSS membership selector");
          goodBeacon = false;
        }
      if ((IsWaitAssocResp () || IsAssociated ()) && hdr->GetAddr3 () != GetBssid ())
        {
          NS_LOG_LOGIC ("Beacon is not for us");
          goodBeacon = false;
        }
      if (goodBeacon && m_state == ASSOCIATED)
        {
          m_beaconArrival (Simulator::Now ());
          Time delay = MicroSeconds (beacon.GetBeaconIntervalUs () * m_maxMissedBeacons);
          RestartBeaconWatchdog (delay);
          UpdateApInfoFromBeacon (beacon, hdr->GetAddr2 (), hdr->GetAddr3 ());
        }
      if (goodBeacon && m_state == WAIT_BEACON)
        {
          NS_LOG_DEBUG ("Beacon received while scanning from " << hdr->GetAddr2 ());
          SnrTag snrTag;
          bool removed = copy->RemovePacketTag (snrTag);
          NS_ASSERT (removed);
          ApInfo apInfo;
          apInfo.m_apAddr = hdr->GetAddr2 ();
          apInfo.m_bssid = hdr->GetAddr3 ();
          apInfo.m_activeProbing = false;
          apInfo.m_snr = snrTag.Get ();
          apInfo.m_beacon = beacon;
          UpdateCandidateApList (apInfo);
        }
      return;
    }
  else if (hdr->IsProbeResp ())
    {
      if (m_state == WAIT_PROBE_RESP)
        {
          NS_LOG_DEBUG ("Probe response received while scanning from " << hdr->GetAddr2 ());
          MgtProbeResponseHeader probeResp;
          Ptr<Packet> copy = packet->Copy ();
          copy->RemoveHeader (probeResp);
          if (!probeResp.GetSsid ().IsEqual (GetSsid ()))
            {
              NS_LOG_DEBUG ("Probe response is not for our SSID");
              return;
            }
          SnrTag snrTag;
          bool removed = copy->RemovePacketTag (snrTag);
          NS_ASSERT (removed);
          ApInfo apInfo;
          apInfo.m_apAddr = hdr->GetAddr2 ();
          apInfo.m_bssid = hdr->GetAddr3 ();
          apInfo.m_activeProbing = true;
          apInfo.m_snr = snrTag.Get ();
          apInfo.m_probeResp = probeResp;
          UpdateCandidateApList (apInfo);
        }
      return;
    }
  else if (hdr->IsAssocResp () || hdr->IsReassocResp ())
    {
      if (m_state == WAIT_ASSOC_RESP)
        {
          MgtAssocResponseHeader assocResp;
          packet->PeekHeader (assocResp);
          if (m_assocRequestEvent.IsRunning ())
            {
              m_assocRequestEvent.Cancel ();
            }
          if (assocResp.GetStatusCode ().IsSuccess ())
            {
              SetState (ASSOCIATED);
              m_aid = assocResp.GetAssociationId ();
              if (hdr->IsReassocResp ())
                {
                  NS_LOG_DEBUG ("reassociation done");
                }
              else
                {
                  NS_LOG_DEBUG ("association completed");
                }
              UpdateApInfoFromAssocResp (assocResp, hdr->GetAddr2 ());
              if (!m_linkUp.IsNull ())
                {
                  m_linkUp ();
                }
            }
          else
            {
              NS_LOG_DEBUG ("association refused");
              if (m_candidateAps.empty ())
                {
                  SetState (REFUSED);
                }
              else
                {
                  ScanningTimeout ();
                }
            }
        }
      return;
    }
}

void
AssociationStateMachine::UpdateCandidateApList (ApInfo newApInfo)
{
  NS_LOG_FUNCTION (this << newApInfo.m_bssid << newApInfo.m_apAddr << newApInfo.m_snr
                   << newApInfo.m_activeProbing << newApInfo.m_beacon << newApInfo.m_probeResp);
  // Remove duplicate ApInfo entry
  for (std::vector<ApInfo>::iterator i = m_candidateAps.begin(); i != m_candidateAps.end(); ++i)
    {
      if (newApInfo.m_bssid == (*i).m_bssid)
        {
          m_candidateAps.erase(i);
          break;
        }
    }
  // Insert before the entry with lower SNR
  for (std::vector<ApInfo>::iterator i = m_candidateAps.begin(); i != m_candidateAps.end(); ++i)
    {
      if (newApInfo.m_snr > (*i).m_snr)
        {
          m_candidateAps.insert (i, newApInfo);
          return;
        }
    }
  // If new ApInfo is the lowest, insert at back
  m_candidateAps.push_back(newApInfo);
}

void
AssociationStateMachine::UpdateApInfoFromBeacon (MgtBeaconHeader beacon, Mac48Address apAddr, Mac48Address bssid)
{
  NS_LOG_FUNCTION (this << beacon << apAddr << bssid);
  SetBssid (bssid);
}

void
AssociationStateMachine::UpdateApInfoFromProbeResp (MgtProbeResponseHeader probeResp, Mac48Address apAddr, Mac48Address bssid)
{
  NS_LOG_FUNCTION (this << probeResp << apAddr << bssid);
  SetBssid (bssid);
}

void
AssociationStateMachine::UpdateApInfoFromAssocResp (MgtAssocResponseHeader assocResp, Mac48Address apAddr)
{
  NS_LOG_FUNCTION (this << assocResp << apAddr);
}

void
AssociationStateMachine::SetState (MacState value)
{
  if (value == ASSOCIATED
      && m_state != ASSOCIATED)
    {
      m_assocLogger (GetBssid ());
    }
  else if (value != ASSOCIATED
           && m_state == ASSOCIATED)
    {
      m_deAssocLogger (GetBssid ());
    }
  m_state = value;
}

} //namespace ns3
