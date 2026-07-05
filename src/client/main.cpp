#include <iostream>
#include <sstream>
#include <string>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#undef DELETE
#endif

#include <spdlog/spdlog.h>

#include "config.h"
#include "network.h"
#include "file.pb.h"


static bool parseCommandType(const std::string &option, fs::CommandType &outType)
{
    if (option == "LIST")
    {
        outType = fs::LIST;
        return true;
    }
    if (option == "CREATE")
    {
        outType = fs::CREATE;
        return true;
    }
    if (option == "DELETE")
    {
        outType = fs::REMOVE;
        return true;
    }
    if (option == "HISTORY")
    {
        outType = fs::HISTORY;
        return true;
    }
    return false;
}


static bool isValidUtf8(const std::string &str)
{
    int remainingBytes = 0;

    for (unsigned char c : str)
    {
        if (remainingBytes == 0)
        {
            if ((c & 0x80) == 0x00)
            {
                continue;
            }
            else if ((c & 0xE0) == 0xC0)
            {
                remainingBytes = 1;
            }
            else if ((c & 0xF0) == 0xE0)
            {
                remainingBytes = 2;
            }
            else if ((c & 0xF8) == 0xF0)
            {
                remainingBytes = 3;
            }
            else
            {
                return false;
            }
        }
        else
        {
            if ((c & 0xC0) != 0x80)
            {
                return false;
            }
            --remainingBytes;
        }
    }

    return remainingBytes == 0;
}

#ifdef _WIN32

static bool readLineUtf8(std::string &outLine)
{
    std::wstring wideLine;
    if (!std::getline(std::wcin, wideLine))
    {
        return false;
    }

    if (wideLine.empty())
    {
        outLine.clear();
        return true;
    }

    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8, 0, wideLine.c_str(), static_cast<int>(wideLine.size()),
        nullptr, 0, nullptr, nullptr);

    outLine.resize(sizeNeeded);

    WideCharToMultiByte(
        CP_UTF8, 0, wideLine.c_str(), static_cast<int>(wideLine.size()),
        &outLine[0], sizeNeeded, nullptr, nullptr);

    return true;
}
#endif

int main(int argc, char **argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdin), _O_U16TEXT);
#endif

    std::string configPath = argc > 1 ? argv[1] : "config/client_config.json";

    ClientConfig cfg;
    try
    {
        cfg = loadClientConfig(configPath);
    }
    catch (const std::exception &e)
    {
        spdlog::critical("Failed to load client config: {}", e.what());
        return 1;
    }

    spdlog::info("Client started. Server: {}:{}", cfg.server_host, cfg.server_port);
    spdlog::info("Enter the 'HELP' command for detailed information.");

    std::string line;
#ifdef _WIN32
    while (readLineUtf8(line))
#else
    while (std::getline(std::cin, line))
#endif
    {
        std::stringstream ss(line);
        std::string option;
        std::string path;
        ss >> option >> path;

        if (option == "HELP")
        {
            spdlog::info("CREATE <dirname> --- creates a new directory.");
            spdlog::info("DELETE <path> --- deletes a file or an empty directory.");
            spdlog::info("LIST [path] --- returns a list of files and subdirectories.");
            spdlog::info("HISTORY --- shows the operation history from the database.");
            spdlog::info("EXIT --- exit");
            continue;
        }

        if (option == "EXIT")
        {
            break;
        }

        fs::CommandType type;
        if (!parseCommandType(option, type))
        {
            spdlog::warn("Unknown command. Type HELP for the list of commands.");
            continue;
        }

        if (path.empty() && option != "LIST" && option != "HISTORY")
        {
            spdlog::warn("This command requires a path argument.");
            continue;
        }

        if (!isValidUtf8(path))
        {
            spdlog::error("The path contains invalid characters. Make sure your terminal is using UTF-8 encoding.");
            continue;
        }

        fs::Command command;
        command.set_type(type);
        command.set_path(path);

        try
        {
            fs::Response response = sendCommand(cfg, command);

            if (response.success())
            {
                spdlog::info("{}", response.message());
                for (const auto &entry : response.data())
                {
                    std::cout << "  " << entry << std::endl;
                }
            }
            else
            {
                spdlog::error("{}", response.message());
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error("Connection error: {}", e.what());
            break;
        }
    }

    return 0;
}