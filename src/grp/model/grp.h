/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef GRP_H
#define GRP_H
#include <climits>
#include "grp-header.h"
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
#include "ns3/ip-l4-protocol.h"
#include "ns3/digitalMap.h"
#include "ns3/myserver.h"


namespace ns3 {

struct PacketQueueEntry
{
 	typedef Ipv4RoutingProtocol::UnicastForwardCallback UnicastForwardCallback;

  	Ptr<Packet> m_packet;
 	Ipv4Header m_header;
 	UnicastForwardCallback m_ucb;

  	PacketQueueEntry(Ptr<Packet> p, Ipv4Header h, UnicastForwardCallback u)
 	{
 		this->m_packet = p;
 		this->m_header = h;
 		this->m_ucb = u;
 	}

};

struct DelayPacketQueueEntry
{
 	typedef Ipv4RoutingProtocol::UnicastForwardCallback UnicastForwardCallback;

  	Ptr<Packet> m_packet;
 	Ipv4Header m_header;
 	UnicastForwardCallback m_ucb;
    Ipv4Address m_nexthop;

  	DelayPacketQueueEntry(Ptr<Packet> p, Ipv4Header h, UnicastForwardCallback u, Ipv4Address nexthop)
 	{
 		this->m_packet = p;
 		this->m_header = h;
 		this->m_ucb = u;
        this->m_nexthop = nexthop;
 	}

};

struct QPacketInfo
{
 	Ipv4Address src;
 	Ipv4Address dst;

  	QPacketInfo(Ipv4Address src, Ipv4Address dst)
 	{
 		this->src = src;
 		this->dst = dst;
 	}

  	QPacketInfo(const QPacketInfo& info)
 	{
 		*this = info;
 	}

  	bool operator< (const QPacketInfo& info) const
 	{
 		if(this->src < info.src)
 			return true;
 		else if(this->src == info.src)
 		{
 			if(this->dst < info.dst)
 				return true;
 			else
 				return false;
 		}
 		else
 			return false;
 	}

};

typedef std::map<QPacketInfo, Time> QMap;

struct SendingQueue
{
 	typedef Ipv4RoutingProtocol::UnicastForwardCallback UnicastForwardCallback;

  	Ptr<Packet> m_packet;
 	Ipv4Header m_header;
 	UnicastForwardCallback m_ucb;
 	Ipv4Address nexthop;

  	SendingQueue(Ptr<Packet> p, Ipv4Header h, UnicastForwardCallback u, Ipv4Address nexthop)
 	{
 		this->m_packet = p;
 		this->m_header = h;
 		this->m_ucb = u;
 		this->nexthop = nexthop;
    }

};
namespace grp {


struct NeighborTableEntry
{
    int N_turn;
    int N_direction;
    Time N_time;
    double N_speed;
    double N_location_x;
    double N_location_y;
    uint16_t N_sequenceNum;
    Ipv4Address receiverIfaceAddr;
    Ipv4Address N_neighbor_address;

    enum Status
    {
        STATUS_NOT_SYM = 0,
        STATUS_SYM = 1,
    } N_status;

    NeighborTableEntry () :
        N_turn (-1), N_speed (0), N_location_x (0), N_location_y (0),
        N_sequenceNum (0),N_neighbor_address (), N_status (STATUS_NOT_SYM)
    {

    }
};

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

    typedef void (* m_DropPacketTraceCallback)(const Ipv4Header &header);
    typedef void (* m_StorePacketTraceCallback)(const Ipv4Header &header);

protected:
    virtual void DoInitialize (void);  

private:
    Ptr<Ipv4> m_ipv4;
    Ipv4Address m_mainAddress;

    Time m_helloInterval;
    Timer m_helloTimer;
    Timer m_positionCheckTimer;
    Timer m_queuedMessagesTimer;
    Timer m_speedTimer;

    grp::MessageList m_queuedMessages;

    uint16_t m_packetSequenceNumber;
    uint16_t m_messageSequenceNumber;

    IpL4Protocol::DownTargetCallback m_downTarget;
    Ptr<UniformRandomVariable> m_uniformRandomVariable;
    std::map<Ipv4Address, NeighborTableEntry> m_neiTable;
    

    Ptr<Socket> m_recvSocket;
    std::map<Ptr<Socket>, Ipv4InterfaceAddress > m_sendSockets;
    std::map<Ptr<Socket>, Ipv4InterfaceAddress > m_sendBlockSockets;
    

    TracedCallback <const Ipv4Header &> m_DropPacketTrace;
    TracedCallback <const Ipv4Header &> m_StorePacketTrace;


    inline uint16_t GetPacketSequenceNumber ();
    inline uint16_t GetMessageSequenceNumber ();

/*------------------------------------------------------------------------------------------*/
    //以下参数需要根据实际运行情况调整
    bool m_jqueuetag[49];
    int m_JuncNum=49;
    int m_rsujid = 45;
    double startTime = 5;
    double OutsightTransRange = 100;
    double RoadLength = 500;
    double turnLightRange = 50;
    double PositionCheckThreshold = 11;
    double RoadWidth = 10;
    double JunAreaRadius = 50;
                            
    std::string confile = "scratch/conf.txt";
/*------------------------------------------------------------------------------------------*/


