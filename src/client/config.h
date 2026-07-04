#pragma once
#include <string>

struct ClientConfig
{
    std::string server_host;
    int server_port;
    int timeout_ms;
};

ClientConfig loadClientConfig(const std::string& path);