module Cartridge;

namespace Cartridge
{
	void Initialize()
	{
		sram.resize(0x10000, 0xFF); /* TODO: for now, SRAM is assumed to always exist and be 64 KiB */
		sram_size_mask = 0xFFFF;
	}


	bool LoadRom(const std::string& path)
	{
		auto optional_vec = ReadFileIntoVector(path);
		if (optional_vec) {
			rom = optional_vec.value();
			ResizeRomToPowerOfTwo(rom);
			rom_size_mask = uint(rom.size() - 1);
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


	void ResizeRomToPowerOfTwo(std::vector<u8>& rom)
	{
		size_t actual_size = rom.size();
		size_t pow_two_size = std::bit_ceil(actual_size);
		size_t diff = pow_two_size - actual_size;
		if (diff > 0) {
			rom.resize(pow_two_size);
			std::copy(rom.begin(), rom.begin() + diff, rom.begin() + actual_size);
		}
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