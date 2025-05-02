#ifndef HAKA_ROUTER_HPP
#define HAKA_ROUTER_HPP

// Project includes
#include "haka/core.hpp" // For Request, Response, RouteHandler, log_message

#include <climits>
#include <filesystem> // For path manipulation and checks
#include <functional> // For std::function
#include <sstream>    // For std::istringstream
#include <string>
#include <unordered_map>
#include <utility> // For std::pair
#include <vector>

namespace Haka {

/**
 * @brief Manages the mapping of incoming requests (method, path) to
 * the appropriate RouteHandler functions. Supports static file serving
 * and route grouping.
 */
class Router {
public:
  /**
   * @brief Constructor for the Router.
   * Initializes the current group prefix to the root ("").
   */
  inline Router() {
    // Constructor implementation (currently empty, members are initialized
    // above)
  }

  /**
   * @brief Registers a handler for GET requests at a specific path.
   * The path is combined with the current group prefix.
   * @param path The URL path segment for this route.
   * @param handler The function to execute for this route.
   */
  inline void Get(const std::string &path, RouteHandler handler) {
    add_route("GET", path, handler);
  }

  /**
   * @brief Registers a handler for POST requests at a specific path.
   * The path is combined with the current group prefix.
   * @param path The URL path segment for this route.
   * @param handler The function to execute for this route.
   */
  inline void Post(const std::string &path, RouteHandler handler) {
    add_route("POST", path, handler);
  }

  // TODO: Add methods for other HTTP methods (Put, Delete, Patch, Options,
  // Head)

  /**
   * @brief Registers a directory for serving static files under a specific URL
   * prefix. Files requested under the URL prefix will be served from the
   * corresponding location in the filesystem path. Includes basic path
   * traversal protection.
   * @param path_prefix The URL prefix (e.g., "/static").
   * @param fs_path The filesystem path (e.g., "./public").
   */
  inline void serveStatic(const std::string &path_prefix,
                          const std::string &fs_path) {
    // Normalize the URL prefix: ensure it starts with '/' and remove trailing
    // '/' unless it's just "/"
    std::string clean_prefix = normalize_path_segment(path_prefix);

    // Store the cleaned prefix and the filesystem path
    static_paths_.push_back({clean_prefix, fs_path});
    log_message(LogLevel::INFO,
                fmt::format("Serving static files from '{}' at URL prefix '{}'",
                            fs_path, clean_prefix));
  }

  /**
   * @brief Defines a group of routes that share a common URL prefix.
   * Handlers defined within the config_func will have the prefix applied.
   * Groups can be nested.
   * @param prefix The URL prefix for this group.
   * @param config_func A function (typically a lambda) that defines routes
   * within the group.
   */
  inline void group(const std::string &prefix,
                    std::function<void(Router &)> config_func) {
    // Store the current prefix to restore it later (for nested groups)
    std::string current_prefix_backup = current_group_prefix_;

    // Append the new prefix, ensuring proper normalization
    current_group_prefix_ = normalize_path_segment(
        current_group_prefix_ + normalize_path_segment(prefix));

    log_message(LogLevel::INFO,
                fmt::format("Entering route group with prefix: {}",
                            current_group_prefix_));

    // Execute the configuration function. Any routes defined inside will use
    // the new prefix.
    config_func(*this);

    log_message(LogLevel::INFO,
                fmt::format("Exiting route group. Restoring prefix: {}",
                            current_prefix_backup));

    // Restore the previous prefix
    current_group_prefix_ = current_prefix_backup;
  }

