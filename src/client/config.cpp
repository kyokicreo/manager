#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

ClientConfig loadClientConfig(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open config file: " + path);
    }

    nlohmann::json j;
    file >> j;

    ClientConfig cfg;
    cfg.server_host = j.at("server_host").get<std::string>();
    cfg.server_port = j.at("server_port").get<int>();
    cfg.timeout_ms = j.at("timeout_ms").get<int>();

    return cfg;
}