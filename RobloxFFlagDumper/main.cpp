#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream>

#include "Memory.hpp"

namespace offsets {
    uintptr_t FFlagList = 0x0;
	uintptr_t ValueGetSet = 0x30; // Easy to Dump but im lazy atm
    uintptr_t FlagToValue = 0x0;
}

int main() {
	DWORD PID = Memory::GetPID(L"RobloxPlayerBeta.exe");
	if (!Memory::AttachToProcess(PID)) {
		std::cout << "couldnt attach to Roblox!" << std::endl;
		getchar();
		return 1;
	}
    uintptr_t BaseAddress = Memory::GetModuleBaseAddress(L"RobloxPlayerBeta.exe");
    std::cout << "PID -> " << std::dec << Memory::ProcessId << std::endl;
    std::cout << "BaseAddress -> 0x" << std::hex << BaseAddress << std::endl;

	for (uintptr_t Wow = 0x7A80000; Wow < 0x8000000; Wow += 0x8) {
		uintptr_t MaybeMap = Memory::read<uintptr_t>(BaseAddress + Wow);

		if (!MaybeMap || MaybeMap < 0x10000)
			continue;

		uintptr_t IsThisReallyAMapValidation = Memory::read<uintptr_t>(MaybeMap);
		if (!IsThisReallyAMapValidation || IsThisReallyAMapValidation < 0x10000)
			continue;

		if (IsThisReallyAMapValidation == 0x3F800000) {
			std::cout << "Potential map found at offset: " << std::hex << Wow << std::endl;

			// Yes this is a Map
			uintptr_t MapStart = Memory::read<uintptr_t>(MaybeMap + 0x8);
			uintptr_t Current = Memory::read<uintptr_t>(MapStart);

			if (MapStart < 0x10000 || MapStart > 0x00007FFFFFFFFFFF) {
				std::cout << "MapStart failed! -> 0x" << std::hex << MapStart << std::endl;
				continue;
			}

			if (Current < 0x10000 || Current > 0x00007FFFFFFFFFFF) {
				std::cout << "Current (1) failed! -> 0x" << std::hex << Current << std::endl;
				continue;
			}

			while (Current != 0) {
				std::string Name = Memory::ReadString(Current + 0x10);
				if (Name == "BatchThumbnailMinWaitMs") {
					std::cout << "BatchThumbnailMinWaitMs Found." << std::endl;
					uintptr_t ValueGetSet = Memory::read<uintptr_t>(Current + offsets::ValueGetSet);
					for (uintptr_t PointerToValueOffset = 0x0; PointerToValueOffset < 0x300; PointerToValueOffset = PointerToValueOffset + 0x8) {
						uintptr_t PointerToValue = Memory::read<uintptr_t>(ValueGetSet + PointerToValueOffset);
						int Value = Memory::read<int>(PointerToValue);
						if (Value == 15) {
							offsets::FFlagList = Wow;
							offsets::FlagToValue = PointerToValueOffset;
							goto FlagOffsetFound;
						}
					}
					std::cout << "Give up in life, you are a failure." << std::endl;
					getchar();
				}
				uintptr_t NewCurrent = Memory::read<uintptr_t>(Current);
				if (NewCurrent < 0x10000 || NewCurrent > 0x00007FFFFFFFFFFF) {
					std::cout << "Current (2) failed! -> 0x" << std::hex << Current << std::endl;
					break;
				}
				if (Current == NewCurrent) {
					break;
				}
				Current = NewCurrent;
			}
		}
	}
	std::cout << "FFlag Map not found in scan range." << std::endl;
	getchar();

FlagOffsetFound:
	std::cout << "Map @ 0x" <<  std::hex << offsets::FFlagList << std::endl;
	std::cout << "ValueGetSet -> 0x" << std::hex << offsets::ValueGetSet << std::endl;
	std::cout << "FlagToValue -> 0x" << std::hex << offsets::FlagToValue << std::endl;

	// HELL YEA BABY LETS DUMP THE FFLAGS!!! //
	uintptr_t FFlagPointer1 = Memory::read<uintptr_t>(BaseAddress + offsets::FFlagList);
	uintptr_t FFlagList = Memory::read<uintptr_t>(FFlagPointer1 + 0x8);

	uintptr_t Last = Memory::read<uintptr_t>(FFlagList + 0x8);
	uintptr_t Current = FFlagList;

	bool FirstJsonEntry = true;
	std::ofstream out("FFlags.hpp");
	std::ofstream outjson("FFlags.json");

	// JSON //
	outjson << "{\n";
	outjson << "    \"FFlagOffsets\": {\n";
	outjson << "        \"FFlagList\": \"0x" << std::uppercase << std::hex << offsets::FFlagList << "\",\n";
	outjson << "        \"ValueGetSet\": \"0x" << std::uppercase << std::hex << offsets::ValueGetSet << "\",\n"; 
	outjson << "        \"FlagToValue\": \"0x" << std::uppercase << std::hex << offsets::FlagToValue << "\"\n";
	outjson << "    },\n";
	outjson << "    \"FFlags\": {\n";
	// JSON //

	// hpp //
	out << "#pragma once\n\n";
	out << "namespace FFlagOffsets\n{\n";
	out << "    uintptr_t FFlagList" << " = 0x" << std::uppercase << std::hex << offsets::FFlagList << ";\n";
	out << "    uintptr_t ValueGetSet" << " = 0x" << std::uppercase << std::hex << offsets::ValueGetSet << ";\n";
	out << "    uintptr_t FlagToValue" << " = 0x" << std::uppercase << std::hex << offsets::FlagToValue << ";\n";
	out << "}\n\n";
	out << "namespace FFlags\n{\n";
	// hpp //
	
	// FFlag Dumping
	while (Current != 0 && Current != Last)
	{
		std::string Name = Memory::ReadString(Current + 0x10);
		uintptr_t ValueGetSet = Memory::read<uintptr_t>(Current + offsets::ValueGetSet);

		if (ValueGetSet < 0x10000 || ValueGetSet > 0x00007FFFFFFFFFFF) {
			Current = Memory::read<uintptr_t>(Current);
			continue;
		}

		std::string mhm = Memory::ReadString(ValueGetSet + offsets::FlagToValue); 

		if (mhm == "True" || mhm == "False")
		{
			// gg
		}
		else
		{
			uintptr_t offset = Memory::read<uintptr_t>(ValueGetSet + offsets::FlagToValue) - BaseAddress;

			if (offset < 0x10000 || offset > 0x00007FFFFFFFFFFF) {
				Current = Memory::read<uintptr_t>(Current);
				continue;
			}

			for (auto& c : Name)
			{
				if (!isalnum(static_cast<unsigned char>(c)))
					c = '_';
			}
			out << "    uintptr_t " << Name << " = 0x" << std::uppercase << std::hex << offset << ";\n";

			// json shit
			if (!FirstJsonEntry)
				outjson << ",\n";
			else
				FirstJsonEntry = false;

			outjson << "        \"" << Name << "\": \"0x" << std::uppercase << std::hex << offset << "\"";
		}

		Current = Memory::read<uintptr_t>(Current);
	}

	outjson << "\n    }\n";
	outjson << "}";
	outjson.close();

	out << "}";
	out.close();
	return 0;
}
