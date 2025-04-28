#include "Haka.hpp" // Include the main Haka header
#include <iostream>
#include <vector> // Needed for std::vector
#include <random> // Needed for generating random product data
#include <string> // Needed for string manipulation
#include <chrono> // Needed for random seed
#include <cmath> // Needed for std::round

// Forward declaration for the function that creates the user API router.
// This function is implemented in users.cpp.
Haka::Router createUserApiRouter(); // Function now returns a Router instance

// Define a simple custom struct for JSON serialization.
// This struct is placed here because it's used directly in routes defined in main.cpp.
// For a larger project, common structs would go in a shared header.
struct Product {
    int id;
    std::string name;
    double price;
};

// STRUCT_JSON_DEFINE is not needed due to compile-time reflection


int main(int argc, char* argv[]) { // Added command-line arguments

    // Check for the debug flag
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-debug") {
            Haka::enable_debug_logging = true;
            Haka::log_message("INFO", "Debug logging enabled.");
            break; // Found the flag, no need to check further
        }
    }

    // Define the host and port for the server to listen on.
    std::string host = "127.0.0.1"; // Listen only on localhost
    unsigned short port = 8080;     // Listen on port 8080

    // Create a Haka server instance.
    Haka::Server server(host, port);

    // --- Define Basic Routes Directly on the Server ---

    // GET route returning plain text at the root path "/".
    server.Get("/", [](const Haka::Request& req, Haka::Response& res) {
        res.Text("Welcome to Haka Server!");
    });

    // GET route returning HTML at "/hello".
    server.Get("/hello", [](const Haka::Request& req, Haka::Response& res) {
        res.HTML("<h1>Hello, Haka!</h1><p>This is an HTML response from the /hello route.</p>");
    });

    // GET route returning JSON using the default JsonResponse struct at "/status".
    server.Get("/status", [](const Haka::Request& req, Haka::Response& res) {
        Haka::JsonResponse status_data;
        status_data.title = "Server Status";
        status_data.message = "Haka server is operational and ready!";
        res.status_code = 200; // OK
        res.JSON(status_data); // Serialize and send the JSON response
    });

    // GET route returning custom JSON using the Product struct at "/product/1".
    server.Get("/product/1", [](const Haka::Request& req, Haka::Response& res) {
        Product example_product = {101, "Example Gadget", 19.99};
        res.status_code = 200; // OK
        res.JSON(example_product); // Serialize and send the custom JSON response
    });

    // --- New Route: /info ---
    // GET route returning HTML with links to available routes.
    server.Get("/info", [](const Haka::Request& req, Haka::Response& res) {
        std::string html_body = R"(
            <!DOCTYPE html>
            <html lang="en">
            <head>
                <meta charset="UTF-8">
                <meta name="viewport" content="width=device-width, initial-scale=1.0">
                <title>Haka Server Info</title>
                <style>
                    body { font-family: sans-serif; margin: 20px; }
                    h1 { color: #333; }
                    ul { list-style: none; padding: 0; }
                    li { margin-bottom: 10px; }
                    a { text-decoration: none; color: #007bff; }
                    a:hover { text-decoration: underline; }
                </style>
            </head>
            <body>
                <h1>Haka Server Info</h1>
                <p>Available Routes:</p>
                <ul>
                    <li><a href="/">GET /</a> - Welcome Message</li>
                    <li><a href="/hello">GET /hello</a> - HTML Greeting</li>
                    <li><a href="/status">GET /status</a> - Server Status (JSON)</li>
                    <li><a href="/product/1">GET /product/1</a> - Example Product (JSON)</li>
                    <li><a href="/info">GET /info</a> - This Info Page (HTML)</li>
                    <li><a href="/json">GET /json</a> - 15 Products List (JSON)</li>
                    <li><a href="/api/users/list">GET /api/users/list</a> - List Users (JSON)</li>
                    <li><a href="/api/users/profile">GET /api/users/profile</a> - User Profile (JSON)</li>
                    <li><a href="/static/">Static Files (e.g., /static/index.html)</a> - Serves from ./public/</li>
                </ul>
                <p>Note: Other HTTP methods (POST, etc.) might be available but are not listed here.</p>
            </body>
            </html>
        )";
        res.status_code = 200; // OK
        res.HTML(html_body);
    });


    // --- New Route: /json ---
    // GET route returning a vector of 15 Product objects as JSON.
    server.Get("/json", [](const Haka::Request& req, Haka::Response& res) {
        std::vector<Product> products;
        products.reserve(15); // Reserve space for 15 products

        // Simple random number generation for demo purposes
        std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_real_distribution<double> price_dist(1.0, 100.0);

        for (int i = 0; i < 15; ++i) {
            products.push_back({i + 1, fmt::format("Product {}", i + 1), std::round(price_dist(rng) * 100.0) / 100.0});
        }

        res.status_code = 200; // OK
        res.JSON(products); // Serialize and send the vector as a JSON array
    });


    // --- Mount Modular Router for User API ---
    // Call the function from users.cpp to create and configure the user API router instance.
    Haka::Router user_api_router = createUserApiRouter();

    // Mount the user API router under the "/api/users" prefix on the main server's router.
    // Routes defined in user_api_router (like "/list", "/profile") will now be accessible
    // under this prefix (e.g., "/api/users/list", "/api/users/profile").
    server.mount("/api/users", user_api_router);


    // --- Serve Static Files ---
    // This will serve files from the "./public" directory under the "/static" URL prefix.
    // Create a 'public' directory in the same location as your executable
    // and put files like index.html, style.css, etc. inside it.
    // For example, "./public/index.html" will be accessible at "http://127.0.0.1:8080/static/index.html".
    server.serveStatic("/static", "./public");


    // --- Start the Server ---
    // This call is blocking and will run the server until interrupted (e.g., Ctrl+C).
    try {
        fmt::print(fg(fmt::color::cyan), "Starting Haka server...\n");
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
