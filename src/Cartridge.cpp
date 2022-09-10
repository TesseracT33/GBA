module Cartridge;

namespace Cartridge
{
	void Initialize()
	{

	}


	bool LoadRom(const std::string& path)
	{
		auto optional_vec = ReadFileIntoVector(path);
		if (optional_vec) {
			rom = optional_vec.value();
			assert(std::has_single_bit(rom.size()));
			rom_size_mask = rom.size() - 1;
			return true;
		}
		else {
			return false;
		}
	}


	u8 ReadSram(u32 addr)
	{
		u32 offset = addr & sram_size_mask; /* todo: detect sram size */
		return sram[offset];
	}


	template<std::integral Int>
	Int ReadRom(u32 addr)
	{
		u32 offset = addr & 0x1FF'FFFF & rom_size_mask;
		Int ret;
		std::memcpy(&ret, rom.data() + offset, sizeof(Int));
		return ret;
	}


	void WriteSram(u32 addr, u8 data)
	{
		u32 offset = addr & sram_size_mask;
		sram[offset] = data;
	}


	template u8 ReadRom<u8>(u32);
	template s8 ReadRom<s8>(u32);
	template u16 ReadRom<u16>(u32);
	template s16 ReadRom<s16>(u32);
	template u32 ReadRom<u32>(u32);
	template s32 ReadRom<s32>(u32);
}