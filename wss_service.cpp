#include "wss_service.h"

WebSocketSecureSvr::WebSocketSecureSvr()
{
    //set log level
    server_handle_.set_error_channels(websocketpp::log::elevel::none);
    server_handle_.set_access_channels(websocketpp::log::elevel::none);

    //init asio
    server_handle_.init_asio();

    //set callback
    server_handle_.set_open_handler(std::bind(&WebSocketSecureSvr::HandleOpen, this, std::placeholders::_1)); //success open conncet
    server_handle_.set_validate_handler(std::bind(&WebSocketSecureSvr::HandleValidate, this, std::placeholders::_1)); //valid conncet
    server_handle_.set_close_handler(std::bind(&WebSocketSecureSvr::HandleClose, this, std::placeholders::_1)); // close conncet
    server_handle_.set_message_handler(std::bind(&WebSocketSecureSvr::HandleMsg, this, std::placeholders::_1, std::placeholders::_2)); // get client msg
    server_handle_.set_tls_init_handler(std::bind(&WebSocketSecureSvr::HandleTLSInit, this, std::placeholders::_1));// tls init
}

WebSocketSecureSvr::~WebSocketSecureSvr()
{

}

int WebSocketSecureSvr::SetTLSFiles(const char* certificate_path, const char* private_key_path)
{
    if (!certificate_path || !private_key_path) {
        std::cout << "invalid args" << std::endl;
        return -1;
    }
    certificate_path_ = certificate_path;
    private_key_path_ = private_key_path;

    return 0;
}

int WebSocketSecureSvr::Run(uint16_t port, int thread_count)
{
    websocketpp::lib::error_code ec;
    server_handle_.set_reuse_addr(true);
    server_handle_.listen(port, ec);
    if (ec) {
        std::cout << "listen failed: " << ec.message() << std::endl;
        return -1;
    }
    printf("000000000000\n");

    server_handle_.start_accept(ec);
    if (ec) {
        std::cout << "start accept failed: " << ec.message() << std::endl;
        return -1;
    }

    printf("11111111111111\n");
    for (int a = 0; a < thread_count; a++) {
        threads_.emplace_back(new std::thread([&](){
            server_handle_.run();
        }));
    }
    printf("22222222222222222\n");

    if (conductor_)
        conductor_->SetWsHandle(&server_handle_);
    return 0;
}

void WebSocketSecureSvr::Stop() 
{
    server_handle_.stop();
    for (auto t : threads_) {
        if (t->joinable()) {
            t->join();
        }
    }
}

void WebSocketSecureSvr::HandleOpen(websocketpp::connection_hdl hdl) 
{
    std::cout << "come in wss open" << std::endl;
    if (conductor_)
        conductor_->SetSptrHdl(hdl.lock());
    std::cout << "come out wss open" << std::endl;
}

bool WebSocketSecureSvr::HandleValidate(websocketpp::connection_hdl hdl) 
{
    WssServer::connection_ptr conn = server_handle_.get_con_from_hdl(hdl);
    conn->append_header("cross_origin_embedder_policy", "require-corp");
    conn->append_header("cross_origin_opener_policy", "same-origin");
    return true;
}

void WebSocketSecureSvr::HandleClose(websocketpp::connection_hdl hdl) 
{
    std::cout << "come in wss Close" << std::endl;
    if (conductor_)
        conductor_->Close();
    std::cout << "come out wss Close" << std::endl;
}

void WebSocketSecureSvr::HandleMsg(websocketpp::connection_hdl hdl, WssServer::message_ptr msg) 
{
    std::cout << "come in wss HandleMsg : " << msg->get_payload() << std::endl;
    if (conductor_)
        conductor_->MsgProcess(msg->get_payload());
    std::cout << "come out wss HandleMsg." << std::endl;
}

WssContextPtr WebSocketSecureSvr::HandleTLSInit(websocketpp::connection_hdl hdl) 
{
    std::cout << "come in wss HandleTLSInit" << std::endl;
    boost::system::error_code ec;
    printf("oooooooooooooooooooooooo\n");
    WssContextPtr ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(websocketpp::lib::asio::ssl::context::sslv23);
    printf("ppppppppppppppppp\n");
    ctx->set_options(websocketpp::lib::asio::ssl::context::default_workarounds |
        websocketpp::lib::asio::ssl::context::no_sslv2 |
        websocketpp::lib::asio::ssl::context::no_sslv3 |
        websocketpp::lib::asio::ssl::context::single_dh_use, ec);
    if (ec) {
        std::cout << "set tls options failed: " << ec.message() << std::endl;
        return nullptr;
    }
    printf("zzzzzzzzzzzzzzzzzzzz\n");
    //verify_none不验证证书，任何证书都接受
    ctx->set_verify_mode(websocketpp::lib::asio::ssl::context::verify_none, ec);
    if (ec) {
        std::cout << "set tls verify mode failed: " << ec.message() << std::endl;
        return nullptr;
    }
    printf("aaaaaaaaaaaaaaaaa\n");
    ctx->use_private_key_file(private_key_path_, websocketpp::lib::asio::ssl::context::pem, ec);
    if (ec) {
        std::cout << "set tls private key file failed: " << ec.message() << std::endl;
        return nullptr;
    }
    printf("bbbbbbbbbbbbbbbbbbbbb\n");
    ctx->use_certificate_chain_file(certificate_path_, ec);
    if (ec) {
        std::cout << "set tls certificate chain file failed: " << ec.message() << std::endl;
        return nullptr;
    }
    std::cout << "come out wss HandleTLSInit" << std::endl;
    return ctx;
}

