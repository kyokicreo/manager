#pragma once

#include "config.h"
#include "file.pb.h"


fs::Response sendCommand(const ClientConfig& cfg, const fs::Command& command);