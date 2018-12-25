#include<iostream>
#include<stdio.h>
#include<string.h>
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include<unistd.h>
#include<ctime>
#include "Test.pb.h"
using namespace std;
const int BUFFSIZE = 65536;

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    //server_reply: buffer to parse; server_reply_temp: buffer received with socket interface recv
    char server_reply[BUFFSIZE], server_reply_temp[BUFFSIZE];
    char *p_server_reply = server_reply;

    //Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
    }
    puts("Socket created");
    cout << "socket is: " << sock << endl;

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);

    //Connect to remote server
    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("connect failed. Error");
        return 1;
    }

    puts("Connected\n");

    int count = 0;
    int sum_recv = 0;
    //Loop to receive data from server
    while (1) {
        testprotobuf::HeartInfo myprotobuf;
        if (0 == count || 2 == count) {
            myprotobuf.set_curtime(123);
            myprotobuf.set_hostip("166.0.0.1");
            for (int j = 0; j < 3; ++j) {
                testprotobuf::HeartInfo::MapStmsiEntry *SinMap = myprotobuf.add_mapstmsi();
                SinMap->set_stmsi(555);
                SinMap->set_times(1);
            }
            //protobuf的序列化方式之一
            char buff[BUFFSIZE];
            myprotobuf.SerializeToArray(buff, BUFFSIZE);
            //Send some data to server
            if (send(sock, buff, strlen(buff), 0) < 0) {
                puts("Send failed");
                return 1;
            }
        }

        //Receive a reply from the server
        int irecv = recv(sock, server_reply_temp, BUFFSIZE, 0);
        if (irecv < 0) {
            puts("recv failed");
            break;
        }
        //copy the received data to server_reply for parsing
        std::copy(server_reply_temp, server_reply_temp + irecv, server_reply + sum_recv);
        sum_recv += irecv;

        //Loop to parse the data from server
        while (1) {
            const void *msg_len_void = server_reply;
            uint32_t msg_len_int = *static_cast<const uint32_t *>(msg_len_void);
            uint32_t msg_len = ntohl(msg_len_int);
            cout << "client recved msg_len is: " << msg_len << endl;
            if (myprotobuf.ParseFromArray(server_reply + sizeof(int), msg_len)) {
                cout << "prase success" << endl;
                //prase success, print the server reply message
//                cout << "server reply: last heart time:" << myprotobuf.curtime() << "\t"
//                     << "host ip:" << myprotobuf.hostip() << "myprotobuf.mapstmsi size is: "
//                     << myprotobuf.mapstmsi_size() << endl;
//                for (int i = 0; i < myprotobuf.mapstmsi_size(); ++i) {
//                    testprotobuf::HeartInfo::MapStmsiEntry onemapdata = myprotobuf.mapstmsi(i);
//                    cout << onemapdata.stmsi() << " ;" << onemapdata.times() << endl;
//                }
                sum_recv = sum_recv - (msg_len + 4);
                p_server_reply + msg_len + 4;
                break;
            } else {
                cout << "prase failed" << endl;
                break;
            }
        }
        count++;
        if (count == 3600) {
            break;
        }
    }

    close(sock);
    return 0;
}
