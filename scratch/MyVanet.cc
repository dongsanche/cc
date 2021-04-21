#include <fstream>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/applications-module.h"
#include "ns3/itu-r-1411-los-propagation-loss-model.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/config-store-module.h"
#include "ns3/integer.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/grp-helper.h"
#include "ns3/myserver-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/digitalMap.h"
#include "ns3/wimax-module.h"
#include "ns3/csma-module.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyFirstNS3");

int recount = 0;
int DropCount = 0;
int SendCount = 0;
int StoreCount = 0;
int OutErrCount = 0;
int MaxPacketsNumber = 0;
int SimulationStopTime = 0;
double DistanceRange = 0;
int64_t allTime = 0;
int nNodes = 0;
int pcount[100] = {0};

struct PacketLog
{
	int src;
	int dst;
	int64_t time;

	PacketLog(int src, int dst, int64_t time)
	{
		this->src = src;
		this->dst = dst;
		this->time = time;
	}

	PacketLog(const PacketLog& plog)
	{
		*this = plog;
	}

	bool operator< (const PacketLog& plog) const
	{
		if(this->src < plog.src)
			return true;
		else if(this->src == plog.src)
		{
			if(this->dst < plog.dst)
				return true;
			else if(this->dst == plog.dst)
			{
				if(this->time < plog.time)
					return true;
				else
					return false;
			}
			else
				return false;
		}
		else
			return false;
	}

};

std::map<PacketLog, Time> PLog;

//每0.1s随机挑选一台车辆发送一个数据包到Sink
void SendRandomPacket (int n)
{
	int sinkID = -1, srcID = -1;
	int m_packetSize = 1024;
	int sinkPort = 8080;
	int64_t nano = 0;
	int64_t onano = 0;

	double xs,ys,xd,yd,dis;

	sinkID = rand()%n;
	xd = NodeList().GetNode(sinkID)->GetObject<MobilityModel>()->GetPosition().x;
	yd = NodeList().GetNode(sinkID)->GetObject<MobilityModel>()->GetPosition().y;
	Address SinkAddress = InetSocketAddress(NodeList().GetNode(sinkID)->GetObject<Ipv4>()->GetAddress (1, 0).GetLocal (), sinkPort);

	int tag = -1;
	int tryNum = 0;
	while(tag < 0)
	{
		if(tryNum >= 100)
		{
			break;
		}
		tryNum ++;
		srcID = rand()%n;
		xs = NodeList().GetNode(srcID)->GetObject<MobilityModel>()->GetPosition().x;
		ys = NodeList().GetNode(srcID)->GetObject<MobilityModel>()->GetPosition().y;

		dis = sqrt(pow(xd-xs,2)+pow(yd-ys,2));
		if(dis >= DistanceRange - 200 && dis <= DistanceRange + 200)
		{
			Ptr<Socket> SendSocket = Socket::CreateSocket (NodeList().GetNode(srcID), UdpSocketFactory::GetTypeId ());

			SendSocket->Bind ();
			SendSocket->Connect (SinkAddress);

			uint8_t * buffer = new uint8_t [m_packetSize];
			onano = nano = Simulator::Now().GetNanoSeconds();
			// NS_LOG_UNCOND("" << nano << " Src: " << srcID << " (" << xs << "," << ys << ") " << " sends a packet to Sink: " << sinkID << " (" << xd << "," << yd << ")");
			for(int i = 0; i<8; i++)
			{
				int64_t tmp = nano >> 8;
				int64_t tmp1 = tmp << 8;
				uint8_t val = (uint8_t)(nano - tmp1);
				nano = tmp;
				buffer[i] = val;
			}

			uint16_t sid = (uint16_t)srcID;
			uint16_t tmp = sid >> 8;
			buffer[8] = (uint8_t)(tmp);
			buffer[9] = (uint8_t)(sid - (tmp << 8));

			Ptr<Packet> packet = Create<Packet> (buffer, m_packetSize);
			tag = SendSocket->Send (packet);

			SendSocket->Close();
		}

	}
	if(tryNum == 100)
		NS_LOG_UNCOND("Inappropriate sink node");
	else
	{
		PacketLog log(srcID, sinkID, onano);
		Time &ltime = PLog[log];
		ltime = Simulator::Now();
		SendCount++;
	}
	if(SendCount < MaxPacketsNumber)
		Simulator::Schedule(Seconds(0.1), &SendRandomPacket, n);
}

