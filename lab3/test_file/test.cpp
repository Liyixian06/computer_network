#include <iostream>
using namespace std;
uint16_t flag = 0;

int main(){
    flag = 0x2;
    cout<<(flag&0x20)<<(flag&0x10)<<(flag&0x8)<<(flag&0x4)<<(flag&0x2)<<(flag&0x1)<<endl;
    int size = 4096;
    int sz = 100;
    cout<<size/sz<<endl;
}