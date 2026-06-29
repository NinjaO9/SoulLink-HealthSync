#include "pokerule_cli.hpp"

namespace PokeRule_CLI
{
    HANDLE process = NULL;
    uintptr_t ewramBase = 0;
    uintptr_t romBase = 0;
    const GameConfig* config = nullptr;
    PartyData party;
}


void PokeRule_CLI::runCLI()
{
    cout << "PokeRule CLI starting..." << endl;

    stringstream cmds;
    string instr;
    if (!validateAndReadParty(process, ewramBase, party, *config))
    {
        cout << "Failed to read & validate party" << endl;
    } 
    while (true)
    {
        printparty(party);
        cout << "PKR_CLI> ";        
        std::getline(cin, instr);
        cmds = stringstream(instr);
        if (parseCmd(cmds)) return;
        instr.clear();
    }
    
}

int PokeRule_CLI::parseCmd(stringstream& cmd)
{
    string rcmd; // 'raw command'
    cmd >> rcmd;
    if (rcmd == "quit")
    {
        cout << "Quitting PokeRuleCLI..." << endl;
        return 1;
    }
    else if (rcmd == "edithp")
    {
        short int slot = 0;
        short int hp = -1;
        
        if (!(cmd >> slot >> hp))
        {
            cout << "ERROR: Not enough values in command: edithp <slot> <hpValue>" << endl;
            cmd.clear();
            return 0;
        }

        if (slot > 6 || slot < 1) 
        {
            cout << "ERROR: Value 'slot' must be a number 1-6" << endl;
            cmd.clear();
            return 0;
        }

        setPokemonHPWithBattle(process, ewramBase, slot, hp, *config);
    }        
    else if (rcmd == "update")
    {
        validateAndReadParty(process, ewramBase, party, *config);   
    }
    else if (rcmd == "help")
    {
        cout << "===== ALL COMMANDS =====\n" << "edithp <slot> <hpval>\n" << "update" << endl;
    }
    else
    {
        cout << "Command not recognized. 'help' to see all commands" << endl;
    }

    cmd.clear();
    return 0;
}

void PokeRule_CLI::printparty(const PartyData &party)
{
    for (int i = 0; i < party.count; i++)
    {
        std::cout << std::dec << party.party[i].nickname << " " << party.party[i].currentHP << "/" << party.party[i].maxHP << std::endl;
    }
}
