#pragma once

#include <string>
#include <vector>
#include <cstdint>

enum class CharEncoding {
    GEN3
};

struct GameConfig {
    std::string gameName;
    std::vector<std::string> romCodes;

    uint32_t ewramSize;

    uint32_t partyCountOffset;
    uint32_t partyBaseOffset;

    uint32_t slotSize;

    uint32_t nicknameOffset;
    uint32_t nicknameLength;

    uint32_t currentHpOffset;
    uint32_t maxHpOffset;

    uint32_t trainerIdOffset;

    uint16_t maxPossibleHp;

    CharEncoding encoding;
};