//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <cmath>

#include "AppAppeNB.h"

#define round(x) floor((x) + 0.5)

Define_Module(AppAppeNB);

AppAppeNB::AppAppeNB()
{
    selfSource_ = NULL;
    selfSender_ = NULL;
}

AppAppeNB::~AppAppeNB()
{
    cancelAndDelete(selfSource_);
    cancelAndDelete(selfSender_);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    file_out_send.close();
    file_out_handle.close();
}

void AppAppeNB::initialize(int stage)
{
    EV << "VoIP Sender initialize: stage " << stage << endl;
    cSimpleModule::initialize(stage);

    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    selfSource_ = new cMessage("selfSource");
    selfSender_ = new cMessage("selfSender");
    localPort_ = par("localPort").intValue();
    destPort_ = par("destPort").intValue();

    EV << "VoIPR::initialize - binding to port: local:" << localPort_ << endl;
    if (localPort_ != -1)
    {
        socket.setOutputGate(gate("udpOut"));
        socket.bind(localPort_);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    numSent = 0;
    numXiafa = 0;
    startTime = par("startTime").doubleValue();
    stopTime = par("stopTime").doubleValue();
    sendInterval = par("sendInterval").doubleValue();
    packetName = par("packetName");

    hostpp = getParentModule()->getParentModule();
    localIp = getIpByName(getParentModule()->getFullName());
    host_name = par("host_name");
    print_func = par("print_func").intValue();
    mbcd = par("mbcd");

    //////////////////////////////////////////////////////////////////////////////////
    node_n = par("node_n").intValue();
    max_send = par("max_send").intValue();
    top = par("top").intValue();
    top_interval = par("top_interval").doubleValue();

    char file_send[50], file_handle[50];
    sprintf(file_send, "../file_out/send_enb%s.txt", mbcd);
    sprintf(file_handle, "../file_out/handle_enb%s.txt", mbcd);
    file_out_send.open(file_send, std::ios::out);
    file_out_handle.open(file_handle, std::ios::out);
    file_out_send << "file_out_send: " << simTime().str() << endl;
    file_out_handle << "file_out_handle: " << simTime().str() << endl;

    initTraffic_ = new cMessage("initTraffic");
    initTraffic();

    car_list_with_time.resize(node_n);
    for(int i = 0; i < car_list_with_time.size(); ++i)
        car_list_with_time[i].resize(node_n);
    car_start.resize(node_n);

    std::ifstream fin;
    for(int i = 0; i < node_n; ++i)
    {
        char file_a[50];
        sprintf(file_a, "../file_out/send_car[%d]_mb_1.txt", i);
        fin.open(file_a);
        file_out_send << "read file: " << i << endl;
        if(fin.fail())
            continue ;
        std::string str;
        int dataId, ida, idb;
        double idtt;
        bool idRec;

        getline(fin, str);
        std::stringstream ss(str);
        ss >> str >> idtt;
        car_start[i] = idtt;

        while(getline(fin, str))
        {
            std::stringstream ss(str);
            ss >> str;
            if(str == "nei:")
            {
                ss >> dataId >> ida >> idb >> idRec >> idtt;
//                file_out_send << "ida_idb: " << dataId << "  " << ida << "  " << idb << "  " << idRec << "  " << idtt << endl;
//                car_list[ida].emplace_back(dataId, ida, idb, idRec, idtt);
                if(car_list_with_time[ida][idb].size() == 0 || idtt > car_list_with_time[ida][idb].back().time)
                    car_list_with_time[ida][idb].emplace_back(dataId, idRec, idtt);
            }
        }
        fin.close();
    }
    for(int no_ida = 0; no_ida < node_n; ++no_ida)
    {
        for(int no_idb = 0; no_idb < node_n; ++no_idb)
        {
            auto tt = car_list_with_time[no_ida][no_idb];
            for(auto no = tt.begin(); no != tt.end(); ++no)
                file_out_send << "car_list: " << no->dataId << "  " << no_ida << "  " << no_idb << "  " << no->idRec << "  " << no->time << endl;
        }
    }
}

void AppAppeNB::initTraffic()
{
    destAddresses = updateDestAddrs();
    destIdx = updateDestIdxs();
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if(destAddresses.size() == 0)
    {
        // this might happen when users are created dynamically
        EV << simTime() << "AppAppeNB::initTraffic - destination " << destAddresses.size() << " not found" << endl;

        scheduleAt(simTime() + 0.1, initTraffic_);
    }
    else
    {
        delete initTraffic_;

        // calculating traffic starting time
        simtime_t startTime = par("startTime");

        // TODO maybe un-necesessary
        // this conversion is made in order to obtain ms-aligned start time, even in case of random generated ones
        simtime_t offset = (round(SIMTIME_DBL(startTime) * 1000) / 1000);

        scheduleAt(simTime() + startTime, selfSender_);
        EV << "\t starting traffic in " << startTime << " seconds " << endl;
    }
}

void AppAppeNB::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "selfSender"))
            sendMSGHH();
        else if(!strcmp(msg->getName(), "initTraffic"))
            initTraffic();
    }
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

