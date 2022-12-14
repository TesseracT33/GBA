module Keypad;

import Bus;
import IRQ;

namespace Keypad
{
	void Initialize()
	{
		keyinput = 0x3FF;
		keycnt = 0;
	}


	void NotifyButtonPressed(uint index)
	{
		keyinput &= ~(1 << index);
		UpdateIrq();
	}


	void NotifyButtonReleased(uint index)
	{
		keyinput |= 1 << index;
	}


	template<std::integral Int>
	Int ReadReg(u32 addr)
	{
		if constexpr (sizeof(Int) == 1) {
			switch (addr) {
			case Bus::ADDR_KEYINPUT:     return GetBit(keyinput, 0);
			case Bus::ADDR_KEYINPUT + 1: return GetBit(keyinput, 1);
			case Bus::ADDR_KEYCNT:       return GetBit(keycnt, 0);
			case Bus::ADDR_KEYCNT + 1:   return GetBit(keycnt, 1);
			default: std::unreachable();
			}
		}
		if constexpr (sizeof(Int) == 2) {
			switch (addr) {
			case Bus::ADDR_KEYINPUT:     return keyinput;
			case Bus::ADDR_KEYCNT:       return keycnt;
			default: std::unreachable();
			}
		}
		if constexpr (sizeof(Int) == 4) {
			return keyinput | keycnt << 16;
		}
	}


	void StreamState(SerializationStream& stream)
	{

	}


	void UpdateIrq()
	{
		static constexpr u16 irq_enable_mask = 1 << 14;
		static constexpr u16 irq_cond_mask = 1 << 15;
		if (keycnt & irq_enable_mask) {
			if (keycnt & irq_cond_mask) { /* logical AND mode */
				if ((~keyinput & keycnt & 0x3FF) == keycnt) {
					IRQ::Raise(IRQ::Source::Keypad);
				}
			}
			else { /* logical OR mode */
				if (~keyinput & keycnt & 0x3FF) {
					IRQ::Raise(IRQ::Source::Keypad);
				}
			}
		}
	}


	template<std::integral Int>
	void WriteReg(u32 addr, Int data)
	{
		if constexpr (sizeof(Int) == 1) {
			switch (addr) {
			case Bus::ADDR_KEYCNT:     SetByte(keycnt, 0, data); break;
			case Bus::ADDR_KEYCNT + 1: SetByte(keycnt, 1, data); break;
			default: break;
			}
		}
		if constexpr (sizeof(Int) == 2) {
			if (addr == Bus::ADDR_KEYCNT) {
				keycnt = data;
			}
		}
		if constexpr (sizeof(Int) == 4) {
			keycnt = data >> 16 & 0xFFFF;
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