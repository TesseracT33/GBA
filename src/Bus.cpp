module Bus;

import APU;
import Bios;
import Cartridge;
import DMA;
import Keypad;
import IRQ;
import PPU;
import Timers;

namespace Bus
{
	void Initialize()
	{
		board_wram.fill(0);
		chip_wram.fill(0);
	}


	template<std::integral Int>
	Int Read(u32 addr)
	{
		static_assert(sizeof(Int) == 1 || sizeof(Int) == 2 || sizeof(Int) == 4);

		if constexpr (sizeof(Int) == 2) {
			addr &= ~1; /* TODO: correct? */
		}
		if constexpr (sizeof(Int) == 4) {
			addr &= ~3;
		}

		if (addr & 0xF000'0000) { /* 1000'0000-FFFF'FFFF   Not used (upper 4bits of address bus unused) */
			return ReadOpenBus<Int>(addr);
		}

		switch (addr >> 24 & 0xF) {
		case 0x0: /* 0000'0000-0000'3FFF   BIOS - System ROM */
			if (addr <= 0x3FFF) {
				Int ret;
				std::memcpy(&ret, bios.data() + addr, sizeof(Int));
				return ret;
			}
			else {
				return ReadOpenBus<Int>(addr);
			}

		case 0x1: /* not used */
			return ReadOpenBus<Int>(addr);

		case 0x2: { /* 0200'0000-0203'FFFF   WRAM - On-board Work RAM */
			Int ret;
			std::memcpy(&ret, board_wram.data() + (addr & 0x3FFFF), sizeof(Int));
			return ret;
		}

		case 0x3: { /* 0300'0000-0300'7FFF   WRAM - On-chip Work RAM */
			Int ret;
			std::memcpy(&ret, chip_wram.data() + (addr & 0x7FFF), sizeof(Int));
			return ret;
		}

		case 0x4: /* 0400'0000-0400'03FE   I/O Registers */
			return ReadIo<Int>(addr);

		case 0x5: /* 0500'0000-0500'03FF   BG/OBJ Palette RAM */
			return PPU::ReadPaletteRam<Int>(addr);

		case 0x6: /* 0600'0000-0601'7FFF   VRAM - Video RAM */
			return PPU::ReadVram<Int>(addr);

		case 0x7: /* 0700'0000-0700'03FF   OAM - OBJ Attributes */
			return PPU::ReadOam<Int>(addr);

		case 0x8: /* 0800'0000-09FF'FFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 0 */
		case 0x9:
		case 0xA: /* 0A00'0000-0BFF'FFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 1 */
		case 0xB:
		case 0xC: /* 0C00'0000-0DFF'FFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 2 */
		case 0xD:
			return Cartridge::ReadRom<Int>(addr);

		case 0xE: /* 0E00'0000-0E00'FFFF   Game Pak SRAM    (max 64 KBytes) - 8bit Bus width */
		case 0xF:
			return Cartridge::ReadRam<Int>(addr);

		default:
			std::unreachable();
		}
	}


