#include "gen3_utils.hpp"

// GBA memory constants
const uint32_t EWRAM_SIZE        = 0x40000;   // 256KB
const uint32_t PARTY_COUNT_ADDR  = 0x00024284 - 0x25B;
const uint32_t PARTY_BASE_OFFSET = 0x00024284;
const uint32_t SLOT_SIZE         = 0x64;
const uint32_t NICKNAME_OFFSET   = 0x08;
const uint32_t CURRENT_HP_OFFSET = 0x56;
const uint32_t MAX_HP_OFFSET     = 0x58;
const uint32_t TRAINER_ID_OFFSET = 0x00024592;


// Game codes for FRLG
const char* FIRERED_CODE  = "BPRE";
const char* LEAFGREEN_CODE = "BPGE";

// Gen3 character decode map
std::string decodeGen3String(uint8_t* data, int maxLen) {
    static const std::unordered_map<uint8_t, char> decodeMap = {
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

    std::string result;
    for (int i = 0; i < maxLen; i++) {
        if (data[i] == 0xFF) break;
        auto it = decodeMap.find(data[i]);
        if (it != decodeMap.end())
            result += it->second;
    }
    return result;
}

bool isValidGameCode(const char* code) {
    return memcmp(code, FIRERED_CODE, 4) == 0 ||
           memcmp(code, LEAFGREEN_CODE, 4) == 0;
}

uintptr_t findROMBase(HANDLE process) {
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t addr = 0;

    while (VirtualQueryEx(process, (LPCVOID)addr, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | 
                           PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) &&
            mbi.RegionSize >= 0x1000000) // ROM is 16MB+ 
        {
            // Try reading game code at ROM_GAMECODE_OFFSET
            char gameCode[5] = {0};
            SIZE_T bytesRead;
            if (ReadProcessMemory(process,
                (LPCVOID)((uintptr_t)mbi.BaseAddress + ROM_GAMECODE_OFFSET),
                gameCode, 4, &bytesRead) && bytesRead == 4)
            {
                if (isValidGameCode(gameCode)) {
                    std::cout << "Found ROM base at: 0x" << std::hex 
                              << mbi.BaseAddress << std::endl;
                    std::cout << "Game code: " << gameCode << std::endl;
                    return (uintptr_t)mbi.BaseAddress;
                }
            }
        }

        if (addr + mbi.RegionSize < addr) break;
        addr += mbi.RegionSize;
    }

    std::cerr << "Could not find FRLG ROM in memory." << std::endl;
    return 0;
}

bool validateAndReadParty(HANDLE process, uintptr_t base, PartyData& outParty) {
    auto read = [&](uintptr_t addr, void* buf, size_t size) -> bool {
        SIZE_T bytesRead;
        return ReadProcessMemory(process, (LPCVOID)addr, buf, size, &bytesRead)
               && bytesRead == size;
    };

    // Check 1: party count between 1-6
    uint8_t partyCount = 0;
    if (!read(base + PARTY_COUNT_ADDR, &partyCount, 1)) return false;
    if (partyCount > 6) return false;

    // Check 2: all party slots must have sane HP
    for (int i = 0; i < partyCount; i++) {
        uintptr_t slotAddr = base + PARTY_BASE_OFFSET + (i * SLOT_SIZE);
        uint16_t currentHP = 0, maxHP = 0;
        if (!read(slotAddr + CURRENT_HP_OFFSET, &currentHP, 2)) return false;
        if (!read(slotAddr + MAX_HP_OFFSET, &maxHP, 2)) return false;
        if (maxHP == 0 || maxHP > 714) return false;
        if (currentHP > maxHP) return false;
    }

    // Check 3: slots beyond partyCount must have maxHP == 0
    for (int i = partyCount; i < 6; i++) {
        uintptr_t slotAddr = base + PARTY_BASE_OFFSET + (i * SLOT_SIZE);
        uint16_t maxHP = 0;
        if (!read(slotAddr + MAX_HP_OFFSET, &maxHP, 2)) return false;
        if (maxHP != 0) return false;
    }

    // Passed — read full party
    outParty.count = partyCount;
    for (int i = 0; i < partyCount; i++) {
        uintptr_t slotAddr = base + PARTY_BASE_OFFSET + (i * SLOT_SIZE);
        uint8_t nickname[10];
        if (!read(slotAddr + NICKNAME_OFFSET, nickname, 10)) return false;
        if (!read(slotAddr + CURRENT_HP_OFFSET, &outParty.party[i].currentHP, 2)) return false;
        if (!read(slotAddr + MAX_HP_OFFSET, &outParty.party[i].maxHP, 2)) return false;
        outParty.party[i].nickname = decodeGen3String(nickname, 10);
    }

    return true;
}

uintptr_t findEWRAMBase(HANDLE process, uint16_t trainerID) {
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t addr = 0;

    while (VirtualQueryEx(process, (LPCVOID)addr, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE |
                           PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) &&
            mbi.RegionSize >= EWRAM_SIZE)
        {
            uint32_t foundID = 0;
            SIZE_T bytesRead;
            if (ReadProcessMemory(process,
                (LPCVOID)((uintptr_t)mbi.BaseAddress + TRAINER_ID_OFFSET),
                &foundID, 4, &bytesRead) && bytesRead == 4)
            {
                uint16_t visibleID = foundID & 0xFFFF;
                if (visibleID == trainerID) {
                    std::cout << "Found EWRAM base at: 0x" << std::hex 
                              << mbi.BaseAddress << std::endl;
                    return (uintptr_t)mbi.BaseAddress;
                }
            }
        }

        if (addr + mbi.RegionSize < addr) break;
        addr += mbi.RegionSize;
    }

    std::cerr << "Could not find EWRAM. Is your Trainer ID correct?" << std::endl;
    return 0;
}


HANDLE findProcess(const std::string& processName) {
    DWORD processes[1024];
    DWORD bytesReturned;

    if (!EnumProcesses(processes, sizeof(processes), &bytesReturned)) {
        std::cerr << "Failed to enumerate processes" << std::endl;
        return nullptr;
    }

    DWORD processCount = bytesReturned / sizeof(DWORD);

    for (DWORD i = 0; i < processCount; i++) {
        if (processes[i] == 0) continue;

        HANDLE handle = OpenProcess(
            PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
            FALSE,
            processes[i]
        );

        if (handle == nullptr) continue;

        char name[MAX_PATH];
        if (GetModuleBaseNameA(handle, nullptr, name, sizeof(name))) {
            if (std::string(name) == processName) {
                std::cout << "Found " << processName << " (PID: " << processes[i] << ")" << std::endl;
                return handle;
            }
        }

        CloseHandle(handle);
    }

    std::cerr << processName << " not found. Is it running?" << std::endl;
    return nullptr;
}