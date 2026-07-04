#include "config.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

ServerConfig loadServerConfig(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open config file: " + path);
    }

    nlohmann::json j;
    file >> j;

    ServerConfig cfg;

    cfg.port = j.at("port").get<int>();
    cfg.root_dir = j.at("root_dir").get<std::string>();

    const auto& db = j.at("database");
    cfg.database.host = db.at("host").get<std::string>();
    cfg.database.port = db.at("port").get<int>();
    cfg.database.dbname = db.at("dbname").get<std::string>();
    cfg.database.user = db.at("user").get<std::string>();
    cfg.database.password = db.at("password").get<std::string>();

    const auto& log = j.at("logging");
    cfg.logging.level = log.at("level").get<std::string>();
    cfg.logging.file = log.at("file").get<std::string>();
    cfg.logging.max_size_mb = log.at("max_size_mb").get<int>();
    cfg.logging.max_files = log.at("max_files").get<int>();

    return cfg;
}