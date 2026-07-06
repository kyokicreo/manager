#include "database.h"

#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <sstream>

Database::Database(const DbConfig& config)
{
    std::ostringstream connStr;
    connStr << "host=" << config.host
            << " port=" << config.port
            << " dbname=" << config.dbname
            << " user=" << config.user
            << " password=" << config.password;

    connection_ = std::make_unique<pqxx::connection>(connStr.str());
    spdlog::info("Connected to PostgreSQL at {}:{}, database '{}'", config.host, config.port, config.dbname);
}

Database::~Database() = default;

void Database::ensureTableExists()
{
    try
    {
        pqxx::work txn(*connection_);
        txn.exec(
            "CREATE TABLE IF NOT EXISTS operations ("
            "id SERIAL PRIMARY KEY, "
            "ts TIMESTAMP NOT NULL DEFAULT NOW(), "
            "client_ip TEXT NOT NULL, "
            "command TEXT NOT NULL, "
            "path TEXT NOT NULL, "
            "success BOOLEAN NOT NULL, "
            "message TEXT NOT NULL"
            ")"
        );
        txn.commit();
        spdlog::info("Database table 'operations' is ready");
    }
    catch (const std::exception& e)
    {
        spdlog::critical("Failed to ensure database table exists: {}", e.what());
        throw;
    }
}

void Database::logOperation(const std::string& clientIp,
                             const std::string& command,
                             const std::string& path,
                             bool success,
                             const std::string& message)
{
    try
    {
        pqxx::work txn(*connection_);

        txn.exec_params(
            "INSERT INTO operations (client_ip, command, path, success, message) "
            "VALUES ($1, $2, $3, $4, $5)",
            clientIp, command, path, success, message
        );
        txn.commit();
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to log operation to database: {}", e.what());
    }
}

std::vector<std::string> Database::fetchHistory(int limit)
{
    std::vector<std::string> result;

    try
    {
        pqxx::work txn(*connection_);
        pqxx::result rows = txn.exec_params(
            "SELECT ts, client_ip, command, path, success, message "
            "FROM operations ORDER BY ts DESC LIMIT $1",
            limit
        );

        for (const auto& row : rows)
        {
            std::ostringstream line;
            line << row["ts"].c_str() << " UTC | "
                 << row["client_ip"].c_str() << " | "
                 << row["command"].c_str() << " | "
                 << row["path"].c_str() << " | "
                 << (row["success"].as<bool>() ? "OK" : "FAILED") << " | "
                 << row["message"].c_str();
            result.push_back(line.str());
        }

        txn.commit();

        std::reverse(result.begin(), result.end());
    }
    catch (const std::exception& e)
    {
        spdlog::error("Failed to fetch history from database: {}", e.what());
    }

    return result;
}