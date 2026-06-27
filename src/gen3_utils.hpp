#pragma once

#include <unordered_map>
#include <string>
#include <windows.h>
#include <psapi.h>
#include <iostream>
#include <vector>



// GBA memory constants
extern const uint32_t EWRAM_SIZE;
extern const uint32_t PARTY_COUNT_ADDR;
extern const uint32_t PARTY_BASE_OFFSET;
extern const uint32_t SLOT_SIZE;
extern const uint32_t NICKNAME_OFFSET;
extern const uint32_t CURRENT_HP_OFFSET;
extern const uint32_t MAX_HP_OFFSET;

// Game codes for FRLG
extern const char* FIRERED_CODE;
extern const char* LEAFGREEN_CODE;
extern const uint32_t TRAINER_ID_OFFSET;

struct Pokemon {
    std::string nickname;
    uint16_t currentHP;
    uint16_t maxHP;
};

struct PartyData {
    uint8_t count;
    Pokemon party[6];
};

// ROM header offset for game code
const uint32_t ROM_GAMECODE_OFFSET = 0xAC;

std::string decodeGen3String(uint8_t* data, int maxLen);

uintptr_t findROMBase(HANDLE process);

bool validateAndReadParty(HANDLE process, uintptr_t base, PartyData& outParty);

bool isValidGameCode(const char* code);

uintptr_t findEWRAMBase(HANDLE process, uint16_t trainerID);

HANDLE findProcess(const std::string& processName);