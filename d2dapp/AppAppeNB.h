//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_AppAppeNB_H_
#define _LTE_AppAppeNB_H_

#include <string.h>
#include <omnetpp.h>

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3AddressResolver.h"

// Receiver
#include <list>

//////////////////////////////////////////////////////////
#include <map>
#include <unordered_map>
#include <fstream>
#include "d2dvoip/MSGHH_m.h"

/////////////////////////////////////////////////////////////////
#include "d2dvoip/topu.h"
struct car_struct
{
    int dataId;
    bool idRec;
    double time;
    car_struct(int d, bool b, double t) : dataId(d), idRec(b), time(t) { }
//    bool operator<(car_struct& cs)
//    {
//        this->time < cs.time;
//    }
};

class AppAppeNB : public cSimpleModule
{
    inet::UDPSocket socket;

    cMessage* selfSource_;
    cMessage *selfSender_;
    cMessage *initTraffic_;

    int localPort_;
    int destPort_;

    void initTraffic();
    void sendMSGHH();

    virtual void finish();

    ////////////////////////////////////////////////////////////////
    enum MyMsgKinds
    {
        qunfa = 11,
        shangbao,
        xiafa,
        senddirect
    };

    std::ofstream file_out_send;
    std::ofstream file_out_handle;

    inet::L3Address localIp;
    cModule* hostpp;
    const char * host_name;
    int print_func;
    const char * mbcd;

    std::vector<inet::L3Address> destAddresses;
    std::set<int> destIdx;
    simtime_t startTime;
    simtime_t stopTime;
    double sendInterval;
    const char *packetName = nullptr;
    int numSent = 0;
    int numReceived = 0;

    ///////////////////////////////////////////////////////////
//    std::map<int, std::vector<car_struct>> car_list;
    std::vector<std::vector<std::vector<car_struct>>> car_list_with_time;
    std::map<int, std::set<std::pair<int, int>>> car_map;

    std::vector<double> car_start;

    int node_n, top, k_seed, max_send, gap_n, numXiafa;
    double top_interval;

  public:
    ~AppAppeNB();
    AppAppeNB();

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    void initialize(int stage);
    void handleMessage(cMessage *msg);

    // Receiver
    double eModel(simtime_t delay, double loss);
    void playout(bool finish);

    ///////////////////////////////////////////////////////////////////
    void xiafaMsg();
    void chuliMsg(cMessage* msg);
    void chuliNode();
    int chuliPath_match(MSGHH *payload);
    int chuliPath(MSGHH *payload, char ch);

    std::vector<inet::L3Address> updateDestAddrs();
    std::set<int> updateDestIdxs();

    inet::L3Address getIpByName(std::string name);
    inet::L3Address getIpByIndex(std::string name, int idx);
    inet::L3Address getIpById(int id);
    int getIndexById(int id);
    int getIdByIndex(int idx);
};

#endif

