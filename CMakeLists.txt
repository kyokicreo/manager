cmake_minimum_required(VERSION 3.15)
project(manager LANGUAGES CXX)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)

if(WIN32)
    find_package(Protobuf CONFIG REQUIRED)
else()
    find_package(Protobuf REQUIRED)
endif()

find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED IMPORTED_TARGET libpqxx)

set(PROTO_FILE ${CMAKE_SOURCE_DIR}/proto/file.proto)
set(PROTO_SRCS ${CMAKE_CURRENT_BINARY_DIR}/file.pb.cc)
set(PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR}/file.pb.h)

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
    COMMAND protobuf::protoc
    ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
         -I${CMAKE_SOURCE_DIR}/proto
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating C++ code from file.proto"
    VERBATIM
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(WIN32)
    set(PLATFORM_NETWORK_LIBS ws2_32 mswsock)
else()
    set(PLATFORM_NETWORK_LIBS "")
endif()


add_executable(server
    src/server/main.cpp
    src/server/manager.cpp
    src/server/config.cpp
    src/server/session.cpp
    src/server/database.cpp
    ${PROTO_SRCS}
)

target_link_libraries(server PRIVATE
    Boost::system
    protobuf::libprotobuf
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    PkgConfig::PQXX
    ${PLATFORM_NETWORK_LIBS}
)


add_executable(client
    src/client/main.cpp
    src/client/config.cpp
    src/client/network.cpp
    ${PROTO_SRCS}
)

target_link_libraries(client PRIVATE
    Boost::system
    protobuf::libprotobuf
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    ${PLATFORM_NETWORK_LIBS}
)