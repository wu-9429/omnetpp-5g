/*
 * topu.cc
 *
 *  Created on: Jan 6, 2021
 *      Author: ttf
 */

#include "topu.h"


topu::topu(int node_n, int top, int k_seed, int max_send, map<int, set<pair<int, int>>> car_map)
{
    this->node_n = node_n;
    this->top = top;
    this->k_seed = k_seed;
    this->max_send = max_send;
    this->car_map = car_map;
    atob.clear();
    btoa.clear();

    file_out.open("../file_out/topu_out.txt", ios::out);
}


topu::~topu()
{

}


vector<vector<int>> topu::before_compute_bcdr(int idx)
{
    vector<int> BCD(node_n, 0); // save BC CC DC value of nodes
    vector<vector<int>> node_nei(node_n, vector<int>());    // jiedian de lingju jihe

    // read nodes
    for(auto it = car_map[idx].cbegin(); it != car_map[idx].cend(); ++it)
    {
//        node_nei[atob[it->first]].push_back(atob[it->second]);
        node_nei[it->first].push_back(it->second);
    }
    return node_nei;
}

vector<int> topu::sort_bcdr(const vector<double>& tot_bcdr)
{
    vector<pair<int, double>> tem_tot;  // save the value of the node for sort
    vector<int> final_sort; // save the final desc list
    for (int iv = 0; iv < node_n; ++iv)
    {
        tem_tot.push_back({ iv, tot_bcdr[iv] });
    }

    sort(tem_tot.begin(), tem_tot.end(), [](pair<int, double> a, pair<int, double> b) {
        return a.second > b.second;
    });

    for (int i = 0; i < node_n; ++i)
    {
        final_sort.push_back(tem_tot[i].first);
    }
    return final_sort;
}

vector<int> topu::sort_match_bcdr(const vector<int>& vetx_num, const vector<double>& totc)
{

    int max_match = 0;             // get max match value
    for (int i : vetx_num)
        max_match = max(max_match, i);

    map<int, vector<int>> match_to_idx; // node set of same match value
    for (int i = 0; i < node_n; ++i)
    {
        match_to_idx[vetx_num[i]].push_back(i);
    }

    // compute node sort, add totc value for judge
    vector<int> final_sorted;   // sort by totc for same match value
    for (int i = max_match; i >= 0; --i)
    {
        vector<pair<int, double>> match_deg;    // save totc value for sort
        for (auto j : match_to_idx[i])
        {
            match_deg.push_back({ j, totc[j] });
        }
        // sort by totc for same match value
        sort(match_deg.begin(), match_deg.end(), [](pair<int, double>& a, pair<int, double>& b) {
            return a.second > b.second;
        });
        // add sorted list by totc to final sorted list
        for (auto j : match_deg)
        {
            final_sorted.push_back(j.first);
        }
    }
    return final_sorted;
}


vector<double> topu::compute_bc(const vector<vector<int>>& node_nei)
{
    vector<double> BC(node_n, 0);
    for (int s = 0; s < node_n; ++s)
    {
        vector<vector<int>> path_pre(node_n, vector<int>());
        vector<double> path_num(node_n, 0.0);
        vector<int> path_len(node_n, -1);

        path_num[s] = 1.0;
        path_len[s] = 0;
        vector<int> vi_S;
        queue<int> Q;
        Q.push(s);

        while (!Q.empty())
        {
            int mid = Q.front();
            Q.pop();
            vi_S.push_back(mid);
            int p_l_mid = path_len[mid];
            double p_n_mid = path_num[mid];
            for (auto i : node_nei[mid])
            {
                if (path_len[i] < 0)
                {
                    Q.push(i);
                    path_len[i] = p_l_mid + 1;
                }
                if (path_len[i] == p_l_mid + 1)
                {
                    path_num[i] += p_n_mid;
                    path_pre[i].push_back(mid);
                }
            }
        }

        map<int, double> sum_p;
        stack<int> S;
        for (auto i : vi_S)
        {
            sum_p.insert({ i, 0.0 });
            S.push(i);
        }
        while (!S.empty())
        {
            int tem = S.top();
            S.pop();
            double coeff = (1.0 + sum_p[tem]) / path_num[tem];
            for (auto i : path_pre[tem])
            {
                sum_p[i] += path_num[i] * coeff;
            }
            if (tem != s)
            {
                BC[tem] += sum_p[tem] / 2;
            }
        }
    }
    return BC;
}



