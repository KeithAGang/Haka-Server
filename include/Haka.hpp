#ifndef HAKA_HPP
#define HAKA_HPP

// This is the main header file for the Haka HTTP Server library.
// Including this file provides access to all core components,
// the router for defining routes, and the server itself.

// Include the core types and utilities
#include "haka/core.hpp"

// Include the router for defining request handlers and static files
#include "haka/router.hpp"

// Include the server class for running the HTTP server
#include "haka/server.hpp"

// Optional: You could add using directives here if you want users
// to be able to use Haka components without the Haka:: prefix,
// but it's generally better practice to require the namespace.
// using Haka::Request;
// using Haka::Response;
// using Haka::RouteHandler;
// using Haka::Router;
// using Haka::Server;

#endif // HAKA_HPP
