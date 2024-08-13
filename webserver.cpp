#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;    // from Boost.Beast
namespace http = beast::http;      // from Boost.Beast
namespace net = boost::asio;       // from Boost.Asio
using tcp = boost::asio::ip::tcp;  // from Boost.Asio

// Handles an HTTP request and generates a response
void handle_request(beast::tcp_stream& stream, http::request<http::string_body> req) {
    // Create a response object
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "Boost.Beast");
    res.set(http::field::content_type, "text/plain");
    res.body() = "Hello, World!";
    res.prepare_payload();

    // Send the response
    http::write(stream, res);
}

// Accepts incoming connections and handles them
void do_session(tcp::socket& socket) {
    try {
        beast::tcp_stream stream(std::move(socket));
        beast::flat_buffer buffer;

        // Read the request
        http::request<http::string_body> req;
        http::read(stream, buffer, req);

        // Handle the request
        handle_request(stream, req);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    try {
        // Create an io_context object
        net::io_context ioc;

        // Create an acceptor object to listen for incoming connections
        tcp::acceptor acceptor{ioc, tcp::endpoint{tcp::v4(), 8080}};

        std::cout << "Server running on port 8080\n";

        while (true) {
            // Create a socket to handle the new connection
            tcp::socket socket{ioc};

            // Accept a new connection
            acceptor.accept(socket);

            // Handle the connection in a new thread
            std::thread{[socket = std::move(socket)]() mutable {
                do_session(socket);
            }}.detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
