/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <cmath>
#include "ns3/assert.h"
#include "ns3/log.h"
#include "grp-header.h"

#define GRP_DATA_PKT_HEADER_SIZE 3
#define GRP_CTR_PKT_HEADER_SIZE 4
#define GRP_BLOCK_PKT_HEADER_SIZE 24
#define GRP_MSG_HEADER_SIZE 12
#define IPV4_ADDRESS_SIZE 4

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GrpHeader");

namespace grp {

#define GRP_C 0.0625

uint8_t SecondsToEmf (double seconds)
{
  int a, b = 0;
  NS_ASSERT_MSG (seconds >= GRP_C, "SecondsToEmf - Can not convert a value less than GRP_C");
  for (b = 1; (seconds / GRP_C) >= (1 << b); ++b)
    {
    }
  NS_ASSERT ((seconds / GRP_C) < (1 << b));
  b--;
  NS_ASSERT ((seconds / GRP_C) >= (1 << b));
  double tmp = 16 * (seconds / (GRP_C * (1 << b)) - 1);
  a = (int) std::ceil (tmp - 0.5);
  if (a == 16)
    {
      b += 1;
      a = 0;
    }
  NS_ASSERT (a >= 0 && a < 16);
  NS_ASSERT (b >= 0 && b < 16);
  return (uint8_t)((a << 4) | b);
}

double EmfToSeconds (uint8_t grpFormat)
{
  int a = (grpFormat >> 4);
  int b = (grpFormat & 0xf);
  return GRP_C * (1 + a / 16.0) * (1 << b);
}

uint64_t LocToUint64 (int64_t loc)
{
	uint64_t res = 0;
	if(loc >= 0)
	{
		res = (uint64_t)loc;
	}
	else
	{
		res = (uint64_t)(-loc);
		res = res | 0x8000000000000000;
	}
	return res;

}

int64_t Uint64ToLoc (uint64_t uin)
{
	int64_t res = 0;
	if(uin >> 63 != 0)
	{
		res = -(int64_t)(uin & 0x7fffffffffffffff);
	}
	else
	{
		res = (int64_t) uin;
	}
	return res;
}

/// ----------------- GRP Block Packet ---------------------------------
NS_OBJECT_ENSURE_REGISTERED (BlockPacketHeader);

BlockPacketHeader::BlockPacketHeader()
{

}

BlockPacketHeader::~BlockPacketHeader()
{

}

TypeId
BlockPacketHeader::GetTypeId()
{
	static TypeId tid = TypeId("ns3::grp::BlockPacketHeader")
			.SetParent<Header>()
			.SetGroupName("grp")
			.AddConstructor<BlockPacketHeader>()
			;
	return tid;
}

TypeId
BlockPacketHeader::GetInstanceTypeId() const
{
	return GetTypeId();
}

void
BlockPacketHeader::Print(std::ostream &os) const
{

}

uint32_t
BlockPacketHeader::GetSerializedSize() const
{
	return GRP_BLOCK_PKT_HEADER_SIZE;
}

void
BlockPacketHeader::Serialize(Buffer::Iterator start) const
{
	Buffer::Iterator i = start;
	i.WriteU32(m_addr.Get());
	i.WriteU32(m_speed);
	i.WriteU64(m_posx);
	i.WriteU64(m_posy);
}

uint32_t
BlockPacketHeader::Deserialize(Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_addr = Ipv4Address (i.ReadU32 ());
	m_speed = i.ReadU32();
	m_posx = i.ReadU64();
	m_posy = i.ReadU64();
	return GetSerializedSize ();
}



//// ---------------- GRP Controll Packet -------------------------------

NS_OBJECT_ENSURE_REGISTERED (CtrPacketHeader);

CtrPacketHeader::CtrPacketHeader ()
{
}

CtrPacketHeader::~CtrPacketHeader ()
{
}

TypeId
CtrPacketHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::grp::CtrPacketHeader")
    .SetParent<Header> ()
    .SetGroupName ("grp")
    .AddConstructor<CtrPacketHeader> ()
  ;
  return tid;
}
TypeId
CtrPacketHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
CtrPacketHeader::GetSerializedSize (void) const
{
  return GRP_CTR_PKT_HEADER_SIZE;
}

void
CtrPacketHeader::Print (std::ostream &os) const
{
  /// \todo
}

void
CtrPacketHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU16 (m_packetLength);
  i.WriteU16 (m_packetSequenceNumber);
}

uint32_t
CtrPacketHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_packetLength  = i.ReadU16 ();
  m_packetSequenceNumber = i.ReadU16 ();
  return GetSerializedSize ();
}

// ---------------- Data Packet -------------------------------
NS_OBJECT_ENSURE_REGISTERED (DataPacketHeader);

DataPacketHeader::DataPacketHeader ()
{
}

DataPacketHeader::~DataPacketHeader ()
{
}

TypeId
DataPacketHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::grp::DataPacketHeader")
    .SetParent<Header> ()
    .SetGroupName ("grp")
    .AddConstructor<DataPacketHeader> ()
  ;
  return tid;
}
TypeId
DataPacketHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
DataPacketHeader::GetSerializedSize (void) const
{
  return GRP_DATA_PKT_HEADER_SIZE;
}

