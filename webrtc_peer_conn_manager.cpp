#include "webrtc_peer_conn_manager.h"
#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/writer.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"
#include <regex>

std::string prioritizeH264(const std::string& sdp) {
    std::istringstream stream(sdp);
    std::string line;
    std::string modifiedSdp;

    while (getline(stream, line)) {
        if (line.find("m=video") == 0) {
            line = "m=video 9 UDP/TLS/RTP/SAVPF 126 127 97 98 120 124 121 125";
        }
        modifiedSdp += line + "\r\n";
    }
    return modifiedSdp;
}

std::string formatCandidate(const std::string& original) {
    // 假设 original 格式如：'Cand[:2977391278:1:tcp:1518280447:172.16.41.98:40413:local::0:1Kzk:Tw4yc5x/w6dDDORiNijVzBwO:1:50:0]'
    std::regex regex("Cand\\[:([0-9]+):([0-9]+):(tcp|udp):([0-9]+):([0-9\\.]+):([0-9]+):([a-z]+).*\\]");
    std::smatch match;
    if (std::regex_search(original, match, regex) && match.size() > 7) {
        std::string formatted = "candidate:" + match.str(1) + " " + match.str(2) + " " + match.str(3) +
                                " " + match.str(4) + " " + match.str(5) + " " + match.str(6) + " typ " + match.str(7);
        return formatted;
    }
    return ""; // 返回空字符串，如果无法匹配格式
}

WebRTCPeerConnectionManager::WebRTCPeerConnectionManager()
    : ws_server_(nullptr),
      video_decoder_factory_(absl::make_unique<webrtc::WebRTCVideoDecoderFactory>(&decode_callback_))
{

}

WebRTCPeerConnectionManager::~WebRTCPeerConnectionManager()
{
    if (signaling_thread_)
        signaling_thread_->Stop();
    if (network_thread_)
        network_thread_->Stop();
    if (worker_thread_)
        worker_thread_->Stop();
}

bool WebRTCPeerConnectionManager::Init()
{
    // peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
    //     nullptr /* network_thread */, nullptr /* worker_thread */,
    //     nullptr /* signaling_thread */, nullptr /* default_adm */,
    //     webrtc::CreateBuiltinAudioEncoderFactory(),
    //     webrtc::CreateBuiltinAudioDecoderFactory(),
    //     webrtc::CreateBuiltinVideoEncoderFactory(),
    //     webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
    //     nullptr /* audio_processing */);

    signaling_thread_ = rtc::Thread::Create();
    worker_thread_ = rtc::Thread::Create();
    network_thread_ = rtc::Thread::CreateWithSocketServer();
    signaling_thread_->Start();
    worker_thread_->Start();
    network_thread_->Start();
    webrtc::PeerConnectionFactoryDependencies dependencies;
    dependencies.network_thread = network_thread_.release();
    dependencies.worker_thread = worker_thread_.release();
    dependencies.signaling_thread = signaling_thread_.release();
    dependencies.task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
    dependencies.call_factory = webrtc::CreateCallFactory();
    dependencies.event_log_factory = absl::make_unique<webrtc::RtcEventLogFactory>(dependencies.task_queue_factory.get());

    cricket::MediaEngineDependencies media_dependencies;
    media_dependencies.task_queue_factory = dependencies.task_queue_factory.get();
    media_dependencies.adm = nullptr;
    media_dependencies.audio_encoder_factory = std::move(webrtc::CreateBuiltinAudioEncoderFactory());
    media_dependencies.audio_decoder_factory = std::move(webrtc::CreateBuiltinAudioDecoderFactory());
    media_dependencies.audio_processing = webrtc::AudioProcessingBuilder( ).Create();
    media_dependencies.audio_mixer = nullptr;
    media_dependencies.video_encoder_factory = std::move(webrtc::CreateBuiltinVideoEncoderFactory());
    // media_dependencies.video_decoder_factory = std::move(webrtc::CreateBuiltinVideoDecoderFactory());
    media_dependencies.video_decoder_factory = std::move(video_decoder_factory_);
    dependencies.media_engine = cricket::CreateMediaEngine(std::move(media_dependencies));

    peer_connection_factory_ = webrtc::CreateModularPeerConnectionFactory(std::move(dependencies));
    if (!peer_connection_factory_) {
        std::cout << "peer connection factory create error" << std::endl;
        return false;
    }

    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    config.enable_dtls_srtp = true;

    peer_connection_ = peer_connection_factory_->CreatePeerConnection(config, nullptr, nullptr, this);
    if (!peer_connection_) {
        peer_connection_factory_ = nullptr;
        std::cout << "peer connection create error" << std::endl;
        return false;
    }

    std::cout << "peer connection init" << std::endl;
    return true;
}

