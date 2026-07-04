#pragma once

#include <string>
#include <vector>
#include <memory>

#include "config.h"

namespace pqxx
{
    class connection;
}


class Database
{
public:
    explicit Database(const DbConfig& config);
    ~Database();

    void ensureTableExists();

    void logOperation(const std::string& clientIp,
                       const std::string& command,
                       const std::string& path,
                       bool success,
                       const std::string& message);


    std::vector<std::string> fetchHistory(int limit);

private:
    std::unique_ptr<pqxx::connection> connection_;
};