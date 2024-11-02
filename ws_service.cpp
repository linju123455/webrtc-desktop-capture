#include "ws_service.h"

WebSocketSvr::WebSocketSvr()
{
    //set log level
    server_handle_.set_error_channels(websocketpp::log::elevel::none);
    server_handle_.set_access_channels(websocketpp::log::elevel::none);

    //init asio
    server_handle_.init_asio();

    //set callback
    server_handle_.set_open_handler(std::bind(&WebSocketSvr::HandleOpen, this, std::placeholders::_1)); //success open conncet
    server_handle_.set_validate_handler(std::bind(&WebSocketSvr::HandleValidate, this, std::placeholders::_1)); //valid conncet
    server_handle_.set_close_handler(std::bind(&WebSocketSvr::HandleClose, this, std::placeholders::_1)); // close conncet
    server_handle_.set_message_handler(std::bind(&WebSocketSvr::HandleMsg, this, std::placeholders::_1, std::placeholders::_2)); // get client msg
}

WebSocketSvr::~WebSocketSvr()
{

}

int WebSocketSvr::Run(uint16_t port, int thread_count) 
{
    websocketpp::lib::error_code ec;
    server_handle_.set_reuse_addr(true);
    server_handle_.listen(port, ec);
    if (ec) {
        std::cout << "listen failed: " << ec.message() << std::endl;
        return -1;
    }

    server_handle_.start_accept(ec);
    if (ec) {
        std::cout << "start accept failed: " << ec.message() << std::endl;
        return -1;
    }

    for (int a = 0; a < thread_count; a++) {
        threads_.emplace_back(new std::thread([&](){
            server_handle_.run();
        }));
    }

    if (conductor_)
        conductor_->SetWsHandle(&server_handle_);
    return 0;
}

void WebSocketSvr::Stop() 
{
    server_handle_.stop();
    for (auto t : threads_) {
        if (t->joinable()) {
            t->join();
        }
    }
}

void WebSocketSvr::HandleOpen(websocketpp::connection_hdl hdl) 
{
    std::cout << "come in open" << std::endl;
    if (conductor_)
        conductor_->SetSptrHdl(hdl.lock());
    std::cout << "come out open" << std::endl;
}

bool WebSocketSvr::HandleValidate(websocketpp::connection_hdl hdl) 
{
    std::cout << "come in HandleValidate" << std::endl;
    return true;
}

void WebSocketSvr::HandleClose(websocketpp::connection_hdl hdl) 
{
    std::cout << "come in Close" << std::endl;
    if (conductor_)
        conductor_->Close();
    std::cout << "come out Close" << std::endl;
}

void WebSocketSvr::HandleMsg(websocketpp::connection_hdl hdl, WsServer::message_ptr msg) 
{
    std::cout << "come in HandleMsg : " << msg->get_payload() << std::endl;
    if (conductor_)
        conductor_->MsgProcess(msg->get_payload());
    std::cout << "come out HandleMsg." << std::endl;
}