void WebRTCPeerConnectionManager::Close()
{
    peer_connection_ = nullptr;
    peer_connection_factory_ = nullptr;
    std::cout << "peer connect close" << std::endl;
}

int WebRTCPeerConnectionManager::MsgProcess(const std::string& msg)
{
    if (!peer_connection_factory_ || !peer_connection_) {
        std::cout << "##############come in init" << std::endl;
        if (!Init()) {
            return -1;
        }
    }

    rapidjson::Document doc;
    if (doc.Parse(msg.c_str()).HasParseError()) {
        std::cout << "msg parse error; " << std::endl;
        return -1;
    }

    if (!doc.HasMember("type") || !doc["type"].IsString()) {
        std::cout << "msg type parse error; " << std::endl;
        return -1;
    }

    rapidjson::Value value;
    if (!doc.HasMember("content") || !doc["content"].IsObject()) {
        std::cout << "msg content parse error; " << std::endl;
        return -1;
    }

    value = doc["content"];

    std::string type = doc["type"].GetString();
    if (type == "sdp") {
        if (!value.HasMember("type") || !value["type"].IsString()) {
            std::cout << "msg content type parse error; " << std::endl;
            return -1;
        }

        std::string content_type = value["type"].GetString();
        std::cout << " content_type is : " << content_type << std::endl;
        if (content_type != "offer") {
            std::cout << "msg content type is not offer; " << std::endl;
            return -1;
        }
        
        if (!value.HasMember("sdp") || !value["sdp"].IsString()) {
            std::cout << "msg content sdp parse error; " << std::endl;
            return -1;
        }

        std::string sdp = value["sdp"].GetString();
        std::cout << " sdp is : " << sdp << std::endl;

        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::SessionDescriptionInterface> session_description
                 = webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp, &error);
        std::cout << "########################finish  CreateSessionDescription " << std::endl;
        if (!session_description) {
            if (!error.description.empty()) {
                std::cerr << "Failed to parse SDP: " << error.description << std::endl;
                std::cerr << "Error at line: " << error.line << std::endl;
            } else {
                std::cerr << "Failed to create session description for unknown reasons." << std::endl;
            }
            return -1;
        }

        peer_connection_->SetRemoteDescription(DummySetSessionDescriptionObserver::Create(), session_description.release());
        std::cout << "########################finish  SetRemoteDescription " << std::this_thread::get_id() << std::endl;
        peer_connection_->CreateAnswer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
        std::cout << "########################finish CreateAnswe " << std::endl;
    } else if (type == "candidate") {
        std::cout << "################come in candidate" << std::this_thread::get_id() << std::endl;
        if (!value.HasMember("candidate") || !value["candidate"].IsString()) {
            std::cout << "msg content candidate parse error; " << std::endl;
            return -1;
        }
        std::string candidate = value["candidate"].GetString();
        
        if (!value.HasMember("sdpMid") || !value["sdpMid"].IsString()) {
            std::cout << "msg content sdpMid parse error; " << std::endl;
            return -1;
        }
        std::string sdp_mid = value["sdpMid"].GetString();

        if (!value.HasMember("sdpMLineIndex") || !value["sdpMLineIndex"].IsInt()) {
            std::cout << "msg content sdpMLineIndex parse error; " << std::endl;
            return -1;
        }
        int sdp_mlineindex = value["sdpMLineIndex"].GetInt();

        webrtc::SdpParseError error;
        std::unique_ptr<webrtc::IceCandidateInterface> candidate_interface(
            webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, candidate, &error));
        if (!candidate_interface.get()) {
            RTC_LOG(WARNING) << "Can't parse received candidate message. "
                            << "SdpParseError was: " << error.description;
            return -1;
        }

        if (!peer_connection_->AddIceCandidate(candidate_interface.get())) {
            RTC_LOG(WARNING) << "Failed to apply the received candidate";
            return -1;
        }
    } else {
        std::cout << "msg type is error; type is : " << type << std::endl;
        return -1;
    }
    return 0;
}

