#ifndef HAKA_SERVER_HPP
#define HAKA_SERVER_HPP

#include <string>

// External library includes (Asio for networking)
#define ASIO_STANDALONE // Define this if using Asio standalone
#include <asio.hpp>

// Project includes
#include "haka/core.hpp"   // For Request, Response, RouteHandler, log_message
#include "haka/router.hpp" // For Router class

#include <array>   // For buffer_
#include <memory>  // For std::shared_ptr, std::enable_shared_from_this
#include <sstream> // For std::istringstream // Needed for Connection methods

namespace Haka {

// ASCII art for the Haka logo
const std::string haka_logo = R"(
                ██   ██   █████   ██   ██   █████
                ██   ██  ██   ██  ██   ██  ██   ██
                ███████  ███████  █████    ███████
                ██   ██  ██   ██  ██   ██  ██   ██
                ██   ██  ██   ██  ██   ██  ██   ██
    )";

// Forward declaration of the Server class (needed by Connection)
class Server;

/**
 * @brief Represents a single client connection.
 * Handles reading the request, processing it, and sending the response.
 * Uses shared_from_this to manage its lifetime during asynchronous operations.
 * Defined BEFORE Server because Server's do_accept needs the full definition.
 */
class Connection : public std::enable_shared_from_this<Connection> {
public:
  /**
   * @brief Constructor for the Connection.
   * @param socket The connected socket for this client.
   * @param server Reference to the parent Server instance.
   */
  inline Connection(asio::ip::tcp::socket socket, Server &server)
      : socket_(std::move(socket)), // Take ownership of the socket
        server_(server)             // Store a reference to the server
  {
    try {
      log_message("INFO",
                  fmt::format("New Connection From {}",
                              socket_.remote_endpoint().address().to_string()));
    } catch (const asio::system_error &e) {
      log_message(
          "WARN",
          fmt::format("Could not get remote endpoint address: {}", e.what()));
    }
  }

  /**
   * @brief Starts the asynchronous process for this connection.
   * Typically begins with reading the request.
   */
  inline void start() { read_request(); }

private:
  // Implementation details for read_request, process_request, send_response
  // remain the same as previously defined, using the Request and Response
  // members. These methods are defined inline below.
  inline void read_request();
  inline void process_request();
  inline void send_response();

  asio::ip::tcp::socket socket_;    // The socket for this connection
  Server &server_;                  // Reference to the parent server
  Request request_;                 // Stores the parsed incoming request
  Response response_;               // Stores the response to be sent
  std::array<char, 8192> buffer_{}; // Buffer for reading incoming data
  std::string request_buffer_; // Accumulates incoming request data for parsing
};

/**
 * @brief The main HTTP server class.
 * Handles accepting new connections and managing the I/O context.
 * Provides methods for defining routes and static file serving
 * by delegating to an internal Router instance.
 */
class Server {
public:
  /**
   * @brief Constructor for the Server.
   * @param host Host address to bind to (e.g., "127.0.0.1" or "0.0.0.0").
   * @param port Port to listen on.
   */
  inline Server(const std::string &host, unsigned short port)
      : io_context_(), // Initialize io_context
                       // Initialize acceptor with the specified host and port
        acceptor_(io_context_,
                  asio::ip::tcp::endpoint(asio::ip::make_address(host), port)),
        host_(host), port_(port), router_() // Initialize the router
  {
    log_message("INFO",
                fmt::format("Server initialized on {}:{}", host_, port_));
  }

  /**
   * @brief Destructor for the Server.
   * Cleans up resources (io_context and acceptor handle this via RAII).
   */
  inline ~Server() { log_message("INFO", "Server shutting down."); }

  // --- Public methods for defining routes (Delegating to internal router) ---

  /**
   * @brief Registers a handler for GET requests at a specific path.
   * @param path The URL path.
   * @param handler The function to execute for this route.
   */
  inline void Get(const std::string &path, RouteHandler handler) {
    router_.Get(path, handler); // Delegate to the internal router
  }

