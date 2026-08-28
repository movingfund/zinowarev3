#pragma once
#include <windows.h>
#include <winternl.h>
#include <ntstatus.h>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

class SyscallManager {
public:
    SyscallManager();
    ~SyscallManager();

    bool AttachToProcess(const std::string& name1, const std::string& name2);
    void Close();

    uintptr_t BaseAddress() const { return m_baseAddr; }
    HANDLE    ProcessHandle() const { return m_hProcess; }

    template<typename T>
    T Read(uintptr_t addr) {
        T buf{};
        ReadRaw(addr, &buf, sizeof(T));
        return buf;
    }

    template<typename T>
    bool ReadArray(uintptr_t addr, T* out, size_t count) {
        return ReadRaw(addr, out, sizeof(T) * count);
    }

    bool ReadRaw(uintptr_t addr, void* buffer, size_t size);

    std::string ReadString(uintptr_t addr, size_t maxLen = 256);

    template<typename T>
    bool Write(uintptr_t addr, const T& value) {
        return WriteRaw(addr, const_cast<T*>(&value), sizeof(T));
    }

    bool WriteRaw(uintptr_t addr, void* buffer, size_t size);

private:
    HANDLE    m_hProcess = nullptr;
    uintptr_t m_baseAddr = 0;

    uint32_t m_syscallNrRead  = 0;
    uint32_t m_syscallNrWrite = 0;
    void*    m_stubRead       = nullptr;
    void*    m_stubWrite      = nullptr;

    bool InitSyscalls();
    uint32_t ExtractSyscallNumber(void* funcAddr);
};
