#include "game_registry.hpp"

const GameConfig FRLG_CONFIG = {
    "FireRed/LeafGreen", // Game name
    {"BPRE", "BPGE"}, // rom coeds

    0x40000, // ewramSize

    0x24029, // partyCountOffset
    0x24284, // partyBaseOffset

    0x64, // slotSize

    0x8, // NicknameOffset
    10, // NicknameLength

    0x56, // currentHpOffset
    0x58, // maxHpOffset

    0x24592, // trainerIdOffset

    714, // maxPossibleHp (May remove later)

    CharEncoding::GEN3
};

const GameConfig* getConfigByGameCode(const std::string& code)
{
    for (const auto& rom : FRLG_CONFIG.romCodes) {
        if (rom == code) return &FRLG_CONFIG;
    }

    return nullptr;
}