module Timers;

import Bus;

namespace Timers
{
	void Initialize()
	{
		for (Timer& t : timer) {
			std::memset(&t, 0, sizeof(Timer));
		}
	}


	template<std::integral Int>
	Int ReadReg(u32 addr)
	{
		auto ReadByte = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_TM0CNT_L:     return GetByte(timer[0].counter, 0);
			case Bus::ADDR_TM0CNT_L + 1: return GetByte(timer[0].counter, 1);
			case Bus::ADDR_TM0CNT_H:     return std::bit_cast<u8>(timer[0].control);
			case Bus::ADDR_TM0CNT_H + 1: return u8(0);
			case Bus::ADDR_TM1CNT_L:     return GetByte(timer[1].counter, 0);
			case Bus::ADDR_TM1CNT_L + 1: return GetByte(timer[1].counter, 1);
			case Bus::ADDR_TM1CNT_H:     return std::bit_cast<u8>(timer[1].control);
			case Bus::ADDR_TM1CNT_H + 1: return u8(0);
			case Bus::ADDR_TM2CNT_L:     return GetByte(timer[2].counter, 0);
			case Bus::ADDR_TM2CNT_L + 1: return GetByte(timer[2].counter, 1);
			case Bus::ADDR_TM2CNT_H:     return std::bit_cast<u8>(timer[2].control);
			case Bus::ADDR_TM2CNT_H + 1: return u8(0);
			case Bus::ADDR_TM3CNT_L:     return GetByte(timer[3].counter, 0);
			case Bus::ADDR_TM3CNT_L + 1: return GetByte(timer[3].counter, 1);
			case Bus::ADDR_TM3CNT_H:     return std::bit_cast<u8>(timer[3].control);
			case Bus::ADDR_TM3CNT_H + 1: return u8(0);
			default:                     return Bus::ReadOpenBus<u8>(addr);
			}
		};

		auto ReadHalf = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_TM0CNT_L: return timer[0].counter;
			case Bus::ADDR_TM0CNT_H: return u16(std::bit_cast<u8>(timer[0].control));
			case Bus::ADDR_TM1CNT_L: return timer[1].counter;
			case Bus::ADDR_TM1CNT_H: return u16(std::bit_cast<u8>(timer[1].control));
			case Bus::ADDR_TM2CNT_L: return timer[2].counter;
			case Bus::ADDR_TM2CNT_H: return u16(std::bit_cast<u8>(timer[2].control));
			case Bus::ADDR_TM3CNT_L: return timer[3].counter;
			case Bus::ADDR_TM3CNT_H: return u16(std::bit_cast<u8>(timer[3].control));
			default:                 return Bus::ReadOpenBus<u16>(addr);
			}
		};

		auto ReadWord = [&](u32 addr) {
			u16 lo = ReadHalf(addr);
			u16 hi = ReadHalf(addr + 2);
			return lo | hi << 16;
		};

		if constexpr (sizeof(Int) == 1) {
			return ReadByte(addr);
		}
		if constexpr (sizeof(Int) == 2) {
			return ReadHalf(addr);
		}
		if constexpr (sizeof(Int) == 4) {
			return ReadWord(addr);
		}
	}


	template<std::integral Int>
	void WriteReg(u32 addr, Int data)
	{
		auto WriteByte = [](u32 addr, u8 data) {
			switch (addr) {
			case Bus::ADDR_TM0CNT_L:     break;
			case Bus::ADDR_TM0CNT_L + 1: break;
			case Bus::ADDR_TM0CNT_H:     break;
			case Bus::ADDR_TM0CNT_H + 1: break;
			case Bus::ADDR_TM1CNT_L:     break;
			case Bus::ADDR_TM1CNT_L + 1: break;
			case Bus::ADDR_TM1CNT_H:     break;
			case Bus::ADDR_TM1CNT_H + 1: break;
			case Bus::ADDR_TM2CNT_L:     break;
			case Bus::ADDR_TM2CNT_L + 1: break;
			case Bus::ADDR_TM2CNT_H:     break;
			case Bus::ADDR_TM2CNT_H + 1: break;
			case Bus::ADDR_TM3CNT_L:     break;
			case Bus::ADDR_TM3CNT_L + 1: break;
			case Bus::ADDR_TM3CNT_H:     break;
			case Bus::ADDR_TM3CNT_H + 1: break;
			default: break;
			}
		};

		auto WriteHalf = [](u32 addr, u16 data) {
			switch (addr) {
			case Bus::ADDR_TM0CNT_L:     break;
			case Bus::ADDR_TM0CNT_H:     break;
			case Bus::ADDR_TM1CNT_L:     break;
			case Bus::ADDR_TM1CNT_H:     break;
			case Bus::ADDR_TM2CNT_L:     break;
			case Bus::ADDR_TM2CNT_H:     break;
			case Bus::ADDR_TM3CNT_L:     break;
			case Bus::ADDR_TM3CNT_H:     break;
			default: break;
			}
		};

		auto WriteWord = [&](u32 addr, u32 data) {
			WriteHalf(addr, data & 0xFFFF);
			WriteHalf(addr + 2, data >> 16 & 0xFFFF);
		};

		if constexpr (sizeof(Int) == 1) {
			WriteByte(addr, data);
		}
		if constexpr (sizeof(Int) == 2) {
			WriteHalf(addr, data);
		}
		if constexpr (sizeof(Int) == 4) {
			WriteWord(addr, data);
		}
	}


	template u8 ReadReg<u8>(u32);
	template s8 ReadReg<s8>(u32);
	template u16 ReadReg<u16>(u32);
	template s16 ReadReg<s16>(u32);
	template u32 ReadReg<u32>(u32);
	template s32 ReadReg<s32>(u32);
	template void WriteReg<u8>(u32, u8);
	template void WriteReg<s8>(u32, s8);
	template void WriteReg<u16>(u32, u16);
	template void WriteReg<s16>(u32, s16);
	template void WriteReg<u32>(u32, u32);
	template void WriteReg<s32>(u32, s32);
}