	template<std::integral Int>
	Int ReadIo(u32 addr)
	{
		/* Reads are aligned, and all regions start at word-aligned addresses, so there cannot be a cross-region read. */
		/* TODO: measure if jump table will be faster */
		if (addr < 0x400'0060) {
			return PPU::ReadReg<Int>(addr);
		}
		else if (addr < 0x400'00B0) {
			return APU::ReadReg<Int>(addr);
		}
		else if (addr < 0x400'0100) {
			return DMA::ReadReg<Int>(addr);
		}
		else if (addr < 0x400'0120) {
			return Timers::ReadReg<Int>(addr);
		}
		else if (addr < 0x400'0130) {
			return 0; /* serial #1 */
		}
		else if (addr < 0x400'0134) {
			return Keypad::ReadReg<Int>(addr);
		}
		else if (addr < 0x400'0200) {
			return 0; /* serial #2 */
		}
		else {
			auto ReadByte = [](u32 addr) {
				switch (addr) {
				case ADDR_IE:     return IRQ::ReadIE(0);
				case ADDR_IE + 1: return IRQ::ReadIE(1);
				case ADDR_IF:     return IRQ::ReadIF(0);
				case ADDR_IF + 1: return IRQ::ReadIF(1);
				case ADDR_IME:    return u8(IRQ::ReadIME());
				case ADDR_IME + 1: u8(0);
				default: return ReadOpenBus<u8>(addr);
				}
			};
			auto ReadHalf = [](u32 addr) {
				switch (addr) {
				case ADDR_IE:  return IRQ::ReadIE();
				case ADDR_IF:  return IRQ::ReadIF();
				case ADDR_IME: return IRQ::ReadIME();
				default: return ReadOpenBus<u16>(addr);
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
	}


	template<std::integral Int>
	Int ReadOpenBus(u32 addr)
	{
		return 0;
	}


	template<std::integral Int>
	void Write(u32 addr, Int data)
	{
		static_assert(sizeof(Int) == 1 || sizeof(Int) == 2 || sizeof(Int) == 4);
		
		if (addr & 0xF000'0000) { /* 1000'0000-FFFF'FFFF   Not used (upper 4bits of address bus unused) */
			return;
		}
		if constexpr (sizeof(Int) == 2) {
			addr &= ~1;
		}
		if constexpr (sizeof(Int) == 4) {
			addr &= ~3;
		}

		switch (addr >> 24 & 0xF) {
		case 0x2: /* 0200'0000-0203'FFFF   WRAM - On-board Work RAM */
			std::memcpy(board_wram.data() + (addr & 0x3FFFF), &data, sizeof(Int));
			break;

		case 0x3: /* 0300'0000-0300'7FFF   WRAM - On-chip Work RAM */
			std::memcpy(chip_wram.data() + (addr & 0x7FFF), &data, sizeof(Int));
			break;

		case 0x4: /* 0400'0000-0400'03FE   I/O Registers */
			WriteIo<Int>(addr, data);
			break;

		case 0x5: /* 0500'0000-0500'03FF   BG/OBJ Palette RAM */
			PPU::WritePaletteRam<Int>(addr, data);
			break;

		case 0x6: /* 0600'0000-0601'7FFF   VRAM - Video RAM */
			PPU::WriteVram<Int>(addr, data);
			break;

		case 0x7: /* 0700'0000-0700'03FF   OAM - OBJ Attributes */
			PPU::WriteOam<Int>(addr, data);
			break;

		case 0xE: /* 0E00'0000-0E00'FFFF   Game Pak SRAM    (max 64 KBytes) - 8bit Bus width */
		case 0xF:
			Cartridge::WriteRam<Int>(addr, data);
			break;

		default:
			break;
		}
	}


	template<std::integral Int>
	void WriteIo(u32 addr, Int data)
	{
		/* Writes are aligned, and all regions start at word-aligned addresses, so there cannot be a cross-region write. */
		if (addr < 0x400'0060) {
			PPU::WriteReg(addr, data);
		}
		else if (addr < 0x400'00B0) {
			APU::WriteReg(addr, data);
		}
		else if (addr < 0x400'0100) {
			DMA::WriteReg(addr, data);
		}
		else if (addr < 0x400'0120) {
			Timers::WriteReg(addr, data);
		}
		else if (addr < 0x400'0130) {
			/* serial #1 */
		}
		else if (addr < 0x400'0134) {
			Keypad::WriteReg(addr, data);
		}
		else if (addr < 0x400'0200) {
			/* serial #2 */
		}
		else {
			auto WriteByte = [](u32 addr, u8 data) {
				switch (addr) {
				case ADDR_IE:     IRQ::WriteIE(data, 0);
				case ADDR_IE + 1: IRQ::WriteIE(data, 1);
				case ADDR_IF:     IRQ::WriteIF(data, 0);
				case ADDR_IF + 1: IRQ::WriteIF(data, 1);
				case ADDR_IME:    IRQ::WriteIME(data);
				}
			};
			auto WriteHalf = [](u32 addr, u16 data) {
				switch (addr) {
				case ADDR_IE:  IRQ::WriteIE(data);
				case ADDR_IF:  IRQ::WriteIF(data);
				case ADDR_IME: IRQ::WriteIME(data);
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
	}


	template s8 Read<s8>(u32);
	template u8 Read<u8>(u32);
	template s16 Read<s16>(u32);
	template u16 Read<u16>(u32);
	template s32 Read<s32>(u32);
	template u32 Read<u32>(u32);
	template void Write<s8>(u32, s8);
	template void Write<u8>(u32, u8);
	template void Write<s16>(u32, s16);
	template void Write<u16>(u32, u16);
	template void Write<s32>(u32, s32);
	template void Write<u32>(u32, u32);
}