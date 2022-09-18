module APU;

import Bus;

namespace APU
{
	void Initialize()
	{

	}


	template<std::integral Int>
	Int ReadReg(u32 addr)
	{
		auto ReadByte = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_SOUND1CNT_L:
			case Bus::ADDR_SOUND1CNT_L + 1:
			case Bus::ADDR_SOUND1CNT_H: 
			case Bus::ADDR_SOUND1CNT_H + 1:
			case Bus::ADDR_SOUND1CNT_X:
			case Bus::ADDR_SOUND1CNT_X + 1:
			case Bus::ADDR_SOUND1CNT_X + 2:
			case Bus::ADDR_SOUND1CNT_X + 3:
			case Bus::ADDR_SOUND2CNT_L:
			case Bus::ADDR_SOUND2CNT_L + 1:
			case Bus::ADDR_SOUND2CNT_L + 2:
			case Bus::ADDR_SOUND2CNT_L + 3:
			case Bus::ADDR_SOUND2CNT_H:
			case Bus::ADDR_SOUND2CNT_H + 1:
			case Bus::ADDR_SOUND2CNT_H + 2:
			case Bus::ADDR_SOUND2CNT_H + 3:
			case Bus::ADDR_SOUND3CNT_L:
			case Bus::ADDR_SOUND3CNT_L + 1:
			case Bus::ADDR_SOUND3CNT_H:
			case Bus::ADDR_SOUND3CNT_H + 1:
			case Bus::ADDR_SOUND3CNT_X:
			case Bus::ADDR_SOUND3CNT_X + 1:
			case Bus::ADDR_SOUND3CNT_X + 2:
			case Bus::ADDR_SOUND3CNT_X + 3:
			case Bus::ADDR_SOUND4CNT_L:
			case Bus::ADDR_SOUND4CNT_L + 1:
			case Bus::ADDR_SOUND4CNT_L + 2:
			case Bus::ADDR_SOUND4CNT_L + 3:
			case Bus::ADDR_SOUND4CNT_H:
			case Bus::ADDR_SOUND4CNT_H + 1:
			case Bus::ADDR_SOUND4CNT_H + 2:
			case Bus::ADDR_SOUND4CNT_H + 3:
			case Bus::ADDR_SOUNDBIAS:
				return u8(0);
			case Bus::ADDR_SOUNDBIAS + 1:
				return u8(0x2);
			case Bus::ADDR_SOUNDBIAS + 2:
			case Bus::ADDR_SOUNDBIAS + 3:
			case Bus::ADDR_WAVE_RAM:
			case Bus::ADDR_WAVE_RAM + 1:
			case Bus::ADDR_WAVE_RAM + 2:
			case Bus::ADDR_WAVE_RAM + 3:
			case Bus::ADDR_WAVE_RAM + 4:
			case Bus::ADDR_WAVE_RAM + 5:
			case Bus::ADDR_WAVE_RAM + 6:
			case Bus::ADDR_WAVE_RAM + 7:
			case Bus::ADDR_WAVE_RAM + 8:
			case Bus::ADDR_WAVE_RAM + 9:
			case Bus::ADDR_WAVE_RAM + 10:
			case Bus::ADDR_WAVE_RAM + 11:
			case Bus::ADDR_WAVE_RAM + 12:
			case Bus::ADDR_WAVE_RAM + 13:
			case Bus::ADDR_WAVE_RAM + 14:
			case Bus::ADDR_WAVE_RAM + 15:
				return u8(0);
			default: return Bus::ReadOpenBus<u8>(addr);
			}
		};

		auto ReadHalf = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_SOUND1CNT_L:
			case Bus::ADDR_SOUND1CNT_H:
			case Bus::ADDR_SOUND1CNT_X:
			case Bus::ADDR_SOUND1CNT_X + 2:
			case Bus::ADDR_SOUND2CNT_L:
			case Bus::ADDR_SOUND2CNT_L + 2:
			case Bus::ADDR_SOUND2CNT_H:
			case Bus::ADDR_SOUND2CNT_H + 2:
			case Bus::ADDR_SOUND3CNT_L:
			case Bus::ADDR_SOUND3CNT_H:
			case Bus::ADDR_SOUND3CNT_X:
			case Bus::ADDR_SOUND3CNT_X + 2:
			case Bus::ADDR_SOUND4CNT_L:
			case Bus::ADDR_SOUND4CNT_L + 2:
			case Bus::ADDR_SOUND4CNT_H:
			case Bus::ADDR_SOUND4CNT_H + 2:
				return u16(0);
			case Bus::ADDR_SOUNDBIAS:
				return u16(0x200);
			case Bus::ADDR_SOUNDBIAS + 2:
			case Bus::ADDR_WAVE_RAM:
			case Bus::ADDR_WAVE_RAM + 2:
			case Bus::ADDR_WAVE_RAM + 4:
			case Bus::ADDR_WAVE_RAM + 6:
			case Bus::ADDR_WAVE_RAM + 8:
			case Bus::ADDR_WAVE_RAM + 10:
			case Bus::ADDR_WAVE_RAM + 12:
			case Bus::ADDR_WAVE_RAM + 14:
				return u16(0);
			default: return Bus::ReadOpenBus<u16>(addr);
			}
		};

		if constexpr (sizeof(Int) == 1) {
			return ReadByte(addr);
		}
		if constexpr (sizeof(Int) == 2) {
			return ReadHalf(addr);
		}
		if constexpr (sizeof(Int) == 4) {
			u16 lo = ReadHalf(addr);
			u16 hi = ReadHalf(addr + 2);
			return lo | hi << 16;
		}
	}


	template<std::integral Int>
	void WriteReg(u32 addr, Int data)
	{
		auto WriteByte = [](u32 addr, u8 data) {
			switch (addr) {
			case Bus::ADDR_SOUND1CNT_L:
			case Bus::ADDR_SOUND1CNT_L + 1:
			case Bus::ADDR_SOUND1CNT_H:
			case Bus::ADDR_SOUND1CNT_H + 1:
			case Bus::ADDR_SOUND1CNT_X:
			case Bus::ADDR_SOUND1CNT_X + 1:
			case Bus::ADDR_SOUND1CNT_X + 2:
			case Bus::ADDR_SOUND1CNT_X + 3:
			case Bus::ADDR_SOUND2CNT_L:
			case Bus::ADDR_SOUND2CNT_L + 1:
			case Bus::ADDR_SOUND2CNT_L + 2:
			case Bus::ADDR_SOUND2CNT_L + 3:
			case Bus::ADDR_SOUND2CNT_H:
			case Bus::ADDR_SOUND2CNT_H + 1:
			case Bus::ADDR_SOUND2CNT_H + 2:
			case Bus::ADDR_SOUND2CNT_H + 3:
			case Bus::ADDR_SOUND3CNT_L:
			case Bus::ADDR_SOUND3CNT_L + 1:
			case Bus::ADDR_SOUND3CNT_H:
			case Bus::ADDR_SOUND3CNT_H + 1:
			case Bus::ADDR_SOUND3CNT_X:
			case Bus::ADDR_SOUND3CNT_X + 1:
			case Bus::ADDR_SOUND3CNT_X + 2:
			case Bus::ADDR_SOUND3CNT_X + 3:
			case Bus::ADDR_SOUND4CNT_L:
			case Bus::ADDR_SOUND4CNT_L + 1:
			case Bus::ADDR_SOUND4CNT_L + 2:
			case Bus::ADDR_SOUND4CNT_L + 3:
			case Bus::ADDR_SOUND4CNT_H:
			case Bus::ADDR_SOUND4CNT_H + 1:
			case Bus::ADDR_SOUND4CNT_H + 2:
			case Bus::ADDR_SOUND4CNT_H + 3:
			case Bus::ADDR_SOUNDBIAS:
			case Bus::ADDR_SOUNDBIAS + 1:
			case Bus::ADDR_SOUNDBIAS + 2:
			case Bus::ADDR_SOUNDBIAS + 3:
			case Bus::ADDR_WAVE_RAM:
			case Bus::ADDR_WAVE_RAM + 1:
			case Bus::ADDR_WAVE_RAM + 2:
			case Bus::ADDR_WAVE_RAM + 3:
			case Bus::ADDR_WAVE_RAM + 4:
			case Bus::ADDR_WAVE_RAM + 5:
			case Bus::ADDR_WAVE_RAM + 6:
			case Bus::ADDR_WAVE_RAM + 7:
			case Bus::ADDR_WAVE_RAM + 8:
			case Bus::ADDR_WAVE_RAM + 9:
			case Bus::ADDR_WAVE_RAM + 10:
			case Bus::ADDR_WAVE_RAM + 11:
			case Bus::ADDR_WAVE_RAM + 12:
			case Bus::ADDR_WAVE_RAM + 13:
			case Bus::ADDR_WAVE_RAM + 14:
			case Bus::ADDR_WAVE_RAM + 15:
				break;
			}
		};

		auto WriteHalf = [](u32 addr, u16 data) {
			switch (addr) {
			case Bus::ADDR_SOUND1CNT_L:
			case Bus::ADDR_SOUND1CNT_H:
			case Bus::ADDR_SOUND1CNT_X:
			case Bus::ADDR_SOUND1CNT_X + 2:
			case Bus::ADDR_SOUND2CNT_L:
			case Bus::ADDR_SOUND2CNT_L + 2:
			case Bus::ADDR_SOUND2CNT_H:
			case Bus::ADDR_SOUND2CNT_H + 2:
			case Bus::ADDR_SOUND3CNT_L:
			case Bus::ADDR_SOUND3CNT_H:
			case Bus::ADDR_SOUND3CNT_X:
			case Bus::ADDR_SOUND3CNT_X + 2:
			case Bus::ADDR_SOUND4CNT_L:
			case Bus::ADDR_SOUND4CNT_L + 2:
			case Bus::ADDR_SOUND4CNT_H:
			case Bus::ADDR_SOUND4CNT_H + 2:
			case Bus::ADDR_SOUNDBIAS:
			case Bus::ADDR_SOUNDBIAS + 2:
			case Bus::ADDR_WAVE_RAM:
			case Bus::ADDR_WAVE_RAM + 2:
			case Bus::ADDR_WAVE_RAM + 4:
			case Bus::ADDR_WAVE_RAM + 6:
			case Bus::ADDR_WAVE_RAM + 8:
			case Bus::ADDR_WAVE_RAM + 10:
			case Bus::ADDR_WAVE_RAM + 12:
			case Bus::ADDR_WAVE_RAM + 14:
				break;
			}
		};

		if constexpr (sizeof(Int) == 1) {
			WriteByte(addr, data);
		}
		if constexpr (sizeof(Int) == 2) {
			WriteHalf(addr, data);
		}
		if constexpr (sizeof(Int) == 4) {
			WriteHalf(addr, data & 0xFFFF);
			WriteHalf(addr + 2, data >> 16 & 0xFFFF);
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