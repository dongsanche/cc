/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "grp.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "ns3/ipv4-route.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/network-module.h"
#include "ns3/tag.h"
#include <cmath>

#define GRP_MAX_MSGS 64
#define GRP_PORT_NUMBER 12345
#define GRP_MAX_SEQ_NUM 65535

#define GRP_REFRESH_INTERVAL   m_helloInterval
#define GRP_NEIGHB_HOLD_TIME   Time (1 * GRP_REFRESH_INTERVAL)
#define GRP_BLOCK_CHECK_TIME   Time (2 * GRP_REFRESH_INTERVAL)
#define GRP_HEADER_LOC_INTERVAL Time (1 * GRP_REFRESH_INTERVAL)

#define GRP_MAXJITTER          (m_helloInterval.GetSeconds () / 10)
#define JITTER (Seconds (m_uniformRandomVariable->GetValue (0, GRP_MAXJITTER)))

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GrpRoutingProtocol");

namespace grp
{
NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::grp::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("grp")
    .AddConstructor<RoutingProtocol> ()
    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
                   TimeValue (Seconds (1)),
                   MakeTimeAccessor (&RoutingProtocol::m_helloInterval),
                   MakeTimeChecker ())
    .AddTraceSource ("DropPacket", "Drop data packet.",
					MakeTraceSourceAccessor (&RoutingProtocol::m_DropPacketTrace),
					"ns3::grp::RoutingProtocol::m_DropPacketTraceCallback")
    .AddTraceSource ("StorePacket", "Store and carry data packets.",
                MakeTraceSourceAccessor (&RoutingProtocol::m_StorePacketTrace),
                "ns3::grp::RoutingProtocol::m_StorePacketTraceCallback")
  ;
  return tid;
}

RoutingProtocol::RoutingProtocol ()
  : m_ipv4 (0),
  m_helloTimer (Timer::CANCEL_ON_DESTROY),
  m_positionCheckTimer (Timer::CANCEL_ON_DESTROY),
  m_queuedMessagesTimer (Timer::CANCEL_ON_DESTROY),
  m_speedTimer(Timer::CANCEL_ON_DESTROY)
{
  m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
}

RoutingProtocol::~RoutingProtocol ()
{
}

void
RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4)
{
  NS_ASSERT (ipv4 != 0);
  NS_ASSERT (m_ipv4 == 0);
  NS_LOG_DEBUG ("Created grp::RoutingProtocol");
  m_helloTimer.SetFunction (&RoutingProtocol::HelloTimerExpire, this);
  m_positionCheckTimer.SetFunction(&RoutingProtocol::CheckPositionExpire, this);
  m_speedTimer.SetFunction(&RoutingProtocol::SpeedCheckExpire, this);
  m_queuedMessagesTimer.SetFunction (&RoutingProtocol::SendQueuedMessages, this);

  m_packetSequenceNumber = GRP_MAX_SEQ_NUM;
  m_messageSequenceNumber = GRP_MAX_SEQ_NUM;

  m_ipv4 = ipv4;

}

void RoutingProtocol::DoDispose ()
{
  m_ipv4 = 0;

  if (m_recvSocket)
    {
      m_recvSocket->Close ();
      m_recvSocket = 0;
    }

  for (std::map< Ptr<Socket>, Ipv4InterfaceAddress >::iterator iter = m_sendSockets.begin ();
       iter != m_sendSockets.end (); iter++)
    {
      iter->first->Close ();
    }
  m_sendSockets.clear ();


  for (std::map< Ptr<Socket>, Ipv4InterfaceAddress >::iterator biter = m_sendBlockSockets.begin ();
       biter != m_sendBlockSockets.end (); biter++)
    {
      biter->first->Close ();
    }
  m_sendBlockSockets.clear ();

  m_neiTable.clear ();

    m_wTimeCache.clear();
    m_tracelist.clear();
    m_squeue.clear();
    m_pwaitqueue.clear();
    m_delayqueue.clear();
    m_map.clear();

    delete[] Graph;



  Ipv4RoutingProtocol::DoDispose ();
}

void
RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
}

void RoutingProtocol::InitialMID()
{
	m_id = AddrToID(m_mainAddress);
}

void RoutingProtocol::InitialPosition()
{
	Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
	double xn = MM->GetPosition ().x;
	double yn = MM->GetPosition ().y;

	m_last_x = xn;
	m_last_y = yn;

	int i = 0;
	int idx = -1;
	double min = 100000;
	for(std::vector<VTrace>::iterator itr = m_tracelist.begin(); itr!=m_tracelist.end(); itr++)
	{
		double dis = sqrt(pow(xn-itr->x, 2)+pow(yn-itr->y, 2));
		if(dis < min)
		{
			min = dis;
			idx = i;
		}
		i++;
	}

	std::vector<int> jlist = m_tracelist[idx].jlist;
	for(std::vector<int>::iterator itr = jlist.begin(); itr!=jlist.end(); itr++)
	{
		m_trailTrace.push(*itr);
	}
    
    m_currentJID = m_trailTrace.front();
	m_trailTrace.pop();
	m_nextJID = m_trailTrace.front();
	m_trailTrace.pop();

	m_direction = GetDirection(m_currentJID, m_nextJID);

}

