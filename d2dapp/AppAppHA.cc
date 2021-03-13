//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <cmath>

#include "AppAppHA.h"

#define round(x) floor((x) + 0.5)

Define_Module(AppAppHA);

AppAppHA::AppAppHA()
{
    selfSource_ = NULL;
    selfSender_ = NULL;
}

AppAppHA::~AppAppHA()
{
    cancelAndDelete(selfSource_);
    cancelAndDelete(selfSender_);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    file_out_send.close();
    file_out_handle.close();
}

void AppAppHA::initialize(int stage)
{
    EV << "VoIP Sender initialize: stage " << stage << endl;

    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    selfSource_ = new cMessage("selfSource");
    selfSender_ = new cMessage("selfSender");
    localPort_ = par("localPort");
    destPort_ = par("destPort");

    EV << "VoIPR::initialize - binding to port: local:" << localPort_ << endl;
    if (localPort_ != -1)
    {
        socket.setOutputGate(gate("udpOut"));
        socket.bind(localPort_);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    numSent = 0;
    numQunfa = 0;
    numShangbao = 0;
    dc = par("dc").intValue();
    file_pre = par("file_pre");
    mbcd = par("mbcd");

    host_name = par("host_name");
    print_func = par("print_func").intValue();
    node_n = par("node_n").intValue();
    startTime = par("startTime").doubleValue();
    stopTime = par("stopTime").doubleValue();
    sendInterval = par("sendInterval").doubleValue();
    packetName = par("packetName");

    char file_send[50], file_handle[50];
    sprintf(file_send, "../file_out/send_%s%s%s.txt", getParentModule()->getFullName(), mbcd, file_pre);
    sprintf(file_handle, "../file_out/handle_%s%s%s.txt", getParentModule()->getFullName(), mbcd, file_pre);
    file_out_send.open(file_send, std::ios::out);
    file_out_handle.open(file_handle, std::ios::out);
    file_out_send << "file_out_send: " << simTime().str() << endl;
    file_out_handle << "file_out_handle: " << simTime().str() << endl;

    hostpp = getParentModule()->getParentModule();
    localIp = getIpByName(getParentModule()->getFullName());
    localIdx = getIndexById(getParentModule()->getId());


    initTraffic_ = new cMessage("initTraffic");
    initTraffic();

    file_out_send << "destAddresses.size(): " << destAddresses.size() << endl;
    for(int i=0;i<destAddresses.size();++i)
    {
        file_out_send << i << " destAddresses: " << destAddresses[i] << endl;
    }
}

void AppAppHA::initTraffic()
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    destAddresses = updateDestAddrs();
    destIdx = updateDestIdxs();
    if(destAddresses.size() == 0)
    {
        // this might happen when users are created dynamically
        EV << simTime() << "AppAppHA::initTraffic - destination " << destAddresses.size() << " not found" << endl;

        scheduleAt(simTime() + 0.1, initTraffic_);
    }
    else
    {
        delete initTraffic_;

        // calculating traffic starting time
        simtime_t startTime = par("startTime");

        // this conversion is made in order to obtain ms-aligned start time, even in case of random generated ones
        simtime_t offset = (round(SIMTIME_DBL(startTime) * 1000) / 1000);

        scheduleAt(simTime() + startTime, selfSender_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void AppAppHA::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))
            sendMSGHH();
        else if (!strcmp(msg->getName(), "initTraffic"))
            initTraffic();
    }

    // Receiver
    ///////////////////////////////////////////////////////////////////////////
    else if (msg->getKind() == inet::UDP_I_DATA)
    {
        chuliMsg(msg);
    }
    else if (msg->getKind() == inet::UDP_I_ERROR)
    {
        EV_WARN << "Ignoring UDP error report\n";
        delete msg;
        return ;
    }
    else
    {
        throw cRuntimeError("Unrecognized message (%s)%s", msg->getClassName(), msg->getFullName());
        return ;
    }
}

void AppAppHA::sendMSGHH()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(numSent % 2 == 0)
    {
        if(numSent % 8)
            qunfaMsg();
        else
            shangbaoMsg();
    }
    ++numSent;

    scheduleAt(simTime() + sendInterval, selfSender_);
}

