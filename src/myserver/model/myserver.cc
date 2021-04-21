/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "myserver.h"
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

namespace ns3 {
namespace myserver {

NS_OBJECT_ENSURE_REGISTERED (RoutingProtocol);

TypeId
RoutingProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::myserver::RoutingProtocol")
    .SetParent<Ipv4RoutingProtocol> ()
    .SetGroupName ("myserver")
    .AddConstructor<RoutingProtocol> ()
    ;
  return tid;
}

RoutingProtocol::RoutingProtocol (): m_ipv4 (0) 
{
  m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
}
RoutingProtocol::~RoutingProtocol () {}
void RoutingProtocol::SetIpv4 (Ptr<Ipv4> ipv4) 
{
  m_ipv4 = ipv4;
}
void RoutingProtocol::DoDispose () 
{
  m_ipv4 = 0;
}
void RoutingProtocol::PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit) const {}
void RoutingProtocol::SetMainInterface (uint32_t interface)
{
  m_mainAddress = m_ipv4->GetAddress (interface, 0).GetLocal ();
}
void RoutingProtocol::SetDownTarget (IpL4Protocol::DownTargetCallback callback) {}
void RoutingProtocol::AddHeader (Ptr<Packet> p, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route) {}
void RoutingProtocol::NotifyInterfaceUp (uint32_t i) {}
void RoutingProtocol::NotifyInterfaceDown (uint32_t i) {}
void RoutingProtocol::NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address) {}
void RoutingProtocol::NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address) {}

int64_t
RoutingProtocol::AssignStreams (int64_t stream)
{
  m_uniformRandomVariable->SetStream (stream);
  return 1;
}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput (Ptr<Packet> p, const Ipv4Header &header, Ptr<NetDevice> oif, Socket::SocketErrno &sockerr)
{
  Ptr<Ipv4Route> rtentry;
  return rtentry;
}

bool RoutingProtocol::RouteInput  (Ptr<const Packet> p,
                                   const Ipv4Header &header, Ptr<const NetDevice> idev,
                                   UnicastForwardCallback ucb, MulticastForwardCallback mcb,
                                   LocalDeliverCallback lcb, ErrorCallback ecb)
{
  return true;
}

void
RoutingProtocol::SendRoadConInfoViaLTE(RoadSegment info, Time time)
{
    std::map<RoadSegment, Time>::iterator itr = m_contable.find(info);
    if(itr != m_contable.end())
    {
        if(itr->second < time)
            itr->second = time;
    }
    else
    {
        m_contable[info] = time;
    }
}

void
RoutingProtocol::ReturnTokenToLC(int jid)
{
  junSinkList[jid] = 0;
}

int
RoutingProtocol::RequireTokenFromLC(int jid)
{ 
    if(junSinkList[jid] > 0)
        return -1;
    else
    {
        junSinkList[jid] = 1;
        return 1;
    }
}

bool BlockSortFunU(const BlockMemberInfo& info1, const BlockMemberInfo& info2)
{
  return info1.location < info2.location;
}

bool BlockSortFunD(const BlockMemberInfo& info1, const BlockMemberInfo& info2)
{
  return info1.location > info2.location;
}

void 
RoutingProtocol::PurgeBlockInfo(RoadSegment seg)
{
    std::vector<BlockMemberInfo>& rlist = m_bnodeTable[seg];
    double time = Simulator::Now().GetSeconds();
    std::vector<BlockMemberInfo>::iterator itr = rlist.begin();
    while(itr != rlist.end())
    {
        if(itr->time < time)
        {
            rlist.erase(itr);
            continue;
        }
        itr++;
    }
    UpdateAvailableBlock(seg);
}

void
RoutingProtocol::UpdateAvailableBlock(RoadSegment seg)
{
    std::vector<BlockMemberInfo> rlist = m_bnodeTable[seg];

    if(rlist.empty() == true)
        return;
    
    int dir = GetRoadDirection(seg.sjid, seg.ejid);
    if(dir == 0 || dir == 1)
    {
        sort(rlist.begin(), rlist.end(), BlockSortFunU);
    }
    else
    {
        sort(rlist.begin(), rlist.end(), BlockSortFunD);
    }

    std::vector<BlockInfo> rblock;
    std::vector<BlockMemberInfo>::iterator itr = rlist.begin(); 
    std::vector<BlockMemberInfo>::iterator rend = rlist.end();

    if(itr->vType == BlockMemberInfo::HEADER)
    {
        BlockInfo block;
        if(dir == 0 || dir == 2)
            block.tloc = m_map[seg.sjid].x;
        else
            block.tloc = m_map[seg.sjid].y;
        block.hloc = itr->location;
        rblock.push_back(block);
        itr++;
    }
    if((rend - 1)->vType == BlockMemberInfo::TAILER)
    {
        BlockInfo block;
        if(dir == 0 || dir == 2)
            block.hloc = m_map[seg.ejid].x;
        else
            block.hloc = m_map[seg.ejid].y;
        block.tloc = rend->location;
        rblock.push_back(block);
        rend--;
    }

    std::stack<BlockMemberInfo> bstack;
    while(itr != rend)
    {
        BlockInfo block;

        if(itr->vType == BlockMemberInfo::ISOLATED)
        {
            block.hloc = block.tloc = itr->location;
            rblock.push_back(block);
        }
        else if(itr->vType == BlockMemberInfo::TAILER)
        {
            bstack.push(*itr);
        }
        else
        {
            if(bstack.empty() == false)
            {
                BlockMemberInfo tailer = bstack.top();
                bstack.pop();
                block.hloc = itr->location;
                block.tloc = tailer.location;
                rblock.push_back(block);

                if(itr->dir < 2 && block.tloc > block.hloc)
                {
                    NS_LOG_UNCOND("ERROR!");
                }
                if(itr->dir >= 2 && block.tloc < block.hloc)
                {
                    NS_LOG_UNCOND("ERROR!");
                }
            }
        }
        itr++;
    }

    blocklist[seg] = rblock;
}

