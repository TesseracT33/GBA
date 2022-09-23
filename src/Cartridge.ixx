export module Cartridge;

import Util;

import <algorithm>;
import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;
import <string>;
import <vector>;

namespace Cartridge
{
	export
	{
		void Initialize();
		bool LoadRom(const std::string& path);
		u8 ReadSram(u32 addr);
		template<std::integral Int> Int ReadRom(u32 addr);
		void WriteSram(u32 addr, u8 data);
	}

	void ResizeRomToPowerOfTwo(std::vector<u8>& rom);

	u32 rom_size_mask;
	u32 sram_size_mask;

	std::vector<u8> rom;
	std::vector<u8> sram;
}