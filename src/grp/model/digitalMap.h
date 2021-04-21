#ifndef DIGITALMAP_H
#define DIGITALMAP_H
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "ns3/core-module.h"
#include <queue>
#include "ns3/node.h"
#include "ns3/node-list.h"


namespace ns3 {

struct VTrace
{
	float x;
	float y;
	std::vector<int> jlist;
};


struct DigitalMapEntry
{
	float x;
	float y;
	int edgeNum;
	std::map<int, std::vector<float>> outedge;
};

struct EdgeListEntry
{
	int a;
	int b;
};

class DigitalMap
{
public:
	DigitalMap();
	virtual ~DigitalMap();

	void setMapFilePath(std::string path);
	void readMapFromCsv(std::vector<DigitalMapEntry>& m_map);

	void readTraceFromCsv(std::string path, double xn, double yn, int vid);

	void readTraceCsv(std::string path, std::vector<VTrace>& tracelist);


private:
	std::string m_path;

};



}
#endif