//每0.1s在Sink的指定范围内随机挑选一台车辆发送一个数据包到Sink
void SendTestPacketToLC_DIS()
{
	int sinkID = -1, srcID = -1;
	int m_packetSize = 1024;
	int sinkPort = 8080;
	int64_t nano = 0;
	int64_t onano = 0;

	sinkID = nNodes;
	Address SinkAddress = InetSocketAddress(NodeList().GetNode(sinkID)->GetObject<Ipv4>()->GetAddress (1, 0).GetLocal (), sinkPort);

	// srcID = srclist[srcidx++];
    double dx = 1900;
    double dy = 600;
    double vx = 0, vy = 0, dis = DistanceRange;
    while(dis >= DistanceRange)
    {
        srcID = rand() % nNodes;
        vx = NodeList().GetNode(srcID)->GetObject<MobilityModel>()->GetPosition().x;
        vy = NodeList().GetNode(srcID)->GetObject<MobilityModel>()->GetPosition().y;
        dis = fabs(vx - dx) + fabs(vy - dy);
    }


	Ptr<Socket> SendSocket = Socket::CreateSocket (NodeList().GetNode(srcID), UdpSocketFactory::GetTypeId ());

	SendSocket->Bind ();
	SendSocket->Connect (SinkAddress);

	uint8_t * buffer = new uint8_t [m_packetSize];
	onano = nano = Simulator::Now().GetNanoSeconds();
	// NS_LOG_UNCOND("" << nano << " Src: " << srcID << " sends a packet to Sink: " << sinkID);
	for(int i = 0; i<8; i++)
	{
		int64_t tmp = nano >> 8;
		int64_t tmp1 = tmp << 8;
		uint8_t val = (uint8_t)(nano - tmp1);
		nano = tmp;
		buffer[i] = val;
	}

	uint16_t sid = (uint16_t)srcID;
	uint16_t tmp = sid >> 8;
	buffer[8] = (uint8_t)(tmp);
	buffer[9] = (uint8_t)(sid - (tmp << 8));

	Ptr<Packet> packet = Create<Packet> (buffer, m_packetSize);
	SendSocket->Send (packet);

	SendSocket->Close();

	PacketLog log(srcID, sinkID, onano);
	Time &ltime = PLog[log];
	ltime = Simulator::Now();
	SendCount++;

	if(SendCount < MaxPacketsNumber)
		Simulator::Schedule(Seconds(0.1), &SendTestPacketToLC_DIS);
}

//从指定的源节点发送一个数据包到指定的目标节点
void SendSpecificPacket (int srcID, int sinkID)
{
	int m_packetSize = 1024;
	int sinkPort = 8080;
	SendCount++;

	Address SinkAddress = InetSocketAddress(NodeList().GetNode(sinkID)->GetObject<Ipv4>()->GetAddress (1, 0).GetLocal (), sinkPort);

	Ptr<Socket> SendSocket = Socket::CreateSocket (NodeList().GetNode(srcID), UdpSocketFactory::GetTypeId ());

	SendSocket->Bind ();
	SendSocket->Connect (SinkAddress);

	uint8_t * buffer = new uint8_t [m_packetSize];
	int64_t nano = Simulator::Now().GetNanoSeconds();
	int64_t onano = nano;
	// NS_LOG_UNCOND("" << nano << " Src: " << srcID << " sends a packet to Sink: " << sinkID);
	for(int i = 0; i<8; i++)
	{
		int64_t tmp = nano >> 8;
		int64_t tmp1 = tmp << 8;
		uint8_t val = (uint8_t)(nano - tmp1);
		nano = tmp;
		buffer[i] = val;
	}

	uint16_t sid = (uint16_t)srcID;
	uint16_t tmp = sid >> 8;
	buffer[8] = (uint8_t)(tmp);
	buffer[9] = (uint8_t)(sid - (tmp << 8));

	Ptr<Packet> packet = Create<Packet> (buffer, m_packetSize);

	SendSocket->Send (packet);

	SendSocket->Close();

	PacketLog log(srcID, sinkID, onano);
	Time &ltime = PLog[log];
	ltime = Simulator::Now();
}

