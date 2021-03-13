//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_AppAppHA_H_
#define _LTE_AppAppHA_H_

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

struct nei_struct
{
    int dataId;
    bool idRec;
    double time;
    nei_struct() = default;
    nei_struct(int i, int j, double t) : dataId(i), idRec(j), time(t) { }
};

class AppAppHA : public cSimpleModule
{
    inet::UDPSocket socket;

    cMessage* selfSource_;
    cMessage *selfSender_;
    cMessage *initTraffic_;

    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    void initTraffic();
    void sendMSGHH();

    virtual void finish();

    ////////////////////////////////////////////////////////////////
    enum MyMsgKinds
    {
        qunfa = 11,
        shangbao,
        xiafa,
        sentdirect,
        re_qunfa
    };

    std::ofstream file_out_send;
    std::ofstream file_out_handle;

    inet::L3Address localIp;
    int localIdx;
    cModule* hostpp;
    const char * host_name;
    const char * file_pre;
    int print_func;
    const char * mbcd;

    std::unordered_map<int, std::vector<nei_struct>> nei_list;

    int dc, node_n;

    std::vector<inet::L3Address> destAddresses;
    std::set<int> destIdx;
    simtime_t startTime;
    simtime_t stopTime;
    double sendInterval;
    const char *packetName = nullptr;
    int numSent = 0;
    int numReceived = 0;

    int numQunfa = 0;
    int numShangbao = 0;

  public:
    ~AppAppHA();
    AppAppHA();

  protected:

    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    void initialize(int stage);
    void handleMessage(cMessage *msg);

    ///////////////////////////////////////////////////////////////////
    void qunfaMsg();
    void shangbaoMsg();
    void chuliMsg(cMessage* msg);

    std::vector<inet::L3Address> updateDestAddrs();
    std::set<int> updateDestIdxs();

    inet::L3Address getIpByName(std::string name);
    inet::L3Address getIpByIndex(std::string name, int idx);
    inet::L3Address getIpById(int id);
    int getIndexById(int id);
    int getIdByIndex(int idx);
};

#endif

