//#define BOOST_BEAST_ALLOW_DEPRECATED
//g++ pkgserver.cpp -lboost_system -lpthread -o pkgs
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <future>
#include <memory>
#include <string>
#include <fstream>
#include <thread>
#include <vector>
#include <iostream>
#include <zephyr/email.hpp>

#include <zephyr/pkg.hpp>
extern "C"{
    #include <sibe/ibe.h>
    #include <sibe/ibe_progs.h>
}
CONF_CTX *cnfctx;
params_t params;

namespace beast = boost::beast;         
namespace http = beast::http;           
namespace websocket = beast::websocket; 
namespace net = boost::asio;         
using tcp = boost::asio::ip::tcp;  
std::string serilized_parameters;
PKG p;

std::string from;
std::string from_password;

void send_email(std::string email, std::string code){
    Email e;
	int curlError = 0;
	// e.dump();

	e.setTo(email);
	e.setFrom(from);
	e.setSubject("Zephyr Authentication");
	e.setCc("");
	e.setBody("Enter this " + code);

	e.setSMTP_host("smtps://smtp.gmail.com:465");
	e.setSMTP_username(from);
	e.setSMTP_password(from_password);

	//e.addAttachment("junk.txt");
	// e.addAttachment("email.h");
	// e.addAttachment("main.cpp");

	e.constructEmail();
	e.dump();

	curlError = e.send();

	if (curlError){
		std::cout << "Error sending email!" << std::endl;
	}

	else{
		std::cout << "Email sent successfully!" << std::endl;
	}
}


// Generate a random string to send to email
void gen_random(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::multi_buffer buffer_;

public:
    // Take ownership of the socket
    explicit
    session(tcp::socket&& socket)
        : ws_(std::move(socket))
    {
    }

    // Start the asynchronous operation
    void
    run()
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::suggested_settings(
                websocket::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
            }));

        // Accept the websocket handshake
        ws_.async_accept(
            beast::bind_front_handler(
                &session::on_accept,
                shared_from_this()));
    }

    void
    on_accept(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "accept");

        // Read a message
        do_read();
    }

    void
    do_read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if(ec == websocket::error::closed)
            return;

        if(ec)
            fail(ec, "read");

        // Authenticate T
        std::ostringstream os; os << boost::beast::make_printable(buffer_.data());
        std::string email = os.str();

        char code[24];
        gen_random(code,24);
        std::cout << "Email recieved:" << email << std::endl; 
        send_email(email, code);
        

        ws_.text(ws_.got_text());
        ws_.async_write(
            buffer_.data(),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Do another read
        do_read();
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint)
        : ioc_(ioc)
        , acceptor_(ioc)
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            net::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        if(! acceptor_.is_open())
            return;
        do_accept();
    }

    void
    do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(
            beast::make_strand(ioc_),
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void
    on_accept(beast::error_code ec, tcp::socket socket)
    {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(std::move(socket))->run();
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 6)
    {
        std::cerr <<
            "Usage: pkgs <address> <port> <threads> <gmailuser> <gmailpassword>\n" <<
            "Example:\n" <<
            "    pkgs 0.0.0.0 8080 1 example@gmail.com 'password'\n" <<
            "    your password may need quotes if it has special characters";
        return EXIT_FAILURE;
    }

    // Setup the PKG
    p.setup("dokuenterprise");
    serilized_parameters = p.serialize_params(p.params);

    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const threads = std::max<int>(1, std::atoi(argv[3]));
    from = argv[4];
    from_password = argv[5];

    // The io_context is required for all I/O
    net::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<listener>(ioc, tcp::endpoint{address, port})->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
}