//将IPv4地址转换为对应的ID编号
int AddrToID(Ipv4Address addr)
{
	int tnum = addr.Get();
	return tnum / 256 % 256 * 256 + tnum % 256 - 1;
}

//接收到数据包的处理过程
void ReceivePacket (Ptr<Socket> socket)
{
	Ptr<Packet> packet;
	Address srcAddress;
	while ((packet = socket->RecvFrom (srcAddress)))
	{
		std::ostringstream oss;
		int64_t now = Simulator::Now ().GetNanoSeconds();
		oss << now << " " << socket->GetNode ()->GetId ();
		if (InetSocketAddress::IsMatchingType (srcAddress))
		{
			uint8_t buffer[10];
			packet->CopyData(buffer, 10);
			int64_t nano = 0;
			uint16_t sid = 0;
			for(int i = 7; i>=0; i--)
			{
				nano = nano << 8;
				nano += buffer[i];
			}

			sid += buffer[8];
			sid = sid << 8;
			sid += buffer[9];

            double delay = (now - nano) / 1000000.0;
            int idx = ceil(delay / 1000);
            if(idx >= 99)
                pcount[9]++;
            else
                pcount[idx]++;

			allTime += now - nano;

			InetSocketAddress addr = InetSocketAddress::ConvertFrom (srcAddress);
		    oss << " received one packet from " << AddrToID(addr.GetIpv4 ());
		    recount++;


		    int tsrc = (int)sid;
		    int tdst = socket->GetNode ()->GetId ();
		    PacketLog tlg(tsrc, tdst, nano);
		    std::map<PacketLog, Time>::iterator pitr = PLog.find(tlg);
		    if(pitr != PLog.end())
		    	PLog.erase(pitr);

		}
		else
		{
		    oss << " received one packet!";
		}
	    NS_LOG_UNCOND (oss.str());
	}
}

void
DropPacket (Ptr<OutputStreamWrapper> stream, std::string context, const Ipv4Header &header )
{
	NS_LOG_UNCOND ("d " << Simulator::Now ().GetSeconds () << " " << context << " "
			<< AddrToID(header.GetSource()) << " > " << AddrToID(header.GetDestination()) << " "
			<< "id " << header.GetIdentification());
	*stream->GetStream () << "d " << Simulator::Now ().GetSeconds () << " " << context << " "
			<< header.GetSource() << " > " << header.GetDestination() << " "
			<< "id " << header.GetIdentification()
			<< std::endl;
	DropCount++;
}

void
StorePacket (Ptr<OutputStreamWrapper> stream, std::string context, const Ipv4Header &header)
{
 	StoreCount ++;
 	NS_LOG_UNCOND ("s " << Simulator::Now ().GetSeconds () << " " << context << " "
 				<< header.GetSource() << " > " << header.GetDestination() << " "
 				<< "id " << header.GetIdentification());
 	*stream->GetStream () << "s " << Simulator::Now ().GetSeconds () << " " << context << " "
 				<< header.GetSource() << " > " << header.GetDestination() << " "
 				<< "id " << header.GetIdentification()
 				<< std::endl;

}

