#include <iostream>
#include <windows.h>
#include <time.h>
using namespace std;
uint16_t flag = 0;
#define max_t CLOCKS_PER_SEC*0.1

int main(){
    clock_t start = clock();
    flag = 0x2;
    cout<<(flag&0x20)<<(flag&0x10)<<(flag&0x8)<<(flag&0x4)<<(flag&0x2)<<(flag&0x1)<<endl;
    int size = 4096;
    int sz = 100;
    cout<<size/sz<<endl;
    Sleep(100);
    if(clock()-start>max_t){
        cout<<"yes"<<endl;
    }
}