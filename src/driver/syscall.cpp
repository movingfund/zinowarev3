#include "syscall.hpp"
#include <tlhelp32.h>
#include <cstring>

#pragma pack(push, 1)
struct SyscallStub {
    uint8_t  mov_r10_rcx[3] = {0x4C, 0x8B, 0xD1};
    uint8_t  mov_eax[1]     = {0xB8};
    uint32_t syscall_number = 0;
    uint8_t  syscall_insn[2]= {0x0F, 0x05};
    uint8_t  ret_insn[1]    = {0xC3};
};
#pragma pack(pop)
static_assert(sizeof(SyscallStub) == 11, "bad stub size");

SyscallManager::SyscallManager() {
    if (!InitSyscalls())
        std::cerr << "[!] Failed to initialise syscall stubs\n";
}

SyscallManager::~SyscallManager() {
    Close();
    if (m_stubRead)  VirtualFree(m_stubRead, 0, MEM_RELEASE);
    if (m_stubWrite) VirtualFree(m_stubWrite, 0, MEM_RELEASE);
}

uint32_t SyscallManager::ExtractSyscallNumber(void* funcAddr) {
    uint8_t code[64];
    memcpy(code, funcAddr, 64);

    for (int i = 0; i < 60; ++i) {
        if (code[i] == 0xB8) {
            return *reinterpret_cast<uint32_t*>(&code[i + 1]);
        }
        if (i + 3 < 64 && code[i] == 0x4C && code[i+1] == 0x8B && code[i+2] == 0xD1 && code[i+3] == 0xB8) {
            return *reinterpret_cast<uint32_t*>(&code[i + 4]);
        }
    }
    return 0;
}

bool SyscallManager::InitSyscalls() {
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) {
        std::cerr << "[!] Cannot open ntdll.dll\n";
        return false;
    }

    void* fnRead  = reinterpret_cast<void*>(GetProcAddress(ntdll, "NtReadVirtualMemory"));
    void* fnWrite = reinterpret_cast<void*>(GetProcAddress(ntdll, "NtWriteVirtualMemory"));
    if (!fnRead || !fnWrite) {
        std::cerr << "[!] Cannot resolve Nt* functions\n";
        return false;
    }

    m_syscallNrRead  = ExtractSyscallNumber(fnRead);
    m_syscallNrWrite = ExtractSyscallNumber(fnWrite);

    if (!m_syscallNrRead || !m_syscallNrWrite) {
        std::cerr << "[!] Failed to extract syscall numbers\n";
        return false;
    }

    auto makeStub = [&](uint32_t nr) -> void* {
        SyscallStub stub;
        stub.syscall_number = nr;
        void* mem = VirtualAlloc(nullptr, sizeof(SyscallStub), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!mem) return nullptr;
        memcpy(mem, &stub, sizeof(SyscallStub));
        return mem;
    };

    m_stubRead  = makeStub(m_syscallNrRead);
    m_stubWrite = makeStub(m_syscallNrWrite);

    if (!m_stubRead || !m_stubWrite) {
        std::cerr << "[!] Failed to allocate syscall stubs\n";
        return false;
    }

    std::cout << "[+] Syscall stubs ready (read=0x" << std::hex << m_syscallNrRead
              << " write=0x" << m_syscallNrWrite << std::dec << ")\n";
    return true;
}

bool SyscallManager::AttachToProcess(const std::string& name1, const std::string& name2) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "[!] CreateToolhelp32Snapshot failed\n";
        return false;
    }

    PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
    DWORD pid = 0;

    if (Process32FirstW(snapshot, &pe)) {
        do {
            std::wstring ws(pe.szExeFile);
            std::string exe(ws.begin(), ws.end());
            if (exe == name1 || exe == name2) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &pe));
    }
    CloseHandle(snapshot);

    if (!pid) {
        std::cerr << "[!] Roblox process not found (tried: " << name1 << ", " << name2 << ")\n";
        return false;
    }

    m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!m_hProcess) {
        std::cerr << "[!] OpenProcess failed (pid=" << pid << "). Try running as admin.\n";
        return false;
    }

    HMODULE mods[1024];
    DWORD needed;
    if (EnumProcessModules(m_hProcess, mods, sizeof(mods), &needed)) {
        wchar_t modPath[MAX_PATH];
        if (GetModuleFileNameExW(m_hProcess, mods[0], modPath, MAX_PATH)) {
            m_baseAddr = reinterpret_cast<uintptr_t>(mods[0]);
        }
    }

    std::cout << "[+] Attached to " << name1 << " (pid=" << pid
              << ") base=0x" << std::hex << m_baseAddr << std::dec << "\n";
    return true;
}

void SyscallManager::Close() {
    if (m_hProcess) {
        CloseHandle(m_hProcess);
        m_hProcess = nullptr;
    }
}

bool SyscallManager::ReadRaw(uintptr_t addr, void* buffer, size_t size) {
    if (!m_hProcess || !m_stubRead) return false;

    auto fn = reinterpret_cast<NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T)>(m_stubRead);
    SIZE_T bytesRead = 0;
    NTSTATUS status = fn(m_hProcess, reinterpret_cast<PVOID>(addr), buffer, size, &bytesRead);
    return NT_SUCCESS(status) && bytesRead == size;
}

bool SyscallManager::WriteRaw(uintptr_t addr, void* buffer, size_t size) {
    if (!m_hProcess || !m_stubWrite) return false;

    auto fn = reinterpret_cast<NTSTATUS(NTAPI*)(HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T)>(m_stubWrite);
    SIZE_T bytesWritten = 0;
    NTSTATUS status = fn(m_hProcess, reinterpret_cast<PVOID>(addr), buffer, size, &bytesWritten);
    return NT_SUCCESS(status) && bytesWritten == size;
}

std::string SyscallManager::ReadString(uintptr_t addr, size_t maxLen) {
    std::string result;
    result.resize(maxLen);
    size_t read = 0;
    if (!ReadRaw(addr, &result[0], maxLen))
        return {};
    result.resize(strnlen(result.c_str(), maxLen));
    return result;
}
