#pragma once

#include <string>

struct Pokemon {
    std::string nickname;
    uint16_t currentHP;
    uint16_t maxHP;
};

// Might update to a class later if I need to make functions for pkmn stuff; we will see
struct PartyData {
    uint8_t count;
    Pokemon party[6];
};

