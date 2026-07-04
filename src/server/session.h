#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <vector>
#include <array>

#include "file.pb.h"
#include "manager.h"
#include "database.h"

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket, fileManager& manager, Database& database);

    void start();

private:
    void readLength();
    void readBody(uint32_t length);
    void handleCommand(const fs::Command& command);
    void sendResponse(const fs::Response& response);

    tcp::socket socket_;
    fileManager& manager_;
    Database& database_;

    std::array<char, 4> lengthBuffer_;
    std::vector<char> bodyBuffer_;

    std::string clientIp_;
};