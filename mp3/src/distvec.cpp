#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <climits>
#include <queue>
#define AR_LEN 21

using namespace std;

int tp[AR_LEN][AR_LEN];

set<int> nodes;

// the forwarding table consists of two different tables, the middle nodes from source to dest and the cost to get from the src to dest.
int mid_table[AR_LEN][AR_LEN];
int cost_table[AR_LEN][AR_LEN];
vector<string> msgs;
vector<pair<int, int>> msg_ids;

ofstream fpOut("output.txt");

void send_msg()
{
    int src_id, dst_id, temp_id;
    for (int i = 0; i < msgs.size(); i++)
    {
        src_id = msg_ids[i].first;
        dst_id = msg_ids[i].second;
        temp_id = src_id;

        fpOut << "from " << src_id << " to " << dst_id;
        if (cost_table[src_id][dst_id] < 0)
        {
            fpOut << " cost infinite hops unreachable ";
        }
        else
        {
            fpOut << " cost " << cost_table[src_id][dst_id] << " hops ";
            if (cost_table[src_id][dst_id] > 0)
            {
                while (temp_id != dst_id)
                {
                    fpOut << temp_id << " ";
                    temp_id = mid_table[temp_id][dst_id];
                }
            }
        }
        fpOut << "message " << msgs[i] << endl;
    }
    fpOut << endl;
}

void read_msgs(ifstream &messagefile)
{
    int src_id, dst_id;

    string line, info;
    while (getline(messagefile, line))
    {
        if (line != "")
        {
            stringstream line_ss(line);
            line_ss >> src_id >> dst_id;
            getline(line_ss, info);
            info = info.substr(1);
            pair<int, int> id_pair;
            id_pair.first = src_id;
            id_pair.second = dst_id;
            msgs.push_back(info);
            msg_ids.push_back(id_pair);
        }
    }
}

void find_min(int src, int dest)
{
    int temp_id, min_cost;
    temp_id = mid_table[src][dest];
    min_cost = cost_table[src][dest];
    for (int m : nodes)
    {
        //checks to see if there is an edge from m to the src node.
        if (tp[src][m] >= 0 && cost_table[m][dest] >= 0)
        {
            //if there is no edge leading between the middle node and the src node, or the cost is cheaper
            if (min_cost < 0 || min_cost > tp[src][m] + cost_table[m][dest])
            {
                temp_id = m;
                min_cost = tp[src][m] + cost_table[m][dest];
            }
        }
    }
    mid_table[src][dest] = temp_id;
    cost_table[src][dest] = min_cost;
}

void forwarding_table()
{
    // first we initialize the forwarding table to just the neighboring funcitons.
    for (auto src : nodes)
    {
        for (auto dest : nodes)
        {
            // if (tp[src][dest] < 0 ){
            //     mid_table[src][dest] = -999;
            // }
            // else{
            //     mid_table[src][dest] = dest;
            // }
            mid_table[src][dest] = dest;
            cost_table[src][dest] = tp[src][dest];
            
        }
    }

    int n_nodes = nodes.size();
    // next we fill out the forwarding table.
    // because we are using the bellman-ford algorithm, we run the algorithm the amount of times
    // equal to the amount of nodes there are in the graph.

    for (int i = 0; i <= n_nodes; i++)
    {
        for (auto src : nodes)
        {
            for (auto dest : nodes)
            {
                find_min(src, dest);
            }
        }
    }
}

void forward_output()
{
    for (auto src : nodes)
    {
        for (auto dest : nodes)
        {
            if (cost_table[src][dest] >= 0)
            {
                fpOut << dest << " " << mid_table[src][dest] << " " << cost_table[src][dest] << endl;
            }
        }
        fpOut << endl;
    }
    
}

void read_in(ifstream &topofile)
{
    int src_id, dst_id, cost;

    for (int i = 1; i < AR_LEN; i++)
    {
        for (int j = 1; j < AR_LEN; j++)
        {
            //if the nodes are the same, the cost is equal to zero
            if (i == j)
            {
                tp[i][j] = 0;
            }
            //else we don't know what the cost will be.
            else
            {
                tp[i][j] = -999;
            }
        }
    }
    string line;
    while (getline(topofile, line))
    {
        topofile >> src_id >> dst_id >> cost;
        tp[src_id][dst_id] = cost;
        tp[dst_id][src_id] = cost;
        nodes.insert(src_id);
        nodes.insert(dst_id);
    }
}

int change_table(ifstream &changesfile)
{
    int src_id, dest_id, cost;
    if (changesfile.eof())
    {
        return -1;
    }
    else
    {
        changesfile >> src_id >> dest_id >> cost;
        tp[src_id][dest_id] = cost;
        tp[dest_id][src_id] = cost;
        return 0;
    }

    return -1;
}

int main(int argc, char **argv)
{
    //printf("Number of arguments: %d", argc);
    if (argc != 4)
    {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }
    ifstream messagefile(argv[2]);
    read_msgs(messagefile);
    ifstream topofile(argv[1]);
    read_in(topofile);

    // FILE *fpOut;
    // fpOut = fopen("output.txt", "w");

    /**
     * 
     * Implement distance vector routing algorithm / bellman-ford
     * 
     **/
    forwarding_table();
    forward_output();
    send_msg();

    int src_id, dest_id, cost;
    ifstream changesfile(argv[3]);
    // while (change_table(changesfile) == 0)
    while (changesfile >> src_id >> dest_id >> cost)
    {
        tp[src_id][dest_id] = cost;
        tp[dest_id][src_id] = cost;
        forwarding_table();
        forward_output();
        send_msg();
    }

    fpOut.close();

    return 0;

    //     // close the output file after algorithm is complete
    //     fclose(fpOut);

    //     return 0;
}