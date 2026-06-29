#include "gen3_utils.hpp"
#include "game_registry.hpp"

#include <unordered_map>
#include <iostream>
// ─────────────────────────────────────────────────────────────────────────────
// Character decoding
// ─────────────────────────────────────────────────────────────────────────────

static const std::unordered_map<uint8_t, char> GEN3_CHAR_MAP = {
    {0xBB,'A'},{0xBC,'B'},{0xBD,'C'},{0xBE,'D'},{0xBF,'E'},
    {0xC0,'F'},{0xC1,'G'},{0xC2,'H'},{0xC3,'I'},{0xC4,'J'},
    {0xC5,'K'},{0xC6,'L'},{0xC7,'M'},{0xC8,'N'},{0xC9,'O'},
    {0xCA,'P'},{0xCB,'Q'},{0xCC,'R'},{0xCD,'S'},{0xCE,'T'},
    {0xCF,'U'},{0xD0,'V'},{0xD1,'W'},{0xD2,'X'},{0xD3,'Y'},
    {0xD4,'Z'},{0xD5,'a'},{0xD6,'b'},{0xD7,'c'},{0xD8,'d'},
    {0xD9,'e'},{0xDA,'f'},{0xDB,'g'},{0xDC,'h'},{0xDD,'i'},
    {0xDE,'j'},{0xDF,'k'},{0xE0,'l'},{0xE1,'m'},{0xE2,'n'},
    {0xE3,'o'},{0xE4,'p'},{0xE5,'q'},{0xE6,'r'},{0xE7,'s'},
    {0xE8,'t'},{0xE9,'u'},{0xEA,'v'},{0xEB,'w'},{0xEC,'x'},
    {0xED,'y'},{0xEE,'z'}
};

