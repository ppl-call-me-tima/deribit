#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/context.hpp> 

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

#include <nlohmann/json.hpp>
 
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>
 
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;

using json = nlohmann::json;

class connection_metadata {
private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
    std::vector<std::string> m_messages;

public: 
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;
    typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;
    
    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
      : m_id(id)
      , m_hdl(hdl)
      , m_status("Connecting")
      , m_uri(uri)
      , m_server("N/A")
    {}

    int get_id(){return m_id;}
    websocketpp::connection_hdl get_hdl(){return m_hdl;}
    std::string get_status(){return m_status;}
 
    void on_open(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Open";
 
        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
    }

    void on_close(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Closed";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        std::stringstream s;
        s << "close code: " << con->get_remote_close_code() << " (" 
          << websocketpp::close::status::get_string(con->get_remote_close_code()) 
          << "), close reason: " << con->get_remote_close_reason();
        m_error_reason = s.str();
    }
 
    void on_fail(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Failed";
 
        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
    }

    void record_sent_message(std::string message) {
        m_messages.push_back(">> " + message);
    }

    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
            m_messages.push_back("<< " + msg->get_payload());
        } else {
            m_messages.push_back("<< " + websocketpp::utility::to_hex(msg->get_payload()));
        }
    }
 
    friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);
};
 
std::ostream & operator<< (std::ostream & out, connection_metadata const & data) {
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason)
        << "> Messages Processed: (" << data.m_messages.size() << ") \n";
 
        std::vector<std::string>::const_iterator it;
        for (it = data.m_messages.begin(); it != data.m_messages.end(); ++it) {
            out << *it << "\n";
        }
 
    return out;
}

context_ptr on_tls_init() {
    context_ptr context = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try {
        context->set_options(boost::asio::ssl::context::default_workarounds |
                        boost::asio::ssl::context::no_sslv2 |
                        boost::asio::ssl::context::no_sslv3 |
                        boost::asio::ssl::context::single_dh_use);

    } catch (std::exception &e) {
        std::cout << "Error in context pointer: " << e.what() << std::endl;
    }
    return context;
}

class websocket_endpoint {
private:
    typedef std::map<int,connection_metadata::ptr> con_list;
 
    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;
 
    con_list m_connection_list;
    int m_next_id;

public:
    websocket_endpoint () : m_next_id(0) {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);
 
        m_endpoint.init_asio();
        m_endpoint.start_perpetual();
 
        m_thread.reset(new websocketpp::lib::thread(&client::run, &m_endpoint));
    }

    ~websocket_endpoint() {
        m_endpoint.stop_perpetual();
        
        for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
            if (it->second->get_status() != "Open") {
                continue;
            }
            
            std::cout << "> Closing websocket connection" << std::endl;
            
            websocketpp::lib::error_code ec;
            m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
            if (ec) {
                std::cout << "> Error closing connection " << it->second->get_id() << ": "  
                          << ec.message() << std::endl;
            }
        }
        
        m_thread->join();
    }

    void send(int id, std::string message) {
        websocketpp::lib::error_code ec;
        
        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }
        
        m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
        if (ec) {
            std::cout << "> Error sending message: " << ec.message() << std::endl;
            return;
        }
        
        metadata_it->second->record_sent_message(message);
    }
 
    int connect(std::string const & uri) {
        m_endpoint.set_tls_init_handler(websocketpp::lib::bind(
            &on_tls_init
        ));

        websocketpp::lib::error_code ec;
        client::connection_ptr con = m_endpoint.get_connection(uri, ec);
 
        if (ec) {
            std::cout << "> Connect initialization error: " << ec.message() << std::endl;
            return -1;
        }
 
        int new_id = m_next_id++;
        connection_metadata::ptr metadata_ptr(new connection_metadata(new_id, con->get_handle(), uri));
        m_connection_list[new_id] = metadata_ptr;
 
        con->set_open_handler(websocketpp::lib::bind(
            &connection_metadata::on_open,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_fail_handler(websocketpp::lib::bind(
            &connection_metadata::on_fail,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));

        con->set_close_handler(websocketpp::lib::bind(
            &connection_metadata::on_close,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));

        con->set_message_handler(websocketpp::lib::bind(
            &connection_metadata::on_message,
            metadata_ptr,
            websocketpp::lib::placeholders::_1,
            websocketpp::lib::placeholders::_2
        ));
 
        m_endpoint.connect(con);
 
        return new_id;
    }

    void close(int id, websocketpp::close::status::value code) {
        websocketpp::lib::error_code ec;
        
        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }
        
        m_endpoint.close(metadata_it->second->get_hdl(), code, "", ec);
        if (ec) {
            std::cout << "> Error initiating close: " << ec.message() << std::endl;
        }

        std::cout << "> Connection to WebSocket closed." << std::endl;
    }
 
    connection_metadata::ptr get_metadata(int id) const {
        con_list::const_iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            return connection_metadata::ptr();
        } else {
            return metadata_it->second;
        }
    }
};

void deribit_menu(websocket_endpoint& endpoint);

 
int main() {
    websocket_endpoint endpoint;
 
    int id = endpoint.connect("wss://test.deribit.com/ws/api/v2");
    if (id == -1){
        return -1;
    }
    std::cout << "\nWebSocket connection to Deribit established." << std::endl;
    
    bool done = false;
    std::string input;

    while (!done) {
        std::cout << "Enter Command: ";
        std::getline(std::cin, input);
 
        if (input == "quit") {
            done = true;
        } else if (input == "help") {
            std::cout
                << "\nCommand List:\n"
                << "show: Show connection metadata\n"
                << "deribit: Enter deribit menu\n"
                << "help: Display this help text\n"
                << "quit: Exit the program\n"
                << std::endl;
        } else if (input == "show") {
            int id = 0;
 
            connection_metadata::ptr metadata = endpoint.get_metadata(id);
            if (metadata) {
                std::cout << *metadata << std::endl;
            }
        } else if (input == "deribit"){
            deribit_menu(endpoint);
            std::cout << std::endl;
        } else if (input.substr(0,4) == "send") {
            std::stringstream ss(input);
            
            std::string cmd;
            int id;
            std::string message;
            
            ss >> cmd >> id;
            if (ss.fail()) {
                std::cout << "Error: Invalid ID format" << std::endl;
                return -1;
            }
            
            ss.ignore();
            std::getline(ss, message);
            
            if (message.empty()) {
                std::cout << "Error: Empty message" << std::endl;
                return -1;
            }
            
            endpoint.send(id, message);
        } else {
            std::cout << "> Unrecognized Command" << std::endl;
        }
    }
 
    return 0;
}

void deribit_menu(websocket_endpoint &endpoint) {
    bool done = false;
    std::string input;

    std::cout << std::endl;

    while (!done){
        std::cout << "Enter deribit command: ";
        std::getline(std::cin, input);

        if (input == "quit") {
            std::cout << "> Your are leaving deribit menu" << std::endl;
            done = true;
        } else if (input == "help") {
            std::cout
                << "\nDeribit command list:\n"
                << "auth <cilent_id> <client_secret>\n"
                << "help: Show deribit command list\n"
                << "quit: Exit deribit menu\n"
                << std::endl;
        } if (input.substr(0,4) == "auth") {
            // implement auth
        } else{
            std::cout << "> Unrecognised Command" << std::endl;
        }
    }
}