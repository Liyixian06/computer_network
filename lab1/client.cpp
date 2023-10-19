#include "initsock.h"
#include <iostream>
#include <ctime>
using namespace std;
InitSock sock;

int ServerPort = 1234;
int ClientPort = 5678;
const int buffer_sz = 1024; 

SOCKET LocalSocket;
sockaddr_in LocalAddr, RemoteAddr;
int addr_len = sizeof(RemoteAddr);
HANDLE RecvThread;

char sendbuf[buffer_sz];
char recvbuf[buffer_sz];
char input[buffer_sz];

char* current_time(){
    time_t now = time(nullptr);
    tm* t = localtime(&now);
    string hour = to_string(t->tm_hour);
    if (hour.size() < 2) hour = "0" + hour;
    string minute = to_string(t->tm_min);
    if (minute.size() < 2) minute = "0" + minute;
    string second = to_string(t->tm_sec);
    if (second.size() < 2) second = "0" + second;
    string tmp = hour + ":" + minute + ":" + second;
    char* res = (char*)tmp.data();
    return res;
}

DWORD WINAPI recv_message(LPVOID){
    while(1){
        int len_recv = recv(LocalSocket, recvbuf, buffer_sz, 0);
        if(len_recv > 0){
            cout<<recvbuf<<endl<<endl;
            memset(recvbuf, 0, buffer_sz);
        }
    }
}

int main()
{
    LocalSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(LocalSocket == INVALID_SOCKET){
        cout<<"LocalSocket is invalid."<<endl;
        return -1;
    }
    //LocalAddr.sin_family = AF_INET;
    //LocalAddr.sin_port = htons(ClientPort);
    //LocalAddr.sin_addr.S_un.S_addr = inet_addr("169.254.153.161");

    RemoteAddr.sin_family = AF_INET;
    RemoteAddr.sin_port = htons(ServerPort);
    //RemoteAddr.sin_addr.S_un.S_addr = inet_addr("169.254.153.161");
    RemoteAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    if(connect(LocalSocket, (sockaddr*)&RemoteAddr, addr_len)== -1){
        cout<<"Connect to server failed."<<endl;
        return -1;
    }
    cout<<"Connected to server successfully."<<endl;
    RecvThread = CreateThread(NULL, NULL, recv_message, NULL, 0, NULL);
    if(RecvThread == 0){
        cout<<"Create client receive thread failed."<<endl;
        return -1;
    }
    
    cout<<"Please input your user name (anonymous by default): ";
    char user_name[buffer_sz];
    cin.getline(user_name, buffer_sz);
    memset(sendbuf, 0, buffer_sz);
    if(strlen(user_name)!=0)
        strcat(sendbuf, user_name);
    else strcat(sendbuf, "anonymous");
    send(LocalSocket, sendbuf, buffer_sz, 0);
    cout<<"Welcome to the chat room! "<<endl;
    cout<<"You can input 'quit' to quit the room anytime."<<endl<<endl;

    while(1){
        memset(input, 0, buffer_sz);
        cin.getline(input, buffer_sz);
        cout<<endl;
        if(strcmp(input, "quit")==0){
            send(LocalSocket, input, buffer_sz, 0);
            cout<<"bye."<<endl;
            closesocket(LocalSocket);
            return 0;
        }
        else{
            memset(sendbuf, 0, buffer_sz);
            char* now = current_time();
            strcat(sendbuf, now);
            strcat(sendbuf, input);
            send(LocalSocket, sendbuf, buffer_sz, 0);
        }
    }
    
    return 0;
}