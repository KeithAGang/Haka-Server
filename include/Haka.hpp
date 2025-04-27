#ifndef HAKA_HTTP_SERVER_HPP
#define HAKA_HTTP_SERVER_HPP

#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>      // For string streams
#include <system_error>
#include <fstream>      // For serving files
#include <filesystem>   // For checking file existence and paths (C++17+)
#include <iomanip>      // For std::put_time
#include <ctime>        // For logging timestamps
#include <chrono>       // For system clock
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/color.h>
#include <ylt/struct_json/json_writer.h>
#define ASIO_STANDALONE
#include <asio.hpp>

namespace Haka 
{
  /**
   * Helper to guess MIME type based on file extension
   * @param file_path The file path to analyze
   * @return The appropriate MIME type string
   */
  std::string guess_mime_type(const std::string& file_path) {
      std::filesystem::path p(file_path);
      std::string ext = p.extension().string();
      if (ext == ".html" || ext == ".htm") return "text/html";
      if (ext == ".css") return "text/css";
      if (ext == ".js") return "application/javascript";
      if (ext == ".json") return "application/json";
      if (ext == ".png") return "image/png";
      if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
      if (ext == ".gif") return "image/gif";
      if (ext == ".svg") return "image/svg+xml";
      if (ext == ".pdf") return "application/pdf";
      // Add more types as needed
      return "application/octet-stream"; // Default binary type
  }