void
DataPacketHeader::Print (std::ostream &os) const
{
}

void
DataPacketHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (nextjid);
  i.WriteU16 (sender);
}

uint32_t
DataPacketHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  nextjid  = i.ReadU8 ();
  sender = i.ReadU16 ();
  return GetSerializedSize ();
}

// ---------------- GRP Message -------------------------------

NS_OBJECT_ENSURE_REGISTERED (MessageHeader);

MessageHeader::MessageHeader ()
  : m_messageType (MessageHeader::MessageType (0))
{
}

MessageHeader::~MessageHeader ()
{
}

TypeId
MessageHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::grp::MessageHeader")
    .SetParent<Header> ()
    .SetGroupName ("grp")
    .AddConstructor<MessageHeader> ()
  ;
  return tid;
}
TypeId
MessageHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
MessageHeader::GetSerializedSize (void) const
{
  uint32_t size = GRP_MSG_HEADER_SIZE;
  switch (m_messageType)
    {
    case HELLO_MESSAGE:
      NS_LOG_DEBUG ("Hello Message Size: " << size << " + " << m_message.hello.GetSerializedSize ());
      size += m_message.hello.GetSerializedSize ();
      break;
    default:
      NS_ASSERT (false);
    }
  return size;
}

void
MessageHeader::Print (std::ostream &os) const
{
  /// \todo
}

void
MessageHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU8 (m_messageType);
  i.WriteU8 (m_vTime);
  i.WriteU16 (GetSerializedSize ());
  i.WriteU32 (m_originatorAddress.Get ());
  i.WriteU8 (m_timeToLive);
  i.WriteU8 (m_hopCount);
  i.WriteU16 (m_messageSequenceNumber);

  switch (m_messageType)
    {
    case HELLO_MESSAGE:
      m_message.hello.Serialize (i);
      break;
    default:
      NS_ASSERT (false);
    }

}

uint32_t
MessageHeader::Deserialize (Buffer::Iterator start)
{
  uint32_t size;
  Buffer::Iterator i = start;
  m_messageType  = (MessageType) i.ReadU8 ();
  NS_ASSERT (m_messageType >= HELLO_MESSAGE && m_messageType <= CPACK_MESSAGE);
  m_vTime  = i.ReadU8 ();
  m_messageSize  = i.ReadU16 ();
  m_originatorAddress = Ipv4Address (i.ReadU32 ());
  m_timeToLive  = i.ReadU8 ();
  m_hopCount  = i.ReadU8 ();
  m_messageSequenceNumber = i.ReadU16 ();
  size = GRP_MSG_HEADER_SIZE;
  switch (m_messageType)
    {
    case HELLO_MESSAGE:
      size += m_message.hello.Deserialize (i, m_messageSize - GRP_MSG_HEADER_SIZE);
      break;
    default:
      NS_ASSERT (false);
    }
  return size;
}

// ---------------- GRP HELLO Message -------------------------------

uint32_t
MessageHeader::Hello::GetSerializedSize (void) const
{
    uint32_t size = 28;
    size += this->neighborInterfaceAddresses.size () * IPV4_ADDRESS_SIZE;
    size += this->conlist.size();

    for(std::vector<JunInfo>::const_iterator itr = this->conlist.begin(); itr != this->conlist.end(); itr++)
    {
        size += 6 + itr->list.size();
    }
    
    return size;
}

void
MessageHeader::Hello::Print (std::ostream &os) const
{
}

void
MessageHeader::Hello::Serialize (Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteU64 (this->locationX);
    i.WriteU64 (this->locationY);
    i.WriteU32 (this->speed);
    i.WriteU32 (this->direction);
    i.WriteU16 (this->turn);

    i.WriteU8 ((uint8_t)this->conlist.size());

    int lsize = 0;
    for(std::vector<JunInfo>::const_iterator itr = this->conlist.begin(); itr != this->conlist.end(); itr++)
    {
        lsize += 6 + itr->list.size();
    }
    i.WriteU8 (lsize);

    for (std::vector<Ipv4Address>::const_iterator iter = this->neighborInterfaceAddresses.begin ();
       iter != this->neighborInterfaceAddresses.end (); iter++)
    {
        i.WriteU32 (iter->Get ());
    }

    for(std::vector<JunInfo>::const_iterator itr = this->conlist.begin(); itr != this->conlist.end(); itr++)
    {
        i.WriteU8 (itr->jid);
        i.WriteU32 ((uint32_t)(itr->version));
        i.WriteU8 ((uint8_t)itr->list.size());
        for(std::vector<int>::const_iterator li = itr->list.begin(); li != itr->list.end(); li ++)
        {
            i.WriteU8 (*li);
        }
    }
}

