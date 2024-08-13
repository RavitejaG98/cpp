#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <ctime>

namespace beast = boost::beast;    // from Boost.Beast
namespace http = beast::http;      // from Boost.Beast
namespace net = boost::asio;       // from Boost.Asio
using tcp = boost::asio::ip::tcp;  // from Boost.Asio

// Function to generate HTML content with basic styling
std::string generate_html(const std::string& title, const std::string& body) {
    return R"(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>)" + title + R"(</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                background-color: #f4f4f4;
                color: #333;
                text-align: center;
                margin: 0;
                padding: 20px;
            }
            h1 {
                color: #1a73e8;
            }
            p {
                font-size: 18px;
            }
            .container {
                max-width: 800px;
                margin: 0 auto;
                padding: 20px;
                background-color: #fff;
                border-radius: 8px;
                box-shadow: 0 0 10px rgba(0,0,0,0.1);
            }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>)" + title + R"(</h1>
            <p>)" + body + R"(</p>
        </div>
    </body>
    </html>
    )";
}

// Function to generate JSON content
std::string generate_json(const std::string& content) {
    return content;
}

// Function to handle JSON data for employees
std::string get_employees_json() {
    // Example JSON data for employees
    return R"(
    [
        { "id": 1, "name": "John Doe", "position": "Software Engineer" },
        { "id": 2, "name": "Jane Smith", "position": "Project Manager" },
        { "id": 3, "name": "Alice Johnson", "position": "UX Designer" }
    ]
    )";
}

// Function to handle JSON data for an individual employee
std::string get_employee_json(int id) {
    // Example JSON data for a specific employee
    if (id == 1) {
        return R"(
        { "id": 1, "name": "John Doe", "position": "Software Engineer" }
        )";
    } else if (id == 2) {
        return R"(
        { "id": 2, "name": "Jane Smith", "position": "Project Manager" }
        )";
    } else if (id == 3) {
        return R"(
        { "id": 3, "name": "Alice Johnson", "position": "UX Designer" }
        )";
    } else {
        return R"(
        { "error": "Employee not found" }
        )";
    }
}

// Logs a message with a timestamp
void log_message(const std::string& message) {
    std::time_t now = std::time(nullptr);
    char time_buf[100];
    if (std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now))) {
        std::cout << "[" << time_buf << "] " << message << std::endl;
    } else {
        std::cerr << "Error formatting time." << std::endl;
    }
}

// Handles an HTTP request and generates a response
void handle_request(beast::tcp_stream& stream, http::request<http::string_body> req) {
    http::response<http::string_body> res;
    std::string target = std::string(req.target());
    std::string response_body;
    bool json_response = false;

    log_message("Received request: " + target);

    if (req.method() == http::verb::get) {
        if (target == "/employee") {
            response_body = get_employees_json();
            res.result(http::status::ok);
            res.set(http::field::content_type, "application/json");
            json_response = true;
        } else if (target.find("/employee/") == 0) {
            std::string id_str = target.substr(10); // Extract ID from URL
            int id;
            try {
                id = std::stoi(id_str);
            } catch (const std::exception& e) {
                id = -1; // Invalid ID
            }
            response_body = get_employee_json(id);
            if (response_body.find("error") != std::string::npos) {
                res.result(http::status::not_found);
            } else {
                res.result(http::status::ok);
            }
            res.set(http::field::content_type, "application/json");
            json_response = true;
        } else {
            // Serve the 404 Not Found page for any other path
            response_body = generate_html(
                "404 Not Found",
                "Sorry, the page you are looking for does not exist."
            );
            res.result(http::status::not_found);
            res.set(http::field::content_type, "text/html");
        }
    } else {
        // Method not allowed
        response_body = generate_html(
            "405 Method Not Allowed",
            "The HTTP method is not allowed for this endpoint."
        );
        res.result(http::status::method_not_allowed);
        res.set(http::field::content_type, "text/html");
    }

    if (json_response) {
        res.body() = response_body;  // No need to wrap in `generate_json` as response_body is already valid JSON
    } else {
        res.body() = response_body;
    }

    res.prepare_payload();
    http::write(stream, res);

    log_message("Response sent with status: " + std::to_string(res.result_int()));
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
        log_message("Error: " + std::string(e.what()));
    }
}

int main() {
    try {
        // Create an io_context object
        net::io_context ioc;

        // Create an acceptor object to listen for incoming connections
        tcp::acceptor acceptor{ioc, tcp::endpoint{tcp::v4(), 8080}};

        log_message("Server running on port 8080");

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
        log_message("Error: " + std::string(e.what()));
    }
}
