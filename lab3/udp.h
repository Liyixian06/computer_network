#include <iostream>
#include <stdlib.h>
#include <string.h>
using namespace std;
const unsigned int MAX_LENGTH = 4096;

class Header
{
public:
    uint16_t datasize = 0;
    uint16_t sum = 0;
    uint16_t flag = 0; // END START OVER FIN ACK SYN
    uint16_t ack = 0;
    uint16_t seq = 0;
    Header(){}
    Header(uint16_t datasize, uint16_t sum, uint16_t flag, uint16_t ack, uint16_t seq) : datasize(datasize),sum(sum),flag(flag),ack(ack),seq(seq){}
    void set(uint16_t datasize, uint16_t sum, uint16_t flag, uint16_t ack, uint16_t seq){
        this->datasize = datasize;
        this->sum = sum;
        this->flag = flag;
        this->ack = ack;
        this->seq = seq;
    }
    bool isSYN(){return this->flag & 0x1;}
    bool isACK(){return this->flag & 0x2;}
    bool isFIN(){return this->flag & 0x4;}
    bool isOVER(){return this->flag & 0x8;}
    bool isSTART(){return this->flag & 0x10;}
    bool isEND(){return this->flag & 0x20;}
};

const uint16_t SYN = 0x1; // FIN = 0 ACK = 0 SYN = 1
const uint16_t ACK = 0x2; // FIN = 0 ACK = 1 SYN = 0
const uint16_t SYN_ACK = 0x3; // FIN = 0 ACK = 1 SYN = 1
const uint16_t FIN = 0x4; // FIN = 1 ACK = 0 SYN = 0
const uint16_t FIN_ACK = 0x6; // FIN = 1 ACK = 1 SYN = 0
const uint16_t OVER = 0x8; // 最后一个包
const uint16_t START = 0x10; // 第一个包，内容是文件名
const uint16_t END = 0x20; // 连接结束

class Packet
{
public:
    Header header;
    char buffer[MAX_LENGTH] = "";
    Packet(){}
    Packet(Header& header) : header(header){}
    void set(Header& _header, char* data_segment, int size){
        header = _header;
        memcpy(buffer, data_segment, size);
    }
};
const int packet_length = sizeof(Packet);

// 计算校验和
uint16_t check_sum(uint16_t* pa, int size){
    int count = (size+1)/2;
    uint16_t* buf = (uint16_t*)malloc(size);
    memset(buf, 0, size);
    memcpy(buf, pa, size);
    unsigned long sum = 0;
    while(count--){
        sum += *buf++;
        if (sum & 0xffff0000) {
            sum &= 0xffff;
            sum++;
        }
    }
    return ~(sum & 0xffff);
}

void print_log(Packet& pa){
    cout<<"size: "<<pa.header.datasize<<endl;
    cout<<"seq: "<<pa.header.seq<<" ack: "<<pa.header.ack<<endl;
    if(pa.header.flag!=0x0){
        string printflag = "flags: ";
        if(pa.header.isEND()) printflag += "END ";
        if(pa.header.isSTART()) printflag += "START ";
        if(pa.header.isOVER()) printflag += "OVER ";
        if(pa.header.isFIN()) printflag += "FIN ";
        if(pa.header.isACK()) printflag += "ACK ";
        if(pa.header.isSYN()) printflag += "SYN ";
        cout<<printflag<<endl;
    }
    cout<<"checksum: "<<pa.header.sum<<endl<<endl;
}