vector<double> topu::compute_cc(const vector<vector<int>>& node_nei)
{
    vector<double> CC(node_n, 0);
    for (int i = 0; i < node_n; ++i)
    {
        vector<int> sp(node_n, INT_MAX);
        int level = 0;
        set<int> no_o;
        set<int> no_c;
        no_c.insert(i);

        while (no_c.size() > 0)
        {
            no_o = no_c;
            no_c.clear();
            for (auto ii : no_o)
            {
                if (sp[ii] == INT_MAX)
                {
                    sp[ii] = level;
                    for (auto ij : node_nei[ii])
                    {
                        if (no_c.find(ij) == no_c.end())
                        {
                            no_c.insert(ij);
                        }
                    }
                }
            }
            ++level;
        }

        int totsp = 0;
        int sp_n = 0;
        for (int j = 0; j < node_n; ++j)
        {
            if (sp[j] != INT_MAX)
            {
                ++sp_n;
                totsp += sp[j];
            }
        }
        int _node = 0;
        for (auto& j : node_nei)
            if (j.size())
                ++_node;
        if (totsp > 0 && _node > 1)
        {
            CC[i] = (sp_n - 1.0) / totsp;
            double s = (sp_n - 1.0) / (_node - 1);
            CC[i] *= s;
        }
        else
        {
            CC[i] = 0;
        }
    }
    return CC;
}



vector<double> topu::compute_dc(const vector<vector<int>>& node_nei)
{
    vector<double> DC(node_n, 0);
    for (int i = 0; i < node_n; ++i)
    {
        DC[i] = node_nei[i].size();
    }
    return DC;
}


vector<int> topu::test(char ch)
{
    vector<double> totc(node_n, 0.0);    // save the bcdr value of node for all top
    map<int, vector<vector<int>>> node_nei_all; // save the neighbors set of node for all top
    string bcdr;    // flag the select duli method

    // for read file info
    for (int i = 0; i < top; ++i)
    {
        vector<vector<int>> node_nei = before_compute_bcdr(i);
        vector<double> BCCCDC;  // save BC or CC or DC value

        if (ch == 'B' || ch == 'b')
        {
            BCCCDC = compute_bc(node_nei);
            bcdr = "bc";
        }
        else if (ch == 'C' || ch == 'c')
        {
            BCCCDC = compute_cc(node_nei);
            bcdr = "cc";
        }
        else if (ch == 'D' || ch == 'd')
        {
            BCCCDC = compute_dc(node_nei);
            bcdr = "dc";
        }
        else
        {
            BCCCDC.assign(node_n, 0);
        }

        node_nei_all[i] = node_nei;
        for (int iv = 0; iv < node_n; ++iv)
        {
            totc[iv] += BCCCDC[iv];
        }
    } //end for è¯»å…¥æ–‡ä»¶ä¿¡æ�¯

    if (ch == 'R' || ch == 'r')
    {
        default_random_engine e(clock());  // random engine
        for (int i = 0; i < node_n; ++i)
            totc[i] = i;
        //shuffle(final_sort.begin(), final_sort.end(), e);
        bcdr = "rand";
    }

    vector<int> final_sort = sort_bcdr(totc);

    file_out << "final_sort: ";
    for(int i : final_sort)
        file_out << i << " ";
    file_out << endl << endl;

    return final_sort;
}


// test bcdr code
pair<vector<int>, vector<int>> topu::test_match(char ch)
{
    vector<double> totc(node_n, 0.0);   // save the bcdr value of node for all top
    map<int, vector<vector<int>>> node_nei_all; // save the neighbors set of node for all top
    string bcdr;    // flag the select duli method
    vector<int> vetx_num(node_n, 0);    // save the match times of all node
    vector<vector<int>> match_all(top, vector<int>());  // save all top all node if match

//    atob.clear();
//    btoa.clear();
//    int idx = 0;
//    for(int i = 0; i < top; ++i)
//    {
//        auto it = car_map[i].cbegin();
//        for(; it != car_map[i].cend(); ++it)
//        {
//            if(atob.find(it->first) == atob.end())
//            {
//                atob[it->first] = idx;
//                btoa[idx] = it->first;
//                ++idx;
//            }
//            file_out << "con: " << it->first << "  " << it->second << endl;
//        }
//    }
//    for(auto it : atob)
//        file_out << "atob: " << it.first << "  " << it.second << endl;

    // for read file info
    for(int i = 0; i < top; ++i)
    {
        vector<vector<int>> node_nei = before_compute_bcdr(i);
        vector<double> BCCCDC;  // save BC or CC or DC value

        if (ch == 'B' || ch == 'b')
        {
            BCCCDC = compute_bc(node_nei);
            bcdr = "bc";
        }
        else if (ch == 'C' || ch == 'c')
        {
            BCCCDC = compute_cc(node_nei);
            bcdr = "cc";
        }
        else if (ch == 'D' || ch == 'd')
        {
            BCCCDC = compute_dc(node_nei);
            bcdr = "dc";
        }
        else
        {
            BCCCDC.assign(node_n, 0);
        }

        node_nei_all[i] = node_nei;
        for (int iv = 0; iv < node_n; ++iv)
        {
            totc[iv] += BCCCDC[iv];
        }

        match.assign(node_n, -1);
        int max_m = max_match(node_nei);
        match_all[i] = match;
        for (int ii = 0; ii < node_n; ++ii)
        {
            if (match[ii] != -1)
            {
                ++vetx_num[ii];
            }
        }
    } //end for read file info

    if (ch == 'R' || ch == 'r')
    {
        default_random_engine e(clock());   // random engine
        for (int i = 0; i < node_n; ++i)
            totc[i] = i;
        //shuffle(final_sort.begin(), final_sort.end(), e);
        bcdr = "rand";
    }
    vector<int> final_sort = sort_match_bcdr(vetx_num, totc);

//    cout << endl << "Data in transit: " << endl;
//    contact_simu(node_nei_all, match_all, final_sort, vetx_num, bcdr);
//    for(int &i : final_sort)
//        i = btoa[i];
    file_out << "final_sort: ";
    for(int i : final_sort)
        file_out << i << " ";
    file_out << endl << endl;
    return make_pair(final_sort, match_all[0]);
}