void WebRTCPeerConnectionManager::SendMessage(const std::string& msg)
{
    if (!ws_server_ || !sptr_hdl_) {
        std::cout << "ws handler is null" << std::endl;
        return;
    }
    websocketpp::lib::error_code ec;
    websocketpp::server<websocketpp::config::asio_tls>* ws_server = (websocketpp::server<websocketpp::config::asio_tls>*)ws_server_;
    ws_server->send(websocketpp::connection_hdl(sptr_hdl_), msg, websocketpp::frame::opcode::text, ec);
}

// SetLocalDesp之后触发，带着本端的ice候选发送给对端
void WebRTCPeerConnectionManager::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
    std::cout << "#############come in OnIceCandidate" << std::endl;
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("type");
    writer.String("answer_candidate");
    writer.Key("content");
        writer.StartObject();
        writer.Key("candidate");
        std::string candidate_str;
        candidate->ToString(&candidate_str);
        writer.String(candidate_str.c_str());
        std::cout << "candidate is : " << candidate_str << std::endl;
        writer.Key("sdpMid");
        writer.String(candidate->sdp_mid().c_str());
        writer.Key("sdpMLineIndex");
        writer.Int(candidate->sdp_mline_index());
        writer.EndObject();
    writer.EndObject();

    std::string candidate_json = buffer.GetString();
    SendMessage(candidate_json);
    std::cout << "#############Send local Ice success" << std::endl;
}

// CreateAnswer成功后调用，设置本地描述，然后发给对端
void WebRTCPeerConnectionManager::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
    std::cout << "#############come in SetLocalDescription" << std::endl;
    peer_connection_->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);
    std::string sdp;
    desc->ToString(&sdp);
    // std::string new_sdp = prioritizeH264(sdp);
    std::cout << "my sdp is : " << sdp << std::endl;

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("type");
    writer.String("answer_sdp");
    writer.Key("content");
        writer.StartObject();
        writer.Key("type");
        writer.String("answer");
        writer.Key("sdp");
        writer.String(sdp.c_str());
        writer.EndObject();
    writer.EndObject();

    std::string sdp_json = buffer.GetString();
    SendMessage(sdp_json);
    std::cout << "#############Send local sdp success" << std::endl;
}

// CreateAnswer失败后调用
void WebRTCPeerConnectionManager::OnFailure(webrtc::RTCError error)
{
    std::cout << "################come in OnFailure" << std::endl;
    RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}

void WebRTCPeerConnectionManager::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                            const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&streams)
{
    std::cout << "################come in OnAddTrack" << std::endl;
    // auto track = receiver->track().release();
    // if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
    //     auto video_track = static_cast<webrtc::VideoTrackInterface*>(track);
    //     video_sink_.reset(new VideoSink(video_track));
    // }
    // track->Release();
}

void WebRTCPeerConnectionManager::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    auto track = receiver->track().release();
    track->Release();
}

