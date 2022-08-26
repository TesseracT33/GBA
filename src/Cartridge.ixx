export module Cartridge;

import Util;

import <concepts>;
import <cstring>;
import <string>;
import <vector>;

namespace Cartridge
{
	export
	{
		bool LoadRom(const std::string& path);
		template<std::integral Int> Int ReadRam(u32 addr);
		template<std::integral Int> Int ReadRom(u32 addr);
		template<std::integral Int> void WriteRam(u32 addr, Int data);
	}

	std::vector<u8> rom;

	///////////// template definitions /////////////
	template<std::integral Int>
	Int ReadRam(u32 addr)
	{
		return 0;
	}


	template<std::integral Int>
	Int ReadRom(u32 addr)
	{
		addr -= 0x0800'0000;
		addr %= rom.size(); /* TODO: optimize */
		Int ret;
		std::memcpy(&ret, rom.data() + addr, sizeof(Int));
		return ret;
	}


	template<std::integral Int>
	void WriteRam(u32 addr, Int data)
	{

	}
}