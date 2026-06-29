#include "gen3_utils.hpp"
#include "game_registry.hpp"

#define MAX_POLLING_INTERVALS 120

int main()
{
    HANDLE mgba = findProcess("mGBA.exe");
    if (!mgba) return 1;

    std::string gameCode;

    uintptr_t romBase =
        findROMBase(mgba, gameCode);

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
    system("pause");


    if (!ewramBase) return 1;

    unsigned polls = 0;


    while (polls < MAX_POLLING_INTERVALS)
    {
        PartyData party;
        //setPokemonHPWithBattle(mgba, ewramBase, 0, 0, *config);

        if (validateAndReadParty(mgba, ewramBase, party, *config))
        {
            system("cls");

            for (int i = 0; i < party.count; i++)
            {
                std::cout << std::dec << party.party[i].nickname << " " << party.party[i].currentHP << "/" << party.party[i].maxHP << std::endl;
            }
        }


        polls++;
        Sleep(500);
    }
    
    system("pause");
    CloseHandle(mgba);
}