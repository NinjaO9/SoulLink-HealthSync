#pragma once

#include <iostream>
#include <string>
#include <sstream>

#include "gen3_utils.hpp"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::stringstream;

namespace PokeRule_CLI
{

    extern HANDLE process;
    extern uintptr_t ewramBase;
    extern uintptr_t romBase;
    extern const GameConfig* config;
    extern PartyData party;

    void runCLI();

    int parseCmd(stringstream& cmd);

    void printparty(const PartyData& party);
}