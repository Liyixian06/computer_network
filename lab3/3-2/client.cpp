#include "initsock.h"
#include "udp.h"
#include "time.h"
#include <iostream>
#include <fstream>
#include <queue>
using namespace std;
InitSock sock;

SOCKET ClientSocket;
sockaddr_in ClientAddr;
const char* IP = "127.0.0.1";
int ClientPort = 5678;
sockaddr_in ServerAddr;
int ServerPort = 1234;
int ServerAddrSize = sizeof(ServerAddr);

const int Max_time = 0.5*CLOCKS_PER_SEC;
uint16_t seq_num = 0;
int cwnd = 5;
uint16_t last_ack = 0; // 最后一个收到确认的packet的ack
uint16_t nxt = 1; // 下一个发送的 packet 的序列号
bool resend = false;
clock_t start;
long filesz = 0;
string file_dir = "../test_file/";

bool Connect()
{
    Packet connect;
    char* connect_buf = new char[packet_length];
    int res;

    u_long mode = 1;
    ioctlsocket(ClientSocket, FIONBIO, &mode); // 设置为非阻塞模式

    // 发送第一次握手信息
    connect.header.flag = SYN;
    connect.header.seq = 0xFFFF;
    connect.header.sum = check_sum((uint16_t*)&connect, packet_length);
    memcpy(connect_buf, &connect, packet_length);
    res = sendto(ClientSocket, connect_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, ServerAddrSize);
    if(res == SOCKET_ERROR){
        cout<<"first handshake failed."<<endl;
        return 0;
    }
    clock_t start = clock();

    // 接收第二次握手信息，超时重传
    while(recvfrom(ClientSocket, connect_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, &ServerAddrSize)<=0){
        if(clock()-start > Max_time){
            cout<<"first handshake timed out. Resending..."<<endl;
            res = sendto(ClientSocket, connect_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, ServerAddrSize);
            if(res == SOCKET_ERROR){
                cout<<"first handshake failed."<<endl;
                return 0;
            }
            start = clock();
        }
    }
    cout<<"first handshake finished."<<endl;

    memcpy(&connect, connect_buf, packet_length);
    if((connect.header.flag == SYN_ACK) && (connect.header.seq == 0xFFFF) && (connect.header.ack == 0x0) && check_sum((uint16_t*)&connect, packet_length)==0){
        cout<<"second handshake finished."<<endl;
    }
    else {
        cout<<"second handshake failed."<<endl;
        return 0;
    }

    // 发送第三次握手信息
    memset(&connect, 0, packet_length);
    memset(connect_buf, 0, packet_length);
    connect.header.flag = ACK;
    connect.header.seq = 0x0;
    connect.header.ack = 0x0;
    connect.header.sum = check_sum((uint16_t*)&connect, packet_length);
    memcpy(connect_buf, &connect, packet_length);
    res = sendto(ClientSocket, connect_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, ServerAddrSize);
    if(res == SOCKET_ERROR){
        cout<<"third handshake failed."<<endl;
        return 0;
    }
    cout<<"third handshake finished."<<endl;

    delete[] connect_buf;
    return 1;
}

