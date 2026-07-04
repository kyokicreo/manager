#include "session.h"

#include <spdlog/spdlog.h>
#include <boost/asio/detail/socket_ops.hpp>
#include <cstring>

namespace
{

    std::string commandTypeToString(fs::CommandType type)
    {
        switch (type)
        {
            case fs::LIST:    return "LIST";
            case fs::CREATE:  return "CREATE";
            case fs::REMOVE:  return "DELETE";
            case fs::HISTORY: return "HISTORY";
            default:          return "UNKNOWN";
        }
    }
}

Session::Session(tcp::socket socket, fileManager& manager, Database& database)
    : socket_(std::move(socket))
    , manager_(manager)
    , database_(database)
{
}

void Session::start()
{
    boost::system::error_code ec;
    auto endpoint = socket_.remote_endpoint(ec);
    if (!ec)
    {
        clientIp_ = endpoint.address().to_string();
    }
    else
    {
        clientIp_ = "unknown";
    }

    spdlog::info("Client connected: {}", clientIp_);

    readLength();
}

void Session::readLength()
{
    auto self = shared_from_this();

    boost::asio::async_read(
        socket_,
        boost::asio::buffer(lengthBuffer_),
        [this, self](const boost::system::error_code& ec, std::size_t)
        {
            if (ec)
            {
                spdlog::info("Client disconnected: {} ({})", clientIp_, ec.message());
                return;
            }

            uint32_t lengthNetworkOrder;
            std::memcpy(&lengthNetworkOrder, lengthBuffer_.data(), sizeof(lengthNetworkOrder));
            uint32_t length = boost::asio::detail::socket_ops::network_to_host_long(lengthNetworkOrder);

            constexpr uint32_t kMaxMessageSize = 16 * 1024 * 1024;
            if (length > kMaxMessageSize)
            {
                spdlog::warn("Client {} sent an oversized message ({} bytes), dropping connection", clientIp_, length);
                return;
            }

            readBody(length);
        });
}

void Session::readBody(uint32_t length)
{
    auto self = shared_from_this();

    bodyBuffer_.resize(length);

    boost::asio::async_read(
        socket_,
        boost::asio::buffer(bodyBuffer_),
        [this, self](const boost::system::error_code& ec, std::size_t)
        {
            if (ec)
            {
                spdlog::info("Client disconnected: {} ({})", clientIp_, ec.message());
                return;
            }

            fs::Command command;
            if (!command.ParseFromArray(bodyBuffer_.data(), static_cast<int>(bodyBuffer_.size())))
            {
                spdlog::warn("Client {} sent an invalid message, dropping connection", clientIp_);
                return;
            }

            handleCommand(command);
        });
}

void Session::handleCommand(const fs::Command& command)
{
    fs::Response response;
    std::string commandName = commandTypeToString(command.type());

    try
    {
        switch (command.type())
        {
            case fs::LIST:
                manager_.list(command.path(), response);
                break;
            case fs::CREATE:
                manager_.create(command.path(), response);
                break;
            case fs::REMOVE:
                manager_.remove(command.path(), response);
                break;
            case fs::HISTORY:
            {
                std::vector<std::string> history = database_.fetchHistory(50);
                for (const auto& line : history)
                {
                    response.add_data(line);
                }
                response.set_success(true);
                response.set_message("OK");
                break;
            }
            default:
                response.set_success(false);
                response.set_message("Unknown command type");
                break;
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("Unhandled exception while processing command: {}", e.what());
        response.set_success(false);
        response.set_message("Internal server error");
    }

    spdlog::info("[{}] command={} path='{}' success={} message='{}'",
        clientIp_, commandName, command.path(),
        response.success(), response.message());


    database_.logOperation(clientIp_, commandName, command.path(), response.success(), response.message());

    sendResponse(response);
}

void Session::sendResponse(const fs::Response& response)
{
    auto self = shared_from_this();

    auto body = std::make_shared<std::string>();
    if (!response.SerializeToString(body.get()))
    {
        spdlog::error("Failed to serialize response for client {}", clientIp_);
        return;
    }

    auto lengthNetworkOrder = std::make_shared<uint32_t>(
        boost::asio::detail::socket_ops::host_to_network_long(static_cast<uint32_t>(body->size())));

    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(lengthNetworkOrder.get(), sizeof(uint32_t)));
    buffers.push_back(boost::asio::buffer(*body));

    boost::asio::async_write(
        socket_,
        buffers,
        [this, self, body, lengthNetworkOrder](const boost::system::error_code& ec, std::size_t)
        {
            if (ec)
            {
                spdlog::info("Client disconnected: {} ({})", clientIp_, ec.message());
                return;
            }

            readLength();
        });
}