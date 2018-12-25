#include<iostream>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include <fcntl.h>
#include"Test.pb.h"

using namespace std;
const int BUFFSIZE = 65536;


/*server message to client:
 * Header+message
 * Header(length is sizeof(int)) indicate the message length
 * message is the information*/

void *connection_handler(void *);

void prepend(char *pbuff, const void *data, size_t len) {
    const char *d = static_cast<const char *>(data);
    std::copy(d, d + len, pbuff);
}

void prependInt32(char *pbuff, int32_t x) {
    uint32_t new_x = htonl(x);
    prepend(pbuff, &new_x, sizeof new_x);
}

#define exit_if(r, ...)                                                                          \
    if (r) {                                                                                     \
        printf(__VA_ARGS__);                                                                     \
        printf("%s:%d error no: %d error msg %s\n", __FILE__, __LINE__, errno, strerror(errno)); \
        exit(1);                                                                                 \
    }

int main(int argc, char *argv[]) {
    int socket_desc, client_sock, c, *new_sock;
    struct sockaddr_in server, client;

    //Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }
    puts("Socket created");

    int optval = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    //server.sin_addr.s_addr = inet_addr("192.168.1.149");
//    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(8888);

    //Bind
    if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");

    //Listen
    listen(socket_desc, 3);
    c = sizeof(struct sockaddr_in);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while (1) {
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if(client_sock == -1)
        {
            perror("server accept failed");
            break;
        }
        sockaddr_in peer;
        socklen_t alen = sizeof(peer);
        int r = getpeername(client_sock, (sockaddr *) &peer, &alen);
        exit_if(r < 0, "getpeername failed");
        printf("accept a connection from %s\n", inet_ntoa(client.sin_addr));
        pthread_t sniffer_thread;
        new_sock = &client_sock;

        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*)new_sock) < 0)
        {
            perror("could not create thread");
            return NULL;
        }


//        (client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t *) &c))
//        puts("Connection accepted");
//        pthread_t sniffer_thread;
//        new_sock = (int *) malloc(1);
//        *new_sock = client_sock;
//
//        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void *) new_sock) < 0) {
//            perror("could not create thread");
//            return 1;
//        }
//
//        //Now join the thread , so that we dont terminate before the thread
//        pthread_join(sniffer_thread, NULL);
//        puts("Handler assigned");
    }

    if (client_sock < 0) {
        perror("accept failed");
        return 1;
    }

    return 0;
}

/*This will handle connection for each client*/
void *connection_handler(void *socket_desc) {
    //Get the socket descriptor
    int sock = *(int *) socket_desc;

    //set socket to be no blocking
    int flags = fcntl(sock, F_GETFL, 0);
    exit_if(flags < 0, "fcntl failed");
    int r = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    exit_if(r < 0, "fcntl failed");

    int read_size;
    char client_message[2000];
    int count = 0;
    //Receive a message from client
    while (1) {
        testprotobuf::HeartInfo myprotobuf;
        //receive data from client
        if ((read_size = recv(sock, client_message, 2000, 0)) > 0) {
            //protobuf反序列化
            myprotobuf.ParseFromArray(client_message, 2000);
            //print the mesage from client
//            cout<<"last heart time:"<<myprotobuf.curtime()<<"\t"
//                <<"host ip:"<<myprotobuf.hostip()<<endl;
//            for(int i = 0; i < myprotobuf.mapstmsi_size(); ++i)
//            {
//                testprotobuf::HeartInfo::MapStmsiEntry onemapdata = myprotobuf.mapstmsi(i);
//                cout << onemapdata.stmsi() <<" ;"<< onemapdata.times() << endl;
//            }
        }

        if (0 == read_size) {
            //client disconnected
            cout << "client disconnected..." << endl;
            break;
        }

        //Send the message back to client
        if (0 == count) {
            myprotobuf.clear_curtime();
            myprotobuf.set_curtime(456);
            myprotobuf.clear_hostip();
            myprotobuf.set_hostip("192.0.0.1");
        } else {
            myprotobuf.clear_curtime();
            myprotobuf.set_curtime(456789);
            myprotobuf.clear_hostip();
            myprotobuf.set_hostip("10.11.12.13");
        }

        myprotobuf.clear_mapstmsi();
        for (int j = 0; j < 5000; ++j) {
            testprotobuf::HeartInfo::MapStmsiEntry *SinMap = myprotobuf.add_mapstmsi();
            SinMap->set_stmsi(666);
            SinMap->set_times(2);
        }

        char buff[BUFFSIZE];
        char *pbuff = buff;
        int iSend = 0;
        int iLeft = myprotobuf.ByteSize() + 4;
        prependInt32(pbuff, myprotobuf.ByteSize());
        myprotobuf.SerializeToArray(buff + 4, iLeft - 4);

        int sendtimes = 0;
        while (iLeft > 0) {
            if ((iSend = send(sock, pbuff + iSend, iLeft, 0)) < 0) {
                cout << "server send error: " << errno << endl;
                break;
            } else if (iSend == 0) {
                continue;
            }

            sendtimes++;
            iLeft -= iSend;
            if (iSend == -1) {
                cout << "server send error: " << errno << endl;
            }
        }
        count++;
        cout << "send total sendsize is: " << iSend << endl;
        sleep(1);
    }
    close(sock);
    return 0;
}
