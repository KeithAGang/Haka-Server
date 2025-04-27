#include "haka.hpp" // Your server header file
#include <fmt/format.h>     // For fmt::format
#include <vector>           // Used in the JSON example struct
#include <string>           // Used in various places
#include <iostream>         // For std::cerr
#include <stdexcept>        // For std::exception
#include <filesystem>       // To create the static directory if it doesn't exist

// --- Define a struct for JSON serialization ---

struct MyData {
    std::string message = "Default message";
    int value = 0;
    std::vector<std::string> items;
};


int main() {
    // Define host and port
    const std::string host = "127.0.0.1";
    const unsigned short port = 8080;
    const std::string static_dir = "./public";

    try {
        // --- Create the static directory if it doesn't exist ---
        // This is good practice for static file serving examples
        std::filesystem::create_directories(static_dir);
        Haka::log_message("INFO", fmt::format("Ensured static directory exists: {}", static_dir));

        // --- Create the server instance ---
        Haka::Server app(host, port);

        // --- Print the Haka Welcome Message ---
        // This could also be done inside Server::run() as shown before
        // We'll put it here for this example's main function structure
        fmt::print(fmt::fg(fmt::color::green), "{}\n", Haka::haka_text); // Using the constant from the header
        fmt::print(fmt::fg(fmt::color::blue) | fmt::emphasis::bold, "{}\n", "HAKA SERVER");
        fmt::print(fmt::fg(fmt::color::yellow), "Attempting to run on http://{}:{}\n\n", host, port);


        // --- Register Routes ---

        // 1. Basic Text Route
        app.Get("/", [](const Haka::Request& req, Haka::Response& res) {
            res.Text("Hello From Haka Server!");
            Haka::log_message("INFO", fmt::format("Handled / request with Text response"));
        });

        // 2. HTML Route from String
        app.Get("/info", [](const Haka::Request& req, Haka::Response& res) {
            std::string html_content = R"(
                <!DOCTYPE html>
                <html>
                <head><title>Haka Info</title>
                <meta charset="UTF-8">
                </head>
                <body>
                    <h1>Haka Server</h1>
                    <p>This is a basic HTTP server built with Asio, fmt, and ylt::struct_json.</p>
                    <p>Features demonstrated:</p>
                    <ul>
                        <li>Basic Text Response (<a href="/">/</a>)</li>
                        <li>HTML Response (<a href="/info">/info</a>)</li>
                        <li>JSON Response (<a href="/json">/json</a>)</li>
                        <li>Static File Serving (<a href="/static/index.html">/static/index.html</a>)</li>
                    </ul>
                    <p>Check the server console for logs!</p>
                </body>
                </html>
            )";
            res.HTML(html_content);
             Haka::log_message("INFO", fmt::format("Handled /info request with HTML response"));
        });

        // 3. JSON Route
        app.Get("/json", [](const Haka::Request& req, Haka::Response& res) {
            MyData data;
            data.message = "Data from Haka Server";
            data.value = 42;
            data.items = {"alpha", "beta", "gamma"};

            // Serialize the struct to JSON using the Response::JSON method
            res.JSON(data);
             Haka::log_message("INFO", fmt::format("Handled /json request with JSON response"));
        });

        // 4. Static File Serving
        // Files placed in ./public will be served under /static/
        app.serveStatic("/static", static_dir);
        // To test this, create a 'public' folder in your project root
        // and put an index.html file and maybe a style.css inside it.
        // e.g., public/index.html, public/css/style.css

        // --- Run the server ---
        // This call is blocking and will run the io_context.

        // 5. Post Request
        app.Post("/post", [](const Haka::Request& req, Haka::Response& res)
        {
          struct Response {
            std::string title;
            std::string message;
          };
          Response response {
            .title = "message",
            .message = "Post Successfull"
          };
          res.JSON(response);
        });

        app.run();

    } catch (const std::exception& e) {
        // Catch any exceptions during server setup or runtime
        Haka::log_message("FATAL", fmt::format("Exception during server startup or run: {}", e.what()));
        return -1; // Indicate failure
    } catch (...) {
        // Catch any other unhandled exceptions
         Haka::log_message("FATAL", "Unknown exception during server startup or run.");
         return -1; // Indicate failure
    }


    // Server::run() is blocking, so this code is typically not reached
    // unless the io_context is stopped from another thread or via signals.
    return 0; // Indicate success (if run was non-blocking or stopped gracefully)
}