pair<vector<int>, vector<int>> topu::test_match_bc()
{
    return test_match('b');
}

pair<vector<int>, vector<int>> topu::test_match_cc()
{
    return test_match('c');
}

pair<vector<int>, vector<int>> topu::test_match_dc()
{
    return test_match('d');
}

pair<vector<int>, vector<int>> topu::test_match_rand()
{
    return test_match('r');
}


// about max match code
int topu::max_match(const vector<vector<int>>& nei)
{
    int res = 0;
    for (int ii = 0; ii < node_n; ++ii)
    {
        if (match[ii] != -1) continue;      // find matched node
        for (int jj = 0; jj < nei[ii].size(); ++jj)
        {
            if (match[nei[ii][jj]] == -1)
            {
                match[ii] = nei[ii][jj];
                match[nei[ii][jj]] = ii;
                res += 2;
                break;
            }
        }
    }
    for (int ii = 0; ii < node_n; ++ii)
        if (match[ii] == -1)
            res += BFS(nei, ii);
    return res;
}

// BFS find zengguanglu, give a node no, jianli jiaocuo tree
int topu::BFS(const vector<vector<int>>& nei, int no)
{
    int u, v, b, front = 0, rear = 0;
    vector<int> pre(node_n, -1);
    vector<int> flag(node_n, 0);
    vector<int> base(node_n, 0);
    vector<int> que(node_n, 0);
    vector<int> inblossom(node_n, 0);
    for (int i = 0; i < node_n; ++i)
    {
        base[i] = i;
    }
    que[rear++] = no;
    flag[no] = 1;
    while (front != rear)
    {
        u = que[front++];
        for (int i = 0; i < nei[u].size(); ++i)
        {
            v = nei[u][i];
            if (base[u] != base[v] && match[u] != v)
            {
                if (v == no || (match[v] != -1 && pre[match[v]] != -1))
                {
                    b = contract(u, v, inblossom, base, pre);
                    for (int j = 0; j < node_n; ++j)
                    {
                        if (inblossom[base[j]])
                        {
                            base[j] = b;
                            if (flag[j] == 0)
                            {
                                flag[j] = 1;
                                que[rear++] = j;
                            }
                        }
                    }
                }
                else if (pre[v] == -1)
                {
                    pre[v] = u;
                    if (match[v] == -1)
                    {
                        argument(v, pre);
                        return 2;
                    }
                    else
                    {
                        que[rear++] = match[v];
                        flag[match[v]] = 1;
                    }
                }
            }
        }
    }
    return 0;
}

// hebing
int topu::contract(int u, int v, vector<int>& inblossom, vector<int>& base, vector<int>& pre)
{
    fill(inblossom.begin(), inblossom.end(), 0);
    int b = findBase(base[u], base[v], base, pre);
    changeBlossom(b, u, inblossom, base, pre);
    changeBlossom(b, v, inblossom, base, pre);
    if (base[u] != b) pre[u] = v;
    if (base[v] != b) pre[v] = u;
    return b;
}

void topu::argument(int u, vector<int>& pre)
{
    int v, k;
    while (u != -1)
    {
        v = pre[u], k = match[v];
        match[u] = v, match[v] = u;
        u = k;
    }
}

// find same node id
int topu::findBase(int u, int v, vector<int>& base, vector<int>& pre)
{
    vector<int> inpath(node_n, 0);
    while (true)
    {
        inpath[u] = 1;
        if (match[u] == -1) break;
        u = base[pre[match[u]]];
    }
    while (!inpath[v])
        v = base[pre[match[v]]];
    return v;
}

void topu::changeBlossom(int b, int u, vector<int>& inblossom, vector<int>& base, vector<int>& pre)
{
    int v;
    while (base[u] != b)
    {
        v = match[u];
        inblossom[base[v]] = inblossom[base[u]] = 1;
        u = pre[v];
        if (base[u] != b)
            pre[u] = v;
    }
}

