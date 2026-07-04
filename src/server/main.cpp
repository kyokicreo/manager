#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <boost/asio.hpp>

#include "config.h"
#include "manager.h"
#include "database.h"
#include "session.h"

using boost::asio::ip::tcp;

class Server
{
public:
    Server(boost::asio::io_context& io, unsigned short port, fileManager& manager, Database& database)
        : acceptor_(io, tcp::endpoint(tcp::v4(), port))
        , manager_(manager)
        , database_(database)
    {
    }

    void start()
    {
        acceptOne();
    }

private:
    void acceptOne()
    {
        acceptor_.async_accept(
            [this](const boost::system::error_code& ec, tcp::socket socket)
            {
                if (!ec)
                {
                    auto session = std::make_shared<Session>(std::move(socket), manager_, database_);
                    session->start();
                }
                else
                {
                    spdlog::error("Accept failed: {}", ec.message());
                }


                acceptOne();
            });
    }

    tcp::acceptor acceptor_;
    fileManager& manager_;
    Database& database_;
};

int main(int argc, char** argv)
{
    std::string configPath = argc > 1 ? argv[1] : "config/server_config.json";

    ServerConfig cfg;
    try
    {
        cfg = loadServerConfig(configPath);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to load config: " << e.what() << std::endl;
        return 1;
    }


    try
    {
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            cfg.logging.file,
            static_cast<size_t>(cfg.logging.max_size_mb) * 1024 * 1024,
            static_cast<size_t>(cfg.logging.max_files));

        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        auto logger = std::make_shared<spdlog::logger>(
            "server", spdlog::sinks_init_list{fileSink, consoleSink});

        spdlog::set_default_logger(logger);

        if (cfg.logging.level == "debug")
            spdlog::set_level(spdlog::level::debug);
        else if (cfg.logging.level == "warning")
            spdlog::set_level(spdlog::level::warn);
        else if (cfg.logging.level == "error")
            spdlog::set_level(spdlog::level::err);
        else
            spdlog::set_level(spdlog::level::info);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to set up logging: " << e.what() << std::endl;
        return 1;
    }

    spdlog::info("Server starting on port {}, root_dir='{}'", cfg.port, cfg.root_dir);

    fileManager manager(cfg.root_dir);

    try
    {
        spdlog::info("Connecting to database...");


        std::unique_ptr<Database> database;
        const int maxAttempts = 10;
        const int delaySeconds = 2;

        for (int attempt = 1; attempt <= maxAttempts; ++attempt)
        {
            try
            {
                database = std::make_unique<Database>(cfg.database);
                break; 
            }
            catch (const std::exception& e)
            {
                if (attempt == maxAttempts)
                {
                    throw; 
                }
                spdlog::warn("Database connection attempt {}/{} failed: {}. Retrying in {}s...",
                    attempt, maxAttempts, e.what(), delaySeconds);
                std::this_thread::sleep_for(std::chrono::seconds(delaySeconds));
            }
        }

        spdlog::info("Ensuring database table exists...");
        database->ensureTableExists();

        boost::asio::io_context io;
        Server server(io, static_cast<unsigned short>(cfg.port), manager, *database);
        server.start();

        spdlog::info("Server is running. Waiting for connections...");
        io.run();
    }
    catch (const std::exception& e)
    {
        spdlog::critical("Fatal server error: {}", e.what());
        std::cerr << "Fatal server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}