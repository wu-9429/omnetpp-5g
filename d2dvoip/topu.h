/*
 * topu.cpp
 *
 *  Created on: Nov 18, 2020
 *      Author: ttf
 */

#ifndef _topu_H_
#define _topu_H_

#include<fstream>
#include<queue>
#include<stack>
#include<set>
#include<iostream>
#include<random>
#include<ctime>
#include<vector>
#include<string>
#include<map>
#include<algorithm>

using namespace std;

class topu
{
public:
    topu(int node_n, int top, int k_seed, int max_send, map<int, set<pair<int, int>>> car_map);
    ~topu();
    pair<vector<int>, vector<int>> test_match_bc();
    pair<vector<int>, vector<int>> test_match_cc();
    pair<vector<int>, vector<int>> test_match_dc();
    pair<vector<int>, vector<int>> test_match_rand();

    vector<int> test(char ch);

private:
    vector<vector<int>> before_compute_bcdr(int idx); // fanhui jiedian de lingju jihe
    vector<int> sort_match_bcdr(const vector<int>& vetx_num, const vector<double>& totc);
    vector<double> compute_bc(const vector<vector<int>>&);
    vector<double> compute_cc(const vector<vector<int>>&);
    vector<double> compute_dc(const vector<vector<int>>&);
    pair<vector<int>, vector<int>> test_match(char ch);

    vector<int> sort_bcdr(const vector<double>& tot_bcdr);

    int node_n; // node num
    int top;    // top num, input value
    int k_seed;   // input value
    int max_send;   // input value

    map<int, int> atob;
    map<int, int> btoa;
    map<int, set<pair<int, int>>> car_map;

    // max match code
    int max_match(const vector<vector<int>>&);
    // BFS find zengguanglu, give a no, jianli jiaocuo tree
    int BFS(const vector<vector<int>>& nei, int no);
    // hebing
    int contract(int u, int v, vector<int>& inblossom, vector<int>& base, vector<int>& pre);
    void argument(int u, vector<int>& pre);
    // zhaodao gongtong jihe bianhao
    int findBase(int u, int v, vector<int>& base, vector<int>& pre);
    void changeBlossom(int b, int u, vector<int>& inblossom, vector<int>& base, vector<int>& pre);

    vector<int> match;

    ofstream file_out;
};

#endif // _topu_H_


