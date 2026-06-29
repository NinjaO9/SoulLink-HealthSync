#pragma once

#include "game_config.hpp"
#include "pokemon_general.hpp"

#include <unordered_map>
#include <string>
#include <windows.h>
#include <psapi.h>
#include <iostream>

const uint32_t ROM_GAMECODE_OFFSET = 0xAC;

std::string decodeString(uint8_t* data, int maxLen, CharEncoding encoding);

bool validateAndReadParty(HANDLE process, uintptr_t base, PartyData& outParty, const GameConfig& config);

uintptr_t findROMBase(HANDLE process, std::string& outGameCode);

uintptr_t findEWRAMBase(HANDLE process, uint16_t trainerID, const GameConfig& config);

HANDLE findProcess(const std::string& processName);

bool setPokemonHPWithBattle(HANDLE process, uintptr_t ewramBase, int partySlot, uint16_t hp, const GameConfig& config);