int
RoutingProtocol::GetDirection(int currentJID, int nextJID)
{
	double cx = m_map[currentJID].x;
	double cy = m_map[currentJID].y;
	double nx = m_map[nextJID].x;
	double ny = m_map[nextJID].y;

	if(ny == cy)
	{
		if(nx > cx)
			return 0;
		else
			return 2;
	}
	else
	{
		if(ny > cy)
			return 1;
		else
			return 3;
	}

	return -1;
}

void RoutingProtocol::ReadConfiguration()
{
    std::ifstream file(confile);
	std::string line;
    while(!file.eof())
	{
		std::getline(file, line);

		std::istringstream iss(line);
		std::string temp;
 
		while (std::getline(iss, temp, '='))
		{
            std::string value = std::move(temp);
            if(value == "vnum")
            {
                std::getline(iss, temp, ',');
				value = std::move(temp);
                vnum = atoi(value.c_str());
            }
            else if(value == "range")
            {
                std::getline(iss, temp, ',');
				value = std::move(temp);
                InsightTransRange = atof(value.c_str());
            }
            else if(value == "CarryTimeThreshold")
            {
                std::getline(iss, temp, ',');
				value = std::move(temp);
                CarryTimeThreshold = atof(value.c_str());
            }
            
        }

    }
}

void RoutingProtocol::DoInitialize ()
{
    ReadConfiguration();

	RSSIDistanceThreshold = InsightTransRange * 0.9;
    for(int i = 0; i < m_JuncNum; i++)
    {
        m_jqueuetag[i] = false;
    }

    Graph = new float *[m_JuncNum];
    for(int i = 0;i < m_JuncNum; ++i)
    {
        Graph[i] = new float[m_JuncNum];
        for(int j = 0; j<m_JuncNum; j++)
        {
            Graph[i][j] = INF;
        }
    }

	DigitalMap map;
	std::string mapfile = "TestScenaries/" + std::to_string(vnum) + "/6x6_map.csv";
    std::string tracefile = "TestScenaries/" + std::to_string(vnum) + "/6x6_vtrace.csv";
	map.setMapFilePath(mapfile);
	map.readMapFromCsv(m_map);
	map.readTraceCsv(tracefile, m_tracelist);


  if (m_mainAddress == Ipv4Address ())
    {
      Ipv4Address loopback ("127.0.0.1");
      for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
        {
          // Use primary address, if multiple
          Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();
          if (addr != loopback)
            {
              m_mainAddress = addr;
              break;
            }
        }

      NS_ASSERT (m_mainAddress != Ipv4Address ());
    }

  NS_LOG_DEBUG ("Starting Grp on node " << m_mainAddress);

  Ipv4Address loopback ("127.0.0.1");

  bool canRunGrp = false;
  for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
    {
      Ipv4Address addr = m_ipv4->GetAddress (i, 0).GetLocal ();

      if(addr == Ipv4Address("127.0.0.1"))
      {
    	  continue;
      }
      // Create a socket to listen on all the interfaces
      if (m_recvSocket == 0)
        {
          m_recvSocket = Socket::CreateSocket (GetObject<Node> (),
                                               UdpSocketFactory::GetTypeId ());
          m_recvSocket->SetAllowBroadcast (true);
          InetSocketAddress inetAddr (Ipv4Address::GetAny (), GRP_PORT_NUMBER);
          m_recvSocket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvGrp,  this));
          if (m_recvSocket->Bind (inetAddr))
            {
              NS_FATAL_ERROR ("Failed to bind() grp socket");
            }
          m_recvSocket->SetRecvPktInfo (true);
          m_recvSocket->ShutdownSend ();
        }

      // Create a socket to send packets from this specific interfaces
      Ptr<Socket> socket = Socket::CreateSocket (GetObject<Node> (),
                                                 UdpSocketFactory::GetTypeId ());
      socket->SetAllowBroadcast (true);
      InetSocketAddress inetAddr (m_ipv4->GetAddress (i, 0).GetLocal (), GRP_PORT_NUMBER);
      socket->SetRecvCallback (MakeCallback (&RoutingProtocol::RecvGrp,  this));
      socket->BindToNetDevice (m_ipv4->GetNetDevice (i));
      if (socket->Bind (inetAddr))
        {
          NS_FATAL_ERROR ("Failed to bind() GRP socket");
        }
      socket->SetRecvPktInfo (true);
      m_sendSockets[socket] = m_ipv4->GetAddress (i, 0);

      canRunGrp = true;
    }

  if (canRunGrp)
    {
        startTime += 1;
        Simulator::Schedule(Seconds(0.01), &RoutingProtocol::InitialMID, this);
        Simulator::Schedule(Seconds(startTime), &RoutingProtocol::InitialPosition, this);
        double helloStartTime = startTime+1+AddrToID(m_mainAddress) * 0.001;
        Simulator::Schedule(Seconds(helloStartTime), &RoutingProtocol::HelloTimerExpire, this);
        Simulator::Schedule(Seconds(startTime+2), &RoutingProtocol::CheckPositionExpire, this);
        Simulator::Schedule(Seconds(startTime+3), &RoutingProtocol::SpeedCheckExpire, this);

        NS_LOG_DEBUG ("Grp on node " << m_mainAddress << " started");
    }
}