std::string decodeString(uint8_t* data, int maxLen, CharEncoding encoding)
{
    const std::unordered_map<uint8_t, char>* decodeMap = nullptr;

    switch (encoding)
    {
        case CharEncoding::GEN3:
            decodeMap = &GEN3_CHAR_MAP;
            break;

        default:
            return "";
    }

    std::string result;

    for (int i = 0; i < maxLen; i++)
    {
        if (data[i] == 0xFF)
            break;

        auto it = decodeMap->find(data[i]);

        if (it != decodeMap->end())
            result += it->second;
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// ROM Detection
// ─────────────────────────────────────────────────────────────────────────────

HANDLE findProcess(const std::string& processName)
{
    DWORD processes[1024];
    DWORD bytesReturned;

    if (!EnumProcesses(processes, sizeof(processes), &bytesReturned))
    {
        std::cerr << "Failed to enumerate processes" << std::endl;
        return nullptr;
    }

    DWORD processCount =
        bytesReturned / sizeof(DWORD);

    for (DWORD i = 0; i < processCount; i++)
    {
        if (processes[i] == 0) continue;

        HANDLE handle = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, processes[i]);

        if (handle == nullptr)
            continue;

        char name[MAX_PATH];

        if (GetModuleBaseNameA(handle, nullptr, name, sizeof(name)))
        {
            if (std::string(name) == processName)
            {
                std::cout << "Found " << processName << " (PID: " << processes[i] << ")" << std::endl;

                return handle;
            }
        }

        CloseHandle(handle);
    }

    std::cerr << processName << " not found. Is it running?" << std::endl;

    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// ROM Detection
// ─────────────────────────────────────────────────────────────────────────────

uintptr_t findROMBase(HANDLE process,std::string& outGameCode)
{
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t addr = 0;

    while (VirtualQueryEx(process, (LPCVOID)addr, &mbi, sizeof(mbi)))
    {
        if (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) && mbi.RegionSize >= 0x1000000)
        {
            char gameCode[5] = {0};

            SIZE_T bytesRead;

            if (ReadProcessMemory(process, (LPCVOID)((uintptr_t)mbi.BaseAddress + ROM_GAMECODE_OFFSET), gameCode, 4, &bytesRead) && bytesRead == 4)
            {
                std::string code(gameCode, 4);

                if (getConfigByGameCode(code) != nullptr)
                {
                    outGameCode = code;

                    std::cout << "Found ROM base at: 0x" << std::hex << mbi.BaseAddress << std::endl;

                    std::cout << "Game code: " << code << std::endl;

                    return (uintptr_t)mbi.BaseAddress;
                }
            }
        }

        if (addr + mbi.RegionSize < addr)
            break;

        addr += mbi.RegionSize;
    }

    std::cerr << "Could not locate supported ROM." << std::endl;

    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// EWRAM Detection
// ─────────────────────────────────────────────────────────────────────────────

uintptr_t findEWRAMBase(HANDLE process, uint16_t trainerID, const GameConfig& config)
{
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t addr = 0;

    while (VirtualQueryEx(process, (LPCVOID)addr, &mbi, sizeof(mbi)))
    {
        if (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) && mbi.RegionSize >= config.ewramSize)
        {
            uint32_t foundID = 0;

            SIZE_T bytesRead;

            if (ReadProcessMemory(process, (LPCVOID)((uintptr_t)mbi.BaseAddress + config.trainerIdOffset), &foundID, 4, &bytesRead) && bytesRead == 4)
            {
                uint16_t visibleID = foundID & 0xFFFF;

                if (visibleID == trainerID)
                {
                    std::cout << "Found EWRAM base at: 0x" << std::hex << mbi.BaseAddress << std::endl;

                    return (uintptr_t)mbi.BaseAddress;
                }
            }
        }

        if (addr + mbi.RegionSize < addr) break;

        addr += mbi.RegionSize;
    }

    std::cerr << "Could not find EWRAM. " << "Is your Trainer ID correct?" << std::endl;

    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Party Reading
// ─────────────────────────────────────────────────────────────────────────────

bool validateAndReadParty(HANDLE process, uintptr_t base, PartyData& outParty, const GameConfig& config)
{
    auto read = [&](uintptr_t addr, void* buf, size_t size) -> bool
    {
        SIZE_T bytesRead;

        return ReadProcessMemory(process, (LPCVOID)addr, buf, size, &bytesRead) && bytesRead == size;
    };

    uint8_t partyCount = 0;

    if (!read(base + config.partyCountOffset, &partyCount, 1)) return false;

    if (partyCount > 6) return false;

    // Validate occupied slots

    for (int i = 0; i < partyCount; i++)
    {
        uintptr_t slotAddr = base + config.partyBaseOffset + (i * config.slotSize);

        uint16_t currentHP = 0;
        uint16_t maxHP = 0;

        if (!read(slotAddr + config.currentHpOffset, &currentHP, 2)) return false;

        if (!read(slotAddr + config.maxHpOffset, &maxHP, 2)) return false;

        if (maxHP == 0 || maxHP > config.maxPossibleHp) return false;

        if (currentHP > maxHP) return false;
    }

    // Validate empty slots

    for (int i = partyCount; i < 6; i++)
    {
        uintptr_t slotAddr = base + config.partyBaseOffset + (i * config.slotSize);

        uint16_t maxHP = 0;

        if (!read(slotAddr + config.maxHpOffset, &maxHP, 2)) return false;

        if (maxHP != 0) return false;
    }

    // Read party

    outParty.count = partyCount;

    for (int i = 0; i < partyCount; i++)
    {
        uintptr_t slotAddr = base + config.partyBaseOffset + (i * config.slotSize);

        std::vector<uint8_t> nickname(config.nicknameLength);

        if (!read(slotAddr + config.nicknameOffset, nickname.data(), config.nicknameLength)) return false;

        if (!read(slotAddr + config.currentHpOffset, &outParty.party[i].currentHP, 2)) return false;

        if (!read(slotAddr + config.maxHpOffset, &outParty.party[i].maxHP, 2)) return false;

        outParty.party[i].nickname = decodeString(nickname.data(), config.nicknameLength, config.encoding);
    }

    return true;
}

bool setPokemonHP(HANDLE process, uintptr_t ewramBase, int slot, uint16_t hp, const GameConfig& config)
{
    if (slot < 0 || slot >= 6) return false;

    uintptr_t slotAddr = ewramBase + config.partyBaseOffset + (slot * config.slotSize);
    size_t byteswritten;
    return WriteProcessMemory(process, (LPVOID)(slotAddr + config.currentHpOffset), &hp, sizeof(hp), &byteswritten);
}

bool setPokemonHPWithBattle(HANDLE process, uintptr_t ewramBase, int partySlot, uint16_t hp, const GameConfig& config)
{
    if (partySlot < 0 || partySlot >= 6) return false;

    // Step 1: Write to party data
    uintptr_t slotAddr = ewramBase + config.partyBaseOffset + (partySlot * config.slotSize);
    SIZE_T bytesWritten;

    if (!WriteProcessMemory(process,
        (LPVOID)(slotAddr + config.currentHpOffset),
        &hp, sizeof(hp), &bytesWritten)) return false;

    // Step 2: Find which battler slot(s) correspond to this party slot
    // gBattlerPartyIndexes[battlerIndex] = partyIndex
    // There are 4 battler slots
    uintptr_t battlerPartyIndexesAddr = ewramBase + config.battlerPartyIndexesOffset;

    for (int battlerIndex = 0; battlerIndex < 4; battlerIndex++) {
        uint8_t partyIndex = 0;
        SIZE_T br;

        if (!ReadProcessMemory(process,
            (LPCVOID)(battlerPartyIndexesAddr + battlerIndex),
            &partyIndex, 1, &br) || br != 1) continue;

        if (partyIndex != partySlot) continue;

        // This battler is using our party slot — update battle HP too
        uintptr_t battleMonAddr = ewramBase + config.battleMonsOffset 
                                  + (battlerIndex * config.battleMonSize);

        WriteProcessMemory(process,
            (LPVOID)(battleMonAddr + config.battleMonHpOffset),
            &hp, sizeof(hp), &bytesWritten);
    }

    return true;
}