  /**
   * @brief Registers a handler for POST requests at a specific path.
   * @param path The URL path.
   * @param handler The function to execute for this route.
   */
  inline void Post(const std::string &path, RouteHandler handler) {
    router_.Post(path, handler); // Delegate to the internal router
  }

  // TODO: Add wrapper methods for other HTTP methods (Put, Delete, etc.)

  /**
   * @brief Registers a directory for serving static files under a specific URL
   * prefix.
   * @param path_prefix The URL prefix (e.g., "/static").
   * @param fs_path The filesystem path (e.g., "./public").
   */
  inline void serveStatic(const std::string &path_prefix,
                          const std::string &fs_path) {
    router_.serveStatic(path_prefix,
                        fs_path); // Delegate to the internal router
  }

  /**
   * @brief Defines a group of routes that share a common URL prefix.
   * @param prefix The URL prefix for this group.
   * @param config_func A function (typically a lambda) that defines routes
   * within the group.
   */
  inline void group(const std::string &prefix,
                    std::function<void(Router &)> config_func) {
    router_.group(prefix, config_func); // Delegate to the internal router
  }

  /**
   * @brief Mounts another Router's routes and static paths under a specific
   * prefix into this Server's main router. This is useful for creating modular
   * route definitions.
   * @param prefix The URL prefix under which the other router's routes will be
   * mounted.
   * @param sub_router The Router instance whose routes/static paths will be
   * merged.
   */
  inline void mount(const std::string &prefix, const Router &sub_router) {
    router_.mount(prefix, sub_router); // Delegate to the internal router
  }

  // --- Server control methods ---

  /**
   * @brief Starts the server's event loop and begins accepting connections.
   * This method will block until the io_context is stopped.
   */
  inline void run() {
    // Print the Haka logo in white color
    fmt::print(fg(fmt::color::white), "{}\n\n", haka_logo);
    // Print the running address in yellow color
    fmt::print(fg(fmt::color::yellow), "Running on http://{}:{}\n\n", host_,
               port_);
    log_message("INFO", "Haka server starting...");
    do_accept();       // Start the asynchronous accept operation
    io_context_.run(); // Run the I/O event loop (this call blocks)
    log_message("INFO", "Haka server stopped.");
  }

  /**
   * @brief Finds the appropriate handler for a given request.
   * This method is called by the Connection class and delegates
   * the actual routing logic to the internal Router instance.
   * @param req The incoming Request object.
   * @return The RouteHandler function to process the request.
   */
  inline RouteHandler get_handler(const Request &req) const {
    return router_.match(req); // Delegate routing to the Router
  }

  /**
   * @brief Provides access to the internal io_context.
   * Useful if other parts of the application need to interact with the
   * same I/O service (e.g., for timers, other network operations).
   * @return Reference to the Server's io_context.
   */
  inline asio::io_context &get_io_context() { return io_context_; }

private:
  /**
   * @brief Initiates an asynchronous accept operation.
   * Waits for a new client connection. When a connection is accepted,
   * it creates a new Connection object and starts processing it.
   */
  inline void do_accept() {
    acceptor_.async_accept([this](asio::error_code ec,
                                  asio::ip::tcp::socket socket) {
      if (!ec) {
        auto conn = std::make_shared<Connection>(std::move(socket), *this);
        conn->start(); // Connection is fully defined above
      } else {
        if (ec != asio::error::operation_aborted) {
          log_message("ERROR", fmt::format("Accept error: {}", ec.message()));
        }
      }
      do_accept(); // Continue accepting new connections
    });
  }

  asio::io_context io_context_;      // Manages asynchronous operations
  asio::ip::tcp::acceptor acceptor_; // Listens for incoming connections
  std::string host_;                 // Server host address
  unsigned short port_;              // Server port
  Router router_; // The router instance to handle route matching
};

// --- Connection Method Definitions (Defined inline in header) ---

