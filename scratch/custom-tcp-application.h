/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef CUSTOM_TCP_APPLICATION_H
#define CUSTOM_TCP_APPLICATION_H

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

namespace ns3 {

/********************************************************
 *            Custom TCP Send Application
 ********************************************************/

NS_LOG_COMPONENT_DEFINE ("TcpSendApplication");

/**
 * This code defines an application to run during the simulation that
 * setups connections and manages sending data.
 */
class TcpSendApplication : public Application
{
public:
  /**
   * Default constructors.
   */
  TcpSendApplication ();
  virtual ~TcpSendApplication ();

  /**
   * Setup the TCP send application.
   *
   * \param socket Socket to send data to.
   * \param address Address to send data to.
   * \param packetSize Size of the packets to send.
   * \param dataRate Data rate used to determine when to send the packets.
   * \param isBulk The Application behaves as BulkSendApplication or as OnOffApplication.
   */
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate, bool isBulk);
  /**
   * \return the total packets transmitted
   */
  uint64_t GetTotalTxPackets (void) const;
  /**
   * \return the total bytes transmitted
   */
  uint64_t GetTotalTxBytes (void) const;

  virtual void StartApplication (void);
  virtual void StopApplication (void);

private:

  /**
   * Schedule when the next packet will be sent.
   */
  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint64_t        m_packetsSent;
  uint64_t        m_totBytes;     //!< Total bytes sent so far.
  bool            m_connected;    //!< True if connected.
  bool            m_bulk;         //!< True if Bulk, otherwise OnOff.

private:
  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
  void DataSend (Ptr<Socket>, uint32_t); // for socket's SetSendCallback

};

TcpSendApplication::TcpSendApplication ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0),
    m_totBytes (0),
    m_connected (false),
    m_bulk (true)
{
}

TcpSendApplication::~TcpSendApplication ()
{
  m_socket = 0;
}

void
TcpSendApplication::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate, bool isBulk)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_dataRate = dataRate;
  m_bulk = isBulk;
}

void
TcpSendApplication::StartApplication (void)
{
  m_running = true;
  // Make sure the socket is created
  if (m_socket)
    {
      // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
      if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
          m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
        {
          NS_FATAL_ERROR ("Using BulkSend with an incompatible socket type. "
                          "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                          "In other words, use TCP instead of UDP.");
        }

      if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind6 ();
        }
      else if (InetSocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind ();
        }

      m_socket->Connect (m_peer);
      m_socket->ShutdownRecv ();
      m_socket->SetConnectCallback (
        MakeCallback (&TcpSendApplication::ConnectionSucceeded, this),
        MakeCallback (&TcpSendApplication::ConnectionFailed, this));
      m_socket->SetSendCallback (
        MakeCallback (&TcpSendApplication::DataSend, this));
    }
  if (m_connected)
    {
      SendPacket ();
    }
}

void
TcpSendApplication::StopApplication (void)
{
  m_running = false;
  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
      m_connected = false;
    }
  else
    {
      NS_LOG_WARN ("TcpSendApplication found null socket to close in StopApplication");
    }
}

void
TcpSendApplication::SendPacket (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  TimestampTag timestamp;
  timestamp.SetTimestamp (Simulator::Now ());
  packet->AddByteTag (timestamp);

  if (m_bulk)
    {
      while (true)
        {
          NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());
          int actual = m_socket->Send (packet);
          if (actual > 0)
            {
              m_totBytes += actual;
              m_packetsSent++;
            }
          // We exit this loop when actual < toSend as the send side
          // buffer is full. The "DataSent" callback will pop when
          // some buffer space has freed ip.
          if ((unsigned)actual != m_packetSize)
            {
              break;
            }
        }
    }
  else
    {
      m_socket->Send (packet);
      ScheduleTx ();
    }
}

void
TcpSendApplication::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &TcpSendApplication::SendPacket, this);
    }
}

void
TcpSendApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("TcpSendApplication Connection succeeded");
  m_connected = true;
  SendPacket ();
}

void
TcpSendApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("TcpSendApplication, Connection Failed");
}

void
TcpSendApplication::DataSend (Ptr<Socket>, uint32_t)
{
  NS_LOG_FUNCTION (this);
  if (m_connected  && m_bulk)
    { // Only send new data if the connection has completed
      SendPacket ();
    }
}

uint64_t
TcpSendApplication::GetTotalTxPackets (void) const
{
  return m_packetsSent;
}

uint64_t
TcpSendApplication::GetTotalTxBytes (void) const
{
  return m_totBytes;
}

} // namespace ns3

#endif // CUSTOM_TCP_APPLICATION_H
