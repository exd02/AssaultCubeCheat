#include "Process.h"

Process::Process(const wchar_t* processName)
{
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        attached = false;
        std::cerr << "[ERROR] Could not snapshot process\n";
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(pe32);

    if (!Process32First(hProcessSnap, &pe32)) {
        std::cerr << "[ERROR] error geting process entry\n";

        attached = false;
        CloseHandle(hProcessSnap);
        return;
    }

    do {
        // _tprintf(TEXT("Checking '%s' == '%s'\n"), pe32.szExeFile, processName);
        if (!_wcsicmp(pe32.szExeFile, processName)) {
            this->processID = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);

    // Now that we have the PID
    // Create a Handle to the process
    this->hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, this->processID);
    attached = true;

    if (hProcess == NULL)
    {
        attached = false;
        DWORD error = GetLastError();
        std::cerr << "Error accessing the proccess, ERROR CODE: " << error << std::endl;
    }
}

Process::~Process()
{
    attached = false;

    if (hProcess && hProcess != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->hProcess);
        this->hProcess = NULL;
    }
}

DWORD Process::getModuleBaseAddress(const wchar_t* moduleName)
{
    DWORD address = NULL;

    HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->processID);

    if (hModuleSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "[ERROR] Could not create a Snapshot of the Modules of the Process\n";
        return NULL;
    }

    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);

    if (!Module32First(hModuleSnap, &me32)) {
        std::cerr << "[ERROR] Could not get the first module entry\n";
        CloseHandle(hModuleSnap);
        return NULL;
    }

    do {
        if (!_wcsicmp(me32.szModule, moduleName)) {
            address = (DWORD)me32.modBaseAddr;
            break;
        }
    } while (Module32Next(hModuleSnap, &me32));


    CloseHandle(hModuleSnap);

    return address;
}

bool Process::isAttached()
{
    if (!attached)
        return false;

    DWORD dwExit = NULL;
    GetExitCodeProcess(this->hProcess, &dwExit);

    if (dwExit == STILL_ACTIVE)
        return true;

    this->~Process();
    return false;
}

DWORD Process::resolvePtrChainLinks(DWORD address, std::vector<DWORD> offsets)
{
    const int totalOffsets = offsets.size();

    if (totalOffsets == 0) return address;

    for (int i = 0; i < totalOffsets - 1; i++)
    {
        RPM<DWORD>(address + offsets[i], address);
    }

    // Add the last offset to the final address to get the ultimate result.
    return address + offsets[totalOffsets - 1];
}

bool Process::patchEx(BYTE* destination, BYTE* source, unsigned int size)
{
    DWORD oldProtect = NULL;
    if (!VirtualProtectEx(this->hProcess, destination, size, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        DWORD error = GetLastError();
        std::cerr << "Error changing memory protection at \"" << (void*)destination << "\", ERROR CODE: " << error << std::endl;
        return false;
    }

    WPM<BYTE *>((DWORD)destination, source, size);


    if (!VirtualProtectEx(this->hProcess, destination, size, oldProtect, &oldProtect))
    {
        DWORD error = GetLastError();
        std::cerr << "Error changing memory protection back to normal at \"" << (void*)destination << "\", ERROR CODE: " << error << std::endl;
        return false;
    }

    return true;
}

bool Process::nopEx(BYTE* destination, unsigned int size)
{
    BYTE* nopArray = new BYTE[size];
    memset(nopArray, 0x90, size);

    if (!patchEx(destination, nopArray, size))
    {
        delete[] nopArray;
        return false;
    }

    delete[] nopArray;
    return true;
}