  std::string haka_text = R"(
  .----------------.  .----------------.  .----------------.  .----------------. 
| .--------------. || .--------------. || .--------------. || .--------------. |
| |  ____  ____  | || |      __      | || |  ___  ____   | || |      __      | |
| | |_   ||   _| | || |     /  \     | || | |_  ||_  _|  | || |     /  \     | |
| |   | |__| |   | || |    / /\ \    | || |   | |_/ /    | || |    / /\ \    | |
| |   |  __  |   | || |   / ____ \   | || |   |  __'.    | || |   / ____ \   | |
| |  _| |  | |_  | || | _/ /    \ \_ | || |  _| |  \ \_  | || | _/ /    \ \_ | |
| | |____||____| | || ||____|  |____|| || | |____||____| | || ||____|  |____|| |
| |              | || |              | || |              | || |              | |
| '--------------' || '--------------' || '--------------' || '--------------' |
 '----------------'  '----------------'  '----------------'  '----------------'
  )";

  /**
   * Basic logging function using fmt library for formatted output
   * @param level The log level (e.g., "INFO", "ERROR")
   * @param message The message to log
   */
  void log_message(const std::string& level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    fmt::print("[{}] [{}] {}\n", ss.str(), level, message);
  }

  /**
   * Request class to handle incoming HTTP requests
   * Stores method, path, and headers
   */
  class Request 
  {
    public:
      std::string method;     // HTTP method (GET, POST, etc.)
      std::string path;       // Request URL path
      std::unordered_map<std::string, std::string> headers;  // HTTP headers
      
      /**
       * Checks if the request path starts with a given prefix
       * @param prefix The prefix to check against
       * @return true if path starts with prefix, false otherwise
       */
      bool path_starts_with(const std::string& prefix) const {
        return path.rfind(prefix, 0) == 0;
      }
      
      /**
       * Extracts the part of the path after a given prefix
       * @param prefix The prefix to remove
       * @return The remaining path after the prefix
       */
      std::string path_after_prefix(const std::string& prefix) const {
        if (path_starts_with(prefix)) {
          return path.substr(prefix.length());
        }
        return path; // Should not happen if path_starts_with is true
      }
  };

  /**
   * Response class to handle outgoing HTTP responses
   * Manages status code, headers, and body content
   */
  class Response 
  {
    public:
      int status_code = 200;  // HTTP status code
      std::unordered_map<std::string, std::string> headers;  // HTTP headers
      std::string body;       // Response body

    /**
     * Default constructor - sets default content type to text/plain
     */
    Response()
    {
      headers["Content-Type"] = "text/plain";  // Note: Content-Type with capital T is standard
    }

    /**
     * Set JSON response with automatic serialization
     * @param json_content The object to serialize to JSON
     */
    template <typename T>
    void JSON(const T& json_content)
    {
      try {
        headers["Content-Type"] = "application/json";
        // Serialize the content to the body member
        struct_json::to_json(json_content, body);
      } catch (const std::exception& e) {
        log_message("ERROR", fmt::format("JSON serialization error: {}!", e.what()));
        status_code = 500;
        body = "Internal Server Error";
        headers["Content-Type"] = "text/plain";
      }
    }

    /**
     * Set HTML content from string
     * @param html_content The HTML content to set
     */
    void HTML(const std::string& html_content)
    {
      headers["Content-Type"] = "text/html";
      body = html_content;
    }

    /**
     * Set plain text content
     * @param text_content The text content to set
     */
  
    void Text(const std::string& text_content)
    {
      headers["Content-Type"] = "text/plain";
      body = text_content;
    }
    

   
    /**
     * Set response content by reading from a file
     * @param file_path Path to the file to send
     * @return true if file was successfully read, false otherwise
     */
    bool sendFile(const std::string& file_path) 
    {
      std::ifstream file(file_path, std::ios::binary | std::ios::ate);
      if (!file.is_open()) {
        log_message("WARN", fmt::format("File not found: {}", file_path));
        return false; // Indicate failure
      }

      std::streamsize size = file.tellg();
      file.seekg(0, std::ios::beg);

      body.resize(size);
      if (!file.read(&body[0], size)) {
        log_message("ERROR", fmt::format("Error reading file: {}", file_path));
        status_code = 500;
        body = "Internal Server Error";
        headers["Content-Type"] = "text/plain";
        return false; // Indicate failure
      }

      headers["Content-Type"] = guess_mime_type(file_path);
      status_code = 200;
      return true; // Indicate success
    }
        
    /**
     * Convert the response to a string suitable for HTTP transmission
     * @return The formatted HTTP response as a string
     */
    std::string to_string() const {
      std::ostringstream response;  // Fixed: using ostringstream instead of ostrstream
      // Copy The Body Before Modifying It 
      std::string body_copy = body;

      response << "HTTP/1.1 " << status_code << " ";
      switch(status_code) {
        case 200: response << "OK"; break;  // Fixed: "OK" instead of "Ok" per HTTP spec
        case 400: response << "Bad Request"; break;
        case 404: response << "Not Found"; break;
        case 500: response << "Internal Server Error"; break;
        default: response << "Unknown Status"; break;
      }
      
      response << "\r\n";

      for (const auto& header : headers)
      {
        response << header.first << ": " << header.second << "\r\n";  // Fixed: removed extraneous semicolon
      }

      response << "Content-Length: " << body_copy.size() << "\r\n\r\n";  // Fixed: Content-Length with capital L
      response << body_copy;
      return response.str();
    }
  };

  using RouteHandler = std::function<void(const Request&, Response&)>;
  class Server;

  /**
   * Connection class to handle individual client connections
   * Manages reading requests and sending responses
   */
  class Connection: public std::enable_shared_from_this<Connection>
  {
    public:
      /**
       * Constructor
       * @param socket The connected socket
       * @param server Reference to the server instance
       */
      Connection(asio::ip::tcp::socket socket, Server& server) 
        : socket_(std::move(socket)), server_(server)
      {
        log_message("INFO", fmt::format("New Connection From {}", socket_.remote_endpoint().address().to_string()));
      }

      /**
       * Start processing the connection
       */
      void start() { read_request(); }

    private:
      void read_request();
      void process_request();
      void send_response();

      asio::ip::tcp::socket socket_;
      Server& server_;
      Request request_;
      Response response_;
      std::array<char, 8192> buffer_{}; // Buffer for reading request data
  };

  /**
   * Server class to manage HTTP server functionality
   * Handles connections, routes, and static file serving
   */
  class Server {
    public:
      /**
       * Constructor
       * @param host Host address to bind to
       * @param port Port to listen on
       */
      Server(const std::string& host, unsigned short port)
          : io_context_(),
          acceptor_(io_context_, asio::ip::tcp::endpoint(asio::ip::make_address(host), port)),
          host_(host),
          port_(port) {
          log_message("INFO", fmt::format("Server initialized on {}:{}", host_, port_));
      }

      /**
       * Destructor
       */
      ~Server() {
          log_message("INFO", "Server shutting down.");
          // io_context_ and acceptor_ handle their cleanup via RAII
      }

      /**
       * Register a GET route handler
       * @param path The URL path to handle
       * @param handler The handler function
       */
      void Get(const std::string& path, RouteHandler handler) {
          routes_["GET " + path] = handler;
          log_message("INFO", fmt::format("Registered GET route: {}", path));
      }

      /**
       * Register a POST route handler
       * @param path The URL path to handle
       * @param handler The handler function
       */
      void Post(const std::string& path, RouteHandler handler) {
          routes_["POST " + path] = handler;
          log_message("INFO", fmt::format("Registered POST route: {}", path));
      }

      // Add more methods (Put, Delete, etc.) as needed...

      /**
       * Register a directory for serving static files
       * @param path_prefix The URL prefix to serve files under
       * @param fs_path The filesystem path to serve files from
       */
      void serveStatic(const std::string& path_prefix, const std::string& fs_path) {
          // Ensure prefix starts and ends with '/' unless it's just "/"
          std::string clean_prefix = path_prefix;
          if (clean_prefix.empty() || clean_prefix[0] != '/') clean_prefix = "/" + clean_prefix;
          if (clean_prefix.length() > 1 && clean_prefix.back() == '/') clean_prefix.pop_back();

          static_paths_.push_back({clean_prefix, fs_path});
          log_message("INFO", fmt::format("Serving static files from '{}' at URL prefix '{}'", fs_path, clean_prefix));
      }

      /**
       * Start the server and begin accepting connections
       */
      void run() {            
          fmt::print(fg(fmt::color::yellow), "Running on http://{}:{}\n\n", host_, port_);
          log_message("INFO", "Haka server starting...");
          do_accept();
          io_context_.run(); // This blocks until io_context is stopped
          log_message("INFO", "Haka server stopped.");
      }

      /**
       * Find the appropriate handler for a request
       * @param req The request to find a handler for
       * @return The handler function
       */
      RouteHandler get_handler(const Request& req) {
          // 1. Check Static Files first
          for (const auto& static_entry : static_paths_) {
              const std::string& url_prefix = static_entry.first;
              const std::string& fs_root = static_entry.second;

              if (req.path_starts_with(url_prefix)) {
                  std::string file_sub_path = req.path_after_prefix(url_prefix);
                  // Append "index.html" for directory requests if needed
                  if (file_sub_path.empty() || file_sub_path == "/") file_sub_path = "/index.html";

                  // Check if the file exists and is a regular file
                  std::filesystem::path fs_root_path = std::filesystem::absolute(fs_root);
                  std::filesystem::path relative_path = file_sub_path.substr(1); // remove leading / from sub_path
                  std::filesystem::path full_fs_path = std::filesystem::absolute(fs_root_path / relative_path); 

                  // Basic security check: ensure path is within the served directory
                  if (!full_fs_path.string().starts_with(fs_root_path.string())) {
                      log_message("WARN", fmt::format("Attempted directory traversal: {}", req.path));
                      return [](const Request& r, Response& res) {
                          res.status_code = 400; // Bad Request
                          res.Text("Invalid path.");
                      };
                  }

                  // Check if the file exists and is a regular file
                  if (std::filesystem::exists(full_fs_path) && std::filesystem::is_regular_file(full_fs_path)) {
                      log_message("INFO", fmt::format("Serving static file: {}", full_fs_path.string()));
                      return [file_path = full_fs_path.string()](const Request& r, Response& res) {
                          if (!res.sendFile(file_path)) {
                              res.status_code = 500;
                              res.Text("Error serving file.");
                          }
                      };
                  } else {
                      log_message("INFO", fmt::format("Static file not found: {}", full_fs_path.string()));
                      // Continue to check routes if static file not found
                  }
              }
          }

          // 2. Check registered routes
          auto it = routes_.find(req.method + " " + req.path);
          if (it != routes_.end()) {
              log_message("INFO", fmt::format("Matched route: {} {}", req.method, req.path));
              return it->second;
          }

          // 3. No match found
          log_message("INFO", fmt::format("Route not found: {} {}", req.method, req.path));
          return [](const Request& r, Response& res) {
              res.status_code = 404;
              res.Text(fmt::format("Not found: {}", r.path));
          };
      }

    private:
      /**
       * Begin accepting new connections
       */
      void do_accept() {
          acceptor_.async_accept([this](asio::error_code ec, asio::ip::tcp::socket socket) {
              if (!ec) {
                  auto conn = std::make_shared<Connection>(std::move(socket), *this);
                  conn->start();
              } else {
                  log_message("ERROR", fmt::format("Accept error: {}", ec.message()));
              }
              do_accept(); // Continue accepting new connections
          });
      }

      asio::io_context io_context_;
      asio::ip::tcp::acceptor acceptor_;
      std::string host_;
      unsigned short port_;
      std::unordered_map<std::string, RouteHandler> routes_;
      std::vector<std::pair<std::string, std::string>> static_paths_; // {url_prefix, fs_path}
  };

  /**
   * Read the HTTP request from the socket
   */
  void Connection::read_request() {
      auto self = shared_from_this();
      // Use asio::async_read until we hit a delimiter for more robustness
      socket_.async_read_some(asio::buffer(buffer_),
          [this, self](asio::error_code ec, std::size_t bytes_transferred) {
              if (!ec) {
                  std::string data(buffer_.data(), bytes_transferred);
                  std::istringstream stream(data);
                  std::string line;

                  // Read request line (e.g., GET / HTTP/1.1)
                  if (!std::getline(stream, line) || line.empty()) {
                      log_message("WARN", "Received empty or invalid request line.");
                      response_.status_code = 400;
                      response_.Text("Bad Request");
                      send_response();
                      return;
                  }

                  std::istringstream request_line(line);
                  std::string http_version; // We'll read and ignore version for now
                  request_line >> request_.method >> request_.path >> http_version;

                  if (request_.method.empty() || request_.path.empty()) {
                      log_message("WARN", fmt::format("Malformed request line: {}", line));
                      response_.status_code = 400;
                      response_.Text("Bad Request");
                      send_response();
                      return;
                  }

                  log_message("INFO", fmt::format("Request: {} {}", request_.method, request_.path));

                  // Read headers
                  while (std::getline(stream, line) && line != "\r") {
                      std::size_t colon_pos = line.find(':');
                      if (colon_pos != std::string::npos) {
                          std::string header_name = line.substr(0, colon_pos);
                          std::string header_value = line.substr(colon_pos + 1);
                          // Trim leading/trailing whitespace from value
                          header_value.erase(0, header_value.find_first_not_of(" \t"));
                          header_value.erase(header_value.find_last_not_of("\r\n") + 1); // Remove trailing \r\n
                          request_.headers[header_name] = header_value;
                          // log_message("DEBUG", fmt::format("Header: {}: {}", header_name, header_value)); // Optional: log headers
                      } else {
                          log_message("WARN", fmt::format("Malformed header line: {}", line));
                          // Handle malformed header? For now, ignore and continue.
                      }
                  }

                  // TODO: Read request body if Content-Length is present
                  // This basic read_some doesn't automatically handle body reading.
                  // A more robust parser is needed for POST/PUT requests with bodies.

                  process_request();
              } else if (ec != asio::error::operation_aborted) {
                  log_message("ERROR", fmt::format("Read error: {}", ec.message()));
              }
              // else operation_aborted, connection closed cleanly
          });
  }

  /**
   * Process the request by finding and executing the appropriate handler
   */
  void Connection::process_request() {
      // Get handler (checks static files first, then routes)
      auto handler = server_.get_handler(request_);

      // Execute the handler
      try {
          handler(request_, response_);
      } catch (const std::exception& e) {
          log_message("ERROR", fmt::format("Handler threw exception for {} {}: {}", request_.method, request_.path, e.what()));
          response_.status_code = 500;
          response_.Text("Internal Server Error");
      } catch (...) {
          log_message("ERROR", fmt::format("Handler threw unknown exception for {} {}", request_.method, request_.path));
          response_.status_code = 500;
          response_.Text("Internal Server Error");
      }

      // Send the response back to the client
      send_response();
  }

  /**
   * Send the HTTP response back to the client
   */
  void Connection::send_response() {
      auto self = shared_from_this();
      auto response_str = std::make_shared<std::string>(response_.to_string());

      asio::async_write(socket_, asio::buffer(*response_str),
          [this, self, response_str](asio::error_code ec, std::size_t bytes_transferred) {
              if (!ec) {
                  log_message("INFO", fmt::format("Sent response ({} bytes) for {} {} with status {}",
                                                bytes_transferred,
                                                request_.method,
                                                request_.path,
                                                response_.status_code));
                  // Initiate graceful socket shutdown
                  asio::error_code shutdown_ec;
                  socket_.shutdown(asio::ip::tcp::socket::shutdown_both, shutdown_ec);
                  if (shutdown_ec) {
                      log_message("WARN", fmt::format("Socket shutdown error: {}", shutdown_ec.message()));
                  }

                  // Close the socket
                  asio::error_code close_ec;
                  socket_.close(close_ec);
                  if (close_ec) {
                      log_message("WARN", fmt::format("Socket close error: {}", close_ec.message()));
                  }

              } else if (ec != asio::error::operation_aborted) {
                  log_message("ERROR", fmt::format("Write error for {} {}: {}", request_.method, request_.path, ec.message()));
              }
              // else operation_aborted, connection closed cleanly
          });
  }
} // namespace Haka

#endif // HAKA_HTTP_SERVER_HPP