  /**
   * @brief Mounts another Router's routes and static paths under a specific
   * prefix into this Router. This is useful for creating modular route
   * definitions.
   * @param prefix The URL prefix under which the other router's routes will be
   * mounted.
   * @param other_router The Router instance whose routes/static paths will be
   * merged.
   */
  inline void mount(const std::string &prefix, const Router &other_router) {
    std::string mount_prefix = normalize_path_segment(prefix);
    log_message(LogLevel::INFO,
                fmt::format("Mounting router at prefix: {}", mount_prefix));

    // Merge explicit routes
    for (const auto &pair : other_router.routes_) {
      // pair.first is "METHOD /path"
      std::istringstream key_stream(pair.first);
      std::string method, path;
      key_stream >> method >> path; // Extract method and path

      // Combine the mount prefix with the route's path
      std::string full_path =
          normalize_path_segment(mount_prefix + normalize_path_segment(path));

      // Add the route to this router
      routes_[method + " " + full_path] = pair.second;
      log_message(LogLevel::INFO,
                  fmt::format("   Mounted route: {} {}", method, full_path));
    }

    // Merge static paths
    for (const auto &static_entry : other_router.static_paths_) {
      const std::string &other_prefix =
          static_entry.first; // Prefix in the other router
      const std::string &fs_path = static_entry.second;

      // Combine the mount prefix with the other router's static prefix
      std::string full_static_prefix = normalize_path_segment(
          mount_prefix + normalize_path_segment(other_prefix));

      // Add the static path to this router
      static_paths_.push_back({full_static_prefix, fs_path});
      log_message(
          LogLevel::INFO,
          fmt::format(
              "   Mounted static path: '{}' from '{}' at URL prefix '{}'",
              fs_path, other_prefix, full_static_prefix));
    }
  }

