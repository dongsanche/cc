/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "myserver-helper.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/myserver.h"

#include "ns3/callback.h"
#include "ns3/udp-l4-protocol.h"


namespace ns3 {

MyServerHelper::MyServerHelper ()
{
  m_agentFactory.SetTypeId ("ns3::myserver::RoutingProtocol");
}

MyServerHelper::MyServerHelper (const MyServerHelper &o)
  : m_agentFactory (o.m_agentFactory)
{
}

MyServerHelper*
MyServerHelper::Copy (void) const
{
  return new MyServerHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
MyServerHelper::Create (Ptr<Node> node) const
{
  Ptr<myserver::RoutingProtocol> agent = m_agentFactory.Create<myserver::RoutingProtocol> ();
  node->AggregateObject (agent);

  return agent;
}

void MyServerHelper::Install (NodeContainer c) const
{
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = (*i);
      Ptr<UdpL4Protocol> udp = node->GetObject<UdpL4Protocol> ();
      Ptr<myserver::RoutingProtocol> myserver = node->GetObject<myserver::RoutingProtocol> ();
      myserver->SetDownTarget (udp->GetDownTarget ());
      udp->SetDownTarget (MakeCallback(&myserver::RoutingProtocol::AddHeader, myserver));
    }


}




void
MyServerHelper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}



}

