/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef MYSERVER_H
#define MYSERVER_H

#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/event-garbage-collector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/timer.h"
#include "ns3/traced-callback.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/mobility-model.h"
#include <vector>
#include <map>
#include <queue>
#include <stack>
#include "ns3/ip-l4-protocol.h"
#include "ns3/digitalMap.h"

#define INF 1000000
#define PI 3.14159265358979323846


namespace ns3 {

struct RoadSegment
{
    int sjid;
    int ejid;

    RoadSegment()
    {
        
    }

    RoadSegment(int sjid, int ejid)
    {
        this->sjid = sjid;
        this->ejid = ejid;
    }

    RoadSegment(const RoadSegment& seg)
    {
        *this = seg;
    }

    bool operator< (const RoadSegment& seg) const
    {
        if(this->sjid < seg.sjid)
            return true;
        else if(this->sjid == seg.sjid)
        {
            if(this->ejid < seg.ejid)
                return true;
            else
                return false;
        }
        else
            return false;
    }
};

struct BlockMemberInfo
{
    enum VehType
    {
      HEADER = 0,
      TAILER = 1,
      ISOLATED = 2,
    } vType;

    int id, dir;
    double location;
    Time time;

    BlockMemberInfo(VehType vtype, int id, double location, int dir, Time time)
    {
        this->vType = vtype;
        this->id = id;
        this->location = location;
        this->dir = dir;
        this->time = time;
    }

};

struct BlockInfo
{
    double hloc;
    double tloc;

    BlockInfo()
    {
        
    }
};

typedef std::map<RoadSegment, std::vector<BlockMemberInfo>> BlockNodeTable;
typedef std::map<RoadSegment, std::vector<BlockInfo>> BlockList;

namespace myserver {
class RoutingProtocol;

class RoutingProtocol : public Ipv4RoutingProtocol
{
public:
    static TypeId GetTypeId (void);
    RoutingProtocol ();
    virtual ~RoutingProtocol ();
    int64_t AssignStreams (int64_t stream);
    void SetMainInterface (uint32_t interface);
    void SetDownTarget (IpL4Protocol::DownTargetCallback callback);
    void AddHeader(Ptr<Packet> p, Ipv4Address source, Ipv4Address destination, uint8_t protocol, Ptr<Ipv4Route> route);

    void SetDigitalMap(std::vector<DigitalMapEntry> map);
    void AddRSU();
    
    void SendRoadConInfoViaLTE(RoadSegment info, Time time);
    int RequireTokenFromLC(int jid);
    void ReturnTokenToLC(int jid);

    void AddBlockInfo(int sjid, int ejid, BlockMemberInfo info);
    
    int GetRoadDirection(int i, int j);
    int GetNextJunInPath(int src);
    void UpdateAvailableBlock(RoadSegment seg);
    void PurgeBlockInfo(RoadSegment seg);
    void GetRoadCostGraph();
    double CalculateRoadCost(RoadSegment pseg, RoadSegment nseg);
    int DijkstraAlgorithm(int srcjid);
    double GetMaxBlankLen(std::vector<BlockInfo> block, RoadSegment seg);
    double GetMaxBlockLen(std::vector<BlockInfo> block, RoadSegment seg);
    double GetCoverRate(std::vector<BlockInfo> block, RoadSegment seg);
    void PrintRoadConInfo();

protected:
    virtual void DoInitialize (void);
private:
    Ptr<Ipv4> m_ipv4;
    Ptr<UniformRandomVariable> m_uniformRandomVariable;
    Ipv4Address m_mainAddress;
    // From Ipv4RoutingProtocol
    virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p,
                                        const Ipv4Header &header,
                                        Ptr<NetDevice> oif,
                                        Socket::SocketErrno &sockerr);
    virtual bool RouteInput (Ptr<const Packet> p,
                            const Ipv4Header &header,
                            Ptr<const NetDevice> idev,
                            UnicastForwardCallback ucb,
                            MulticastForwardCallback mcb,
                            LocalDeliverCallback lcb,
                            ErrorCallback ecb);
    virtual void NotifyInterfaceUp (uint32_t interface);
    virtual void NotifyInterfaceDown (uint32_t interface);
    virtual void NotifyAddAddress (uint32_t interface, Ipv4InterfaceAddress address);
    virtual void NotifyRemoveAddress (uint32_t interface, Ipv4InterfaceAddress address);
    virtual void SetIpv4 (Ptr<Ipv4> ipv4);
    virtual void PrintRoutingTable (Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;
    void DoDispose ();

    int jnum = 9;
    double insightTransRange = 500;
    int junSinkList[9] = {0};
    double BlockUpdateTime = 1;

    double ** CityRoadCostGraph;
    BlockList blocklist;
    std::vector<int> RSUSet;
    BlockNodeTable m_bnodeTable;
    std::vector<DigitalMapEntry> m_map;
    std::map<RoadSegment, Time> m_contable;

};

}

}

#endif /* MYSERVER_H */