void RoutingProtocol::SetMainInterface (uint32_t interface)
{
  m_mainAddress = m_ipv4->GetAddress (interface, 0).GetLocal ();
}

void
RoutingProtocol::RecvGrp (Ptr<Socket> socket)
{
  Ptr<Packet> receivedPacket;
  Address sourceAddress;
  receivedPacket = socket->RecvFrom (sourceAddress);

  Ipv4PacketInfoTag interfaceInfo;
  if (!receivedPacket->RemovePacketTag (interfaceInfo))
    {
      NS_ABORT_MSG ("No incoming interface on GRP message, aborting.");
    }
  uint32_t incomingIf = interfaceInfo.GetRecvIf ();
  Ptr<Node> node = this->GetObject<Node> ();
  Ptr<NetDevice> dev = node->GetDevice (incomingIf);
  uint32_t recvInterfaceIndex = m_ipv4->GetInterfaceForDevice (dev);

  InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom (sourceAddress);
  Ipv4Address senderIfaceAddr = inetSourceAddr.GetIpv4 ();

  int32_t interfaceForAddress = m_ipv4->GetInterfaceForAddress (senderIfaceAddr);
  if (interfaceForAddress != -1)
    {
      NS_LOG_LOGIC ("Ignoring a packet sent by myself.");
      return;
    }

  Ipv4Address receiverIfaceAddr = m_ipv4->GetAddress (recvInterfaceIndex, 0).GetLocal ();
  NS_ASSERT (receiverIfaceAddr != Ipv4Address ());
  NS_LOG_DEBUG ("GRP node " << m_mainAddress << " received a GRP packet from "
                             << senderIfaceAddr << " to " << receiverIfaceAddr);

  // All routing messages are sent from and to port RT_PORT,
  // so we check it.
  NS_ASSERT (inetSourceAddr.GetPort () == GRP_PORT_NUMBER);

  Ptr<Packet> packet = receivedPacket;

  grp::CtrPacketHeader GrpPacketHeader;
  packet->RemoveHeader (GrpPacketHeader);
  NS_ASSERT (GrpPacketHeader.GetPacketLength () >= GrpPacketHeader.GetSerializedSize ());
  uint32_t sizeLeft = GrpPacketHeader.GetPacketLength () - GrpPacketHeader.GetSerializedSize ();

  MessageList messages;

  while (sizeLeft)
    {
      MessageHeader messageHeader;
      if (packet->RemoveHeader (messageHeader) == 0)
        {
          NS_ASSERT (false);
        }

      sizeLeft -= messageHeader.GetSerializedSize ();

      NS_LOG_DEBUG ("Grp Msg received with type "
                    << std::dec << int (messageHeader.GetMessageType ())
                    << " TTL=" << int (messageHeader.GetTimeToLive ())
                    << " origAddr=" << messageHeader.GetOriginatorAddress ());
      messages.push_back (messageHeader);
    }

  for (MessageList::const_iterator messageIter = messages.begin ();
       messageIter != messages.end (); messageIter++)
  {
      const MessageHeader &messageHeader = *messageIter;
      if (messageHeader.GetTimeToLive () == 0
          || messageHeader.GetOriginatorAddress () == m_mainAddress)
        {
          packet->RemoveAtStart (messageHeader.GetSerializedSize ()
                                 - messageHeader.GetSerializedSize ());
          continue;
        }

      switch (messageHeader.GetMessageType ())
	  {
		case grp::MessageHeader::HELLO_MESSAGE:
			NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
							<< "s GRP node " << m_mainAddress
							<< " received HELLO message of size " << messageHeader.GetSerializedSize ());
			ProcessHello (messageHeader, receiverIfaceAddr, senderIfaceAddr);
			break;
		default:
		NS_LOG_DEBUG ("GRP message type " <<
						int (messageHeader.GetMessageType ()) <<
						" not implemented");
	 }
  }
}

void
RoutingProtocol::SendFromDelayQueue()
{
    if(m_delayqueue.empty() == false)
 	{
 		DelayPacketQueueEntry sentry = m_delayqueue.back();
 		m_delayqueue.pop_back();

  		Ptr<Ipv4Route> rtentry;
 		rtentry = Create<Ipv4Route> ();
 		rtentry->SetDestination (sentry.m_header.GetDestination ());
 		rtentry->SetSource (sentry.m_header.GetSource());
 		rtentry->SetGateway (sentry.m_nexthop);
 		rtentry->SetOutputDevice (m_ipv4->GetNetDevice (0));
        sentry.m_header.SetTtl(sentry.m_header.GetTtl() + 1);
 		sentry.m_ucb(rtentry, sentry.m_packet, sentry.m_header);
 	}
}

