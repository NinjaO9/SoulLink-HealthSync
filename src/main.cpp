#include "gen3_utils.hpp"
#include "game_registry.hpp"
#include "pokerule_cli.hpp"

#define MAX_POLLING_INTERVALS 120

int main()
{
    HANDLE mgba = findProcess("mGBA.exe");
    if (!mgba) return 1;

    std::string gameCode;

    uintptr_t romBase = findROMBase(mgba, gameCode);

    if (!romBase) {
        std::cerr << "Unsupported game.\n";
        return 1;
    }

    const GameConfig* config = getConfigByGameCode(gameCode);

    if (!config) {
        std::cerr << "Unsupported game.\n";
        return 1;
    }

    std::cout << "Game: " << config->gameName << std::endl;

    uint16_t trainerID;

    std::cout << "Enter Trainer ID: ";
    std::cin >> trainerID;

    uintptr_t ewramBase = findEWRAMBase(mgba, trainerID, *config);
    if (!ewramBase) return 1;
    
    PokeRule_CLI::process = mgba;
    PokeRule_CLI::romBase = romBase;
    PokeRule_CLI::ewramBase = ewramBase;
    PokeRule_CLI::config = config;

    
    unsigned polls = 0;

    PokeRule_CLI::runCLI();
    
    system("pause");
    CloseHandle(mgba);
}