#pragma once

#include <string>


struct DbConfig
{
    std::string host;
    int port;
    std::string dbname;
    std::string user;
    std::string password;
};


struct LoggingConfig
{
    std::string level;    
    std::string file;    
    int max_size_mb;       
    int max_files;          
};


struct ServerConfig
{
    int port;                
    std::string root_dir;    
    DbConfig database;
    LoggingConfig logging;
};


ServerConfig loadServerConfig(const std::string& path);