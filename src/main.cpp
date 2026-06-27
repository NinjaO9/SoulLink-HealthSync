
#include "gen3_utils.hpp"

// Polling happens every ~ half second? So this is basically 4 minutes I think
#define MAX_POLLING_INTERVALS 120

int main() {
    HANDLE mgba = findProcess("mGBA.exe");
    if (mgba == nullptr) return 1;

    uintptr_t romBase = findROMBase(mgba);
    if (romBase == 0) {
        std::cerr << "Is a FRLG ROM running?" << std::endl;
        CloseHandle(mgba);
        return 1;
    }

    uint16_t trainerID;
    std::cout << "Enter your Trainer ID: ";
    std::cin >> trainerID;

    uintptr_t ewramBase = findEWRAMBase(mgba, trainerID);
    if (ewramBase == 0) {
        system("pause");
        CloseHandle(mgba);
        return 1;
    }

    std::cout << "FRLG confirmed and EWRAM located." << std::endl;

    unsigned int polls = 0;
    // polling loop
    while (polls < MAX_POLLING_INTERVALS) {
        PartyData party;
        if (validateAndReadParty(mgba, ewramBase, party)) {
            system("cls");
            std::cout << "Party count: " << (int)party.count << std::endl;
            for (int i = 0; i < party.count; i++) {
                std::cout << "Slot " << i+1 << ": "
                          << party.party[i].nickname << " "
                          << std::dec << party.party[i].currentHP << "/"
                          << party.party[i].maxHP << " HP"
                          << std::endl;
            }
        }
        polls++;
        Sleep(500);
    }

    system("pause");
    CloseHandle(mgba);
    return 0;
}