int idx = -1;
int hops = 0;
double CarryTimeThreshold = 0;
double range = -1;
void ReadConfiguration()
{
    std::ifstream file("scratch/conf.txt");
	std::string line;
    while(!file.eof())
	{
		std::getline(file, line);

		std::istringstream iss(line);
		std::string temp;
 
		while (std::getline(iss, temp, '='))
		{
            std::string value = std::move(temp);

            if(value == "idx")
            {
                std::getline(iss, temp, ',');
				value = std::move(temp);
                idx = atof(value.c_str());
            }
            else if(value == "vnum")
            {
                std::getline(iss, temp, ',');
				value = std::move(temp);
                nNodes = atof(value.c_str());
            }
            else if(value == "range")
            {
                std::getline(iss, temp, ',');
				value = std::move(temp);
                range = atof(value.c_str());
            }
			else if(value == "seghop")
            {
                std::getline(iss, temp, ',');
				value = std::move(temp);
                hops = atof(value.c_str());
            }
            else if(value == "CarryTimeThreshold")
            {
                std::getline(iss, temp, ',');
				value = std::move(temp);
                CarryTimeThreshold = atof(value.c_str());
            }
            else if(value == "DistanceRange")
            {
                std::getline(iss, temp, ',');
				value = std::move(temp);
                DistanceRange = atof(value.c_str());
            }
        }
        

    }
}

int main (int argc, char *argv[])
{
	CommandLine cmd;
	cmd.Parse (argc, argv);

    //从配置文件中读取网络实验参数
    ReadConfiguration();
    //配置随机参数种子
    srand((unsigned int)(idx*10));
	
/* ------------------------------------ 节点配置，包括：物理层、MAC层、网络层、运输层和应用层-----------------------------*/

/*---------------------------------创建节点----------------------------------*/
    //Node Creation
    NodeContainer Vehicles;
    Vehicles.Create(nNodes);

	//Local Server
	NodeContainer Server;
	Server.Create(1);

	NodeContainer Nodes;
	Nodes.Add(Vehicles);
	Nodes.Add(Server);

/*---------------------------------为节点配置移动性----------------------------------*/
    //根据配置参数设置车辆数据
    std::string m_traceFile = "TestScenaries/" + std::to_string(nNodes) + "/6x6_" + std::to_string(nNodes) + ".tcl";
    std::string mapfile = "TestScenaries/" + std::to_string(nNodes) + "/6x6_map.csv";
    
    //设置Sink的位置
    Vector ServerPos(1900, 600, 0);

    //configure vehicle's mobility
    Ns2MobilityHelper ns2 = Ns2MobilityHelper (m_traceFile);
    ns2.Install(NodeList::Begin (), NodeList::End ()-1);

	//configure position of the server
	Ptr<ConstantPositionMobilityModel> ServerPosition = CreateObject<ConstantPositionMobilityModel> ();
	ServerPosition->SetPosition (ServerPos);
	Server.Get (0)->AggregateObject (ServerPosition);

/*---------------------------------为节点配置物理层----------------------------------*/
	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss ("ns3::TwoRayGroundPropagationLossModel",
									"SystemLoss", DoubleValue(1),
									"HeightAboveZ", DoubleValue(1.5));

	YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
	wifiPhy.SetChannel (wifiChannel.Create ());
	wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);

	if (range == 500)
	{
		wifiPhy.Set ("RxSensitivity", DoubleValue(-68));
		wifiPhy.Set ("CcaEdThreshold", DoubleValue(-71));
	}
	else if (range == 250){
		wifiPhy.Set ("RxSensitivity", DoubleValue(-61.8));
		wifiPhy.Set ("CcaEdThreshold", DoubleValue(-64.8));
	}

	// Values for typical VANET scenarios according to 802.11p
	wifiPhy.Set ("TxPowerStart", DoubleValue(33));
	wifiPhy.Set ("TxPowerEnd", DoubleValue(33));
	wifiPhy.Set ("TxPowerLevels", UintegerValue(1));
	wifiPhy.Set ("TxGain", DoubleValue(0));
	wifiPhy.Set ("RxGain", DoubleValue(0));

/*---------------------------------为节点配置MAC层----------------------------------*/
	NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
	Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
	wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",StringValue ("OfdmRate6MbpsBW10MHz"), "ControlMode",StringValue ("OfdmRate6MbpsBW10MHz"));
	NetDeviceContainer VDevices = wifi80211p.Install (wifiPhy, wifi80211pMac, Nodes);

