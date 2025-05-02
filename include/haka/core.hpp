#ifndef HAKA_CORE_HPP
#define HAKA_CORE_HPP

// Standard library includes
#include <chrono>     // For system clock
#include <ctime>      // For logging timestamps
#include <exception>  // For std::exception
#include <filesystem> // For checking file existence and paths (C++17+)
#include <fstream>    // For serving files
#include <functional> // For std::function
#include <iomanip>    // For std::put_time
#include <sstream>    // For string streams
#include <string>
#include <unordered_map>

// External library includes
#define FMT_HEADER_ONLY // Define this if you are using fmt as a header-only
                        // library
#include <fmt/color.h>
#include <fmt/core.h>

// Include struct_json for JSON serialization (needed for Response::JSON
// template)
#include <ylt/struct_json/json_writer.h>

namespace Haka {
// Forward declarations for classes used by pointers/references
class Request;
class Response;
class Server; // Needed for RouteHandler alias
enum class LogLevel { DEBUG, INFO, WARN, ERROR };

// Global flag to enable debug logging
inline bool enable_debug_logging = false; // Default is false

/**
 * @brief Helper to guess MIME type based on file extension.
 * @param file_path The file path to analyze.
 * @return The appropriate MIME type string.
 */
inline std::string guess_mime_type(const std::string &file_path) {
  std::filesystem::path p(file_path);
  std::string ext = p.extension().string();
  if (ext == ".html" || ext == ".htm")
    return "text/html";
  if (ext == ".css")
    return "text/css";
  if (ext == ".js")
    return "application/javascript";
  if (ext == ".json")
    return "application/json";
  if (ext == ".png")
    return "image/png";
  if (ext == ".jpg" || ext == ".jpeg")
    return "image/jpeg";
  if (ext == ".gif")
    return "image/gif";
  if (ext == ".svg")
    return "image/svg+xml";
  if (ext == ".pdf")
    return "application/pdf";
  // Add more types as needed
  return "application/octet-stream"; // Default binary type
}

/**
 * @brief Basic logging function using fmt library for formatted output.
 * Only prints DEBUG level messages if enable_debug_logging is true.
 * @param level The log level (e.g., "INFO", "DEBUG", "ERROR").
 * @param message The message to log.
 */

inline const char *log_level_to_string(const LogLevel &level) {
  switch (level) {
  case LogLevel::DEBUG:
    return "DEBUG";
  case LogLevel::INFO:
    return "INFO";
  case LogLevel::WARN:
    return "WARN";
  case LogLevel::ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

inline void log_message(const LogLevel &level, const std::string &message) {
  // Only print DEBUG messages if the flag is set
  if (level == LogLevel::DEBUG && !enable_debug_logging) {
    return;
  }

  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");

  switch (level) {
  case LogLevel::ERROR:
    fmt::print(fg(fmt::color::red), "[{}] [{}] {}\n", ss.str(),
               log_level_to_string(level), message);
    break;
  case LogLevel::WARN:
    fmt::print(fg(fmt::color::yellow), "[{}] [{}] {}\n", ss.str(),
               log_level_to_string(level), message);
    break;
  case LogLevel::INFO:
    fmt::print(fg(fmt::color::green), "[{}] [{}] {}\n", ss.str(),
               log_level_to_string(level), message);
    break;
  case LogLevel::DEBUG:
    fmt::print(fg(fmt::color::blue), "[{}] [{}] {}\n", ss.str(),
               log_level_to_string(level),
               message); // Use blue for debug
    break;
  default:
    fmt::print(fmt::fg(fmt::color::white), "[{}] [{}] {}\n", ss.str(),
               log_level_to_string(level), message);
  }
}

/**
 * @brief Represents an incoming HTTP request.
 * Stores method, path, and headers.
 */
class Request {
public:
  std::string method; // HTTP method (GET, POST, etc.)
  std::string path;   // Request URL path
  std::unordered_map<std::string, std::string> headers; // HTTP headers
  // TODO: Add members for request body, query parameters, form data, etc.

  /**
   * @brief Checks if the request path starts with a given prefix.
   * @param prefix The prefix to check against.
   * @return true if path starts with prefix, false otherwise.
   */
  inline bool path_starts_with(const std::string &prefix) const {
    return path.rfind(prefix, 0) == 0;
  }

  /**
   * @brief Extracts the part of the path after a given prefix.
   * @param prefix The prefix to remove.
   * @return The remaining path after the prefix.
   */
  inline std::string path_after_prefix(const std::string &prefix) const {
    if (path_starts_with(prefix)) {
      if (path.length() == prefix.length()) {
        return "/";
      }
      return path.substr(prefix.length());
    }
    return path;
  }
};

/**
 * @brief A simple structure for consistent JSON responses.
 * Can be easily serialized to JSON using struct_json.
 */
struct JsonResponse {
  std::string title;
  std::string message;
  // Add more fields as needed for common response data
};

// STRUCT_JSON_DEFINE is not needed due to compile-time reflection

/**
 * @brief Represents an outgoing HTTP response.
 * Manages status code, headers, and body content.
 */
class Response {
public:
  int status_code = 200;                                // HTTP status code
  std::unordered_map<std::string, std::string> headers; // HTTP headers
  std::string body;                                     // Response body

  /**
   * @brief Default constructor - sets default content type to text/plain.
   */
  inline Response() { headers["Content-Type"] = "text/plain"; }

  /**
   * @brief Set JSON response content with automatic serialization.
   * Uses struct_json to serialize the provided object.
   * @param json_content The object to serialize to JSON.
   */
  template <typename T> inline void JSON(const T &json_content) {
    try {
      headers["Content-Type"] = "application/json";
      // Serialize the content to the body member
      struct_json::to_json(json_content, body);
    } catch (const std::exception &e) {
      log_message(LogLevel::ERROR,
                  fmt::format("JSON serialization error: {}!", e.what()));
      status_code = 500;
      body = "Internal Server Error";
      headers["Content-Type"] = "text/plain";
    }
  }

  /**
   * @brief Set HTML content from string.
   * @param html_content The HTML content to set.
   */
  inline void HTML(const std::string &html_content) {
    headers["Content-Type"] = "text/html";
    body = html_content;
  }

  /**
   * @brief Set plain text content.
   * @param text_content The text content to set.
   */
  inline void Text(const std::string &text_content) {
    headers["Content-Type"] = "text/plain";
    body = text_content;
  }

  /**
   * @brief Set response content by reading from a file.
   * @param file_path Path to the file to send.
   * @return true if file was successfully read, false otherwise.
   */
  inline bool sendFile(const std::string &file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      log_message(LogLevel::WARN, fmt::format("File not found: {}", file_path));
      status_code = 404;
      body = fmt::format("File not found: {}", file_path);
      headers["Content-Type"] = "text/plain";
      return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    body.resize(size);
    if (!file.read(&body[0], size)) {
      log_message(LogLevel::ERROR,
                  fmt::format("Error reading file: {}", file_path));
      status_code = 500;
      body = "Internal Server Error";
      headers["Content-Type"] = "text/plain";
      return false;
    }

    headers["Content-Type"] = guess_mime_type(file_path);
    status_code = 200;
    return true;
  }

  /**
   * @brief Convert the response to a string suitable for HTTP transmission.
   * @return The formatted HTTP response as a string.
   */
  inline std::string to_string() const {
    std::ostringstream response_stream;

    response_stream << "HTTP/1.1 " << status_code << " ";
    switch (status_code) {
    case 100:
      response_stream << "Continue";
      break;
    case 101:
      response_stream << "Switching Protocols";
      break;
    case 200:
      response_stream << "OK";
      break;
    case 201:
      response_stream << "Created";
      break;
    case 202:
      response_stream << "Accepted";
      break;
    case 204:
      response_stream << "No Content";
      break;
    case 301:
      response_stream << "Moved Permanently";
      break;
    case 302:
      response_stream << "Found";
      break;
    case 304:
      response_stream << "Not Modified";
      break;
    case 400:
      response_stream << "Bad Request";
      break;
    case 401:
      response_stream << "Unauthorized";
      break;
    case 403:
      response_stream << "Forbidden";
      break;
    case 404:
      response_stream << "Not Found";
      break;
    case 405:
      response_stream << "Method Not Allowed";
      break;
    case 500:
      response_stream << "Internal Server Error";
      break;
    case 501:
      response_stream << "Not Implemented";
      break;
    case 503:
      response_stream << "Service Unavailable";
      break;
    default:
      response_stream << "Unknown Status";
      break;
    }
    response_stream << "\r\n";

    for (const auto &header : headers) {
      response_stream << header.first << ": " << header.second << "\r\n";
    }

    response_stream << "Content-Length: " << body.size() << "\r\n";
    response_stream << "\r\n";

    response_stream << body;

    return response_stream.str();
  }
};

// Type alias for a function that handles a request and prepares a response
using RouteHandler = std::function<void(const Request &, Response &)>;

} // namespace Haka

#endif // HAKA_CORE_HPP
