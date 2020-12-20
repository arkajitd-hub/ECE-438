/* 
 * File:   sender_main.c
 * Author: 
 *
 * Created on 
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <time.h>
#include <errno.h>
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
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#define TIMEOUTSEC 50
#define WINDOW_SIZE 200
#define BUFFER_SIZE 1472
#define HEADER_SIZE 40
#define SLOW_START 8000
#define CONG_AVOID 8001
#define FAST_RECOV 8002

struct sockaddr_in si_other;
int s, slen;
int current_ack;
char ack_buffer[HEADER_SIZE];
float cw;
int ssthresh = 64;
int base = 0;
int ack = 0;
int dup_ack_counter = 0;
int message_size = 1464;
int ackflag = SLOW_START;

using namespace std;

void diep(char *s)
{
    perror(s);
    exit(1);
}

void intToByte(int n, char *result)
{

    result[0] = n & 0x000000ff;
    result[1] = n & 0x0000ff00 >> 8;
    result[2] = n & 0x00ff0000 >> 16;
    result[3] = n & 0xff000000 >> 24;
    result[4] = 0x00;
    result[5] = 0x00;
    result[6] = 0x00;
    result[7] = 0x00;
}

int convertToInt(char *myarray)
{
    int i;
    sscanf(myarray, "%d", &i);
    return i;
}

string fileToString(char *filename)
{
    ifstream t(filename);
    stringstream t_buffer;
    t_buffer << t.rdbuf();
    return t_buffer.str();
}

template <typename T>
std::string to_string2(const T &value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

int ackChecker(int ack)
{
    switch (ack > base - 1)
    {
    case 1:
        dup_ack_counter = 0;
        if (cw < ssthresh)
        {
            //slow start phase
            cw++;
            ackflag = SLOW_START;
        }
        else
        {
            //congestion avoid phase
            cw += 1.0 / cw;
            ackflag = CONG_AVOID;
        }
        base = ack;
        break;
    case 0:
        if (ack < base - 1)
        {
            cout << "ignore the ack since it is less than the base we are looking for" << endl;
            break;
        }

        dup_ack_counter++;
        if (dup_ack_counter == 3) // we got 3 duplicate acks.
        {
            ssthresh = cw / 2;
            cw = ssthresh + 3;
            ackflag = FAST_RECOV;
        }
        else if (dup_ack_counter > 3)
        {
            cw++;
        }
        break;
    default:
        cout << "we should never get here" << endl;
    }
}

void reliablyTransfer(char *hostname, unsigned short int hostUDPport, char *filename, unsigned long long int bytesToTransfer)
{
    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        printf("Could not open file to send.");
        exit(1);
    }

    int bytesTrans = 0;
    string file_contents = fileToString(filename);
    bytesToTransfer = min<unsigned int>(bytesToTransfer, file_contents.length());

    /* Determine how many bytes to transfer */

    slen = sizeof(si_other);

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *)&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    struct timeval t;
    t.tv_sec = 0;
    t.tv_usec = TIMEOUTSEC * 1000;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(struct timeval)) < 0)
    {
        diep("error setting time for socket");
    }

    int sentBytes, recv;

    /* Send data and receive acknowledgements on s*/

    // string packet = "0000"
    //     //NOTE: add a case for when the syn is noticed and then move on to the actual message
    // }

    //code for sending the packet to the receiver
    cw = 1.0;
    unsigned long long int packet_number = 0;
    //the while loop checks if bytes transfered has not exceeded the bytes that need to be transferred.
    while (bytesTrans < bytesToTransfer)
    {
        int lastpacket = base + ((int)cw);
        //w indexes the base of the window to length cw. packet 1, packet 2, packet 3.
        for (int w = base; w < lastpacket; w++)
        {
            char n[7];
            char m[1];
            string nstring;
            memset(n, 0, sizeof n);
            memset(m, 0, sizeof 1);
            intToByte(packet_number, n);
            string number(n);
            char message_buffer[1464];
            char packet_buffer[1472];
            intToByte(packet_number, packet_buffer);
            memset(message_buffer, 0, sizeof message_buffer);
            memset(packet_buffer, 0, sizeof packet_buffer);
            cout << "packet_buffer: " << packet_buffer << "\n"
                 << endl;
            string message = file_contents.substr(w * message_size, message_size);
            strcpy(message_buffer, message.c_str());
            // sprintf(packet_buffer, "%s", message_buffer);
            memset(packet_buffer, '-', 8);
            nstring = to_string2(packet_number);
            strcpy(n, nstring.c_str());
            size_t p_length = strlen(n);
            sprintf(m, "%d", p_length);

            memcpy(packet_buffer, m, 1);
            memcpy(packet_buffer + 1, nstring.c_str(), strlen(nstring.c_str()));

            cout << "packet_number" << nstring.c_str() << endl;

            memcpy((packet_buffer + 8), message_buffer, 1464);

            if ((sentBytes = sendto(s, packet_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&si_other, slen)) == -1)
            {
                diep((char *)"send");
            }
            cout << "packet_buffer after sending: " << packet_buffer << "\n"
                 << endl;
            bytesTrans += sentBytes - sizeof(unsigned long long int);
            if (w * message_size > bytesToTransfer)
            {
                break;
            }
            packet_number++;
        }

        cout << "ack: " << ack << "lastpakcet: " << lastpacket
             << "\n";
        while (ack < lastpacket)
        {
            if ((recv = recvfrom(s, ack_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&si_other, (socklen_t *)&slen)) == -1)
            {
                // if the socket is blocked or there is no data available
                if (errno != EAGAIN || errno != EWOULDBLOCK)
                {
                    perror("recvfrom");
                    exit(1);
                }
                ssthresh = cw / 2;
                cw = 1;
                dup_ack_counter = 0;
                break;
            }
            ack = convertToInt(ack_buffer);
            int flag = ackChecker(ack);
            cout << flag << endl;
        }
    }

    //send the fin ack when done
    string finpacket = "FINISHED_ACK";
    if ((sentBytes = sendto(s, finpacket.data(), BUFFER_SIZE, 0, (struct sockaddr *)&si_other, slen)) == -1)
    {
        diep((char *)"send");
    }
    cout << "sent fin ack:  " << finpacket.data() << "\n";

    printf("Closing the socket\n");
    close(s);
    return;
}

/*
 * 
 */
int main(int argc, char **argv)
{

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5)
    {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int)atoi(argv[2]);
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);

    return (EXIT_SUCCESS);
}
