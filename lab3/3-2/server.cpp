#include "initsock.h"
#include "udp.h"
#include "time.h"
#include <iostream>
#include <fstream>
using namespace std;
InitSock sock;

SOCKET ServerSocket;
sockaddr_in ServerAddr;
const char* IP = "127.0.0.1";
int ServerPort = 1234;
sockaddr_in ClientAddr;
int ClientAddrSize = sizeof(ClientAddr);
int ClientPort = 5678;

const int Max_time = 0.2*CLOCKS_PER_SEC;
const int Max_filesz = 1024*1024*20; // 20MB
bool quit = false;
uint16_t seq_num = 0; // 最后一个确认收到的packet的seq
string output_dir = "./output/";

bool Connect()
{
    Packet connect;
    char* connect_buf = new char[packet_length];
    int res;

    // 接收第一次握手信息
    while(1){
        res = recvfrom(ServerSocket, connect_buf, packet_length, 0, (SOCKADDR*)&ClientAddr, &ClientAddrSize);
        if(res == SOCKET_ERROR){
            cout<<"first handshake failed."<<endl;
            return 0;
        }
        memcpy(&connect, connect_buf, packet_length);
        if((connect.header.flag == SYN) && (connect.header.seq == 0xFFFF) && check_sum((uint16_t*)&connect, packet_length)==0){
            cout<<"first handshake finished."<<endl;
            break;
        }
    }

    u_long mode = 1;
    ioctlsocket(ServerSocket, FIONBIO, &mode); // 设置为非阻塞模式

    // 发送第二次握手信息
    memset(&connect, 0, packet_length);
    memset(connect_buf, 0, packet_length);
    connect.header.flag = SYN_ACK;
    connect.header.seq = 0xFFFF;
    connect.header.ack = 0x0;
    connect.header.sum = check_sum((uint16_t*)&connect, packet_length);
    memcpy(connect_buf, &connect, packet_length);
    res = sendto(ServerSocket, connect_buf, packet_length, 0, (SOCKADDR*)&ClientAddr, ClientAddrSize);
    if(res == SOCKET_ERROR){
        cout<<"second handshake failed."<<endl;
        return 0;
    }
    clock_t start = clock();

    // 接收第三次握手信息，超时重传
    while(recvfrom(ServerSocket, connect_buf, packet_length, 0, (SOCKADDR*)&ClientAddr, &ClientAddrSize)<=0){
        if(clock()-start > Max_time){
            cout<<"second handshake timed out. Resending..."<<endl;
            res = sendto(ServerSocket, connect_buf, packet_length, 0, (SOCKADDR*)&ClientAddr, ClientAddrSize);
            if(res == SOCKET_ERROR){
                cout<<"second handshake failed."<<endl;
                return 0;
            }
            start = clock();
        }
    }
    cout<<"second handshake finished."<<endl;

    memcpy(&connect, connect_buf, packet_length);
    if((connect.header.flag == ACK) && (connect.header.seq == 0x0) && (connect.header.ack == 0x0) && check_sum((uint16_t*)&connect, packet_length)==0){
        cout<<"third handshake finished."<<endl;
    }
    else{
        cout<<"third handshake failed."<<endl;
        return 0;
    }

    mode = 0;
    ioctlsocket(ServerSocket, FIONBIO, &mode); // 阻塞模式
    delete[] connect_buf;
    return 1;
}

bool Disconnect()
{
    Packet disconnect;
    char* disconnect_buf = new char[packet_length];
    int res;

    // 接收第一次挥手信息
    while(1){
        res = recvfrom(ServerSocket, disconnect_buf, packet_length, 0, (SOCKADDR*)&ClientAddr, &ClientAddrSize);
        if(res == SOCKET_ERROR){
            cout<<"first handwave failed."<<endl;
            return 0;
        }
        memcpy(&disconnect, disconnect_buf, packet_length);
        if((disconnect.header.flag == FIN) && (disconnect.header.seq == 0xFFFF) && check_sum((uint16_t*)&disconnect, packet_length)==0){
            cout<<"first handwave finished."<<endl;
            break;
        }
    }

    u_long mode = 1;
    ioctlsocket(ServerSocket, FIONBIO, &mode); // 非阻塞模式

    // 发送第二次挥手信息
    memset(&disconnect, 0, packet_length);
    memset(disconnect_buf, 0, packet_length);
    disconnect.header.flag = ACK;
    disconnect.header.seq = 0x0;
    disconnect.header.ack = 0x0;
    disconnect.header.sum = check_sum((uint16_t*)&disconnect, packet_length);
    memcpy(disconnect_buf, &disconnect, packet_length);
    res = sendto(ServerSocket, disconnect_buf, packet_length, 0, (SOCKADDR*)&ClientAddr, ClientAddrSize);
    if(res == SOCKET_ERROR){
        cout<<"second handwave failed."<<endl;
        return 0;
    }
    cout<<"second handwave finished."<<endl;
    
    // 发送第三次挥手信息
    memset(&disconnect, 0, packet_length);
    memset(disconnect_buf, 0, packet_length);
    disconnect.header.flag = FIN_ACK;
    disconnect.header.seq = 0xFFFF;
    disconnect.header.sum = check_sum((uint16_t*)&disconnect, packet_length);
    memcpy(disconnect_buf, &disconnect, packet_length);
    res = sendto(ServerSocket, disconnect_buf, packet_length, 0, (SOCKADDR*)&ClientAddr, ClientAddrSize);
    if(res == SOCKET_ERROR){
        cout<<"third handwave failed."<<endl;
        return 0;
    }
    clock_t start = clock();

    // 接收第四次挥手信息，超时重传
    while(recvfrom(ServerSocket, disconnect_buf, packet_length, 0, (SOCKADDR*)&ClientAddr, &ClientAddrSize)<=0){
        if(clock()-start > Max_time){
            cout<<"third handwave timed out. Resending..."<<endl;
            res = sendto(ServerSocket, disconnect_buf, packet_length, 0, (SOCKADDR*)&ClientAddr, ClientAddrSize);
            if(res == SOCKET_ERROR){
                cout<<"third handwave failed."<<endl;
                return 0;
            }
            start = clock();
        }
    }
    cout<<"third handwave finished."<<endl;

    memcpy(&disconnect, disconnect_buf, packet_length);
    if((disconnect.header.flag == ACK) && (disconnect.header.seq == 0x0) && (disconnect.header.ack == 0x0) && check_sum((uint16_t*)&disconnect, packet_length)==0){
        cout<<"fourth handwave finished."<<endl;
    }
    else{
        cout<<"fourth handwave failed."<<endl;
        return 0;
    }

    delete[] disconnect_buf;
    return 1;
}

