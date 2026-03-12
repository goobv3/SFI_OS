#pragma once
#include "crow.h"

namespace API {
class Router {
public:
    static void setupRoutes(crow::SimpleApp& app);
};
}