void AppAppeNB::sendMSGHH()
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (numSent % 10 == 0)
        xiafaMsg();
    ++numSent;

    scheduleAt(simTime() + sendInterval, selfSender_);
}

// Receiver
void AppAppeNB::finish()
{

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AppAppeNB::xiafaMsg()
{
    if(print_func)
        file_out_send << "start xiafaMsg()" << endl;
//  Generate broadcast message
    std::ostringstream str;
    str << packetName << "-" << numXiafa;
    MSGHH *payload = new MSGHH(str.str().c_str());
// setting message attributes
    payload->setByteLength(par("messageLength").intValue());
    payload->setType(xiafa);
    payload->setDataId(numXiafa + 500);
    payload->setHops(0);
//    payload->setSrcId(getParentModule()->getId());
//  Add path to message
    int chuliff = -1;
    if(strcmp(mbcd, "_mb") == 0)
        chuliff = chuliPath_match(payload);
    else if(strcmp(mbcd, "_b") == 0)
        chuliff = chuliPath(payload, 'b');
    else if(strcmp(mbcd, "_c") == 0)
        chuliff = chuliPath(payload, 'c');
    else if(strcmp(mbcd, "_d") == 0)
        chuliff = chuliPath(payload, 'd');
    if(chuliff < 0)
    {
        delete payload;
        if(print_func)
            file_out_send << "finish xiafaMsg()" << endl;
        return ;
    }
//  Send the message to the seed vehicle
    destAddresses = updateDestAddrs();
    destIdx = updateDestIdxs();
    for (int i = 0; i < k_seed; ++i)
    {
        int dest_idx = payload->getDestIdxs(i);
        if(dest_idx < 0)
            continue ;
        if(destIdx.find(dest_idx) == destIdx.end())
        {
            file_out_send << dest_idx << " this car not appear." << endl;
            continue ;
        }
//        inet::L3Address dest_ip = getIpById(dest_id);
        inet::L3Address dest_ip = getIpByIndex(host_name, dest_idx);
        {
            file_out_send
                << "xiafa dataId: " << payload->getDataId()
                << ", Name " << getParentModule()->getFullName()
                << ", src: " << localIp.str()
                << ", dest: " << dest_ip.str()
                << ", time: " << simTime().str()
                << endl;
        }
        if(dest_ip == inet::L3Address())
        {
            file_out_send << "xiafa error: no ip." << endl;
            continue;
        }
//        MSGHH *payload_copy = payload->dup();
//        socket.sendTo(payload->dup(), dest_ip, destPort_);
        cModule * pp_sub = hostpp->getSubmodule(host_name, dest_idx);
        if(pp_sub != nullptr)
        {
            sendDirect(payload->dup(), pp_sub->getSubmodule("udpApp", 0)->gate("hostIn"));
        }
    }
    file_out_send << endl << endl;
    ++numXiafa;
//    emit(sentPkSignal, payload);
    delete payload;
    if(print_func)
        file_out_send << "finish xiafaMsg()" << endl;
}

void AppAppeNB::chuliMsg(cMessage* msg)
{
    if(print_func)
        file_out_send << "start chuliMsg()" << endl;
    MSGHH *mmsg = check_and_cast<MSGHH *>(msg);
    {
        file_out_handle
            << "senddirect dataId: " << mmsg->getDataId()
            << ", Name " << getParentModule()->getFullName()
            << ", src: " << mmsg->getSrcAddrs()
            << ", dest: " << localIp.str()
            << ", time: " << simTime().str()
            << endl;
    }
    if(print_func)
        file_out_send << "finish chuliMsg()" << endl;
}

void AppAppeNB::chuliNode()
{
    if(print_func)
        file_out_send << "start chuliNode()" << endl;
    car_map.clear();
    double top_start = std::stof(simTime().str());
    double top_end = top_start + top * top_interval;

    for(int no_ida = 0; no_ida < node_n; ++no_ida)
    {
        for(int no_idb = 0; no_idb < node_n; ++no_idb)
        {
            if(car_start[no_ida] > top_start || car_start[no_idb] > top_start)
                continue ;
            auto car_tt = car_list_with_time[no_ida][no_idb];
            if(car_tt.size() == 0)
                continue ;
            int pre_pos = 0;
            while(pre_pos < car_tt.size() - 1)
            {
                if(car_tt[pre_pos + 1].time < top_start)
                    ++pre_pos;
                else
                    break;
            }
            std::set<int> si_pos;
            for(; pre_pos < car_tt.size() - 1; ++pre_pos)
            {
                if(car_tt[pre_pos].time > top_end)
                    break;
                if(car_tt[pre_pos].idRec == 1 && car_tt[pre_pos + 1].idRec == 1)
                {
                    int pos_a = (car_tt[pre_pos].time - top_start) / top_interval;
                    int pos_b = (car_tt[pre_pos + 1].time - top_start) / top_interval;
                    pos_a = fmax(pos_a, 0);
                    pos_b = fmin(pos_b, top - 1);
                    for(int i = pos_a; i <= pos_b; ++i)
                        if(si_pos.find(i) == si_pos.end())
                            si_pos.insert(i);
                }
                else if(car_tt[pre_pos].idRec == 1 && car_tt[pre_pos + 1].idRec == 0)
                {
                    double mid_t = (car_tt[pre_pos + 1].time + car_tt[pre_pos].time) / 2;
                    int pos_a = (car_tt[pre_pos].time - top_start) / top_interval;
                    int pos_b = (mid_t - top_start) / top_interval;
                    pos_a = fmax(pos_a, 0);
                    pos_b = fmin(pos_b, top - 1);
                    for(int i = pos_a; i <= pos_b; ++i)
                        if(si_pos.find(i) == si_pos.end())
                            si_pos.insert(i);
                }
                else if(car_tt[pre_pos].idRec == 0 && car_tt[pre_pos + 1].idRec == 1)
                {
                    double mid_t = (car_tt[pre_pos + 1].time + car_tt[pre_pos].time) / 2;
                    int pos_a = (mid_t - top_start) / top_interval;
                    int pos_b = (car_tt[pre_pos + 1].time - top_start) / top_interval;
                    pos_a = fmax(pos_a, 0);
                    pos_b = fmin(pos_b, top - 1);
                    for(int i = pos_a; i <= pos_b; ++i)
                        if(si_pos.find(i) == si_pos.end())
                            si_pos.insert(i);
                }
            }
            for(auto it = si_pos.begin(); it != si_pos.end(); ++it)
            {
                if(car_map[*it].find(std::make_pair(no_ida, no_idb)) == car_map[*it].end())
                {
                    car_map[*it].insert({no_ida, no_idb});
                    car_map[*it].insert({no_idb, no_ida});
                }
            }
        }
    }
////  Print information about the generated topology
//    for(int i = 0; i < top; ++i)
//        for(auto j : car_map[i])
//            file_out_handle << "id: " << i << ", ida: " << j.first << ", idb: " << j.second << endl;
    if(print_func)
        file_out_send << "finish chuliNode()" << endl;
}

int AppAppeNB::chuliPath(MSGHH *payload, char ch)
{
    if(print_func)
        file_out_send << "start chuliPath()" << endl;
// Generate topology
    chuliNode();
// Sort vehicles by match_bc
    k_seed = numXiafa / 3;
    if (k_seed >= node_n)
    {
        if(print_func)
            file_out_send << "finish chuliPath()" << endl;
        return -1;
    }
    topu tt(node_n, top, k_seed, max_send, car_map);
//    file_out_send << node_n << "  " << top << "  " << k_seed << "  " << max_send << endl;
//    for(auto i = car_map.begin(); i != car_map.end(); ++i)
//    {
//        file_out_send << "topu: " << i->first << endl;
//        for(auto j = i->second.begin(); j != i->second.end(); ++j)
//            file_out_send << "edge: " << j->first << "  " << j->second << endl;
//    }
    vector<int> sort_list = tt.test(ch);

    file_out_send << "node number: " << destIdx.size() << endl;
    file_out_send << "topu sort list: ";
    for(auto i : sort_list)
        file_out_send << i << " ";
    file_out_send << simTime().str() << endl;
    file_out_send << "attrs: node_n " << node_n << " top " << top << " k_seed " << k_seed << " max_send " << max_send << endl;
// find seed node
    vector<int> v_seed;
    set<int> no_if_rea;
    int cnt_s = 0;
    for (auto a_no : sort_list)
    {
        if (cnt_s == k_seed)
            break;
        if (a_no == -1)
            continue ;
        if (no_if_rea.find(a_no) == no_if_rea.end())
        {
            if(destIdx.find(a_no) == destIdx.end())
            {
//                file_out_send << a_no << " this car not appear." << endl;
                continue ;
            }
            ++cnt_s;
            v_seed.push_back(a_no);
            no_if_rea.insert(a_no);
        }
    }
    if(v_seed.size() == 0)
    {
        if(print_func)
            file_out_send << "finish chuliPath()" << endl;
        return 0;
    }
// The seed vehicle information is written into the message
    for(int i = 0; i < payload->getDestIdxsArraySize(); ++i)
        payload->setDestIdxs(i, -1);

    payload->setTop_n(top);
    int pos_t = 0;
    for (int i = 0; i < v_seed.size(); ++i)
    {
        file_out_send << "add seed: " << v_seed[i] << endl;
        payload->setDestIdxs(pos_t, v_seed[i]);
        ++pos_t;
    }
    if(pos_t != k_seed)
        payload->setDestPos(k_seed);
    payload->setDestPos(k_seed);
// Link information is written into the message
    for (int i = 0; i < top; ++i)
    {
    // Modify the format of the topology
        std::map<int, std::vector<int>> nei_node;
        int total_send = 0;
        for (auto p : car_map[i])
        {
            nei_node[p.first].push_back(p.second);
            file_out_send << "topu " << i << ": " << p.first << "  " << p.second << endl;
        }
    // write node path to message
        int sz_t = payload->getDestPos();
        payload->setNodeHops(i, sz_t);
        int cnt_t = -1;
        for (int j = 0; j < sz_t; ++j)
        {
            if(payload->getDestIdxs(j) < 0)
                continue ;
            ++cnt_t;
            int pos_idx = payload->getDestIdxs(j);
            for(auto a_no : nei_node[pos_idx])
            {
                if (no_if_rea.find(a_no) == no_if_rea.end())
                {
                    file_out_send << "now add node: " << pos_idx << "  " << a_no << endl;
                    payload->setDestIdxs(sz_t + cnt_t, a_no);
                    no_if_rea.insert(a_no);
                    ++total_send;
                    break ;
                }
            }
            if(total_send >= max_send)
                break ;
        }
        payload->setDestPos(sz_t + cnt_t + 1);
    }
    file_out_send << "payload dest ids: " ;
    for(int i=0; i<payload->getDestPos(); ++i)
        file_out_send << payload->getDestIdxs(i) << " ";
    file_out_send << simTime().str() << endl;

//    file_out_send << "payload dest loc: " ;
//    for(int i=0; i<payload->getDestPos(); ++i)
//        file_out_send << payload->getDestLoc(i) << " ";
//    file_out_send << simTime().str() << endl;
    if(print_func)
        file_out_send << "finish chuliPath()" << endl;
    return 0;
}

int AppAppeNB::chuliPath_match(MSGHH *payload)
{
    if(print_func)
        file_out_send << "start chuliPath_match()" << endl;
// Generate topology
    chuliNode();
// Sort vehicles by match_bc
    k_seed = numXiafa / 3;
    if (k_seed >= node_n)
    {
        if(print_func)
            file_out_send << "finish chuliPath_match()" << endl;
        return -1;
    }
    topu tt(node_n, top, k_seed, max_send, car_map);
//    file_out_send << node_n << "  " << top << "  " << k_seed << "  " << max_send << endl;
//    for(auto i = car_map.begin(); i != car_map.end(); ++i)
//    {
//        file_out_send << "topu: " << i->first << endl;
//        for(auto j = i->second.begin(); j != i->second.end(); ++j)
//            file_out_send << "edge: " << j->first << "  " << j->second << endl;
//    }
    std::pair<vector<int>, vector<int>> sort_list_match = tt.test_match_bc();
    vector<int> sort_list = sort_list_match.first;
    vector<int> match = sort_list_match.second;

    file_out_send << "node number: " << destIdx.size() << endl;
    file_out_send << "topu sort list: ";
    for(auto i : sort_list)
        file_out_send << i << " ";
    file_out_send << simTime().str() << endl;
    file_out_send << "attrs: node_n " << node_n << " top " << top << " k_seed " << k_seed << " max_send " << max_send << endl;
// find seed node
    vector<int> v_seed;
    set<int> no_if_rea;
    set<int> tem_no;
    int cnt_s = 0;
    destIdx = updateDestIdxs();
    for (auto a_no : sort_list)
    {
        if (cnt_s == k_seed)
            break;
        if (a_no == -1)
            continue ;
        if (no_if_rea.find(a_no) == no_if_rea.end() && tem_no.find(a_no)  == tem_no.end())
        {
            if(destIdx.find(a_no) == destIdx.end())
            {
//                file_out_send << a_no << " this car not appear." << endl;
                continue ;
            }
            ++cnt_s;
            v_seed.push_back(a_no);
            no_if_rea.insert(a_no);
            if (match[a_no] != -1)
            {
                tem_no.insert(match[a_no]);
            }
        }
    }
    if(v_seed.size() == 0)
    {
        if(print_func)
            file_out_send << "finish chuliPath_match()" << endl;
        return 0;
    }
// The seed vehicle information is written into the message
    for(int i = 0; i < payload->getDestIdxsArraySize(); ++i)
        payload->setDestIdxs(i, -1);

    payload->setTop_n(top);
    int pos_t = 0;
    for (int i = 0; i < v_seed.size(); ++i)
    {
        file_out_send << "add seed: " << v_seed[i] << endl;
        payload->setDestIdxs(pos_t, v_seed[i]);
        ++pos_t;
    }
    if(pos_t != k_seed)
        payload->setDestPos(k_seed);
    payload->setDestPos(k_seed);
// Link information is written into the message
    for (int i = 0; i < top; ++i)
    {
    // Modify the format of the topology
        std::map<int, std::set<int>> nei_node;
        int total_send = 0;
        for (auto p : car_map[i])
        {
            nei_node[p.first].insert(p.second);
            file_out_send << "topu " << i << ": " << p.first << "  " << p.second << endl;
        }
    // write node path to message
        int sz_t = payload->getDestPos();
        payload->setNodeHops(i, sz_t);
        int cnt_t = -1;
        for (int j = 0; j < sz_t; ++j)
        {
            if(payload->getDestIdxs(j) < 0)
                continue ;
            ++cnt_t;
            int pos_idx = payload->getDestIdxs(j);
            for (auto a_no : sort_list)
            {
                if (nei_node[pos_idx].find(a_no) != nei_node[pos_idx].end() && no_if_rea.find(a_no) == no_if_rea.end())
                {
                    file_out_send << "now add node: " << pos_idx << "  " << a_no << endl;
                    payload->setDestIdxs(sz_t + cnt_t, a_no);
                    no_if_rea.insert(a_no);
                    ++total_send;
                    break ;
                }
            }
            if(total_send >= max_send)
                break ;
        }
        payload->setDestPos(sz_t + cnt_t + 1);
    }
    file_out_send << "payload dest ids: " ;
    for(int i=0; i<payload->getDestPos(); ++i)
        file_out_send << payload->getDestIdxs(i) << " ";
    file_out_send << simTime().str() << endl;

//    file_out_send << "payload dest loc: " ;
//    for(int i=0; i<payload->getDestPos(); ++i)
//        file_out_send << payload->getDestLoc(i) << " ";
//    file_out_send << simTime().str() << endl;
    if(print_func)
        file_out_send << "finish chuliPath_match()" << endl;
    return 0;
}

std::vector<inet::L3Address> AppAppeNB::updateDestAddrs()
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

std::set<int> AppAppeNB::updateDestIdxs()
{
    if(print_func)
        file_out_send << "start updateDestIdxs()" << endl;
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
        file_out_send << "finish updateDestIdxs()" << endl;
    return destIdx;
}

inet::L3Address AppAppeNB::getIpByName(std::string name)
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

inet::L3Address AppAppeNB::getIpByIndex(std::string name, int idx)
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

inet::L3Address AppAppeNB::getIpById(int id)
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

int AppAppeNB::getIndexById(int id)
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

int AppAppeNB::getIdByIndex(int idx)
{
    return hostpp->findSubmodule(host_name, idx);
}