bool Disconnect()
{
    Packet disconnect;
    char* disconnect_buf = new char[packet_length];
    int res;

    // 发送第一次挥手信息
    disconnect.header.flag = FIN;
    disconnect.header.seq = 0xFFFF;
    disconnect.header.sum = check_sum((uint16_t*)&disconnect, packet_length);
    memcpy(disconnect_buf, &disconnect, packet_length);
    res = sendto(ClientSocket, disconnect_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, ServerAddrSize);
    if(res == SOCKET_ERROR){
        cout<<"first handwave failed."<<endl;
        return 0;
    }
    clock_t start = clock();

    // 接收第二次挥手信息，超时重传
    while(recvfrom(ClientSocket, disconnect_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, &ServerAddrSize)<=0){
        if(clock()-start > Max_time){
            cout<<"first handwave timed out. Resending..."<<endl;
            res = sendto(ClientSocket, disconnect_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, ServerAddrSize);
            if(res == SOCKET_ERROR){
                cout<<"first handwave failed."<<endl;
                return 0;
            }
            start = clock();
        }
    }
    cout<<"first handwave finished."<<endl;

    memcpy(&disconnect, disconnect_buf, packet_length);
    if((disconnect.header.flag == ACK) && (disconnect.header.seq == 0x0) && (disconnect.header.ack == 0x0) && check_sum((uint16_t*)&disconnect, packet_length)==0){
        cout<<"second handwave finished."<<endl;
    }
    else {
        cout<<"second handwave failed."<<endl;
        return 0;
    }

    // 接收第三次挥手信息
    res = recvfrom(ClientSocket, disconnect_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, &ServerAddrSize);
    if(res == SOCKET_ERROR){
        cout<<"third handwave failed."<<endl;
        return 0;
    }
    memcpy(&disconnect, disconnect_buf, packet_length);
    if((disconnect.header.flag == FIN_ACK) && (disconnect.header.seq == 0xFFFF) && check_sum((uint16_t*)&disconnect, packet_length)==0){
        cout<<"third handwave finished."<<endl;
    }
    else {
        cout<<"third handwave failed."<<endl;
        return 0;
    }

    // 发送第四次挥手信息
    memset(&disconnect, 0, packet_length);
    memset(disconnect_buf, 0, packet_length);
    disconnect.header.flag = ACK;
    disconnect.header.seq = 0x0;
    disconnect.header.ack = 0x0;
    disconnect.header.sum = check_sum((uint16_t*)&disconnect, packet_length);
    memcpy(disconnect_buf, &disconnect, packet_length);
    res = sendto(ClientSocket, disconnect_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, ServerAddrSize);
    if(res == SOCKET_ERROR){
        cout<<"fourth handwave failed."<<endl;
        return 0;
    }
    cout<<"fourth handwave finished."<<endl;

    delete[] disconnect_buf;
    return 1;
}

void send_packet(Packet& pa)
{
    char* send_buf = new char[packet_length];
    memcpy(send_buf, &pa, packet_length);    
    
    int res = sendto(ClientSocket, send_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, ServerAddrSize);
    if(res == SOCKET_ERROR){
        cout<<"send error."<<endl;
    }
    cout<<"[Send]"<<endl;
    print_log(pa);
    seq_num++;
    
    delete[] send_buf;
}

DWORD WINAPI send_file(LPVOID para)
{
    string* filename = (string*) para;
    ifstream fin(file_dir + (*filename).c_str(), ifstream::binary);
    char* file_buf = new char[filesz];
    fin.read(&file_buf[0], filesz);
    fin.close();

    int packet_num = filesz/MAX_LENGTH + 1; // 计算需要多少个包
    cout<<"file name: "<<*filename<<" file size: "<<filesz<<endl;
    cout<<"This file will be transfered in "<<packet_num<<" packets."<<endl<<endl;

    char* name = (char*)(*filename).data();
    int name_sz = strlen(name);
    // 第一个包，内容是文件名
    Header send_header;
    Packet send;
    send_header.set(name_sz, 0, START, 0, 0);
    send.set(send_header, name, name_sz);
    send.header.sum = check_sum((uint16_t*)&send, packet_length);
    send_packet(send); // 发送第一个包

    start = clock();
    char* file_pos = file_buf;

    while(last_ack <= packet_num){ // 还没有收到所有ack
        if(nxt <= packet_num && nxt <= last_ack + cwnd - 1){
            if(nxt == packet_num){ // 最后一个包，要标记OVER，另外注意包的大小
                send_header.set(filesz - (nxt-1)*MAX_LENGTH, 0, OVER, 0, nxt);
                send.set(send_header, file_pos, filesz - (nxt-1)*MAX_LENGTH);
                send.header.sum = check_sum((uint16_t*)&send, packet_length);
            }
            else {
                send_header.set(MAX_LENGTH, 0, 0, 0, nxt);
                send.set(send_header, file_pos, MAX_LENGTH);
                send.header.sum = check_sum((uint16_t*)&send, packet_length);
            }
            send_packet(send);
            nxt++;
            file_pos = file_buf + (nxt-1)*MAX_LENGTH;
        }
        
        if(resend){ // 超时重传
            cout<<"Timed out. resending..."<<endl;
            nxt = last_ack;
            file_pos = file_buf + (nxt-1)*MAX_LENGTH;
            cout<<"nxt = "<<last_ack<<endl<<endl;
            resend = false;
            start = clock();
        }
        
    }

    cout<<"file is successfully sent."<<endl<<endl;
    delete[] file_buf;
}

