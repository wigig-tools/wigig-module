#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

#include "dmg-abft-access.h"
#include "mac-low.h"
#include "mac-tx-middle.h"
#include "wifi-mac.h"
#include "wifi-mac-trailer.h"
#include "wifi-mac-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgAbftAccess");

/**
 * Listener for MacLow events. Forwards to DmgAbftAccess.
 */
class DmgAbftAccess::TransmissionListener : public MacLowTransmissionListener
{
public:
  /**
   * Create a TransmissionListener for the given DmgAbftAccess.
   *
   * \param txop
   */
  TransmissionListener (DmgAbftAccess *txop)
    : MacLowTransmissionListener (),
      m_txop (txop)
  {
  }

  virtual ~TransmissionListener ()
  {
  }

  virtual void Cancel (void)
  {
    m_txop->Cancel ();
  }
  virtual void EndTxNoAck (void)
  {
    m_txop->EndTxNoAck ();
  }

private:
  DmgAbftAccess *m_txop;

};

NS_OBJECT_ENSURE_REGISTERED (DmgAbftAccess);

TypeId
DmgAbftAccess::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgAbftAccess")
    .SetParent<ns3::Dcf> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgAbftAccess> ()
    .AddAttribute ("Queue", "The WifiMacQueue object",
                   PointerValue (),
                   MakePointerAccessor (&DmgAbftAccess::GetQueue),
                   MakePointerChecker<WifiMacQueue> ())
  ;
  return tid;
}

DmgAbftAccess::DmgAbftAccess ()
  : m_currentPacket (0)
{
  NS_LOG_FUNCTION (this);
  m_transmissionListener = new DmgAbftAccess::TransmissionListener (this);
  m_queue = CreateObject<WifiMacQueue> ();
}

DmgAbftAccess::~DmgAbftAccess ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgAbftAccess::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_queue = 0;
  m_low = 0;
  m_stationManager = 0;
  delete m_transmissionListener;
  m_transmissionListener = 0;
  m_txMiddle = 0;
}

void
DmgAbftAccess::SetTxMiddle (MacTxMiddle *txMiddle)
{
  m_txMiddle = txMiddle;
}

void
DmgAbftAccess::SetLow (Ptr<MacLow> low)
{
  NS_LOG_FUNCTION (this << low);
  m_low = low;
}

void
DmgAbftAccess::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> remoteManager)
{
  NS_LOG_FUNCTION (this << remoteManager);
  m_stationManager = remoteManager;
}

void
DmgAbftAccess::SetTxOkCallback (TxPacketOk callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txOkCallback = callback;
}

void
DmgAbftAccess::SetTxOkNoAckCallback (TxOk callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txOkNoAckCallback = callback;
}

void
DmgAbftAccess::SetTxFailedCallback (TxFailed callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txFailedCallback = callback;
}

void
DmgAbftAccess::SetWifiMac (Ptr<WifiMac> mac)
{
  m_wifiMac = mac;
}

Ptr<WifiMac>
DmgAbftAccess::GetWifiMac () const
{
  return m_wifiMac;
}

Ptr<WifiMacQueue >
DmgAbftAccess::GetQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
DmgAbftAccess::Queue (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr, packet);
  m_queue->Enqueue (packet, hdr);
  StartAccessIfNeeded ();
}

void
DmgAbftAccess::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket != 0 || !m_queue->IsEmpty ())
    {

    }
}

void
DmgAbftAccess::StartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0 && !m_queue->IsEmpty ())
    {

    }
}

Ptr<MacLow>
DmgAbftAccess::Low (void)
{
  NS_LOG_FUNCTION (this);
  return m_low;
}

void
DmgAbftAccess::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
}

bool
DmgAbftAccess::NeedsAccess (void) const
{
  NS_LOG_FUNCTION (this);
  return !m_queue->IsEmpty () || m_currentPacket != 0;
}

void
DmgAbftAccess::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0)
    {
      if (m_queue->IsEmpty ())
        {
          NS_LOG_DEBUG ("A-BFT queue is empty");
          return;
        }
      m_currentPacket = m_queue->Dequeue (&m_currentHdr);
      NS_ASSERT (m_currentPacket != 0);
      NS_LOG_DEBUG ("dequeued size=" << m_currentPacket->GetSize () << ", to=" << m_currentHdr.GetAddr1 ());

      /* Send A-BFT directly without */
      MacLowTransmissionParameters params;
      params.EnableOverrideDurationId (hdr.GetDuration ());
      params.DisableRts ();
      params.DisableAck ();
      params.DisableNextData ();
      m_low->StartTransmission (packet,
                                &hdr,
                                params,
                                MakeCallback (&DmgStaWifiMac::FrameTxOk, this));
    }
}

void
DmgAbftAccess::NotifyChannelSwitching (void)
{
  NS_LOG_FUNCTION (this);
  m_queue->Flush ();
  m_currentPacket = 0;
}

void
DmgAbftAccess::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Transmission cancelled");
}

void
DmgAbftAccess::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("A transmission that did not require an ACK just finished");
  m_currentPacket = 0;
  if (!m_txOkNoAckCallback.IsNull ())
    {
      m_txOkNoAckCallback (m_currentHdr);
    }
  StartAccessIfNeeded ();
}

} //namespace ns3

