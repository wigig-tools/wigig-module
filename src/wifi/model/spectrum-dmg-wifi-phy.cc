/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 *
 * Ported from yans-wifi-phy.cc by several contributors starting
 * with Nicola Baldo and Dean Armstrong
 */

#include "ns3/abort.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/wifi-spectrum-value-helper.h"

#include "spectrum-dmg-wifi-phy.h"
#include "wifi-spectrum-signal-parameters.h"
#include "wifi-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SpectrumDmgWifiPhy");

NS_OBJECT_ENSURE_REGISTERED (SpectrumDmgWifiPhy);

DmgWifiSpectrumSignalParameters::DmgWifiSpectrumSignalParameters ()
{
  NS_LOG_FUNCTION (this);
}

DmgWifiSpectrumSignalParameters::DmgWifiSpectrumSignalParameters (const DmgWifiSpectrumSignalParameters& p)
  : SpectrumSignalParameters (p)
{
  NS_LOG_FUNCTION (this << &p);
  packet = p.packet;
  plcpFieldType = p.plcpFieldType;
  txVector = p.txVector;
}

Ptr<SpectrumSignalParameters>
DmgWifiSpectrumSignalParameters::Copy ()
{
  NS_LOG_FUNCTION (this);
  // Ideally we would use:
  //   return Copy<WifiSpectrumSignalParameters> (*this);
  // but for some reason it doesn't work. Another alternative is
  //   return Copy<WifiSpectrumSignalParameters> (this);
  // but it causes a double creation of the object, hence it is less efficient.
  // The solution below is copied from the implementation of Copy<> (Ptr<>) in ptr.h
  Ptr<DmgWifiSpectrumSignalParameters> wssp (new DmgWifiSpectrumSignalParameters (*this), false);
  return wssp;
}

TypeId
SpectrumDmgWifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpectrumDmgWifiPhy")
    .SetParent<WifiPhy> ()
    .SetGroupName ("Wifi")
    .AddConstructor<SpectrumDmgWifiPhy> ()
    .AddAttribute ("DisableWifiReception", "Prevent Wi-Fi frame sync from ever happening",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SpectrumDmgWifiPhy::m_disableWifiReception),
                   MakeBooleanChecker ())
    .AddTraceSource ("SignalArrival",
                     "Signal arrival",
                     MakeTraceSourceAccessor (&SpectrumDmgWifiPhy::m_signalCb),
                     "ns3::SpectrumDmgWifiPhy::SignalArrivalCallback")
  ;
  return tid;
}

SpectrumDmgWifiPhy::SpectrumDmgWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

SpectrumDmgWifiPhy::~SpectrumDmgWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
SpectrumDmgWifiPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_wifiSpectrumPhyInterface = 0;
  WifiPhy::DoDispose ();
}

void
SpectrumDmgWifiPhy::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  WifiPhy::DoInitialize ();
  // This connection is deferred until frequency and channel width are set
  if (m_channel && m_wifiSpectrumPhyInterface)
    {
      m_channel->AddRx (m_wifiSpectrumPhyInterface);
    }
  else
    {
      NS_FATAL_ERROR ("SpectrumDmgWifiPhy misses channel and WifiSpectrumPhyInterface objects at initialization time");
    }
}

Ptr<const SpectrumModel>
SpectrumDmgWifiPhy::GetRxSpectrumModel () const
{
  NS_LOG_FUNCTION (this);
    if (m_rxSpectrumModel)
    {
      return m_rxSpectrumModel;
    }
  else
    {
      if (GetFrequency () == 0)
        {
          NS_LOG_DEBUG ("Frequency is not set; returning 0");
          return 0;
        }
      else
        {
          uint16_t channelWidth = GetChannelWidth ();
          NS_LOG_DEBUG ("Creating spectrum model from frequency/width pair of (" << GetFrequency () << ", " << +channelWidth << ")");
          m_rxSpectrumModel = WifiSpectrumValueHelper::GetDmgSpectrumModel (GetFrequency (), channelWidth, GetBandBandwidth (),
                                                                            GetGuardBandwidth (channelWidth));
        }
    }
  return m_rxSpectrumModel;
}

Ptr<Channel>
SpectrumDmgWifiPhy::GetChannel (void) const
{
  return m_channel;
}

void
SpectrumDmgWifiPhy::SetChannel (const Ptr<SpectrumChannel> channel)
{
  m_channel = channel;
}