void 
RoutingProtocol::AddBlockInfo(int sjid, int ejid, BlockMemberInfo info)
{
    RoadSegment seg(sjid, ejid);
    std::vector<BlockMemberInfo>& rlist = m_bnodeTable[seg];
    std::vector<BlockMemberInfo>::iterator itr = rlist.begin();
    while(itr != rlist.end())
    {
        if(itr->id == info.id)
        {
            rlist.erase(itr);
            break;
        }
        itr++;
    }
    rlist.push_back(info);

    UpdateAvailableBlock(seg);
    Simulator::Schedule(Seconds(BlockUpdateTime + 0.1), &RoutingProtocol::PurgeBlockInfo, this, seg);
}

void
RoutingProtocol::SetDigitalMap(std::vector<DigitalMapEntry> map)
{
    m_map.assign(map.begin(), map.end());
}

int 
RoutingProtocol::GetRoadDirection(int i, int j)
{
    double cx = m_map[i].x;
	double cy = m_map[i].y;
	double nx = m_map[j].x;
	double ny = m_map[j].y;

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

double
RoutingProtocol::GetMaxBlankLen(std::vector<BlockInfo> block, RoadSegment seg)
{
    double spos = 0, epos = 0;
    int dir = GetRoadDirection(seg.sjid, seg.ejid);
    int factor = 0;

    if(dir == 0 || dir == 1)
    {
        factor = 1;
    }
    else
    {
        factor = -1;
    }
    

    if(dir == 0 || dir == 2)
    {
        spos = m_map[seg.sjid].x;
        epos = m_map[seg.ejid].x;
    }
    else
    {
        spos = m_map[seg.sjid].y;
        epos = m_map[seg.ejid].y;
    }

    double lmax = -1;
    for(std::vector<BlockInfo>::iterator pitr = block.begin(); pitr != block.end(); pitr++)
    {
        double blen = (pitr->tloc - spos) * factor;
        if(blen > lmax)
        {
            lmax = blen;
        }
        spos = pitr->hloc;
    }
    if(spos != epos && (epos - spos) * factor > lmax)
    {
        lmax = (epos - spos) * factor;
    }
    return lmax;
}

double
RoutingProtocol::GetMaxBlockLen(std::vector<BlockInfo> block, RoadSegment seg)
{
    int factor = 0;
    int dir = GetRoadDirection(seg.sjid, seg.ejid);

    if(dir == 0 || dir == 1)
    {
        factor = 1;
    }
    else
    {
        factor = -1;
    }

    double maxlen = -1;
    for(std::vector<BlockInfo>::iterator itr = block.begin(); itr != block.end(); itr++)
    {
        double len = (itr->hloc - itr->tloc) * factor;
        if(len > maxlen)
        {
            maxlen = len;
        }
    }
    return maxlen;
}

double
RoutingProtocol::GetCoverRate(std::vector<BlockInfo> block, RoadSegment seg)
{
    int factor = 0;
    int dir = GetRoadDirection(seg.sjid, seg.ejid);

    if(dir == 0 || dir == 1)
    {
        factor = 1;
    }
    else
    {
        factor = -1;
    }

    double sum = 0;
    double len = sqrt(pow(m_map[seg.sjid].x - m_map[seg.ejid].x, 2) + pow(m_map[seg.sjid].y - m_map[seg.ejid].y, 2));
    for(std::vector<BlockInfo>::iterator itr = block.begin(); itr != block.end(); itr++)
    {
        sum += (itr->hloc - itr->tloc) * factor;
    }
    return sum / len;
}

double
RoutingProtocol::CalculateRoadCost(RoadSegment pseg, RoadSegment nseg)
{
    double cost = INF;
    std::vector<BlockInfo> pblock = blocklist[pseg];
    std::vector<BlockInfo> nblock = blocklist[nseg];
    if(pblock.empty() == false && nblock.empty() == false)
    {
        double pblen = GetMaxBlankLen(pblock, pseg);
        double nblen = GetMaxBlankLen(nblock, nseg);
        double maxblanklen = 0, maxblocklen = 0;
        
        if(pblen > nblen)
        {
            maxblanklen = pblen;
            maxblocklen = GetMaxBlockLen(pblock, pseg);
        }
        else
        {
            maxblanklen = nblen;
            maxblocklen = GetMaxBlockLen(nblock, nseg);
        }

        double flen = 0;
        if(maxblocklen > 0)
        {
            double x1 = (maxblocklen - 0.5 * maxblanklen) / maxblocklen;
            flen = 1.0 / (1 + exp((0.5 - x1) / 0.1));
        }

        double Ns = 5;
        double x2 = fabs(pblock.size() - nblock.size()) / Ns;
        double fnum = exp(- PI * x2);

        double x3 = GetCoverRate(pblock, pseg);
        double x4 = GetCoverRate(nblock, nseg);
        double pfcov = 1 - exp(-(10 * x3) / PI);
        double nfcov = 1 - exp(-(10 * x4) / PI);
        double fcov = pfcov * nfcov;

        cost = 1 - flen * fnum * fcov;
    }

    return cost;
}

void 
RoutingProtocol::GetRoadCostGraph()
{
    for(int i = 0; i < jnum; i++)
    {
        for(int j = i + 1; j < jnum; j++)
        {
            CityRoadCostGraph[i][j] = CityRoadCostGraph[j][i] = INF;

            RoadSegment pseg(i, j);
            RoadSegment nseg(j, i);

            std::map<RoadSegment, Time>::iterator pitr = m_contable.find(pseg);
            std::map<RoadSegment, Time>::iterator nitr = m_contable.find(nseg);
            if
            (
                pitr != m_contable.end() && pitr->second > Simulator::Now() &&
                nitr != m_contable.end() && nitr->second > Simulator::Now()
            )
            {
                CityRoadCostGraph[i][j] = CityRoadCostGraph[j][i] = 0;
            }
            else
            {
                CityRoadCostGraph[i][j] = CityRoadCostGraph[j][i] = CalculateRoadCost(pseg, nseg);
            }
        }
    }
}

int
RoutingProtocol::DijkstraAlgorithm(int srcjid)
{
    bool visited[jnum];
    double distance[jnum];
    int parent[jnum];

    for(int i = 0; i<jnum; i++)
    {
        visited[i] = false;
        distance[i] = INF;
        parent[i] = -1;
    }

    visited[srcjid] = true;
    distance[srcjid] = 0;

    int curr = srcjid;
    int next = -1;
    for(int count = 1; curr >= 0 && count <= jnum; count++)
    {
        for(int n = 0; n < jnum; n++)
        {
            if(visited[n] == false && distance[curr] + CityRoadCostGraph[curr][n] < distance[n])
            {
                distance[n] = distance[curr] + CityRoadCostGraph[curr][n];
                parent[n] = curr;
                next = n;
            }
        }
        curr = next;
        visited[curr] = true;
    }

    double mincost = INF;
    int minid = -1;
    for(std::vector<int>::iterator itr = RSUSet.begin(); itr != RSUSet.end(); itr ++)
    {
        if(distance[*itr] < mincost)
        {
            mincost = distance[*itr];
            minid = *itr;
        }
    }

    while(minid > 0)
    {
        if(parent[minid] == srcjid)
            break;
        minid = parent[minid];
    }

    return minid; 
}

int
RoutingProtocol::GetNextJunInPath(int src)
{
    GetRoadCostGraph();
    return DijkstraAlgorithm(src);
}


void RoutingProtocol::DoInitialize() 
{
  CityRoadCostGraph = new double *[jnum];
  for(int i = 0;i < jnum; ++i)
  {
    CityRoadCostGraph[i] = new double[jnum];
    for(int j = 0; j<jnum; j++)
    {
      CityRoadCostGraph[i][j] = INF;
    }
  }
  PrintRoadConInfo();
  AddRSU();
}

void
RoutingProtocol::PrintRoadConInfo()
{
    for(std::map<RoadSegment, Time>::iterator itr = m_contable.begin(); itr != m_contable.end(); itr++)
    {
        if(itr->second > Simulator::Now())
            NS_LOG_UNCOND("" << Simulator::Now().GetSeconds() << " " << itr->first.sjid << "->" << itr->first.ejid);
    }

    Simulator::Schedule(Seconds(1), &RoutingProtocol::PrintRoadConInfo, this);
}

void
RoutingProtocol::AddRSU()
{
  RSUSet.push_back(3);
}



}
}

