/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef GRP_HELPER_H
#define GRP_HELPER_H

#include "ns3/grp.h"
#include "ns3/object-factory.h"
#include "ns3/node.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-routing-helper.h"
#include <map>
#include <set>

namespace ns3 {

class GrpHelper : public Ipv4RoutingHelper
{
public:
  GrpHelper ();
  GrpHelper (const GrpHelper &);
  GrpHelper* Copy (void) const;
  virtual Ptr<Ipv4RoutingProtocol> Create (Ptr<Node> node) const;
  void Set (std::string name, const AttributeValue &value);
  void Install (NodeContainer c) const;

private:
  GrpHelper &operator = (const GrpHelper &);
  ObjectFactory m_agentFactory;
};

}

#endif /* GRP_HELPER_H */

