export module Bus;

import NumericalTypes;

import <array>;
import <bit>;
import <concepts>;
import <cstring>;

namespace Bus
{
	export
	{
		template<std::integral Int>
		Int Read(s32 addr);

		template<std::integral Int>
		void Write(s32 addr, Int data);
	}

	enum Addr : s32 {
		DISPCNT = 0x0,
		DISPSTAT = 0x4,
		VCOUNT = 0x6,
		BG0CNT = 0x8,
		BG1CNT = 0xA,
		BG2CNT = 0xC,
		BG3CNT = 0xE,
		BG0HOFS = 0x10,
		BG0VOFS = 0x12,
		BG1HOFS = 0x14,
		BG1VOFS = 0x16,
		BG2HOFS = 0x18,
		BG2VOFS = 0x1A,
		BG3HOFS = 0x1C,
		BG3VOFS = 0x1E,
		BG2PA = 0x20,
		BG2PB = 0x22,
		BG2PC = 0x24,
		BG2PD = 0x26,
		BG2X = 0x28,
		BG2Y = 0x2C,
		BG3PA = 0x30,
		BG3PB = 0x32,
		BG3PC = 0x34,
		BG3PD = 0x36,
		BG3X = 0x38,
		BG3Y = 0x3C,
		WIN0H = 0x40,
		WIN1H = 0x42,
		WIN0V = 0x44,
		WIN1V = 0x46,
		WININ = 0x48,
		WINOUT = 0x4A,
		MOSAIC = 0x4C,
		BLDCNT = 0x50,
		BLDALPHA = 0x52,
		BLDY = 0x54
	};

	std::array<u8, 0x10000> page_table_read;
	std::array<u8, 0x10000> page_table_write;
}