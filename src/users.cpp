#include <haka.hpp> // Include Haka to access Router, Request, Response, JsonResponse

// Define a simple custom struct for JSON serialization within this file's scope
// If User struct was defined in core.hpp or a common header, you wouldn't need this here.
struct User {
    int id;
    std::string name;
};



/**
 * @brief Creates and configures a Router specifically for user-related API endpoints.
 * @return A configured Haka::Router instance.
 */
Haka::Router createUserApiRouter() {
    Haka::Router user_router; // Create a new Router instance

    // Define routes on this new router instance

    // GET /list (will be mounted under a prefix in main.cpp)
    user_router.Get("/list", [](const Haka::Request& req, Haka::Response& res) {
        // Example of returning a vector of custom structs as JSON
        std::vector<User> users = {{1, "Alice"}, {2, "Bob"}, {3, "Charlie"}};
        res.status_code = 200; // OK
        res.JSON(users); // struct_json can serialize vectors
    });

    // GET /profile (will be mounted under a prefix in main.cpp)
    user_router.Get("/profile", [](const Haka::Request& req, Haka::Response& res) {
         Haka::JsonResponse profile_data;
         profile_data.title = "User Profile";
         profile_data.message = "User profile details from the modular router.";
         res.status_code = 200;
         res.JSON(profile_data);
    });

    // You can define groups or static paths within this modular router too
    // user_router.group("/admin", [](Haka::Router& admin_router) {
    //     admin_router.Get("/dashboard", ...); // This would be /users/admin/dashboard if mounted at /users
    // });

    return user_router; // Return the configured router
}