/*---------------------------------为节点配置网络层----------------------------------*/
	GrpHelper grp;
	Ipv4ListRoutingHelper list;
	InternetStackHelper stack;
	list.Add (grp, 100);
	stack.SetRoutingHelper (list);
	stack.Install (Vehicles);
	grp.Install(Vehicles);

	MyServerHelper myserver;
	Ipv4ListRoutingHelper serverlist;
	InternetStackHelper serverStack;
	serverlist.Add(myserver, 100);
	serverStack.SetRoutingHelper(serverlist);
	serverStack.Install(Server);
	myserver.Install(Server);


	// IP Addressing
	Ipv4AddressHelper address;
	address.SetBase("10.1.0.0", "255.255.0.0");
	Ipv4InterfaceContainer VInterface = address.Assign(VDevices);

/*---------------------------------为节点配置运输层----------------------------------*/
    uint16_t sinkPort = 8080;
    for(uint32_t i = 0; i < Nodes.GetN(); i++)
    {
		Ptr<Socket> recvSink = Socket::CreateSocket(Nodes.Get (i), TypeId::LookupByName ("ns3::UdpSocketFactory"));
		InetSocketAddress sinkAddress = InetSocketAddress(VInterface.GetAddress (i), sinkPort);
		recvSink->Bind(sinkAddress);
		recvSink->SetRecvCallback(MakeCallback (&ReceivePacket));
    }

/*---------------------------------为节点配置应用层----------------------------------*/
    //设置数据包发送数量
    MaxPacketsNumber = 1000;
    //设置仿真时间
    SimulationStopTime = 160;
    //在指定时间指定发送节点向指定目标节点发送一个数据包，用以测试算法正确性
	// Simulator::Schedule(Seconds(27.5), &SendSpecificPacket, 179, nNodes);	
    //大规模发包测试，指定传输开始时间，具体的发送方式可以只有指定，当前文件前面定义呢多个测试函数，见上	
    Simulator::Schedule(Seconds(20), &SendTestPacketToLC_DIS);     							

/* ----------------------------------------------配置数据记录---------------------------------------------------*/

    //记录网络运行数据到tr文件并实时打印在屏幕上
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("scratch/grp-trace.tr");
    Config::Connect("/NodeList/*/$ns3::grp::RoutingProtocol/DropPacket", MakeBoundCallback(&DropPacket, stream));
    Config::Connect("/NodeList/*/$ns3::grp::RoutingProtocol/StorePacket", MakeBoundCallback(&StorePacket, stream));

    // 记录网络运行数据，可以使用NetAnim查看这些数据 
    AnimationInterface anim ("scratch/myvanet.xml");

/* ----------------------------------------------仿真的启动与关闭---------------------------------------------------*/
    NS_LOG_UNCOND("Simulation start");

    //启动仿真，仿真结束后销毁仿真程序
    Simulator::Stop(Seconds (SimulationStopTime));
    Simulator::Run ();
    Simulator::Destroy ();

/* ------------------------------------仿真结束后统计和打印网络运行数据--------------------------------------------*/

    //打印未能成功发送的数据包，包括发送时间、源节点和目标节点
    NS_LOG_UNCOND("");
    int lc = 0;
	for(std::map<PacketLog, Time>::iterator itr = PLog.begin(); itr != PLog.end(); itr ++)
	{
		NS_LOG_UNCOND(itr->second.GetSeconds() << " " << itr->first.src << "->" << itr->first.dst);
		lc++;
	}

    //打印统计数据
    NS_LOG_UNCOND("Simulation results");
    NS_LOG_UNCOND("Sent:"<< SendCount << " Received:" << recount 
		<< " Drop:" << DropCount << " delay:" << (double)allTime/recount/1000000 << "ms");
    NS_LOG_UNCOND("Store Error: " << lc - DropCount);

    //将统计数据输出到文件中
    std::ofstream fout("scratch/data.csv", std::ios::app);
	fout << nNodes << "," << DistanceRange << "," << hops << "," << CarryTimeThreshold << ",";
    fout << (recount * 1.0 / SendCount) << "," << (double)allTime/recount/1000000;

    fout << std::endl;
    fout.close();


    return 0;
}