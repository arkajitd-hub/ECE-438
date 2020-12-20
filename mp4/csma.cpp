#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <queue>
#include <sstream>
#include <vector>
#include <climits>
#include <algorithm>
#include <math.h>
using namespace std;
#include "node.h"

int N;
int L;         
vector<int> R; 
int M;         
int T;         

void split(const string &s, vector<string> &sv, const char flag = ' ')
{
  sv.clear();
  istringstream iss(s);
  string temp;
  while (getline(iss, temp, flag))
  {
    sv.push_back(temp);
  }
  return;
}

void readf(string inputFile)
{
  ifstream in(inputFile); 

  if (!in)
  {
    cout << "Cannot open input file.";
    return ;
  }

  char str[255];
  int index = 0;
  while (in)
  {
    in.getline(str, 255); 
    if (in)
      cout << str << endl;
    vector<string> sv; 
    split(str, sv, ' ');
    if (sv.size() == 0)
      continue;

    string type = sv[0];
    if (type == "N")
    {
      N = stoi(sv[1]);
    }
    else if (type == "L")
    {
      L = stoi(sv[1]);
    }
    else if (type == "R")
    {
      sv.erase(sv.begin());
      vector<int> r;
      for (auto &str : sv)
      {
        r.push_back(stoi(str));
      }
      R = r;
    }
    else if (type == "M")
    {
      M = stoi(sv[1]);
    }
    else if (type == "T")
    {
      T = stoi(sv[1]);
    }
  }

  in.close();
}

bool compare_backoff(Node *a, Node *b)
{
  return a->backoff < b->backoff;
}

void simulate(int *dataset)
{
  int clock_global = T;

  int time_util = 0;
  int collision_tot = 0;

  vector<Node *> nodes;

  for (int i = 0; i < N; i++)
  {
    nodes.push_back(new Node(i, R[0]));
    nodes[i]->setRandom(R);
  }
  while (clock_global > 0)
  {
    sort(nodes.begin(), nodes.end(), compare_backoff);
    int backoff_index = 0;
    int minBackOff = nodes[0]->backoff;
    for (int i = 1; i < nodes.size(); i++)
    {
      if (nodes[i]->backoff != minBackOff)
      {
        backoff_index = i - 1;
        break;
      }
      else
      {
        backoff_index++;
      }
    }
    for (int i = 0; i < nodes.size(); i++)
    {
      nodes[i]->backoff = nodes[i]->backoff - minBackOff;
    }

    if (backoff_index > 0)
    {
      for (int i = 0; i < backoff_index + 1; i++)
      {
        nodes[i]->colisionNum++;
        if (nodes[i]->colisionNum >= M)
        {
          nodes[i]->maximalBackoff = R[0];
          nodes[i]->colisionNum = 0;
          nodes[i]->setRandom(R);
        }
        else
        {
          nodes[i]->maximalBackoff = R[nodes[i]->colisionNum];
          nodes[i]->setRandom(R);
        }

        nodes[i]->total_col += 1;
      }
      collision_tot += (backoff_index + 1);
    }
    else
    {
      nodes[0]->setRandom(R);
      time_util += L;
      clock_global -= L;
      nodes[0]->total_trans += L;
    }
    clock_global -= minBackOff;
  }

  dataset[0] = time_util;
  dataset[1] = clock_global;
  dataset[2] = collision_tot;
  double number_nodes = N;
  double var_col_average = collision_tot /number_nodes;
  double var_trans_average = time_util /number_nodes;
  double var_trans = 0;
  double var_col = 0;
  double totalTime = T;
  double util_rate = time_util / totalTime;
  double variance_trans = 0;
  double variance_col = 0;

  for (int i = 0; i < nodes.size(); i++)
  {
    var_col += (nodes[i]->total_col - var_col_average) * (nodes[i]->total_col - var_col_average);

    var_trans += (nodes[i]->total_trans - var_trans_average) * (nodes[i]->total_trans - var_trans_average);
  }
  var_col /= number_nodes;
  var_trans /= number_nodes;
  variance_col = sqrt(var_col);
  variance_trans = sqrt(var_trans);

  ofstream outputFile("output.txt");

  if (outputFile.is_open()){
    outputFile << "Channel utilization : " << util_rate * 100 << '%' << endl;
    outputFile << "Channel idle fraction : " << (1 - util_rate) * 100 << '%' << endl;
    outputFile << "Total number of collisions : " << collision_tot << endl;
    outputFile << "Variance in number of successful transmissions :" << variance_trans * variance_trans << endl;
    outputFile << "Variance in number of collisions :" << variance_col * variance_col << endl;
  };
  outputFile.close();
}

void writeDataToFile3ABC()
{
  ofstream outputFile3A("3_a.txt");
  ofstream outputFile3B("3_b.txt");
  ofstream outputFile3C("3_c.txt");

  for (int i = 5; i <= 500; i++){
    int dataset[3];
    N = i;
    simulate(dataset);
    if (outputFile3A.is_open())
    {
      outputFile3A << N << " " << 100*dataset[0]/ T << endl;
    }
    if (outputFile3B.is_open())
    {
      outputFile3B << N << " " << 100 - 100*dataset[0]/ T << endl;
    }
    if (outputFile3C.is_open())
    {
      outputFile3C << N << " " << dataset[2] << endl;
    }  
  }
}

void writeDataToFile3D()
{
  for (int i = 0; i < 5; i++)
  {
    ofstream outputFile3D("3_d." + to_string(i) + ".txt");
    R.clear();
    int base = pow(2, i);
    for (int k = 0; k < 6; k++)
    {
      R.push_back(base);
      base *= 2;
    }
    int countzero = 0;
    for (int j = 5; j <= 500; j++)
    {
      int dataset[3];
      N = j;
      if (countzero > 20)
      {
        cout << "nodes num: " << j << "utilization util_rate: " << 0 << "%" << endl;
        if (outputFile3D.is_open())
        {
          outputFile3D << j << ' ' << "0" << endl;
        }
        continue;
      }
      simulate(dataset);
      int percentage = 100 * dataset[0] / T;
      if (percentage == 0)
        countzero++;

      if (outputFile3D.is_open())
      {
        outputFile3D << j << ' ' << 100 * dataset[0] / T << endl;
      }
    }
    
  }
}

void writeDataToFile3E()
{
  for (int i = 3; i < 5; i++)
  {
    ofstream outputFile3E("3_e." + to_string(i) + ".txt");
    L = 20 * (i + 1);
    int countzero = 0;

    for (int j = 5; j <= 500; j++)
    {
      int dataset[3];
      N = j;
      if (countzero > 15)
      {
        cout << "nodes num: " << j << "utilization util_rate: " << 0 << "%" << endl;
        if (outputFile3E.is_open())
        {
          outputFile3E << j << ' ' << "0" << endl;
        }
        continue;
      }
      simulate(dataset);
      int percentage = 100 * dataset[0] / T;
      if (percentage <= 1)
        countzero++;

      if (outputFile3E.is_open())
      {
        outputFile3E << j << ' ' << 100 * dataset[0] / T << endl;
      }
    }
  }
}

int main(int argc, char **argv)
{
  int dataset[3];
  readf(argv[1]);
  simulate(dataset);
  /*writeDataToFile3ABC();
  writeDataToFile3D();
  writeDataToFile3E();*/
  return 0;
}