void
RoutingProtocol::CheckPacketQueue()
{
    m_pqueue.assign(m_pwaitqueue.begin(), m_pwaitqueue.end());
 	m_pwaitqueue.clear();

  	while(m_pqueue.empty() == false)
 	{
 		PacketQueueEntry qentry = m_pqueue.back();
 		m_pqueue.pop_back();

 		Ipv4Address dest = qentry.m_header.GetDestination();
 		Ipv4Address origin = qentry.m_header.GetSource();

  		QPacketInfo pInfo(origin, dest);
 		QMap::const_iterator pItr = m_wTimeCache.find(pInfo);
 		if(pItr != m_wTimeCache.end() && Simulator::Now().GetSeconds() - pItr->second.GetSeconds() >= CarryTimeThreshold )
 		{
 			NS_LOG_UNCOND("Store time more than: " << CarryTimeThreshold << "s.");
 			m_DropPacketTrace(qentry.m_header);
 			m_wTimeCache.erase(pInfo);
 			continue;
 		}

  		grp::DataPacketHeader DataPacketHeader;
 		qentry.m_packet->RemoveHeader (DataPacketHeader);
 		int nextjid = (int)DataPacketHeader.GetNextJID();
 		
  		Ipv4Address loopback ("127.0.0.1");
        Ipv4Address nextHop("127.0.0.1");

        if(m_JunAreaTag == true)
        {
            nextjid = GetPacketNextJID(true);
        }
        else
        {
            if(nextjid != m_currentJID && nextjid != m_nextJID)
            {
                nextjid = GetNearestJID();
            }
        }
        nextHop = IntraPathRouting(dest, nextjid);

        DataPacketHeader.SetNextJID(nextjid);
        qentry.m_packet->AddHeader (DataPacketHeader);

  		if(nextHop == loopback)
 			m_pwaitqueue.push_back(qentry);
 		else
 		{
 			m_wTimeCache.erase(pInfo);
 			m_squeue.push_back(SendingQueue(qentry.m_packet, qentry.m_header, qentry.m_ucb, nextHop));
 			// NS_LOG_UNCOND("" << Simulator::Now().GetSeconds() << " " << m_id << " forwards a STORE data packet to " << AddrToID(nextHop));

  		}
 	}

  	if(m_squeue.empty() == false)
 	{
 		SendFromSQueue();
 	}

}

void
RoutingProtocol::SendFromSQueue()
{
 	if(m_squeue.empty() == false)
 	{
 		SendingQueue sentry = m_squeue.back();
 		m_squeue.pop_back();

  		Ptr<Ipv4Route> rtentry;
 		rtentry = Create<Ipv4Route> ();
 		rtentry->SetDestination (sentry.m_header.GetDestination ());
 		rtentry->SetSource (sentry.m_header.GetSource());
 		rtentry->SetGateway (sentry.nexthop);
 		rtentry->SetOutputDevice (m_ipv4->GetNetDevice (0));
 		sentry.m_ucb(rtentry, sentry.m_packet, sentry.m_header);

  		Simulator::Schedule(MilliSeconds(10), &RoutingProtocol::SendFromSQueue, this);
 	}
}

bool
RoutingProtocol::isAdjacentVex(int sjid, int ejid)
{
    for(std::map<int, std::vector<float>>::iterator itr = m_map[sjid].outedge.begin(); 
        itr != m_map[sjid].outedge.end(); itr++)
    {
        if(itr->first == ejid)
            return true;    
    }
    return false;
}

void
RoutingProtocol::ProcessHello (const grp::MessageHeader &msg,
							   const Ipv4Address receiverIfaceAddr,
                               const Ipv4Address senderIface)
{
	const grp::MessageHeader::Hello &hello = msg.GetHello ();

    //Restrict the communication between the vehicles with different direction.
    int cjid = GetNearestJID();
    if((int)hello.GetDirection() != m_direction && (int)hello.GetDirection() != (m_direction + 2)%4)
    {
        double jx = m_map[cjid].x;
        double jy = m_map[cjid].y;
        double nx = hello.GetLocationX();
        double ny = hello.GetLocationY();
        if(m_JunAreaTag == false && sqrt(pow(nx-jx, 2) + pow(ny-jy, 2)) > JunAreaRadius)
        {
            return;
        }    
    }

	Ipv4Address originatorAddress = msg.GetOriginatorAddress();
	std::map<Ipv4Address, NeighborTableEntry>::const_iterator itr = m_neiTable.find (originatorAddress);
	if(itr != m_neiTable.end() && itr->second.N_sequenceNum >= msg.GetMessageSequenceNumber())
		return;
	if(itr != m_neiTable.end())
	{
		m_neiTable.erase(originatorAddress);
	}

	NeighborTableEntry &neiTableTuple = m_neiTable[originatorAddress];
	neiTableTuple.N_neighbor_address = msg.GetOriginatorAddress();
	neiTableTuple.N_speed = hello.GetSpeed();
	neiTableTuple.N_direction = hello.GetDirection();
	neiTableTuple.N_location_x = hello.GetLocationX();
	neiTableTuple.N_location_y = hello.GetLocationY();
	neiTableTuple.receiverIfaceAddr = receiverIfaceAddr;
	neiTableTuple.N_sequenceNum = msg.GetMessageSequenceNumber();
	neiTableTuple.N_time = Simulator::Now () + msg.GetVTime();

	neiTableTuple.N_turn = hello.GetTurn();

    neiTableTuple.N_status = NeighborTableEntry::STATUS_NOT_SYM;
	for (std::vector<Ipv4Address>::const_iterator i = hello.neighborInterfaceAddresses.begin ();
			i != hello.neighborInterfaceAddresses.end (); i++)
	{
		if(m_mainAddress == *i)
		{
			neiTableTuple.N_status = NeighborTableEntry::STATUS_SYM;
			break;
		}
	}

	Simulator::Schedule(GRP_NEIGHB_HOLD_TIME, &RoutingProtocol::NeiTableCheckExpire, this, originatorAddress);

    if(m_pwaitqueue.empty() == false)
 	{
 		CheckPacketQueue();
 	}

}