    int m_id = -1;
    int vnum = 0;
    int m_turn = -1;   //朝向路口
    int m_nextJID = -1;
    int m_currentJID = -1;
    int m_direction = -1;
    bool m_JunAreaTag = false;
    float ** Graph;
    double m_speed = 1;
    double RSSIDistanceThreshold = 0;
    double m_last_x = 0, m_last_y = 0;
    double InsightTransRange = -1;
    double CarryTimeThreshold = -1;
    
    
    QMap m_wTimeCache;
    std::queue<int> m_jqueue;
    std::queue<int> m_trailTrace;
    std::vector<VTrace> m_tracelist;
    std::vector<SendingQueue> m_squeue;
    std::vector<PacketQueueEntry> m_pqueue;		
    std::vector<PacketQueueEntry> m_pwaitqueue;	
    std::vector<DelayPacketQueueEntry> m_delayqueue;	
    std::vector<DigitalMapEntry> m_map;
    //template <typename T>

/*************************配置信息***************************************/
     double a1=0.333;
     double a2=0.333;
     double a3=0.333;
     double Ncon=6;
     double T=0.0003;
     double Tmax=0.0006;
     double C=2*Tmax;
     double b1=0.5;
     double b2=0.5;


/***************************cp包信息*********************************************/

    int cp_currentJID=-1;
    int cp_nextJID=-1;
    int lifetime=INT_MAX; //生存时间
    // int NTOTAL=0;  //总车辆数
    // int NH=0;       //总跳数
    std::vector<std::vector<int>> scores(int TN_J,std::vector<int>(int TN_J));  //路段得分
    
/*------------------------------------------------------------------------------------------*/
    //从配置文件读取实验运行参数
    void ReadConfiguration();

/*------------------------------------------------------------------------------------------*/
    //根据车辆的IPv4地址获取其对应的车辆编号m_id
    int AddrToID(Ipv4Address);

/*------------------------------------------------------------------------------------------*/
    //初始化车辆的编号m_id和起始位置
    void InitialMID();
    void InitialPosition();

/*------------------------------------------------------------------------------------------*/
    //获取与当前车辆距离最近的路口的路口编号
    int GetNearestJID();
    //获取车辆当前的方向信息，方向值0,1,2,3分别对应东北西南
    int GetDirection(int currentJID, int nextJID);
    //获取车辆的坐标信息
    Vector GetPosition(Ipv4Address adr); 
    //计算链接持续时间
    double VPC();

    //计算道路得分
    double RSE(const grp::MessageHeader &msg);
/*------------------------------------------------------------------------------------------*/
//设置协议所需的定时器
    //用以定时发送Beacon
    void HelloTimerExpire ();
    //用以周期性计算当前车辆的实时速度
    void SpeedCheckExpire();
    //用以判断车辆当前在路网中的状态，是位于路段中还是位于路口范围内
    void CheckPositionExpire();
    //用以定期清理邻居表中的过期信息
    void NeiTableCheckExpire(Ipv4Address addr);

/*------------------------------------------------------------------------------------------*/
//路由方法实现，分为路段间路由和路段内路由
    //路段间路由，为数据包挑选合适的下一个传输路段
    int GetPacketNextJID(bool tag);
    //路段内路由，数据包在路段内传播时的路由方法，即如何再路段内挑选数据包的下一跳  
    Ipv4Address IntraPathRouting(Ipv4Address dest, int dstjid);

    Ipv4Address NextHop(Ipv4Address dest, int dstjid);

    int GetNextJID(bool tag);

    //路段间路由采用迪杰斯特拉最短路径算法计算最优的下一路由路段
    int DijkstraAlgorithm(int srcjid, int dstjid);
    //用以在迪杰斯特拉算法中确定某两个路口是否是相邻
    bool isAdjacentVex(int sjid, int ejid);
    //用以确认邻居车辆是否位于两个指定路口所形成的矩形区域内
    bool isBetweenSegment(double nx, double ny, int cjid, int djid);

/*------------------------------------------------------------------------------------------*/
    //收到控制包时的处理逻辑，控制包包括HelloMessage，即Beacon
    void RecvGrp (Ptr<Socket> socket);

    //CP机制处理逻辑
    void SendCP ();
    void ProcessCP (const grp::MessageHeader &msg, const Ipv4Address receiverIfaceAddr, const Ipv4Address senderIface);
    
    //Beacon机制处理逻辑
    void SendHello ();
    void ProcessHello (const grp::MessageHeader &msg, const Ipv4Address receiverIfaceAddr, const Ipv4Address senderIface);
    
    //Beacon机制的具体发送机制
    void SendPacket (Ptr<Packet> packet);
    void QueueMessage (const grp::MessageHeader &message, Time delay);
    void SendQueuedMessages ();
/*------------------------------------------------------------------------------------------*/
//数据包的暂缓发送机制
    //在Carry-And-Forward机制中用以检查车辆是否携带有被暂存的数据包
    void CheckPacketQueue();
    //在Carry-And-Forward机制中用以发送车辆携带的数据包
    void SendFromSQueue();
    //当发现数据包回传时，即可能出现环时，暂缓一段时间再发送，避免TTL快速消耗
    void SendFromDelayQueue();

/*------------------------------------------------------------------------------------------*/
// From Ipv4RoutingProtocol
    //节点本身有数据包要发送时，使用RouteOutput对数据包进行路由
    virtual Ptr<Ipv4Route> RouteOutput (Ptr<Packet> p,
                                        const Ipv4Header &header,
                                        Ptr<NetDevice> oif,
                                        Socket::SocketErrno &sockerr);
    //节点收到一个来自其他节点的数据包时，使用RouteInput对数据包进行路由
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

/*------------------------------------------------------------------------------------------*/
    //程序运行结束后务必回收垃圾
    void DoDispose ();

};

}
}

#endif /* GRP_H */

