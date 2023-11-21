#include <stdlib.h>
#include <iostream>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

class InitSock
{
public:
    InitSock(BYTE minorVer=2, BYTE majorVer=2)
    {
        WSADATA wdata;
        WORD VerRequested = MAKEWORD(2,2);
        int err = WSAStartup(VerRequested, &wdata);
        if(err != 0){
            cout<<("WSAStartup failed with error: ")<<err<<endl;
            exit(0);
        }
    }
    ~InitSock()
    {
        WSACleanup();
    }
};