int 
RoutingProtocol::AddrToID(Ipv4Address addr)
{
	int tnum = addr.Get();
	return tnum / 256 % 256 * 256 + tnum % 256 - 1;
}

void
RoutingProtocol::QueueMessage (const grp::MessageHeader &message, Time delay)
{
  m_queuedMessages.push_back (message);
  if (not m_queuedMessagesTimer.IsRunning ())
    {
      m_queuedMessagesTimer.SetDelay (delay);
      m_queuedMessagesTimer.Schedule ();
    }
}

void
RoutingProtocol::SendQueuedMessages ()
{
  Ptr<Packet> packet = Create<Packet> ();
  int numMessages = 0;

  MessageList msglist;

  for (std::vector<grp::MessageHeader>::const_iterator message = m_queuedMessages.begin ();
       message != m_queuedMessages.end ();
       message++)
    {
      Ptr<Packet> p = Create<Packet> ();
      p->AddHeader (*message);
      packet->AddAtEnd (p);
      msglist.push_back (*message);
      if (++numMessages == GRP_MAX_MSGS)
        {
          SendPacket (packet);
          msglist.clear ();
          numMessages = 0;
          packet = Create<Packet> ();
        }
    }

  if (packet->GetSize ())
    {
      SendPacket (packet);
    }

  m_queuedMessages.clear ();
}

void
RoutingProtocol::SendPacket (Ptr<Packet> packet)
{
  // Add a header
  grp::CtrPacketHeader header;
  header.SetPacketLength (header.GetSerializedSize () + packet->GetSize ());
  header.SetPacketSequenceNumber (GetPacketSequenceNumber ());
  packet->AddHeader (header);

  // Send it
  for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator i =
         m_sendSockets.begin (); i != m_sendSockets.end (); i++)
    {
      Ptr<Packet> pkt = packet->Copy ();
      //TODO need to test the mask is 8bits or 16bits
      Ipv4Address bcast = i->second.GetLocal ().GetSubnetDirectedBroadcast (i->second.GetMask ());
      i->first->SendTo (pkt, 0, InetSocketAddress (bcast, GRP_PORT_NUMBER));
    }
}

int 
RoutingProtocol::GetNearestJID()
{
    Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
	double cx = MM->GetPosition ().x;
	double cy = MM->GetPosition ().y;
    if(pow(cx-m_map[m_currentJID].x, 2) + pow(cy-m_map[m_currentJID].y, 2)
        < pow(cx-m_map[m_nextJID].x, 2) + pow(cy-m_map[m_nextJID].y, 2))
	{
        return m_currentJID;
    }
    else
    {
        return m_nextJID;
    }
    
}

void
RoutingProtocol::SendHello ()
{
	NS_LOG_FUNCTION (this);

	grp::MessageHeader msg;
	Time now = Simulator::Now ();

	msg.SetVTime (GRP_NEIGHB_HOLD_TIME);
	msg.SetOriginatorAddress (m_mainAddress);
	msg.SetTimeToLive (1);
	msg.SetHopCount (0);
	msg.SetMessageSequenceNumber (GetMessageSequenceNumber ());
	grp::MessageHeader::Hello &hello = msg.GetHello ();

	Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
	double positionX = MM->GetPosition ().x;
	double positionY = MM->GetPosition ().y;
	hello.SetLocation(positionX, positionY);

	hello.SetSpeedAndDirection(m_speed, m_direction);

    for (std::map<Ipv4Address, NeighborTableEntry>::const_iterator iter = m_neiTable.begin ();
			iter != m_neiTable.end (); iter++)
	{
		hello.neighborInterfaceAddresses.push_back(iter->first);
	}

	QueueMessage (msg, JITTER);
}

uint16_t RoutingProtocol::GetPacketSequenceNumber ()
{
  m_packetSequenceNumber = (m_packetSequenceNumber + 1) % (GRP_MAX_SEQ_NUM + 1);
  return m_packetSequenceNumber;
}

uint16_t RoutingProtocol::GetMessageSequenceNumber ()
{
  m_messageSequenceNumber = (m_messageSequenceNumber + 1) % (GRP_MAX_SEQ_NUM + 1);
  return m_messageSequenceNumber;
}

void
RoutingProtocol::HelloTimerExpire ()
{
  SendHello ();
  m_helloTimer.Schedule (m_helloInterval);
}

