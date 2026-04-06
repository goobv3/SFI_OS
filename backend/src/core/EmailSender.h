#pragma once

#include <string>

namespace Core {
namespace EmailSender {

bool sendAlert(const std::string& sensor_alias, const std::string& level, double value, const std::string& message);

}
}