void
SpectrumDmgWifiPhy::ResetSpectrumModel (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (IsInitialized (), "Executing method before run-time");
  uint8_t channelWidth = GetChannelWidth ();
  NS_LOG_DEBUG ("Run-time change of spectrum model from frequency/width pair of (" << GetFrequency () << ", " << +channelWidth << ")");
  // Replace existing spectrum model with new one, and must call AddRx ()
  // on the SpectrumChannel to provide this new spectrum model to it
  m_rxSpectrumModel = WifiSpectrumValueHelper::GetSpectrumModel (GetFrequency (), channelWidth, GetBandBandwidth (), GetGuardBandwidth (channelWidth));
  m_channel->AddRx (m_wifiSpectrumPhyInterface);
}

uint16_t
SpectrumDmgWifiPhy::GetGuardBandwidth (uint16_t currentChannelWidth) const
{
  uint16_t guardBandwidth = 0;
  if (currentChannelWidth == 22)
    {
      //handle case of use of legacy DSSS transmission
      guardBandwidth = 10;
    }
  else
    {
      //In order to properly model out of band transmissions for OFDM, the guard
      //band has been configured so as to expand the modeled spectrum up to the
      //outermost referenced point in "Transmit spectrum mask" sections' PSDs of
      //each PHY specification of 802.11-2016 standard. It thus ultimately corresponds
      //to the currently considered channel bandwidth (which can be different from
      //supported channel width).
      guardBandwidth = currentChannelWidth;
    }
  return guardBandwidth;
}

double
SpectrumDmgWifiPhy::GetBandBandwidth (void) const
{
  double bandBandwidth = 0;
  switch (GetStandard ())
    {
    case WIFI_PHY_STANDARD_80211ad:
      // Use OFDM subcarrier width of 5.15625 MHz as band granularity
      bandBandwidth = 5156250;
      break;
    default:
      NS_FATAL_ERROR ("Standard unknown: " << GetStandard ());
      break;
    }
  return bandBandwidth;
}

