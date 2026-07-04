#include "network.h"

#include <boost/asio.hpp>
#include <stdexcept>
#include <vector>
#include <cstdint>

using boost::asio::ip::tcp;

namespace
{
    void writeAll(tcp::socket& socket, const void* data, std::size_t size)
    {
        boost::asio::write(socket, boost::asio::buffer(data, size));
    }

    void readAll(tcp::socket& socket, void* data, std::size_t size)
    {
        boost::asio::read(socket, boost::asio::buffer(data, size));
    }

    void sendMessage(tcp::socket& socket, const google::protobuf::MessageLite& message)
    {
        std::string body;
        if (!message.SerializeToString(&body))
        {
            throw std::runtime_error("Failed to serialize protobuf message");
        }

        uint32_t length = static_cast<uint32_t>(body.size());
        uint32_t lengthNetworkOrder = boost::asio::detail::socket_ops::host_to_network_long(length);

        writeAll(socket, &lengthNetworkOrder, sizeof(lengthNetworkOrder));
        writeAll(socket, body.data(), body.size());
    }

    void receiveMessage(tcp::socket& socket, google::protobuf::MessageLite& outMessage)
    {
        uint32_t lengthNetworkOrder = 0;
        readAll(socket, &lengthNetworkOrder, sizeof(lengthNetworkOrder));
        uint32_t length = boost::asio::detail::socket_ops::network_to_host_long(lengthNetworkOrder);

        constexpr uint32_t kMaxMessageSize = 16 * 1024 * 1024;
        if (length > kMaxMessageSize)
        {
            throw std::runtime_error("Message size too large, possible protocol desync");
        }

        std::vector<char> body(length);
        if (length > 0)
        {
            readAll(socket, body.data(), body.size());
        }

        if (!outMessage.ParseFromArray(body.data(), static_cast<int>(body.size())))
        {
            throw std::runtime_error("Failed to parse protobuf message");
        }
    }
}

fs::Response sendCommand(const ClientConfig& cfg, const fs::Command& command)
{
    try
    {
        boost::asio::io_context io;

        tcp::resolver resolver(io);
        auto endpoints = resolver.resolve(cfg.server_host, std::to_string(cfg.server_port));

        tcp::socket socket(io);

        boost::system::error_code connectError;
        boost::asio::connect(socket, endpoints, connectError);

        if (connectError)
        {
            throw std::runtime_error("Failed to connect to server: " + connectError.message());
        }

        sendMessage(socket, command);

        fs::Response response;
        receiveMessage(socket, response);

        return response;
    }
    catch (const boost::system::system_error& e)
    {
        throw std::runtime_error(std::string("Network error: ") + e.what());
    }
}