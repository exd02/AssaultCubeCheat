#pragma once

#include <Windows.h>
#include <vector>
#include <TlHelp32.h>
#include <iostream>
#include <tchar.h>
#include <stdio.h>

class Process
{
public:
	bool attached;
	DWORD processID = 0;
	HANDLE hProcess = nullptr;
	
	Process(const wchar_t* processName);
	~Process();

	DWORD getModuleBaseAddress(const wchar_t* moduleName);

	bool isAttached();



	/**
	 * @brief Navigates through vector of memory offsets starting from a base address.
	 *
	 * @param address - The base address in memory.
	 * @param offsets - A vector containing memory offsets to navigate.
	 * @return The final address after navigating through the given offsets.
	 */
	DWORD resolvePtrChainLinks(DWORD baseAddress, std::vector<DWORD> offsets);

	bool patchEx(BYTE* destination, BYTE* source, unsigned int size);
	bool nopEx(BYTE* destination, unsigned int size);

	/**
	 * @brief Memory read the address.
	 *
	 * @tparam T Data type to be read from memory.
	 * @param address - Memory address to be read.
	 * @param val - store the read value (passed by reference).
	 * @return Returns true if the read operation is successful, false otherwise.
	 */
	template <typename T>
	bool RPM(DWORD address, T &val) {
		if (ReadProcessMemory(this->hProcess, (LPVOID)address, &val, sizeof(T), nullptr))
			return true;
		
		DWORD error = GetLastError();
		std::cerr << "Error reading address \"" << (void*)address << "\", ERROR CODE: " << error << std::endl;
		return false;

	}

	/**
	 * @brief Writes the memory in the address.
	 *
	 * @tparam T Data type to be write from memory.
	 * @param address - Memory address to be write.
	 * @param val - new value to write to the address.
	 * @return Returns true if the write operation is successful, false otherwise.
	 */
	template <typename T>
	bool WPM(DWORD address, T val)
	{
	
		if (WriteProcessMemory(this->hProcess, (LPVOID)address, &val, sizeof(T), nullptr))
			return true;
		
		DWORD error = GetLastError();
		std::cerr << "Error writting address \"" << (void*)address << "\", ERROR CODE: " << error << std::endl;
		return false;
	}

	/**
	 * @brief Writes the memory in the address.
	 *
	 * @tparam T Data type to be write from memory.
	 * @param address - Memory address to be write.
	 * @param val - new value to write to the address.
	 * @param size - size of the new value.
	 * @return Returns true if the write operation is successful, false otherwise.
	 */
	template <typename T>
	bool WPM(DWORD address, T &val, int size) {
		if (WriteProcessMemory(this->hProcess, (LPVOID)address, val, size, nullptr))
			return true;

		DWORD error = GetLastError();
		std::cerr << "Error writting address \"" << (void*)address << "\", ERROR CODE: " << error << std::endl;
		return false;
	}
};