void
SpectrumDmgWifiPhy::SetChannelNumber (uint8_t nch)
{
  NS_LOG_FUNCTION (this << +nch);
  DmgWifiPhy::SetChannelNumber (nch);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumDmgWifiPhy::SetFrequency (uint16_t freq)
{
  NS_LOG_FUNCTION (this << freq);
  DmgWifiPhy::SetFrequency (freq);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumDmgWifiPhy::SetChannelWidth (uint16_t channelwidth)  //TR++ ChannelWidth needs to be changed for 802.11ad/ay
{
  NS_LOG_FUNCTION (this << +channelwidth);
  DmgWifiPhy::SetChannelWidth (channelwidth);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumDmgWifiPhy::ConfigureStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  DmgWifiPhy::ConfigureStandard (standard);
  if (IsInitialized ())
    {
      ResetSpectrumModel ();
    }
}

void
SpectrumDmgWifiPhy::StartRx (Ptr<SpectrumSignalParameters> rxParams)
{
  NS_LOG_FUNCTION (this << rxParams);
  Time rxDuration = rxParams->duration;
  Ptr<SpectrumValue> receivedSignalPsd = rxParams->psd;
  NS_LOG_DEBUG ("Received signal with PSD " << *receivedSignalPsd << " and duration " << rxDuration.As (Time::NS));
  uint32_t senderNodeId = 0;
  if (rxParams->txPhy)
    {
      senderNodeId = rxParams->txPhy->GetDevice ()->GetNode ()->GetId ();
    }
  NS_LOG_DEBUG ("Received signal from " << senderNodeId << " with unfiltered power " << WToDbm (Integral (*receivedSignalPsd)) << " dBm");
  // Integrate over our receive bandwidth (i.e., all that the receive
  // spectral mask representing our filtering allows) to find the
  // total energy apparent to the "demodulator".

  //TR++ channelWidth must be uint16_t
  uint16_t channelWidth = GetChannelWidth ();
  Ptr<SpectrumValue> filter = WifiSpectrumValueHelper::CreateRfFilter (GetFrequency (), channelWidth, GetBandBandwidth (), GetGuardBandwidth (channelWidth));
  SpectrumValue filteredSignal = (*filter) * (*receivedSignalPsd);
  // Add receiver antenna gain
  NS_LOG_DEBUG ("Signal power received (watts) before antenna gain: " << Integral (filteredSignal));
  double rxPowerW = Integral (filteredSignal) * DbToRatio (GetRxGain ());
  NS_LOG_DEBUG ("Signal power received after antenna gain: " << rxPowerW << " W (" << WToDbm (rxPowerW) << " dBm)");

  Ptr<DmgWifiSpectrumSignalParameters> wifiRxParams = DynamicCast<DmgWifiSpectrumSignalParameters> (rxParams);

  // Log the signal arrival to the trace source
  m_signalCb (wifiRxParams ? true : false, senderNodeId, WToDbm (rxPowerW), rxDuration);
  if (wifiRxParams == 0)
    {
      NS_LOG_INFO ("Received non Wi-Fi signal");
      m_interference.AddForeignSignal (rxDuration, rxPowerW);
      SwitchMaybeToCcaBusy ();
      return;
    }
  if (wifiRxParams && m_disableWifiReception)
    {
      NS_LOG_INFO ("Received Wi-Fi signal but blocked from syncing");
      m_interference.AddForeignSignal (rxDuration, rxPowerW);
      SwitchMaybeToCcaBusy ();
      return;
    }

  if (wifiRxParams->plcpFieldType == PLCP_80211AD_PREAMBLE_HDR_DATA)
    {
      NS_LOG_INFO ("Received DMG WiFi signal");
      Ptr<Packet> packet = wifiRxParams->packet->Copy ();
      StartReceivePreambleAndHeader (packet, rxPowerW, rxDuration);
    }
  else if (wifiRxParams->plcpFieldType == PLCP_80211AD_AGC_SF)
    {
      NS_LOG_INFO ("Received DMG WiFi AGC-SF signal");
      StartReceiveAgcSubfield (wifiRxParams->txVector, WToDbm (rxPowerW));
    }
  else if (wifiRxParams->plcpFieldType == PLCP_80211AD_TRN_CE_SF)
    {
      NS_LOG_INFO ("Received DMG WiFi TRN-CE Subfield signal");
      StartReceiveCeSubfield (wifiRxParams->txVector, WToDbm (rxPowerW));
    }
  else if (wifiRxParams->plcpFieldType == PLCP_80211AD_TRN_SF)
    {
      NS_LOG_INFO ("Received DMG WiFi TRN-SF signal");
      StartReceiveTrnSubfield (wifiRxParams->txVector, WToDbm (rxPowerW));
    }
}

Ptr<DmgWifiSpectrumPhyInterface>
SpectrumDmgWifiPhy::GetSpectrumPhy (void) const
{
  return m_wifiSpectrumPhyInterface;
}

void
SpectrumDmgWifiPhy::CreateWifiSpectrumPhyInterface (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  m_wifiSpectrumPhyInterface = CreateObject<DmgWifiSpectrumPhyInterface> ();
  m_wifiSpectrumPhyInterface->SetSpectrumDmgWifiPhy (this);
  m_wifiSpectrumPhyInterface->SetDevice (device);
}

Ptr<SpectrumValue>
SpectrumDmgWifiPhy::GetTxPowerSpectralDensity (uint16_t centerFrequency, uint16_t channelWidth,
                                               double txPowerW, WifiModulationClass modulationClass) const
{
  NS_LOG_FUNCTION (centerFrequency << channelWidth << txPowerW << modulationClass);
  Ptr<SpectrumValue> v;
  switch (modulationClass)
    {
    case WIFI_MOD_CLASS_DMG_CTRL:
    case WIFI_MOD_CLASS_DMG_SC:
    case WIFI_MOD_CLASS_DMG_LP_SC:
      v = WifiSpectrumValueHelper::CreateDmgControlTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW,
                                                                           GetGuardBandwidth (channelWidth));
      break;
    case WIFI_MOD_CLASS_DMG_OFDM:
      //TODO VINCENT - We consider that every mode creates the same spectral density which should not be the case
      // Create one different function for every different case
      v = WifiSpectrumValueHelper::CreateDmgOfdmTxPowerSpectralDensity (centerFrequency, channelWidth, txPowerW,
                                                                        GetGuardBandwidth (channelWidth));
      break;
    default:
      NS_FATAL_ERROR ("modulation class unknown: " << modulationClass);
      break;
    }
  return v;
}

uint32_t
SpectrumDmgWifiPhy::GetCenterFrequencyForChannelWidth (WifiTxVector txVector) const
{
  NS_LOG_FUNCTION (this << txVector);
  uint32_t centerFrequencyForSupportedWidth = GetFrequency ();
  // TR++ Change supportedWidth and currentWidth to uint16_t
  uint16_t supportedWidth = GetChannelWidth ();
  uint16_t currentWidth = txVector.GetChannelWidth ();
  // TR--
  if (currentWidth != supportedWidth)
    {
      uint32_t startingFrequency = centerFrequencyForSupportedWidth - static_cast<uint32_t> (supportedWidth / 2);
      return startingFrequency + static_cast<uint32_t> (currentWidth / 2); // primary channel is in the lower part (for the time being)
    }
  return centerFrequencyForSupportedWidth;
}

void
SpectrumDmgWifiPhy::StartTx (Ptr<Packet> packet, WifiTxVector txVector, Time txDuration)
{
  NS_LOG_DEBUG ("Start transmission: signal power before antenna gain=" << GetPowerDbm (txVector.GetTxPowerLevel ()) << "dBm");
  double txPowerWatts = DbmToW (GetPowerDbm (txVector.GetTxPowerLevel ()) + GetTxGain ());
  Ptr<SpectrumValue> txPowerSpectrum = GetTxPowerSpectralDensity (GetCenterFrequencyForChannelWidth (txVector), txVector.GetChannelWidth (), txPowerWatts, txVector.GetMode ().GetModulationClass ());
  Ptr<DmgWifiSpectrumSignalParameters> txParams = Create<DmgWifiSpectrumSignalParameters> ();
  txParams->duration = txDuration;
  txParams->psd = txPowerSpectrum;
  NS_ASSERT_MSG (m_wifiSpectrumPhyInterface, "SpectrumPhy() is not set; maybe forgot to call CreateWifiSpectrumPhyInterface?");
  txParams->txPhy = m_wifiSpectrumPhyInterface->GetObject<SpectrumPhy> ();
  txParams->packet = packet;
  txParams->plcpFieldType = PLCP_80211AD_PREAMBLE_HDR_DATA;
  NS_LOG_DEBUG ("Starting transmission with power " << WToDbm (txPowerWatts) << " dBm on channel " << +GetChannelNumber ());
  NS_LOG_DEBUG ("Starting transmission with integrated spectrum power " << WToDbm (Integral (*txPowerSpectrum)) << " dBm; spectrum model Uid: " << txPowerSpectrum->GetSpectrumModel ()->GetUid ());
  m_channel->StartTx (txParams);
}

void
SpectrumDmgWifiPhy::TxSubfield (WifiTxVector txVector, PLCP_FIELD_TYPE fieldType, Time txDuration)
{
  NS_LOG_DEBUG ("Start transmission: signal power before antenna gain=" << GetPowerDbm (txVector.GetTxPowerLevel ()) << "dBm");
  double txPowerWatts = DbmToW (GetPowerDbm (txVector.GetTxPowerLevel ()) + GetTxGain ());
  Ptr<SpectrumValue> txPowerSpectrum = GetTxPowerSpectralDensity (GetCenterFrequencyForChannelWidth (txVector), txVector.GetChannelWidth (), txPowerWatts, txVector.GetMode ().GetModulationClass ());
  Ptr<DmgWifiSpectrumSignalParameters> txParams = Create<DmgWifiSpectrumSignalParameters> ();
  txParams->duration = txDuration;
  txParams->psd = txPowerSpectrum;
  NS_ASSERT_MSG (m_wifiSpectrumPhyInterface, "SpectrumPhy() is not set; maybe forgot to call CreateWifiSpectrumPhyInterface?");
  txParams->txPhy = m_wifiSpectrumPhyInterface->GetObject<SpectrumPhy> ();
  txParams->plcpFieldType = fieldType;
  txParams->txVector = txVector;
  NS_LOG_DEBUG ("Starting transmission with power " << WToDbm (txPowerWatts) << " dBm on channel " << +GetChannelNumber ());
  NS_LOG_DEBUG ("Starting transmission with integrated spectrum power " << WToDbm (Integral (*txPowerSpectrum)) << " dBm; spectrum model Uid: " << txPowerSpectrum->GetSpectrumModel ()->GetUid ());
  m_channel->StartTx (txParams);
}

void
SpectrumDmgWifiPhy::StartAgcSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  TxSubfield (txVector, PLCP_80211AD_AGC_SF, AGC_SF_DURATION);
}

void
SpectrumDmgWifiPhy::StartCeSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  TxSubfield (txVector, PLCP_80211AD_TRN_CE_SF, TRN_CE_DURATION);
}

void
SpectrumDmgWifiPhy::StartTrnSubfieldTx (WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this);
  TxSubfield (txVector, PLCP_80211AD_TRN_SF, TRN_SUBFIELD_DURATION);
}

} //namespace ns3
