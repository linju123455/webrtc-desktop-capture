/*================================================================
*   Copyright (c) 2024, Inc. All Rights Reserved.
*   
*   @file：wss_service.h
*   @author：linju
*   @email：15013144713@163.com
*   @date ：2024-05-20
*   @berief： websocket secure service code
*
================================================================*/
#pragma once
#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include "webrtc_peer_conn_manager.h"

typedef websocketpp::server<websocketpp::config::asio_tls> WssServer;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> WssContextPtr;
class WebSocketSecureSvr
{
public:
    WebSocketSecureSvr();
    virtual ~WebSocketSecureSvr();

public:
    int SetTLSFiles(const char* certificate_path, const char* private_key_path);

    void SetConductor(WebRTCPeerConnectionManager* conductor) { conductor_ = conductor; }

    int Run(uint16_t port, int thread_count);

    void Stop();

protected:
    void HandleOpen(websocketpp::connection_hdl hdl);

    bool HandleValidate(websocketpp::connection_hdl hdl);

    void HandleClose(websocketpp::connection_hdl hdl);

    void HandleMsg(websocketpp::connection_hdl hdl, WssServer::message_ptr msg);
    
    WssContextPtr HandleTLSInit(websocketpp::connection_hdl hdl);

private:
    WssServer server_handle_;
    std::vector<std::shared_ptr<std::thread>> threads_;
    std::string certificate_path_;
    std::string private_key_path_;

    WebRTCPeerConnectionManager* conductor_;
};