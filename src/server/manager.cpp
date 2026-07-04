#include "manager.h"

#include <spdlog/spdlog.h>

fileManager::fileManager(const std::string& root)
    : root_(fs_std::absolute(root))
{
}

bool fileManager::resolvePath(const std::string& relativePath, fs_std::path& outPath)
{
    try
    {
        fs_std::path combined = root_ / relativePath;


        fs_std::path resolved = fs_std::weakly_canonical(combined);
        fs_std::path canonicalRoot = fs_std::weakly_canonical(root_);


        auto rootIt = canonicalRoot.begin();
        auto resolvedIt = resolved.begin();

        for (; rootIt != canonicalRoot.end(); ++rootIt, ++resolvedIt)
        {
            if (resolvedIt == resolved.end() || *resolvedIt != *rootIt)
            {
                return false; 
            }
        }

        outPath = resolved;
        return true;
    }
    catch (const std::exception& e)
    {

        spdlog::warn("resolvePath failed for '{}': {}", relativePath, e.what());
        return false;
    }
}

void fileManager::list(const std::string& path, fs::Response& response)
{
    fs_std::path target;
    if (!resolvePath(path, target))
    {
        response.set_success(false);
        response.set_message("Path is outside of the allowed root directory");
        return;
    }

    try
    {
        if (!fs_std::exists(target))
        {
            response.set_success(false);
            response.set_message("Path does not exist");
            return;
        }

        if (!fs_std::is_directory(target))
        {
            response.set_success(false);
            response.set_message("Path is not a directory");
            return;
        }

        for (const auto& entry : fs_std::directory_iterator(target))
        {
            std::string name = entry.path().filename().string();
            if (entry.is_directory())
            {
                name += "/";
            }
            response.add_data(name);
        }

        response.set_success(true);
        response.set_message("OK");
    }
    catch (const std::exception& e)
    {
        response.set_success(false);
        response.set_message(std::string("Failed to list directory: ") + e.what());
    }
}

void fileManager::create(const std::string& path, fs::Response& response)
{
    fs_std::path target;
    if (!resolvePath(path, target))
    {
        response.set_success(false);
        response.set_message("Path is outside of the allowed root directory");
        return;
    }

    try
    {
        if (fs_std::exists(target))
        {
            response.set_success(false);
            response.set_message("Path already exists");
            return;
        }

        if (fs_std::create_directory(target))
        {
            response.set_success(true);
            response.set_message("Directory created");
        }
        else
        {
            response.set_success(false);
            response.set_message("Failed to create directory");
        }
    }
    catch (const std::exception& e)
    {
        response.set_success(false);
        response.set_message(std::string("Failed to create directory: ") + e.what());
    }
}

void fileManager::remove(const std::string& path, fs::Response& response)
{
    fs_std::path target;
    if (!resolvePath(path, target))
    {
        response.set_success(false);
        response.set_message("Path is outside of the allowed root directory");
        return;
    }

    try
    {
        if (!fs_std::exists(target))
        {
            response.set_success(false);
            response.set_message("Path does not exist");
            return;
        }

        if (fs_std::is_directory(target) && !fs_std::is_empty(target))
        {
            response.set_success(false);
            response.set_message("Directory is not empty");
            return;
        }

        if (fs_std::remove(target))
        {
            response.set_success(true);
            response.set_message("Deleted");
        }
        else
        {
            response.set_success(false);
            response.set_message("Failed to delete");
        }
    }
    catch (const std::exception& e)
    {
        response.set_success(false);
        response.set_message(std::string("Failed to delete: ") + e.what());
    }
}