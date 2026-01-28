#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <string>

namespace Memory {
	DWORD ProcessId = 0;
	HANDLE hProcess = INVALID_HANDLE_VALUE;

	bool AttachToProcess(DWORD PID) {
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, PID);
		if (hProcess) {
			ProcessId = PID;
			return true;
		}
		return false;
	}

	DWORD GetPID(std::wstring processName) {
		DWORD pid = 0;
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot != INVALID_HANDLE_VALUE) {
			PROCESSENTRY32W pe32;
			pe32.dwSize = sizeof(PROCESSENTRY32W);
			if (Process32FirstW(hSnapshot, &pe32)) {
				do {
					if (processName == pe32.szExeFile) {
						pid = pe32.th32ProcessID;
						break;
					}
				} while (Process32NextW(hSnapshot, &pe32));
			}
			CloseHandle(hSnapshot);
		}
		return pid;
	}

	uintptr_t GetModuleBaseAddress(std::wstring moduleName) {
		uintptr_t BaseAddress = 0;
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, ProcessId);

		if (hSnapshot != INVALID_HANDLE_VALUE) {
			MODULEENTRY32W modEntry;
			modEntry.dwSize = sizeof(MODULEENTRY32W);

			if (Module32FirstW(hSnapshot, &modEntry)) {
				do {
					if (_wcsicmp(moduleName.c_str(), modEntry.szModule) == 0){
						BaseAddress = (uintptr_t)modEntry.modBaseAddr;
						break;
					}
				} while (Module32NextW(hSnapshot, &modEntry));
			}
			CloseHandle(hSnapshot);
		}
		return BaseAddress;
	}

	template<typename x>
	x read(uintptr_t Address) {
		x value = {};
		size_t bytesread;
		if (ReadProcessMemory(hProcess, (LPCVOID)Address, &value, sizeof(x), &bytesread)) {
			return value;
		}
	}

	template<typename x>
	x read(uintptr_t Address, bool& SUCCESS) {
		x value = {};
		size_t bytesread;
		if (ReadProcessMemory(hProcess, (LPCVOID)Address, &value, sizeof(x), &bytesread)) {
			SUCCESS = true;
			return value;
		}
		SUCCESS = false;
		return 0;
	}

	bool read_batch(uintptr_t Address, void* Buffer, size_t SizeToRead) {
		SIZE_T bytesRead;
		if (ReadProcessMemory(hProcess, (LPCVOID)Address, Buffer, SizeToRead, &bytesRead)) {
			return bytesRead == SizeToRead;
		}
		return false;
	}

	template<typename x>
	bool write(uintptr_t Address, x ToWrite) {
		size_t byteswritten;
		if (WriteProcessMemory(hProcess, (LPCVOID)Address, &ToWrite, sizeof(x), &byteswritten)) {
			return true;
		}
		return false;
	}

	// Roblox stuff //

	std::string ReadString(uintptr_t Address) {
		int length = Memory::read<int>(Address + 0x18);

		if (length <= 0 || length > 1000)
			return "ERROR (1)";

		uintptr_t DataAddress = Address;

		if (static_cast<uint32_t>(length) >= 16) {
			DataAddress = Memory::read<uintptr_t>(Address);
			if (!DataAddress) return "ERROR (2)";
		}

		std::string result;
		result.resize(length);

		if (Memory::read_batch(DataAddress, &result[0], length)) {
			size_t firstNull = result.find('\0');
			if (firstNull != std::string::npos) {
				result.resize(firstNull);
			}

			return result;
		}

		return "";
	}
}