void
RoutingProtocol::CheckPositionExpire()
{
	Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
	double cvx = MM->GetPosition ().x;
	double cvy = MM->GetPosition ().y;
	double njx = m_map[m_nextJID].x;
	double njy = m_map[m_nextJID].y;
	double cjx = m_map[m_currentJID].x;
	double cjy = m_map[m_currentJID].y;
	
	double disToNextJun = sqrt(pow(cvx-njx, 2) + pow(cvy-njy, 2));
	double disToCurrJun = sqrt(pow(cvx-cjx, 2) + pow(cvy-cjy, 2));
	if(disToNextJun <= PositionCheckThreshold)
	{
		m_turn = -1;
		m_currentJID = m_nextJID; 

		m_nextJID = m_trailTrace.front();
		m_trailTrace.pop();

		m_direction = GetDirection(m_currentJID, m_nextJID);
	}
	else if(disToNextJun <= turnLightRange)
	{
		if(m_turn < 0)
			m_turn = m_trailTrace.front();
	}

    if(m_JunAreaTag == false)
    {
        if(disToNextJun < JunAreaRadius)
        {
            m_JunAreaTag = true;
        }
    }
    else
    {
        if(disToNextJun > JunAreaRadius && disToCurrJun > JunAreaRadius)
        {
            m_JunAreaTag = false;
        }
    }
    
    m_positionCheckTimer.Schedule(Seconds(0.1));
}

void
RoutingProtocol::SpeedCheckExpire()
{
	Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
	double cx = MM->GetPosition ().x;
	double cy = MM->GetPosition ().y;
    m_speed = sqrt(pow(cx-m_last_x, 2) + pow(cy-m_last_y, 2));

	m_last_x = cx;
	m_last_y = cy;

	m_speedTimer.Schedule(GRP_NEIGHB_HOLD_TIME);
}

void
RoutingProtocol::NeiTableCheckExpire(Ipv4Address addr)
{
	NeighborTableEntry nentry = m_neiTable[addr];
	if(nentry.N_time <= Simulator::Now())
	{
		m_neiTable.erase(addr);
	}
}

int64_t
RoutingProtocol::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uniformRandomVariable->SetStream (stream);
  return 1;
}

void
RoutingProtocol::SetDownTarget (IpL4Protocol::DownTargetCallback callback)
{
  m_downTarget = callback;
}

Vector
RoutingProtocol::GetPosition(Ipv4Address adr)
{
	uint32_t n = NodeList().GetNNodes ();
	uint32_t i;
	Ptr<Node> node;

	for(i = 0; i < n; i++)
	{
		node = NodeList().GetNode (i);
		Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
		if(ipv4->GetAddress (1, 0).GetLocal () == adr)
		{
			return (*node->GetObject<MobilityModel>()).GetPosition ();
		}
	}
	Vector v;
	return v;
}

void
RoutingProtocol::AddHeader (Ptr<Packet> p, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route)
{
	Ipv4Mask brocastMask("0.0.255.255");
	if (brocastMask.IsMatch(destination, Ipv4Address("0.0.255.255")) == false)
	{
		Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
        double cx = MM->GetPosition ().x;
        double cy = MM->GetPosition ().y;
        double cjx = m_map[m_currentJID].x;
        double cjy = m_map[m_currentJID].y;
        double njx = m_map[m_nextJID].x;
        double njy = m_map[m_nextJID].y;

        int nextjid;
        if(pow(cx-cjx, 2) + pow(cy-cjy, 2) < pow(cx-njx, 2) + pow(cy-njy, 2))
        {
            nextjid = m_currentJID;
        } 
        else
        {
            nextjid = m_nextJID;
        }

        grp::DataPacketHeader Dheader;
        Time lut = Simulator::Now();
        Dheader.SetNextJID(nextjid);
        p->AddHeader (Dheader);

	}

	m_downTarget (p, source, destination, protocol, route);

}

bool
RoutingProtocol::isBetweenSegment(double nx, double ny, int cjid, int djid)
{
    bool res = false;
    double djx = m_map[djid].x;
    double djy = m_map[djid].y;
    double cjx = m_map[cjid].x;
    double cjy = m_map[cjid].y;

    double minx = (cjx < djx ? cjx : djx);
    double maxx = (cjx > djx ? cjx : djx);
    double miny = (cjy < djy ? cjy : djy);
    double maxy = (cjy > djy ? cjy : djy);
    int dir = GetDirection(cjid, djid);
    if(dir % 2 == 0)
    {
        miny -= RoadWidth;
        maxy += RoadWidth;
    }
    else
    {
        minx -= RoadWidth;
        maxx += RoadWidth;
    }
    
    if(nx >= minx && nx <= maxx && ny >= miny && ny <= maxy)
    {
        res = true;
    }

    return res;
}