DWORD WINAPI recv_ack(LPVOID)
{
    int packet_num = filesz/MAX_LENGTH + 1;
    Packet recv;
    char* recv_buf = new char[packet_length];
    while(last_ack <= packet_num){
        while(recvfrom(ClientSocket, recv_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, &ServerAddrSize)<=0){
            if(clock()-start > Max_time){ // 超时重发
                resend = true;
            }
        }
        start = clock();
        memcpy(&recv, recv_buf, packet_length);
        if(recv.header.flag == ACK && check_sum((uint16_t*)&recv, packet_length)==0){
            if(recv.header.ack >= last_ack+1){ // 收到正确的ack，窗口向前滑动
                cout<<"[Recv]"<<endl;
                cout<<"seq: "<<recv.header.seq<<" ack: "<<recv.header.ack<<endl;
                last_ack = recv.header.ack;
                cout<<"last ack = "<<last_ack<<endl;
                int right_border = last_ack + cwnd - 1 <= packet_num? last_ack + cwnd - 1 : packet_num;
                cout<<"send window: ["<<last_ack<<","<<right_border<<"]"<<endl<<endl;
            }
            else if (recv.header.ack == last_ack){ // 忽视重复的ack
                cout<<"repetitive ack: "<<recv.header.ack<<endl<<endl;
            }
        }
    }

    delete[] recv_buf;
}

void multithread_GBN(string filename)
{
    last_ack = 0;
    nxt = 1;

    ifstream fin(file_dir + filename.c_str(), ifstream::binary);
    fin.seekg(0, ifstream::end);
    long size = fin.tellg();
    filesz = size;
    fin.seekg(0);

    HANDLE GBN_threads[2];
    GBN_threads[0] = CreateThread(NULL, NULL, send_file, &filename, 0, NULL);
    GBN_threads[1] = CreateThread(NULL, NULL, recv_ack, NULL, 0, NULL);

    WaitForMultipleObjects(2, GBN_threads, true, INFINITE);
    CloseHandle(GBN_threads[0]);
    CloseHandle(GBN_threads[1]);
}

int main()
{
    ClientAddr.sin_family = AF_INET;
    ClientAddr.sin_port = htons(ClientPort);
    ClientAddr.sin_addr.S_un.S_addr = inet_addr(IP);
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(ServerPort);
    ServerAddr.sin_addr.S_un.S_addr = inet_addr(IP);

    ClientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(ClientSocket == INVALID_SOCKET){
        cout<<"ClientSocket is invalid."<<endl;
        return -1;
    }
    if(bind(ClientSocket, (SOCKADDR*)&ClientAddr, sizeof(ClientAddr)) == SOCKET_ERROR){
        cout<<"Bind failed."<<endl;
        return -1;
    }
    if(Connect()){
        cout<<"Connection built succeeded. You can start transfer data now."<<endl;
    }
    else {
        cout<<"Connection built failed."<<endl;
        return -1;
    }

    cout<<"You can input 'quit' to disconnect."<<endl;
    cout<<"Please set a send window size: ";
    cin>>cwnd;
    while(1){
        string input;
        cout<<"Please input the file name you want to send: ";
        cin>>input;
        cout<<endl;
        if(input == "quit"){
            char* send_buf = new char[packet_length];
            Packet lastpa;
            lastpa.header.flag = END;
            lastpa.header.sum = check_sum((uint16_t*)&lastpa, packet_length);
            memcpy(send_buf, &lastpa, packet_length);
            if(sendto(ClientSocket, send_buf, packet_length, 0, (SOCKADDR*)&ServerAddr, ServerAddrSize) == SOCKET_ERROR){
                cout<<"quit command sent failed."<<endl;
                return -1;
            }
            else {
                cout<<"Preparing to quit..."<<endl;
                delete[] send_buf;
                break;
            }
        }
        else {
            clock_t start = clock();
            multithread_GBN(input);
            clock_t end = clock();
            cout<<"Transfer Time: "<< (float)(end-start)/CLOCKS_PER_SEC <<"s"<<endl;
            cout<<"Average Throughput: "<< (float)filesz / (end-start) <<"bytes/ms"<<endl<<endl;
        }
    }

    if(Disconnect()){
        cout<<"Disconnect succeeded."<<endl;
    }
    else{
        cout<<"Disconnect failed."<<endl;
    }

    closesocket(ClientSocket);
    cout<<"Exiting..."<<endl;
    system("pause");
    return 0;
}