inline void Connection::read_request() {
  auto self = shared_from_this();
  socket_.async_read_some(
      asio::buffer(buffer_),
      [this, self](asio::error_code ec, std::size_t bytes_transferred) {
        if (!ec) {
          request_buffer_.append(buffer_.data(), bytes_transferred);
          size_t header_end_pos = request_buffer_.find("\r\n\r\n");

          if (header_end_pos != std::string::npos) {
            std::istringstream stream(request_buffer_);
            std::string line;

            if (std::getline(stream, line) && !line.empty()) {
              std::istringstream request_line_stream(line);
              std::string http_version;
              request_line_stream >> request_.method >> request_.path >>
                  http_version;

              if (request_.method.empty() || request_.path.empty()) {
                log_message("WARN",
                            fmt::format("Malformed request line: {}", line));
                response_.status_code = 400;
                response_.Text("Bad Request");
                send_response();
                return;
              }

              log_message("INFO", fmt::format("Request: {} {}", request_.method,
                                              request_.path));

              while (std::getline(stream, line) && line != "\r") {
                std::size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                  std::string header_name = line.substr(0, colon_pos);
                  std::string header_value = line.substr(colon_pos + 1);
                  header_value.erase(0, header_value.find_first_not_of(" \t"));
                  if (!header_value.empty() && header_value.back() == '\r') {
                    header_value.pop_back();
                  }
                  request_.headers[header_name] = header_value;
                } else {
                  log_message("WARN",
                              fmt::format("Malformed header line: {}", line));
                }
              }
              // TODO: Read request body

              process_request();
            } else {
              log_message(
                  "WARN",
                  "Received empty or invalid request line after reading.");
              response_.status_code = 400;
              response_.Text("Bad Request");
              send_response();
            }

          } else {
            read_request();
          }

        } else if (ec != asio::error::operation_aborted &&
                   ec != asio::error::eof) {
          log_message("ERROR", fmt::format("Read error: {}", ec.message()));
        }
      });
}

inline void Connection::process_request() {
  auto handler = server_.get_handler(request_);
  try {
    handler(request_, response_);
  } catch (const std::exception &e) {
    log_message("ERROR", fmt::format("Handler threw exception for {} {}: {}",
                                     request_.method, request_.path, e.what()));
    response_.status_code = 500;
    response_.Text("Internal Server Error");
  } catch (...) {
    log_message("ERROR",
                fmt::format("Handler threw unknown exception for {} {}",
                            request_.method, request_.path));
    response_.status_code = 500;
    response_.Text("Internal Server Error");
  }
  send_response();
}

inline void Connection::send_response() {
  auto self = shared_from_this();
  auto response_str = std::make_shared<std::string>(response_.to_string());

  asio::async_write(
      socket_, asio::buffer(*response_str),
      [this, self, response_str](asio::error_code ec,
                                 std::size_t bytes_transferred) {
        if (!ec) {
          log_message(
              "INFO",
              fmt::format("Sent response ({} bytes) for {} {} with status {}",
                          bytes_transferred, request_.method, request_.path,
                          response_.status_code));
          asio::error_code shutdown_ec;
          socket_.shutdown(asio::ip::tcp::socket::shutdown_both, shutdown_ec);
          if (shutdown_ec && shutdown_ec != asio::error::not_connected) {
            log_message("WARN", fmt::format("Socket shutdown error: {}",
                                            shutdown_ec.message()));
          }
          asio::error_code close_ec;
          socket_.close(close_ec);
          if (close_ec && close_ec != asio::error::not_connected) {
            log_message("WARN", fmt::format("Socket close error: {}",
                                            close_ec.message()));
          }
        } else if (ec != asio::error::operation_aborted) {
          log_message("ERROR",
                      fmt::format("Write error for {} {}: {}", request_.method,
                                  request_.path, ec.message()));
        }
      });
}

} // namespace Haka

#endif // HAKA_SERVER_HPP
