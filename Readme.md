# Haka Server

```
        ██   ██   █████   ██   ██   █████
        ██   ██  ██   ██  ██   ██  ██   ██
        ███████  ███████  █████    ███████
        ██   ██  ██   ██  ██   ██  ██   ██
        ██   ██  ██   ██  ██   ██  ██   ██
```

A lightweight C++ HTTP server built with **Asio (standalone)**, **fmtlib**, and **ylt/struct_json**.

---

## What is Haka?

Haka (pronounced *ha-ka*) means *Pangolin* in Shona, a language spoken in Zimbabwe. Pangolins are known for their protective scales, quiet demeanor, and often hidden lives. This name reflects the design goals for this server:

- **Safe**: Built on robust C++ libraries with a focus on correctness.
- **Fast & Lightweight**: Minimal dependencies, efficient asynchronous I/O with Asio.
- **Reliable**: Designed to handle connections efficiently.

This project aims for steady, thoughtful development, much like the patient nature of the pangolin.

---

## Features

- **Asynchronous request handling** using Asio standalone.
- Clean request and response object model.
- Routing support for HTTP `GET` and `POST` methods.
- Responds with:
  - Plain Text
  - HTML (from string)
  - JSON (via ylt/struct_json)
  - Static files with basic MIME type guessing.
- Basic logging with timestamps powered by fmtlib.
- Dependency management focused on header-only libraries (e.g., fmt, ylt/struct_json, Asio standalone).

---

## Recent Changes

### Modularized Headers (Header-Only Implementation)
- Moved definitions back into header files for a header-only library style.
- Added the `inline` keyword to avoid "multiple definition" linker errors.

### Debug Logging Flag
- Introduced a global flag `Haka::enable_debug_logging` to control debug output.
- Added command-line argument parsing (`-debug`) to enable debug logging.

### Route Grouping and Modular Mounting
- Added `group` and `mount` methods to the `Haka::Router` and `Haka::Server` classes for better route organization.

---

## Dependencies

To build and run Haka Server, ensure the following:

- **Asio Standalone**: Provides asynchronous networking capabilities.
- **fmtlib**: Enables safe and efficient formatted output.
- **ylt/struct_json**: Simplifies JSON serialization and deserialization.
- **C++17 or later**: Required for modern C++ features like `std::filesystem`.

### Platform-Specific Libraries
- **Windows**: Requires Winsock (`ws2_32`, `mswsock`).

---

## Building the Project

This project is designed as a header-only library. Using a build system like **CMake** is recommended.

### Using CMake

1. Clone or download this project.
2. Create a `build` folder inside the project directory.
3. Configure the project with CMake:
   ```bash
   cmake ..
   ```
4. Build the project:
   ```bash
   cmake --build . --config Release
   ```
5. Copy the contents of the `public` directory to the build output directory (if not handled automatically).
6. Run the executable:
   ```bash
   ./haka_example [-debug]
   ```

### Using g++ (Manual Compilation)

```bash
g++ -std=c++20 src/main.cpp src/users.cpp \
    -I/path/to/asio/include \
    -I/path/to/fmt/include \
    -I/path/to/yalantinglibs/include \
    -I./include \
    -lws2_32 -lmswsock -lstdc++fs -lfmt \
    -o build/haka_server
```

Replace `/path/to/...` with the actual paths to the include directories.

---

## Usage Examples

### Example `main.cpp`

```cpp
#include "Haka.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <chrono>
#include <cmath>

struct Product {
    int id;
    std::string name;
    double price;
};

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
```

---

## Contributing

Contributions are welcome! Areas for improvement include:

- Implementing a full HTTP request parser.
- Adding support for more HTTP methods (`PUT`, `DELETE`, etc.).
- Implementing a thread pool for `Asio`'s `io_context`.
- Adding SSL/TLS and WebSocket support.
- Enhancing error handling and response generation.
- Writing comprehensive tests.

Feel free to report issues or submit pull requests.

---

## License

This project is licensed under the MIT License:

```
MIT License

Copyright (c) 2025 KeithAGang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## Acknowledgments

This project is built upon the excellent work of:

- **Boost.Asio (standalone)**: By Christopher Kohlhoff.
- **fmtlib**: By Victor Zverovich.
- **ylt**: By Alibaba.

Their contributions to the C++ ecosystem are greatly appreciated.