  /**
   * @brief Finds the appropriate handler for a given request.
   * Checks static file routes first, then registered explicit routes.
   * Returns a 404 handler if no match is found.
   * @param req The incoming Request object.
   * @return The RouteHandler function to process the request.
   */
  inline RouteHandler match(const Request &req) const {
    log_message(LogLevel::DEBUG,
                fmt::format("Attempting to match request: {} {}", req.method,
                            req.path));

    // 1. Check Static Files first
    for (const auto &static_entry : static_paths_) {
      const std::string &url_prefix = static_entry.first;
      const std::string &fs_root = static_entry.second;

      log_message(LogLevel::DEBUG,
                  fmt::format(" Checking static prefix: '{}' serving from '{}'",
                              url_prefix, fs_root));

      // Check if the request path starts with the static URL prefix
      // We need to handle the case where the prefix is "/"
      bool prefix_matches =
          (url_prefix == "/" && req.path == "/") ||
          (url_prefix != "/" && req.path_starts_with(url_prefix + "/"));

      // Special case: if url_prefix is "/" and req.path starts with "/", it's a
      // match
      if (url_prefix == "/" && req.path.starts_with("/"))
        prefix_matches = true;

      if (prefix_matches) {
        log_message(
            LogLevel::DEBUG,
            fmt::format("  Request path '{}' matches static prefix '{}'",
                        req.path, url_prefix));
        // Get the path relative to the prefix
        std::string file_sub_path;
        if (url_prefix == "/") {
          file_sub_path =
              req.path; // If prefix is root, subpath is the whole path
        } else {
          file_sub_path = req.path_after_prefix(url_prefix);
        }

        // For requests ending in the prefix or "/", append "index.html"
        if (file_sub_path.empty() || file_sub_path == "/") {
          file_sub_path = "/index.html";
        }

        // Construct the full filesystem path
        // Ensure fs_root is treated as a directory base
        std::filesystem::path fs_root_path = std::filesystem::absolute(fs_root);
        // Remove leading slash from file_sub_path for joining
        std::filesystem::path relative_path =
            (file_sub_path.length() > 0 && file_sub_path[0] == '/')
                ? file_sub_path.substr(1)
                : file_sub_path;
        std::filesystem::path full_fs_path = fs_root_path / relative_path;

        log_message(LogLevel::DEBUG,
                    fmt::format("  Attempting to serve file: {}",
                                full_fs_path.string()));

        // Basic security check: ensure the resolved path is within the served
        // directory This helps prevent directory traversal attacks.
        try {
          // Use canonical to resolve symlinks and ..
          std::filesystem::path canonical_fs_root =
              std::filesystem::canonical(fs_root_path);
          std::filesystem::path canonical_full_path =
              std::filesystem::canonical(full_fs_path);

          if (!canonical_full_path.string().starts_with(
                  canonical_fs_root.string())) {
            log_message(
                LogLevel::WARN,
                fmt::format("Attempted directory traversal: {}", req.path));
            // Return a handler for Bad Request
            return [](const Request &r, Response &res) {
              res.status_code = 400; // Bad Request
              res.Text("Invalid path.");
            };
          }
        } catch (const std::filesystem::filesystem_error &e) {
          // If canonical fails (e.g., file doesn't exist), it's not necessarily
          // an error, but we should log the attempt and let the exists check
          // handle it.
          log_message(
              LogLevel::DEBUG,
              fmt::format(
                  "Filesystem error during canonical path check for {}: {}",
                  req.path, e.what()));
          // Continue to exists check below
        }

        // Check if the file exists and is a regular file
        if (std::filesystem::exists(full_fs_path) &&
            std::filesystem::is_regular_file(full_fs_path)) {
          log_message(LogLevel::INFO, fmt::format("Serving static file: {}",
                                                  full_fs_path.string()));
          // Return a handler that serves the file
          return [file_path = full_fs_path.string()](const Request &r,
                                                     Response &res) {
            if (!res.sendFile(file_path)) {
              // sendFile already logs errors and sets 500 status on failure
              // No need to do more here unless you want a different 500 message
            }
          };
        } else {
          log_message(
              LogLevel::DEBUG,
              fmt::format("  Static file not found or not a regular file: {}",
                          full_fs_path.string()));
          // Continue to check explicit routes if static file not found
        }
      } else {
        log_message(
            LogLevel::DEBUG,
            fmt::format("  Request path '{}' does NOT match static prefix '{}'",
                        req.path, url_prefix));
      }
    }

    // 2. Check registered explicit routes
    std::string normalized_req_path = normalize_path_segment(req.path);
    std::string lookup_key = req.method + " " + normalized_req_path;
    log_message(
        LogLevel::DEBUG,
        fmt::format(" Checking explicit route for key: '{}'", lookup_key));

    auto it = routes_.find(lookup_key);
    if (it != routes_.end()) {
      log_message(LogLevel::INFO, fmt::format("Matched explicit route: {} {}",
                                              req.method, req.path));
      return it->second; // Return the found handler
    } else {
      log_message(
          LogLevel::DEBUG,
          fmt::format(" No explicit route found for key: '{}'", lookup_key));
    }

    // 3. No match found - return a 404 Not Found handler
    log_message(LogLevel::INFO,
                fmt::format("Route not found: {} {}", req.method, req.path));
    return [](const Request &r, Response &res) {
      res.status_code = 404;
      res.Text(fmt::format("Not found: {}", r.path));
    };
  }

private:
  /**
   * @brief Helper function to add a route to the internal map,
   * combining the current group prefix with the route path.
   * @param method The HTTP method (e.g., "GET", "POST").
   * @param path The route path segment.
   * @param handler The handler function.
   */
  inline void add_route(const std::string &method, const std::string &path,
                        RouteHandler handler) {
    // Combine the current group prefix with the route path
    std::string full_path = normalize_path_segment(
        current_group_prefix_ + normalize_path_segment(path));

    // Store the handler mapped to "METHOD /full/path"
    routes_[method + " " + full_path] = handler;
    log_message(LogLevel::INFO,
                fmt::format("Registered route: {} {}", method, full_path));
  }

  /**
   * @brief Helper to normalize a path segment, ensuring it starts with '/'
   * and doesn't end with '/' unless it's just "/".
   * @param path The path segment to normalize.
   * @return The normalized path string.
   */
  inline std::string normalize_path_segment(const std::string &path) const {
    std::string normalized = path;
    // Ensure path starts with '/'
    if (normalized.empty() || normalized[0] != '/') {
      normalized = "/" + normalized;
    }
    // Remove trailing slash unless it's just "/"
    if (normalized.length() > 1 && normalized.back() == '/') {
      normalized.pop_back();
    }
    return normalized;
  }

  // Internal storage for explicit routes: maps "METHOD /full/path" to handler
  std::unordered_map<std::string, RouteHandler> routes_;

  // Internal storage for static file configurations: {url_prefix, fs_path}
  std::vector<std::pair<std::string, std::string>> static_paths_;

  // Internal state to track the current prefix when defining routes within a
  // group
  std::string current_group_prefix_ =
      ""; // Start with empty prefix for the root level
};

} // namespace Haka

#endif // HAKA_ROUTER_HPP