uint32_t
MessageHeader::Hello::Deserialize (Buffer::Iterator start, uint32_t messageSize)
{
    Buffer::Iterator i = start;
    int basesize = 28;

    this->neighborInterfaceAddresses.clear ();
    this->locationX = i.ReadU64 ();
    this->locationY = i.ReadU64 ();
    this->speed = i.ReadU32 ();
    this->direction = i.ReadU32 ();
    this->turn = i.ReadU16 ();
    
    int num = i.ReadU8 ();
    int listsize = i.ReadU8 ();

    int numAddresses = (messageSize - basesize - listsize) / IPV4_ADDRESS_SIZE;
    this->neighborInterfaceAddresses.erase (this->neighborInterfaceAddresses.begin (),
                                    this->neighborInterfaceAddresses.end ());
    for (int n = 0; n < numAddresses; ++n)
    {
        this->neighborInterfaceAddresses.push_back (Ipv4Address (i.ReadU32 ()));
    }

    this->conlist.erase(this->conlist.begin(), this->conlist.end());
    for(int n = 0; n < num; ++n)
    {
        JunInfo info;
        info.jid = i.ReadU8();
        info.version = (int)i.ReadU32();
        int size = i.ReadU8();
        for(int k = 0; k < size; k++)
        {
            info.list.push_back(i.ReadU8());
        }
        this->conlist.push_back(info);
    }

    this->bsize = messageSize;
    this->asize = messageSize - basesize - this->neighborInterfaceAddresses.size() * IPV4_ADDRESS_SIZE;

    return GetSerializedSize ();
}



/*****************************CP message***************/
//uint32_t
// MessageHeader::CP::GetSerializedSize (void) const
// {
//     uint32_t size = 28;
//     size += this->neighborInterfaceAddresses.size () * IPV4_ADDRESS_SIZE;
//     size += this->conlist.size();

//     for(std::vector<JunInfo>::const_iterator itr = this->conlist.begin(); itr != this->conlist.end(); itr++)
//     {
//         size += 6 + itr->list.size();
//     }
    
//     return size;
// }

void
MessageHeader::CP::Print (std::ostream &os) const
{
}

void
MessageHeader::CP::Serialize (Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteU64 (this->O_V_ID);
    i.WriteU64 (this->TIMES);
    i.WriteU32 (this->F_Jun_ID);
    i.WriteU32 (this->TO_Jun_ID);
    i.WriteU16 (this->TN_v);
    i.WriteU16 (this->Lifetime);
    i.WriteU16 (this->TN_h);

    //i.WriteU8 ((uint8_t)this->conlist.size());

    // int lsize = 0;
    // for(std::vector<JunInfo>::const_iterator itr = this->conlist.begin(); itr != this->conlist.end(); itr++)
    // {
    //     lsize += 6 + itr->list.size();
    // }
    // i.WriteU8 (lsize);

    // for (std::vector<Ipv4Address>::const_iterator iter = this->neighborInterfaceAddresses.begin ();
    //    iter != this->neighborInterfaceAddresses.end (); iter++)
    // {
    //     i.WriteU32 (iter->Get ());
    // }

    // for(std::vector<JunInfo>::const_iterator itr = this->conlist.begin(); itr != this->conlist.end(); itr++)
    // {
    //     i.WriteU8 (itr->jid);
    //     i.WriteU32 ((uint32_t)(itr->version));
    //     i.WriteU8 ((uint8_t)itr->list.size());
    //     for(std::vector<int>::const_iterator li = itr->list.begin(); li != itr->list.end(); li ++)
    //     {
    //         i.WriteU8 (*li);
    //     }
    // }
}

uint32_t
MessageHeader::CP::Deserialize (Buffer::Iterator start, uint32_t messageSize)
{
    Buffer::Iterator i = start;
    int basesize = 30;

    //this->neighborInterfaceAddresses.clear ();
    this->O_V_ID = i.ReadU64 ();
    this->TIMES = i.ReadU64 ();
    this->F_Jun_ID = i.ReadU32 ();
    this->TO_Jun_ID = i.ReadU32 ();
    this->TN_v = i.ReadU16 ();
    this->Lifetime = i.ReadU16 ();
    this->TN_h = i.ReadU16 ();
    
    // int num = i.ReadU8 ();
    // int listsize = i.ReadU8 ();

    // int numAddresses = (messageSize - basesize - listsize) / IPV4_ADDRESS_SIZE;
    // this->neighborInterfaceAddresses.erase (this->neighborInterfaceAddresses.begin (),
    //                                 this->neighborInterfaceAddresses.end ());
    // for (int n = 0; n < numAddresses; ++n)
    // {
    //     this->neighborInterfaceAddresses.push_back (Ipv4Address (i.ReadU32 ()));
    // }

    // this->conlist.erase(this->conlist.begin(), this->conlist.end());
    // for(int n = 0; n < num; ++n)
    // {
    //     JunInfo info;
    //     info.jid = i.ReadU8();
    //     info.version = (int)i.ReadU32();
    //     int size = i.ReadU8();
    //     for(int k = 0; k < size; k++)
    //     {
    //         info.list.push_back(i.ReadU8());
    //     }
    //     this->conlist.push_back(info);
    // }

    // this->bsize = messageSize;
    // this->asize = messageSize - basesize - this->neighborInterfaceAddresses.size() * IPV4_ADDRESS_SIZE;

    return basesize;
}

}
}
