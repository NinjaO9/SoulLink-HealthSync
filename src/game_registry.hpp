#pragma once

#include "game_config.hpp"

extern const GameConfig FRLG_CONFIG;

const GameConfig* getConfigByGameCode(const std::string& code);