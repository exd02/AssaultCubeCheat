#include <iostream>
#include<thread>


#include "Process.h"


#define	CMD_COLOR_RED		"\033[1;31m"
#define CMD_COLOR_GREEN		"\033[1;32m"
#define CMD_COLOR_DEFAULT	"\033[0m"

namespace offsets {
	DWORD localPlayer = 0x18AC00;
	DWORD health = 0xEC;

	std::vector<DWORD> currentWeaponAmmo = { 0x364, 0x14, 0x0 };
};

std::string strColoredOnOrOff(bool status)
{
	return std::string(CMD_COLOR_DEFAULT) + '[' + (status ? CMD_COLOR_GREEN : CMD_COLOR_RED) + (status ? "ON" : "OFF") + CMD_COLOR_DEFAULT + ']';
}

int main()
{
	const wchar_t* processName = L"ac_client.exe";
	std::wcout << L"Process Name: \t\t\"" << processName << L"\"" << std::endl;

	Process process(processName);
	std::wcout << L"Process ID: \t\t\"" << process.processID << L"\"" << std::endl;

	auto baseAddress = process.getModuleBaseAddress(processName);
	std::wcout << std::hex << L"Base Address: \t\t\"0x" << baseAddress << L"\"" << std::endl;

	DWORD localPlayer = NULL;
	process.RPM<DWORD>(baseAddress + offsets::localPlayer, localPlayer);
	std::wcout << std::hex << L"Local Player Address: \t\"0x" << localPlayer << L"\"" << std::endl;

	bool freezeHealth = false; // f1 activate or deactivate
	bool freezeAmmo = false; // f2
	bool noRecoil = false; // f3
	while (!GetAsyncKeyState(VK_INSERT) && process.isAttached()) {
		std::system("cls");

		if (GetAsyncKeyState(VK_F1) & 1)
			freezeHealth = !freezeHealth;
		if (GetAsyncKeyState(VK_F2) & 1)
			freezeAmmo = !freezeAmmo;
		if (GetAsyncKeyState(VK_F3) & 1)
		{
			noRecoil = !noRecoil;

			if (noRecoil)
			{
				process.patchEx((BYTE*)baseAddress + 0xC8BA0, (BYTE*)"\xC2\x8", 3); // size = 3 to overwrite the \x28 from the originalcode
				/*
				* to turn off recoil -> ac_client.exe+C8BA0
				* Recoil (Original Code):	sub esp,28
				* NoRecoil (Modification):	ret 8
				*/
			}
			else
			{
				process.patchEx((BYTE*)baseAddress + 0xC8BA0, (BYTE*)"\x83\xEC\x28", 3);
			}
		}
			
		
		std::cout << strColoredOnOrOff(freezeHealth) << " Freeze Health (F1)" << std::endl;
		std::cout << strColoredOnOrOff(freezeAmmo) << " Freeze Ammo (F2)" << std::endl;
		std::cout << strColoredOnOrOff(noRecoil) << " NoRecoil (F3)" << std::endl;

		if (freezeHealth)
		{
			// TODO: freeze health nop opcode 
			DWORD healthAddress = localPlayer + offsets::health;
			process.WPM<int>(healthAddress, 1337);
		}

		if (freezeAmmo)
		{
			// TODO: freeze ammo nop opcode 
			DWORD currentWeaponAmmo = process.resolvePtrChainLinks(localPlayer, offsets::currentWeaponAmmo);
			process.WPM<int>(currentWeaponAmmo, 1337);
		}



		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	process.~Process(); // close proc handle

	return 0;
}
