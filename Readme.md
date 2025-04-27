# Haka Server

```ascii
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

```

A lightweight C++ HTTP server built with Asio (standalone), fmtlib, and ylt/struct_json.

---

## What is Haka?

Haka (pronounced ha-ka) means Pangolin in Shona, one of the languages spoken in Zimbabwe. Pangolins are known for their protective scales, quiet demeanor, and often hidden lives. This name reflects the design goals for this server:

- **Safe**: Built on robust C++ libraries with a focus on correctness.
- **Fast & Lightweight**: Minimal dependencies, efficient asynchronous I/O with Asio.
- **Reliable**: Designed to handle connections efficiently.

This project aims for steady, thoughtful development, much like the patient nature of the pangolin.

---

## Features

- Asynchronous request handling using Asio standalone.
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

## Dependencies

To build and run Haka Server, ensure the following dependencies:

1. **Asio Standalone**:
     - Provides asynchronous networking capabilities.

2. **fmtlib**:
     - Enables safe and efficient formatted output (logging, messages).
     - Enables color formatted output.

3. **ylt/struct_json**:
     - Simplifies JSON serialization and deserialization through struct reflection.
     - Compatible with C++17/20.

4. **C++17 or later**:
     - Required for modern C++ features like `std::filesystem`.

---

### Platform-specific Libraries

- **Windows**: Requires Winsock (`ws2_32`, `mswsock`).

---

## Building the Project

This project currently uses a simple structure (single header and source file). For real-world use, employing a build system like CMake is recommended.

### Using CMake

Git clone or download this project, Make a build folder inside this project directory then Build the project with CMake, then run the `Makefile` or `.sln` and copy the contents of `build/include` into your project or include directory.  
You can also run the build/`haka_example` to test the basic features of this project.

### Using g++

Compile the project using g++ with C++20 support:

```bash
g++ -std=c++20 src/main.cpp -I/path/to/asio -I/path/to/fmt -I/path/to/ylt/ -lws2_32 -lmswsock -o build/haka_server
```

**Explanation**:
- Replace `/path/to/...` with the actual paths to Asio, fmtlib, and ylt/ on your system.
- The flags `-lws2_32` and `-lmswsock` link networking libraries necessary for Windows.

---

## Usage Examples

Here’s an example of how to use the `Haka::Server` in your `main.cpp`:

```cpp
#include "haka_server.hpp"
#include <fmt/format.h>
#include <vector>
#include <string>

// Define your data structure for JSON, with ylt/struct_json reflection set up elsewhere
struct MyData {
        std::string message;
        int value;
        std::vector<std::string> items;
};
// Reflection macros are required here: YLT_BEGIN_STRUCT(MyData) ... YLT_END_STRUCT()

int main() {
        try {
                Haka::Server app("127.0.0.1", 8080); // Create server on localhost:8080

                // --- Register Routes ---

                // Text response
                app.Get("/", [](const Haka::Request& req, Haka::Response& res) {
                        res.Text("Hello Haka!");
                });

                // HTML response
                app.Get("/page", [](const Haka::Request& req, Haka::Response& res) {
                        res.HTML("<h1>A Custom Page</h1><p>Served as HTML.</p>");
                });

                // JSON response
                app.Get("/api/data", [](const Haka::Request& req, Haka::Response& res) {
                        MyData response_data = {"Success", 99, {"item1", "item2"}};
                        res.JSON(response_data); // Requires MyData to have ylt/struct_json reflection
                });

                // --- Serve Static Files ---
                app.serveStatic("/assets", "./static_files");
                // Requests like /assets/image.png will look for ./static_files/image.png
                // Requests like /assets/ or /assets will look for ./static_files/index.html

                // --- Run the server (blocking call) ---
                app.run();

        } catch (const std::exception& e) {
                Haka::log_message("FATAL", fmt::format("Server crashed: {}", e.what()));
                return -1;
        }
        return 0;
}
```

---

### Static File Serving

To use the static file serving functionality:

1. Create a directory named `static_files` (or the path you specify in `serveStatic`) in your project's root.
2. Place files like `index.html`, `style.css`, `script.js`, or images in this directory.

---

## Contributing

Contributions are welcome! This project is a work in progress and can be improved in several areas, such as:

- Implementing a full HTTP request parser (including body parsing for POST/PUT).
- Adding support for more HTTP methods (`PUT`, `DELETE`, `PATCH`, `HEAD`, `OPTIONS`).
- Implementing a thread pool for the Asio `io_context` to handle multiple connections concurrently.
- Adding support for SSL/TLS and Websockets.
- Enhancing error handling and response generation.
- Adding request routing with path parameters (e.g., `/users/{id}`).
- Writing comprehensive tests.
- Improving documentation and examples.

If you’d like to contribute, feel free to report issues or submit pull requests on the repository (once hosted).

---

## License

This project is licensed under the MIT License:

```
MIT License

Copyright (c) [2025] [KeithAGang]

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
- **ylt/struct_json**: By Alibaba.

Their contributions to the C++ ecosystem are greatly appreciated.