Ipv4Address
RoutingProtocol::IntraPathRouting(Ipv4Address dest,  int dstjid)
{
	Ipv4Address nextHop = Ipv4Address("127.0.0.1");

    if(dstjid < 0)
    {
        return  nextHop;
    }

	Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
	double cx = MM->GetPosition().x;
	double cy = MM->GetPosition().y;
	
	double dx = GetPosition(dest).x;
	double dy = GetPosition(dest).y;
	double curDisToDst = sqrt(pow(cx-dx, 2) + pow(cy-dy, 2));
	if(curDisToDst < RSSIDistanceThreshold)
		return dest;

    double jx = m_map[dstjid].x;
	double jy = m_map[dstjid].y;
	double mindis = sqrt(pow(cx-jx, 2) + pow(cy-jy, 2));

	for (std::map<Ipv4Address, NeighborTableEntry>::const_iterator i = m_neiTable.begin (); i != m_neiTable.end (); i++)
	{
		if(i->second.N_status == NeighborTableEntry::STATUS_NOT_SYM)
		{
			continue;
		}

		double nx = i->second.N_location_x;
		double ny = i->second.N_location_y;
		double neiDisToJID = sqrt(pow(nx-jx, 2) + pow(ny-jy, 2));
		double curDisToNei = sqrt(pow(cx-nx, 2) + pow(cy-ny, 2));
		if(neiDisToJID < mindis && curDisToNei < RSSIDistanceThreshold)
		{
            int cjid = GetNearestJID();
            //此处的条件判断用以防止当前车辆将数据包传输给其他路段的节点，
            //其他路段的节点同样有可能满足上一个条件判断
			if(m_JunAreaTag == false || isBetweenSegment(nx, ny, cjid, dstjid) == true)
            {
                mindis = neiDisToJID;
                nextHop = i->first;
            }
		}
	}
	
	return nextHop;
}


Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
	NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId () << " " << header.GetDestination () << " " << oif);
	Ptr<Ipv4Route> rtentry = NULL;

	Ipv4Address dest = header.GetDestination ();
	Ipv4Address nextHop = Ipv4Address("127.0.0.1");

    Ptr<MobilityModel> MM = m_ipv4->GetObject<MobilityModel> ();
    double cx = MM->GetPosition ().x;
    double cy = MM->GetPosition ().y;
    double cjx = m_map[m_currentJID].x;
    double cjy = m_map[m_currentJID].y;
    double njx = m_map[m_nextJID].x;
    double njy = m_map[m_nextJID].y;

    int dstjid;
    if(m_JunAreaTag == false)
    {
        dstjid = pow(cx-cjx, 2) + pow(cy-cjy, 2) < pow(cx-njx, 2) + pow(cy-njy, 2)? m_currentJID:m_nextJID;
    }
    else
    {
        dstjid = GetPacketNextJID(true);
    }

    Ipv4Address loopback ("127.0.0.1");
    nextHop = IntraPathRouting(dest, dstjid);
    if(nextHop == loopback || nextHop == dest || m_JunAreaTag == true)
    {
        rtentry = Create<Ipv4Route> ();
        rtentry->SetDestination (header.GetDestination ());
        rtentry->SetSource (m_ipv4->GetAddress (1, 0).GetLocal ());
        rtentry->SetGateway (loopback);
        rtentry->SetOutputDevice (m_ipv4->GetNetDevice (0));
        sockerr = Socket::ERROR_NOTERROR;
    }
    else
    {
        rtentry = Create<Ipv4Route> ();
        rtentry->SetDestination (header.GetDestination ());
        Ipv4Address receiverIfaceAddr = m_neiTable.find(nextHop)->second.receiverIfaceAddr;
            
        rtentry->SetSource (receiverIfaceAddr);
        rtentry->SetGateway (nextHop);
        for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
        {
            for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); j++)
            {
                if (m_ipv4->GetAddress (i,j).GetLocal () == receiverIfaceAddr)
                {
                    rtentry->SetOutputDevice (m_ipv4->GetNetDevice (i));
                    break;
                }
            }
        }

        sockerr = Socket::ERROR_NOTERROR;

    }
	return rtentry;
}

int
RoutingProtocol::DijkstraAlgorithm(int srcjid, int dstjid)
{
    bool visited[m_JuncNum];
    double distance[m_JuncNum];
    int parent[m_JuncNum];

    for(int i = 0; i<m_JuncNum; i++)
    {
        visited[i] = false;
        distance[i] = INF;
        parent[i] = -1;
    }

    visited[srcjid] = true;
    distance[srcjid] = 0;

    int curr = srcjid;
    int next = -1;
    for(int count = 1; curr >= 0 && count <= m_JuncNum; count++)
    {
        double min = INF;
        for(int n = 0; n < m_JuncNum; n++)
        {
            if(visited[n] == false)
            {
                if(distance[curr] + Graph[curr][n] < distance[n])
                {
                    distance[n] = distance[curr] + Graph[curr][n];
                    parent[n] = curr;
                }

                if(distance[n] < min)
                {
                    min = distance[n];
                    next = n;
                }
            }
        }
        curr = next;
        visited[curr] = true;
    }

    int jid = dstjid;
    while(jid > 0)
    {
        if(parent[jid] == srcjid)
            break;
        jid = parent[jid];
    }

    return jid; 
}

int
RoutingProtocol::GetPacketNextJID(bool tag)
{
    int cjid = GetNearestJID();

    if(cjid == m_rsujid)
        return cjid;

    int nextjid = -1;

    for(int i = 0; i < m_JuncNum; i++)
    {
        for(int j = i + 1; j < m_JuncNum; j++)
        {
            if(isAdjacentVex(i, j) == false)
            {
                Graph[i][j] = Graph[j][i] = INF;
            }
            else
            {
                Graph[i][j] = Graph[j][i] = 1;
            }
        }
    }

    nextjid = DijkstraAlgorithm(cjid, m_rsujid);

    return nextjid;
}

