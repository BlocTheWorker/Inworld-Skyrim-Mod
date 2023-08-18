#include <nlohmann/json.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

// pull out the type of messages sent by our config
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_client> client;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

class InworldSocketController {
public:
    client::connection_ptr con;
    client c;
    RE::Actor* conversationActor;

    InworldSocketController() {
        // Set up the connection parameters
        std::string uri = "ws://127.0.0.1:" + std::to_string(getClientPort()) + "/chat";

        try {
            // set logging policy if needed
            c.clear_access_channels(websocketpp::log::alevel::frame_header);
            c.clear_access_channels(websocketpp::log::alevel::frame_payload);

            c.init_asio();

            c.set_message_handler(bind(&on_message, &c, ::_1, ::_2));

            websocketpp::lib::error_code ec;
            con = c.get_connection(uri, ec);
            c.connect(con);

            this->start_connection();
        } catch (const std::exception& e) {
            std::cout << e.what() << std::endl;
        } catch (websocketpp::lib::error_code e) {
            std::cout << e.message() << std::endl;
        } catch (...) {
            std::cout << "other exception" << std::endl;
        }
    }

    int getClientPort() {
        auto mainPath = std::filesystem::current_path();
        auto clientPath = mainPath / "Inworld" / ".env";
        std::ifstream envFile(clientPath);  // Open the environment file for reading
        std::string line;
        int clientPort = 3000;  // Default value if CLIENT_PORT is not found
        while (std::getline(envFile, line)) {                         // Read each line in the file
            if (line.contains("CLIENT_PORT")) {     // Check if the line contains the desired variable
                std::size_t pos = line.find("=");                     // find position of equals sign
                std::string port = line.substr(pos + 1);  // extract substring after equals sign
                clientPort = std::stoi(port);                         // Convert the value to an int
                break;  // Stop reading the file once the variable is found
            }
        }
        envFile.close();  // Close the file
        return clientPort;
    }

    void start_connection() {
        std::thread ws_thread(&InworldSocketController::run, this);
        ws_thread.detach();
    }

    void run() {
        // The WebSocket server connection will be started in a separate thread
        c.run();
    }

    void send_message(const char* type, std::string message, const char* id) {
        // Send a JSON message to the server
        json messageJson = {{"type", type}, {"message", message}, {"id", id}};
        std::string message_str = messageJson.dump();
        c.send(con->get_handle(), message_str, websocketpp::frame::opcode::text);
    }

    static void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
        try {
            json j = json::parse(msg->get_payload());

            std::string message = j["message"];
            std::string phoneme = j["phoneme"];
            std::string type = j["type"];

            if (type == "established") {
                InworldCaller::ShowReplyMessage("Character is now listening you");
            } else if (type == "chat") {
                InworldCaller::DoGameVisuals(phoneme, message);
            } else if (type == "doesntexist") {
                InworldCaller::ShowReplyMessage(message);
            }
        } 
        catch (...) 
        {
        }
    }
};

class SocketManager {
private:
    InworldSocketController* soc;
    const char* lastConnected;
    SocketManager() {}

    SocketManager(const SocketManager&) = delete;
    SocketManager& operator=(const SocketManager&) = delete;

public:
    static SocketManager& getInstance() {
        static SocketManager instance;
        return instance;
    }

    void initSocket() { 
        soc = new InworldSocketController();
    }

    void sendMessage(std::string message, RE::Actor* conversationActor) {
        auto id = conversationActor->GetName();
        InworldCaller::conversationActor = conversationActor;

        if (lastConnected != id) {
            lastConnected = id;
            soc->send_message("connect", "connect request..", id);
        }
        soc->send_message("message", message, id);
    }

    void ValidateSocket() { 
        if (soc == nullptr || soc->con == nullptr) {
            soc = new InworldSocketController();
        }
    }

    void controlVoiceInput(bool talk, RE::Actor* conversationActor) {
        try {
            ValidateSocket();
            auto id = conversationActor->GetName();
            
            if (id == nullptr || id == "") return;
            if (lastConnected != id) return;
            InworldCaller::conversationActor = conversationActor;
            if (talk)
                soc->send_message("start_listen", "start", lastConnected);
            else
                soc->send_message("stop_listen", "stop", lastConnected);
        } catch (...) {
            // Ignore
        }
    }

    void connectTo(RE::Actor* conversationActor) {
        ValidateSocket();
        auto id = conversationActor->GetName();
        if (id == nullptr || id == "") return;
        if (lastConnected == id) return;
        InworldCaller::conversationActor = conversationActor;
        lastConnected = id;
        soc->send_message("connect", "connect", id);
    }
};