void send_ack(uint16_t recv_seq){
    Packet pa_ack;
    char* buf = new char[packet_length];
    pa_ack.header.set(0, 0, ACK, recv_seq+1, seq_num);
    pa_ack.header.sum = check_sum((uint16_t*)&pa_ack, packet_length);
    memcpy(buf, &pa_ack, packet_length);
    int res = sendto(ServerSocket, buf, packet_length, 0, (SOCKADDR*)&ClientAddr, ClientAddrSize);
    if(res == SOCKET_ERROR) cout<<"send ack failed."<<endl;
    else{
        cout<<"[Send]"<<endl;
        cout<<"seq: "<<pa_ack.header.seq<<" ack: "<<pa_ack.header.ack<<endl<<endl;
    }
    delete[] buf;
}

void recv_file(){
    char* file_content = new char[Max_filesz];
    string filename = "";
    long offset = 0;
    int res;
    while(1){
        char* recv_buf = new char[packet_length];
        Packet recv;
        res = recvfrom(ServerSocket, recv_buf, packet_length, 0, (SOCKADDR*)&ClientAddr, &ClientAddrSize);
        if(res == SOCKET_ERROR){
            cout<<"recv error."<<endl;
        }else {
            memcpy(&recv, recv_buf, packet_length);
            // 检查检验和，若有错则直接将该数据包丢弃，等待对面超时重传
            if(check_sum((uint16_t*)&recv, packet_length)!=0){
                cout<<"This packet's check sum is wrong. waiting for resend."<<endl<<endl;
                continue;
            }
            // 传错了，就重传最后一个确认收到的 ack
            if(recv.header.seq!=seq_num && recv.header.seq!=seq_num+1){
                cout<<"[Recv]"<<endl;
                print_log(recv);
                cout<<"NOTICE: something is wrong with this packet. waiting for resend."<<endl<<endl;
                send_ack(seq_num);
                continue;
            }
            // 第一个包，内容是文件名
            if(recv.header.isSTART()){
                filename = recv.buffer;
                cout<<"file name: "<<filename<<endl<<endl;
                cout<<"[Recv]"<<endl;
                print_log(recv);
                send_ack(recv.header.seq);
                //seq_num++;
            }
            // 最后一个包，这个文件全部发送完毕
            else if(recv.header.isOVER()){
                memcpy(file_content + offset, recv.buffer, recv.header.datasize);
                //offset += recv.header.datasize;
                cout<<"[Recv]"<<endl;
                print_log(recv);
                ofstream fout(output_dir + filename, ofstream::binary);
                fout.write(file_content, offset);
                fout.close();
                send_ack(recv.header.seq);
                cout<<"file size: "<<offset<<endl;
                cout<<"file "<<filename<<" received."<<endl<<endl;
                delete[] recv_buf;
                break;
            }
            // client 结束连接
            else if(recv.header.isEND()){
                quit = true;
                cout<<"Preparing to quit..."<<endl;
                delete[] recv_buf;
                break;
            }
            // 文件发送中
            else {
                memcpy(file_content + offset, recv.buffer, recv.header.datasize);
                if(recv.header.seq == seq_num+1){
                    seq_num++;
                    offset += recv.header.datasize;
                }
                cout<<"[Recv]"<<endl;
                print_log(recv);
                send_ack(recv.header.seq);
                //seq_num++;
            }

        }
        delete[] recv_buf;
    }
    
    seq_num = 0; // 每次收到一个文件，就把seq置零
    delete[] file_content;
}

int main()
{
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(ServerPort);
    ServerAddr.sin_addr.S_un.S_addr = inet_addr(IP);
    ClientAddr.sin_family = AF_INET;
    ClientAddr.sin_port = htons(ClientPort);
    ClientAddr.sin_addr.S_un.S_addr = inet_addr(IP);

    ServerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(ServerSocket == INVALID_SOCKET){
        cout<<"ServerSocket is invalid."<<endl;
        return -1;
    }
    if(bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR){
        cout<<"Bind failed."<<endl;
        return -1;
    }
    cout<<"Waiting for connection..."<<endl;

    if(Connect()){
        cout<<"Connection built succeeded. You can start transfer data now."<<endl;
    }
    else {
        cout<<"Connection built failed."<<endl;
        return -1;
    }

    while(!quit){
        recv_file();
    }

    if(Disconnect()){
        cout<<"Disconnect succeeded."<<endl;
    }
    else{
        cout<<"Disconnect failed."<<endl;
    }

    closesocket(ServerSocket);
    cout<<"Exiting..."<<endl;
    system("pause");
    return 0;
}