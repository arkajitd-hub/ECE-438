/* 
 * File:   receiver_main.c
 * Author: 
 *
 * Created on
 */

#include <iostream>
#include <string>
#include <map>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

struct sockaddr_in si_me, si_other;
int s, slen;

using namespace std;

void diep(char *s)
{
    perror(s);
    exit(1);
}

int convertToInt(char *a)
{
    int i = 0;
    int num = 0;
    while (a[i] != 0)
    {
        num = (a[i] - '0') + (num * 10);
        i++;
    }
    return num;
    ;
}

void reliablyReceive(unsigned short int myUDPport, char *destinationFile)
{

    slen = sizeof(si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
        diep("bind");

    map<long long int, string> buffer_map;
    long long int ACKsum = -1;
    long long int Seqn = 0;

    char buf[1472];
    memset(buf, 0, sizeof(buf));
    FILE *fp = fopen(destinationFile, "wb");

    struct sockaddr_in their_addr;
    int numbytes = 0;
    socklen_t their_addr_size = sizeof(their_addr);

    while (1)
    {
        if ((numbytes = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&si_other, (socklen_t *)&slen)) < 0)
        {
            diep("Finished: receive ends!");
        }
        printf("received bytes: %d", numbytes);
        unsigned long long int currentSeqN;
        char ack_len_buf[1];
        char seqNarray[7];
        memset(seqNarray, 0, sizeof(seqNarray));
        char contentBuf[1472];
        memset(contentBuf, 0, sizeof(contentBuf));
        // memcpy(&currentSeqN, buf, 1);
        memcpy(ack_len_buf, buf, 1);
        int ack_len = convertToInt(ack_len_buf);

        memcpy(seqNarray, buf + 1, ack_len);

        currentSeqN = convertToInt(seqNarray);
        cout << "\n"
             << "buffer:"
             << buf
             << endl;
        cout << "ack_len:"
             << ack_len
             << endl;
        cout << "sequence:"
             << seqNarray
             << endl;
        memcpy(contentBuf, buf + 8, numbytes - 8);
        string contentStr = contentBuf;
        char finack[] = "FINISHED_ACK";
        if (strcmp(buf, finack) == 0) //fin_ack
        {
            cout << "received FINACK"
                 << "\n";
            break;
        }

        printf("sequence number: %llu", currentSeqN);

        if (currentSeqN > Seqn && buffer_map[currentSeqN].length() == 0)
        {
            buffer_map[currentSeqN] = contentStr;
            long long int duplicateACK = Seqn - 1;
            char ackChars[40];
            sprintf(ackChars, "%lld", duplicateACK);
            string temp = ackChars;
            sendto(s, ackChars, temp.length(), 0, (struct sockaddr *)&si_other, slen);
        }
        else if (currentSeqN == Seqn)
        {
            fwrite(contentBuf, sizeof(char), numbytes - 8, fp);
            fflush(fp);
            unsigned long long int nextPktIdx = currentSeqN + 1;
            while (buffer_map[nextPktIdx].length() != 0)
            {
                fwrite(buffer_map[nextPktIdx].c_str(), 1, buffer_map[nextPktIdx].length(), fp);
                fflush(fp);
                map<long long int, string>::iterator seaRch = buffer_map.find(nextPktIdx);
                buffer_map.erase(seaRch);
                nextPktIdx++;
            }
            printf("buffered packets have been delivered");
            ACKsum = nextPktIdx - 1;
            Seqn = nextPktIdx;

            char ackChars[40];
            sprintf(ackChars, "%lld", ACKsum);
            string temp = ackChars;
            sendto(s, ackChars, temp.length(), 0, (struct sockaddr *)&si_other, slen);
        }
        else if (currentSeqN < Seqn)
        {
            char ackChars[40];
            sprintf(ackChars, "%lld", currentSeqN);
            string temp = ackChars;
            sendto(s, ackChars, temp.length(), 0, (struct sockaddr *)&si_other, slen);
            printf("Previous ACK: %llu", currentSeqN);
        }
    }

    close(s);
    printf("%s received.", destinationFile);
    return;
}

/*
 * 
 */
int main(int argc, char **argv)
{

    unsigned short int udpPort;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int)atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}
