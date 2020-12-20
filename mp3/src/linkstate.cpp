#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <climits>

using namespace std;

int nodeNum = 20;
int topology[21][21];

set<int> nodes;

typedef struct message
{
    int src_id;
    int dst_id;
    string info;
    message(int src_id, int dst_id, string info) : src_id(src_id), dst_id(dst_id), info(info) {}
} message;

vector<message> msgs;

ofstream fpOut("output.txt");

typedef map<int, pair<int, int>> table;
table temp_table[21];
table forward_table[21];

void send_msg()
{
    int src_id, dst_id, temp_id;
    for (int i = 0; i < msgs.size(); i++)
    {
        src_id = msgs[i].src_id;
        dst_id = msgs[i].dst_id;
        temp_id = src_id;

        fpOut << "from " << src_id << " to " << dst_id << " cost ";
        if (forward_table[src_id][dst_id].second < 0)
        {
            fpOut << "infinite hops unreachable ";
        }
        else if (forward_table[src_id][dst_id].second == 0)
        {
            fpOut << forward_table[src_id][dst_id].second << " hops ";
        }
        else
        {
            fpOut << forward_table[src_id][dst_id].second << " hops ";
            while (temp_id != dst_id)
            {
                fpOut << temp_id << " ";
                temp_id = forward_table[temp_id][dst_id].first;
            }
        }
        fpOut << "message " << msgs[i].info << endl;
    }
    fpOut << endl;
}

void forwarding_table()
{
    int n_nodes = nodes.size();
    int visited[n_nodes + 1];
    int src_id, dst_id, prev_id, temp_id;
    int min_cost;
    for (set<int>::iterator i = nodes.begin(); i != nodes.end(); i++)
    {
        src_id = *i;
        for (int j = 1; j <= n_nodes + 1; j++)
        {
            visited[j] = 0;
        }
        prev_id = src_id;
        min_cost = 0;
        visited[src_id] = 1;
        for (int k = 1; k < n_nodes; k++)
        {
            for (set<int>::iterator m = nodes.begin(); m != nodes.end(); m++)
            {
                dst_id = *m;
                if ((min_cost + topology[prev_id][dst_id] < temp_table[src_id][dst_id].second || temp_table[src_id][dst_id].second < 0) && topology[prev_id][dst_id] >= 0 && visited[dst_id] == 0)
                {
                    temp_table[src_id][dst_id] = make_pair(prev_id, min_cost + topology[prev_id][dst_id]);
                }
                if ((min_cost + topology[prev_id][dst_id] == temp_table[src_id][dst_id].second || temp_table[src_id][dst_id].second < 0) && topology[prev_id][dst_id] >= 0 && visited[dst_id] == 0)
                {
                    if (prev_id < temp_table[src_id][dst_id].first)
                    {
                        temp_table[src_id][dst_id] = make_pair(prev_id, min_cost + topology[prev_id][dst_id]); //choosing the node with the smaller id number
                    }
                }
            }
            min_cost = 9999999;
            for (set<int>::iterator m = nodes.begin(); m != nodes.end(); m++)
            {
                dst_id = *m;
                if (min_cost > temp_table[src_id][dst_id].second && temp_table[src_id][dst_id].second >= 0 && visited[dst_id] == 0)
                {
                    min_cost = temp_table[src_id][dst_id].second;
                    prev_id = dst_id;
                }
            }
            visited[prev_id] = 1;
        }
        forward_table[src_id] = temp_table[src_id];
        for (set<int>::iterator m = nodes.begin(); m != nodes.end(); m++)
        {

            dst_id = *m;
            temp_id = dst_id;
            if (forward_table[src_id][dst_id].second >= 0)
            {
                while (temp_table[src_id][temp_id].first != src_id)
                {
                    temp_id = temp_table[src_id][temp_id].first;
                }
                forward_table[src_id][dst_id].first = temp_id;
                fpOut << dst_id << " " << forward_table[src_id][dst_id].first << " " << forward_table[src_id][dst_id].second << endl;
            }
        }
        fpOut << endl;
    }
}

void make_temp()
{
    int src_id, dst_id;

    for (set<int>::iterator i = nodes.begin(); i != nodes.end(); i++)
    {
        src_id = *i;
        for (set<int>::iterator j = nodes.begin(); j != nodes.end(); j++)
        {
            dst_id = *j;
            temp_table[src_id][dst_id] = make_pair(src_id, topology[src_id][dst_id]);
        }
    }
}

void read_in_msgs(char **argv)
{
    int src_id, dst_id;

    ifstream messagefile(argv[2]);

    string line, info;
    while (getline(messagefile, line))
    {
        if (line != "")
        {
            stringstream line_ss(line);
            line_ss >> src_id >> dst_id;
            getline(line_ss, info);
            message msg(src_id, dst_id, info.substr(1));
            msgs.push_back(msg);
        }
    }
}

void read_in(ifstream &topofile)
{
    int src_id, dst_id, cost;

    for (int i = 1; i <= nodeNum; i++)
    {
        for (int j = 1; j <= nodeNum; j++)
        {
            topology[i][j] = -999;
            if (i == j)
            {
                topology[i][j] = 0;
            }
        }
    }
    while (topofile >> src_id >> dst_id >> cost)
    {
        topology[src_id][dst_id] = cost;
        topology[dst_id][src_id] = cost;
        nodes.insert(src_id);
        nodes.insert(dst_id);
    }
}

int main(int argc, char **argv)
{

    if (argc != 4)
    {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    read_in_msgs(argv);
    ifstream topofile(argv[1]);
    read_in(topofile);

    make_temp();

    forwarding_table();

    send_msg();

    int src_id, dst_id, cost;

    ifstream changesfile(argv[3]);
    while (changesfile >> src_id >> dst_id >> cost)
    {
        topology[src_id][dst_id] = cost;
        topology[dst_id][src_id] = cost;
        make_temp();
        forwarding_table();
        send_msg();
    }

    fpOut.close();

    return 0;
}