bool RoutingProtocol::RouteInput  (Ptr<const Packet> p,
                                   const Ipv4Header &header, Ptr<const NetDevice> idev,
                                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                   LocalDeliverCallback lcb, ErrorCallback ecb)
{
	NS_LOG_FUNCTION (this << " " << m_ipv4->GetObject<Node> ()->GetId () << " " << header.GetDestination ());

	Ipv4Address dest = header.GetDestination ();
    Ipv4Address origin = header.GetSource ();

	NS_ASSERT (m_ipv4->GetInterfaceForDevice (idev) >= 0);
	uint32_t iif = m_ipv4->GetInterfaceForDevice (idev);
	if (m_ipv4->IsDestinationAddress (dest, iif))
	{
		if (!lcb.IsNull ())
		{
			NS_LOG_LOGIC ("Local delivery to " << dest);
			lcb (p, header, iif);
			return true;
		}
		else
		{
			return false;
		}
	}

    //判断当前数据包的TTL是否还存活
	if(header.GetTtl() <= 1)
	{
		NS_LOG_UNCOND("TTL < 0");
		m_DropPacketTrace(header);
		return true;
	}

	Ptr<Ipv4Route> rtentry;
	Ipv4Address loopback ("127.0.0.1");
	Ptr<Packet> packet = p->Copy ();
	grp::DataPacketHeader DataPacketHeader;
	packet->RemoveHeader (DataPacketHeader);
	int nextjid = (int)DataPacketHeader.GetNextJID();
    int senderID = DataPacketHeader.GetSenderID();

    //如果接收到数据包的车辆刚好位于路口范围内，则先进行路段间路由为数据包选定下一路由路段
    if(m_JunAreaTag == true)
    {
        nextjid = GetPacketNextJID(true);
        NS_LOG_UNCOND("JID: " << (int)DataPacketHeader.GetNextJID() << "->" << nextjid);
        NS_LOG_UNCOND("");
    }

    //路段内路由，为数据包选定下一跳节点
	Ipv4Address nextHop = IntraPathRouting(dest, nextjid);

	NS_LOG_UNCOND("" << Simulator::Now().GetSeconds() << " " << m_id << "->" << AddrToID(nextHop));

	if (nextHop != loopback)
	{
        //若返回了下一跳的地址，则将该数据包转发给该下一跳节点
        if(senderID != AddrToID(nextHop))
        {
            rtentry = Create<Ipv4Route> ();
            rtentry->SetDestination (header.GetDestination ());
            Ipv4Address receiverIfaceAddr = m_neiTable.find(nextHop)->second.receiverIfaceAddr;

            if(nextHop == dest)
                receiverIfaceAddr = m_mainAddress;

            rtentry->SetSource (receiverIfaceAddr);
            rtentry->SetGateway (nextHop);

            for (uint32_t i = 0; i < m_ipv4->GetNInterfaces (); i++)
            {
                for (uint32_t j = 0; j < m_ipv4->GetNAddresses (i); j++)
                {
                    if (m_ipv4->GetAddress (i,j).GetLocal () == receiverIfaceAddr)
                    {
                        rtentry->SetOutputDevice (m_ipv4->GetNetDevice (i));
                        break;
                    }
                }
            }

            if(nextHop != header.GetDestination())
            {
                grp::DataPacketHeader DHeader;
                DHeader.SetNextJID(nextjid);
                DHeader.SetSenderID(m_id);
                packet->AddHeader(DHeader);
            }

            ucb (rtentry, packet, header);
        }
        else
        {
            //如果下一跳是自己的上一跳，则可能出现了路由回路，可能导致TTL的快速消耗，故暂缓发送数据包
            grp::DataPacketHeader DHeader;
            DHeader.SetNextJID(nextjid);
            DHeader.SetSenderID(m_id);		
            packet->AddHeader(DHeader);
            DelayPacketQueueEntry qentry(packet, header, ucb, nextHop);		
  		    m_delayqueue.push_back(qentry);	
            Simulator::Schedule(m_helloInterval / 4, &RoutingProtocol::SendFromDelayQueue, this);
        }	
	}
	else
	{
        //如果返回的IPv4地址为127.0.0.1，则说明当前时刻没有合适的下一跳节点
        //节点启用Carry_and_forward机制，将数据包暂时缓存起来，直到有可用下一跳节点或信息过期为止
        grp::DataPacketHeader DHeader;
  		DHeader.SetNextJID(nextjid);	
        DHeader.SetSenderID(m_id);	
  		packet->AddHeader(DHeader);		

    	QPacketInfo pInfo(origin, dest);		
  		QMap::const_iterator pItr = m_wTimeCache.find(pInfo);		
  		if(pItr == m_wTimeCache.end())		
  		{		
  			Time &pTime = m_wTimeCache[pInfo];		
  			pTime = Simulator::Now();		
  		}		

    	PacketQueueEntry qentry(packet, header, ucb);		
  		m_pwaitqueue.push_back(qentry);		
  		m_StorePacketTrace(header);
	}
	return true;
}

void
RoutingProtocol::NotifyInterfaceUp (uint32_t i)
{
}
void
RoutingProtocol::NotifyInterfaceDown (uint32_t i)
{
}
void
RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
}
void
RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address)
{
}


}
}