// Receiver
void AppAppHA::finish()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AppAppHA::qunfaMsg()
{
    if(print_func)
        file_out_send << "start qunfaMsg()" << endl;
//  Generate broadcast message
    std::ostringstream str;
    str << "AppData-" << numQunfa;
    MSGHH *payload = new MSGHH(str.str().c_str());
//  setting message attributes
    payload->setByteLength(par("messageLength").intValue());
    payload->setType(qunfa);
    payload->setDataId(numQunfa + 1000);

    payload->setPosx(std::stof(getParentModule()->getDisplayString().getTagArg("p", 0)));
    payload->setPosy(std::stof(getParentModule()->getDisplayString().getTagArg("p", 1)));

    payload->setSrcIdx(localIdx);
    payload->setSrcAddrs(localIp.str().c_str());
    payload->setSrcHostName(getParentModule()->getFullName());
//  broadcast message
    destAddresses = updateDestAddrs();
    destIdx = updateDestIdxs();
    file_out_send << "destAddresses.size(): " << destAddresses.size() << endl;
//    for (int i = 0; i < destAddresses.size(); ++i)
//    {
//        if(localIp == destAddresses[i])
//            continue;
//        {
//            file_out_send
//                << "qunfa dataId: " << payload->getDataId()
//                << ", Name " << getParentModule()->getFullName()
//                << ", src: " << localIp.str()
//                << ", dest: " << destAddresses[i].str()
//                << ", time: " << simTime().str()
//                << endl;
//        }
////        MSGHH *payload_copy = payload->dup();
//        socket.sendTo(payload->dup(), destAddresses[i], destPort_);
//    }

    payload->setType(sentdirect);
    for (auto i = destIdx.begin(); i != destIdx.end(); ++i)
    {
        if(*i == localIdx)
            continue;
        {
            file_out_send
                << "sentdirect dataId: " << payload->getDataId()
                << ", Name " << getParentModule()->getFullName()
                << ", src: " << localIp.str()
//                << ", dest: " << destAddresses[i].str()
                << ", dest: " << "car[" + std::to_string(*i) + "]"
                << ", time: " << simTime().str()
                << endl;
        }
        cModule * pp_sub = hostpp->getSubmodule(host_name, *i);
        if(pp_sub != nullptr)
        {
            sendDirect(payload->dup(), pp_sub->getSubmodule("udpApp", 0)->gate("hostIn"));
        }
    }

    ++numQunfa;
//    emit(sentPkSignal, payload);
    delete payload;
    if(print_func)
        file_out_send << "finish qunfaMsg()" << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AppAppHA::shangbaoMsg()
{
    if(print_func)
        file_out_send << "start shangbaoMsg()" << endl;
    inet::L3Address eNBIp = getIpByName("hNB");
    {
        file_out_send
            << "shangbao dataId: " << numShangbao
            << ", Name " << getParentModule()->getFullName()
            << ", src: " << localIp.str()
//            << ", dest: " << eNBIp.str()
            << ", dest: " << "eNB"
            << ", time: " << simTime().str()
            << endl;
    }
// Add neighbor node information to the file
    for(auto nei_it = nei_list.begin(); nei_it != nei_list.end(); ++nei_it)
    {
        int i = 0;
        if(nei_it->second.size() > 20)
            i = nei_it->second.size() - 20;
        for(; i < nei_it->second.size(); ++i)
        {
            file_out_send << "nei: " << nei_it->second[i].dataId << "  " << localIdx << "  " << nei_it->first
                    << "  " << nei_it->second[i].idRec << "  " << nei_it->second[i].time << endl;
        }
        file_out_send << "flag_idb" << endl;
    }
//    nei_list.clear();
    ++numShangbao;
    if(print_func)
        file_out_send << "finish shangbaoMsg()" << endl;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AppAppHA::chuliMsg(cMessage* msg)
{
    if(print_func)
        file_out_send << "start chuliMsg()" << endl;
    MSGHH *mmsg = check_and_cast<MSGHH *>(msg);

// Processing of broadcast messages
    if(mmsg->getType() == qunfa)
    {
        {
            file_out_handle
                << "qunfa dataId: " << mmsg->getDataId()
                << ", Name " << getParentModule()->getFullName()
                << ", src: " << mmsg->getSrcAddrs()
                << ", dest: " << localIp.str()
                << ", time: " << simTime().str()
                << endl;
        }
// Record vehicle information first
        int srcId = mmsg->getSrcIdx();
        if(nei_list.find(srcId) == nei_list.end())
        {
            nei_list[srcId].emplace_back(mmsg->getDataId(), 1, std::stof(simTime().str().c_str()));
        }
        else
        {
            if(nei_list[srcId].back().dataId == mmsg->getDataId() - 1)
                nei_list[srcId].emplace_back(mmsg->getDataId(), 1, std::stof(simTime().str().c_str()));
            else if(nei_list[srcId].back().dataId < mmsg->getDataId())
            {
                double old_time = nei_list[srcId].back().time;
                int old_id = nei_list[srcId].back().dataId;
                double diff_tt = std::stof(simTime().str().c_str()) - old_time;
                double diff_id = mmsg->getDataId() - old_id;
                diff_tt /= diff_id;
                for (int i = 1; i < diff_id; ++i)
                {
                    nei_list[srcId].emplace_back(old_id + i, 0, old_time + diff_tt * i);
                }
                nei_list[srcId].emplace_back(mmsg->getDataId(), 1, std::stof(simTime().str().c_str()));
            }
        }
    }
    else if(mmsg->getType() == sentdirect)
    {
        {
            file_out_handle
                << "sentdirect dataId: " << mmsg->getDataId()
                << ", Name " << getParentModule()->getFullName()
                << ", src: " << mmsg->getSrcAddrs()
                << ", dest: " << localIp.str()
                << ", time: " << simTime().str()
                << endl;
        }
        int ix = std::stof(getParentModule()->getDisplayString().getTagArg("p", 0));
        int iy = std::stof(getParentModule()->getDisplayString().getTagArg("p", 1));
        ix -= mmsg->getPosx();
        iy -= mmsg->getPosy();
        if(ix * ix + iy * iy > dc * dc)
        {
            if(print_func)
                file_out_send << "finish chuliMsg()" << endl;
            return ;
        }
//        file_out_send << "sentdirect add nei." << endl;
// Record vehicle information
        int srcId = mmsg->getSrcIdx();
        if(nei_list.find(srcId) == nei_list.end())
        {
            nei_list[srcId].emplace_back(mmsg->getDataId(), 1, std::stof(simTime().str().c_str()));
        }
        else
        {
            if(nei_list[srcId].back().dataId == mmsg->getDataId() - 1)
                nei_list[srcId].emplace_back(mmsg->getDataId(), 1, std::stof(simTime().str().c_str()));
            else if(nei_list[srcId].back().dataId < mmsg->getDataId())
            {
                double old_time = nei_list[srcId].back().time;
                int old_id = nei_list[srcId].back().dataId;
                double diff_tt = std::stof(simTime().str().c_str()) - old_time;
                double diff_id = mmsg->getDataId() - old_id;
                diff_tt /= diff_id;
                for (int i = 1; i < diff_id; ++i)
                {
                    nei_list[srcId].emplace_back(old_id + i, 0, old_time + diff_tt * i);
                }
                nei_list[srcId].emplace_back(mmsg->getDataId(), 1, std::stof(simTime().str().c_str()));
            }
        }
    }
// Processing of send messages
    else if(mmsg->getType() == xiafa)
    {
        {
            file_out_handle
                << "xiafa dataId: " << mmsg->getDataId()
                << ", Name " << getParentModule()->getFullName()
                << ", src: " << mmsg->getSrcAddrs()
                << ", dest: " << localIp.str()
                << ", time: " << simTime().str()
                << ", Hops: " << mmsg->getHops()
                << endl;
        }
// If data is received, record directly
        {
            file_out_handle << "id: " << localIdx << " recevied " << mmsg->getDataId() << " msg " << simTime().str() << endl;
            for(int i = 0; i < mmsg->getDestPos(); ++i)
                file_out_handle << mmsg->getDestIdxs(i) << " ";
            file_out_handle << endl;
        }
// Find the next vehicle number to spread the message
        int my_pos = 0;
        int sz_t = mmsg->getDestPos();
        for (int i = 0; i < sz_t && mmsg->getDestIdxs(i) != localIdx; ++i)
        {
            if(mmsg->getDestIdxs(i) >= 0)
                ++my_pos;
        }
        int hop = mmsg->getHops();
        int top_n = mmsg->getTop_n();
        while (hop < top_n)
        {
            int pos_idx = mmsg->getDestIdxs(mmsg->getNodeHops(hop) + my_pos);
            file_out_send << "DataId: " << mmsg->getDataId() << "  Hops: " << hop << " Pos: " << mmsg->getNodeHops(hop) + my_pos << endl;
            file_out_send << "payload dest ids: " ;
            for(int i=0; i<mmsg->getDestPos(); ++i)
                file_out_send << mmsg->getDestIdxs(i) << " ";
            file_out_send << simTime().str() << endl;

            mmsg->setHops(++hop);
            if(pos_idx < 0)
                continue ;
// Forward the message to the next vehicle
            if(pos_idx == localIdx)
                continue ;
//            inet::L3Address pos_ip = getIpById(pos_idx);
            inet::L3Address pos_ip = getIpByIndex(host_name, pos_idx);
            file_out_send << "host xiafa: " << mmsg->getDataId() << "  " << localIdx << "  " << pos_idx << "  " << hop << "  " << simTime().str() << endl;
            file_out_send << "host xiafa: " << localIdx << "  " << pos_ip.str() << "  " << hop << "  " << simTime().str() << endl << endl << endl;
            if(pos_ip.str() == "")
            {
                file_out_send << "host xiafa error: no ip." << endl;
                continue ;
            }
//            socket.sendTo(mmsg->dup(), pos_ip, destPort_);
            cModule * pp_sub = hostpp->getSubmodule(host_name, pos_idx);
            if(pp_sub != nullptr)
            {
                sendDirect(mmsg->dup(), pp_sub->getSubmodule("udpApp", 0)->gate("hostIn"));
            }
        }
    }
//    delete msg;
    if(print_func)
        file_out_send << "finish chuliMsg()" << endl;
}

std::vector<inet::L3Address> AppAppHA::updateDestAddrs()
{
    if(print_func)
        file_out_send << "start updateDestAddrs()" << endl;
    std::vector<inet::L3Address> destAddresses;
    SubmoduleIterator sub_it(hostpp);
    std::string name = host_name + std::string("[");
    inet::L3Address res;

    for(; !sub_it.end(); ++sub_it)
    {
        if(strstr((*sub_it)->getFullName(), name.c_str()))
        {
            res = getIpByName((*sub_it)->getFullName());
            if(res == inet::L3Address() || res == localIp)
                continue ;
            else
                destAddresses.push_back(res);
        }
    }
    if(print_func)
        file_out_send << "finish updateDestAddrs()" << endl;
    return destAddresses;
}

std::set<int> AppAppHA::updateDestIdxs()
{
    if(print_func)
        file_out_send << "start updateDestAddrs()" << endl;
    std::set<int> destIdx;
    SubmoduleIterator sub_it(hostpp);
    std::string name = host_name + std::string("[");

    for(; !sub_it.end(); ++sub_it)
    {
        if(strstr((*sub_it)->getFullName(), name.c_str()))
        {
            std::string h_name = (*sub_it)->getFullName();
            h_name.pop_back();
            h_name = h_name.substr(name.size());
            destIdx.insert(std::stoi(h_name));
        }
    }
    if(print_func)
        file_out_send << "finish updateDestAddrs()" << endl;
    return destIdx;
}

inet::L3Address AppAppHA::getIpByName(std::string name)
{
    if(print_func)
        file_out_send << "start getIpByName()" << endl;
    inet::L3Address getIp;
    std::string localHost = hostpp->getFullName();
    localHost += '.';
    localHost += name;
    inet::L3AddressResolver().tryResolve(localHost.c_str(), getIp);
    if(print_func)
        file_out_send << "finish getIpByName()" << endl;
    return getIp;
}

inet::L3Address AppAppHA::getIpByIndex(std::string name, int idx)
{
    if(print_func)
        file_out_send << "start getIpByIndex()" << endl;
    inet::L3Address getIp;
    if(hostpp->getSubmodule(name.c_str(), idx) == nullptr)
    {
        if(print_func)
            file_out_send << "finish getIpByIndex()" << endl;
        return inet::L3Address();
    }
    char localHost[30];
    sprintf(localHost, "%s.%s[%d]", hostpp->getFullName(), name.c_str(), idx);
//    std::string localHost = ;
//    localHost += '.';
//    localHost += name + "[" + std::to_string(idx) + "]";
    inet::L3AddressResolver().tryResolve(localHost, getIp);
    if(print_func)
        file_out_send << "finish getIpByIndex()" << endl;
    return getIp;
}

inet::L3Address AppAppHA::getIpById(int id)
{
    if(print_func)
        file_out_send << "start getIpById()" << endl;
    cModule* cm = getSimulation()->getModule(id);
    if(cm == nullptr)
    {
        if(print_func)
            file_out_send << "finish getIpById()" << endl;
        return inet::L3Address();
    }
    if(print_func)
        file_out_send << "finish getIpById()" << endl;
    return getIpByName(cm->getFullName());
}

int AppAppHA::getIndexById(int id)
{
    if(print_func)
        file_out_send << "start getIndexById()" << endl;
    cModule* cm = getSimulation()->getModule(id);
    if(cm == nullptr)
    {
        if(print_func)
            file_out_send << "finish getIpById()" << endl;
        return -1;
    }
    std::string name = host_name + std::string("[");
    std::string h_name = cm->getFullName();
    h_name.pop_back();
    h_name = h_name.substr(name.size());
    if(print_func)
        file_out_send << "finish getIndexById()" << endl;
    return std::stoi(h_name);
}

int AppAppHA::getIdByIndex(int idx)
{
    return hostpp->findSubmodule(host_name, idx);
}
