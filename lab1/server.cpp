#include "initsock.h"
#include <iostream>
using namespace std;
InitSock sock;

int ServerPort = 1234;
const int buffer_sz = 1024; 
const int listenMAX = 5; //最大连接数
int connectedclient = 0;

SOCKET ServerSocket;
SOCKET ClientSockets[listenMAX];
sockaddr_in ServerAddr;
sockaddr_in ClientAddr[listenMAX];
int addr_len = sizeof(ClientAddr);
HANDLE ClientThreads[listenMAX];

char sendbuf[buffer_sz];
char recvbuf[buffer_sz];

struct clientinfo{
    int id;
    char user_name[128];
};
clientinfo Client_info[listenMAX];

DWORD WINAPI recv_message(LPVOID para){
    clientinfo* client = (clientinfo*) para;
    int client_id = client->id;
    char client_name[128];
    strcpy(client_name, client->user_name);
    while(1){
        int len_recv = recv(ClientSockets[client_id], recvbuf, buffer_sz, 0);
        if(len_recv > 0){
            if(recvbuf[0]=='q' && recvbuf[1]=='u' && recvbuf[2]=='i' && recvbuf[3]=='t'){ // 用户退出
                memset(sendbuf, 0, buffer_sz);
                strcat(sendbuf, client_name);
                strcat(sendbuf, " left the chat room.");
                cout<<sendbuf<<endl<<endl;
                connectedclient--;
                for(int i=0; i<listenMAX;i++){
                    if(i==client_id || ClientSockets[i]==INVALID_SOCKET)
                        continue;
                    else send(ClientSockets[i], sendbuf, buffer_sz, 0);
                }
                closesocket(ClientSockets[client_id]);
                break;
            }
            else{
                char* timestr = (char*)malloc(8*sizeof(char));
                int idx = 0;
                for(; idx < 8; idx++){
                    timestr[idx] = recvbuf[idx];
                }
                char content[buffer_sz];
                int idx2 = 0;
                while(recvbuf[idx]!='\0'){
                    content[idx2] = recvbuf[idx];
                    idx++; idx2++;
                }
                content[idx2] = '\0';
                memset(sendbuf, 0, buffer_sz);
                strcat(sendbuf, client_name);
                strcat(sendbuf, " ");
                strcat(sendbuf, timestr);
                strcat(sendbuf, "\n");
                strcat(sendbuf, content);
                cout<<sendbuf<<endl<<endl;
                for(int i=0; i<listenMAX;i++){
                    if(i==client_id || ClientSockets[i]==INVALID_SOCKET)
                        continue;
                    else send(ClientSockets[i], sendbuf, buffer_sz, 0);
                }
            }
        }
    }
}

int main()
{
    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(ServerSocket == INVALID_SOCKET){
        cout<<"ServerSocket is invalid."<<endl;
        return -1;
    }
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(ServerPort);
    ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;

    if(bind(ServerSocket, (LPSOCKADDR)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR){
        cout<<"Bind failed."<<endl;
        return -1;
    }
    if(listen(ServerSocket, listenMAX) == SOCKET_ERROR){
        cout<<"Listen failed."<<endl;
        return -1;
    } else cout<<"Waiting for connection..."<<endl;

    while(1){
        int i=0;
        for(;i<listenMAX;i++){
            if(ClientSockets[i]==0)
                break;
        }
        ClientSockets[i] = accept(ServerSocket, (SOCKADDR*)&ClientAddr[i], &addr_len);
        if(ClientSockets[i] == INVALID_SOCKET){
            cout<<"Connect failed."<<endl;
        }
        else{
            connectedclient++;
            Client_info[i].id = i;
            cout<<"Accept a connection: "<<inet_ntoa(ClientAddr[i].sin_addr)<<", Port: "<<htons(ClientAddr[i].sin_port)<<endl;
            memset(recvbuf, 0, buffer_sz);
            recv(ClientSockets[i], recvbuf, buffer_sz, 0);
            int idx = 0;
            while(recvbuf[idx]!='\0'){
                Client_info[i].user_name[idx] = recvbuf[idx];
                idx++;
            }
            recvbuf[idx] = '\0';
            ClientThreads[i] = CreateThread(NULL, NULL, recv_message, &Client_info[i], 0, NULL);
            if(ClientThreads[i] == 0){
                cout<<"Create client thread failed."<<endl;
                connectedclient--;
                return -1;
            }
            memset(sendbuf, 0, buffer_sz);
            strcat(sendbuf, Client_info[i].user_name);
            strcat(sendbuf, " entered the chat room.");
            cout<<sendbuf<<endl<<endl;
            for(int j=0; j<listenMAX;j++){
                if(j==i || ClientSockets[j]==INVALID_SOCKET)
                    continue;
                else send(ClientSockets[j], sendbuf, buffer_sz, 0);
            }
        }
    }

    closesocket(ServerSocket);

    return 0;
}