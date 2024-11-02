#include <iostream>
#include <execinfo.h>
#include "ws_service.h"
#include "wss_service.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/ssl_identity.h"

// WebSocketSvr svr;
WebSocketSecureSvr svr;

void ProgExit(int signo)
{
    std::cout << "come in prog exit" << std::endl;
    svr.Stop();
    rtc::CleanupSSL();
    exit(0);
}

void errorCb(int signo)
{
    std::cout << "come in errorCb" << std::endl;
    // void *array[10];
    // size_t size;
    // char **strings;

    // // 获取栈帧的地址
    // size = backtrace(array, 10);

    // // 打印出所有栈帧的符号
    // strings = backtrace_symbols(array, size);
    // std::cerr << "Error: program terminating:\n";
    // for (size_t i = 0; i < size; i++)
    //     std::cerr << strings[i] << '\n';

    // free(strings);
    exit(-1);
}

int main(void) 
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGKILL, ProgExit);
    signal(SIGINT, ProgExit);
    signal(SIGTERM, ProgExit);
    signal(SIGSEGV, errorCb);
    signal(SIGABRT, errorCb);

    rtc::InitializeSSL();

    rtc::scoped_refptr<WebRTCPeerConnectionManager> conductor(new rtc::RefCountedObject<WebRTCPeerConnectionManager>());
    svr.SetConductor(conductor);
    svr.SetTLSFiles("/home/linju/webrtc/linux/cert.pem", "/home/linju/webrtc/linux/key.pem");
    svr.Run(20080, 4);

    while (1) {
       std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}