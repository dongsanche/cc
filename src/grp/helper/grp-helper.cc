/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "grp-helper.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/grp.h"

#include "ns3/callback.h"
#include "ns3/udp-l4-protocol.h"


namespace ns3 {

GrpHelper::GrpHelper ()
{
  m_agentFactory.SetTypeId ("ns3::grp::RoutingProtocol");
}

GrpHelper::GrpHelper (const GrpHelper &o)
  : m_agentFactory (o.m_agentFactory)
{
}

GrpHelper*
GrpHelper::Copy (void) const
{
  return new GrpHelper (*this);
}

Ptr<Ipv4RoutingProtocol>
GrpHelper::Create (Ptr<Node> node) const
{
  Ptr<grp::RoutingProtocol> agent = m_agentFactory.Create<grp::RoutingProtocol> ();
  node->AggregateObject (agent);

  return agent;
}

void GrpHelper::Install (NodeContainer c) const
{
  // NodeContainer c = NodeContainer::GetGlobal ();
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = (*i);
      Ptr<UdpL4Protocol> udp = node->GetObject<UdpL4Protocol> ();
      Ptr<grp::RoutingProtocol> grp = node->GetObject<grp::RoutingProtocol> ();
      grp->SetDownTarget (udp->GetDownTarget ());
      udp->SetDownTarget (MakeCallback(&grp::RoutingProtocol::AddHeader, grp));
    }


}




void
GrpHelper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}

}

