cmake_minimum_required(VERSION 3.14)
project(SmartFarmServer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(
  Crow
  GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
  GIT_TAG v1.0+5
)
FetchContent_MakeAvailable(Crow)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB REQUIRED libmariadb)

find_library(PAHO_MQTT_C paho-mqtt3as REQUIRED)
find_library(PAHO_MQTT_CPP paho-mqttpp3 REQUIRED)

find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(smartfarm_server ${SOURCES})

target_include_directories(smartfarm_server PRIVATE src ${MARIADB_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

target_link_libraries(smartfarm_server PRIVATE 
    Crow::Crow 
    cpr::cpr 
    nlohmann_json::nlohmann_json 
    Threads::Threads 
    ${MARIADB_LIBRARIES} 
    ${PAHO_MQTT_C} 
    ${PAHO_MQTT_CPP}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CURL_LIBRARIES}
)

