#pragma once
#include "crow.h"
#include "crow/middlewares/cors.h"

namespace API {
class Router {
public:
    static void setupRoutes(crow::App<crow::CORSHandler>& app);
};
}
