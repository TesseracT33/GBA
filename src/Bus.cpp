module Bus;

import Bios;
import Cartridge;
import PPU;

namespace Bus
{
	template<std::integral Int>
	Int Read(u32 addr)
	{
		static_assert(sizeof(Int) == 1 || sizeof(Int) == 2 || sizeof(Int) == 4);
		/* A byte load expects the data on data bus inputs 7 through 0 if the supplied
		address is on a word boundary, on data bus inputs 15 through 8 if it is a word address
		plus one byte, and so on. The selected byte is placed in the bottom 8 bits of the
		destination register, and the remaining bits of the register are filled with zeros.

		A word load will normally use a word aligned address. However, an address offset
		from a word boundary will cause the data to be rotated into the register so that
		the addressed byte occupies bits 0 to 7. This means that half-words accessed at
		offsets 0 and 2 from the word boundary will be correctly loaded into bits 0 through 15
		of the register. Two shift operations are then required to clear or to sign extend the
		upper 16 bits.
		*/

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
			return ReadIO<Int>(addr);

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
	Int ReadIO(u32 addr)
	{
		return 0;
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
		/* A byte store repeats the bottom 8 bits of the source register four times across
		data bus outputs 31 through 0.
		
		A halfword store repeats the bottom 16 bits of the source register twice across
		the data bus outputs 31 through to 0. Note that the address must be
		halfword aligned; if bit 0 of the address is HIGH this will cause unpredictable
		behaviour.

		A word store should generate a word aligned address. The word presented to the
		data bus is not affected if the address is not word aligned. That is, bit 31 of the
		register being stored always appears on data bus output 31.
		*/

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
			WriteIO<Int>(addr, data);
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
	void WriteIO(u32 addr, Int data)
	{

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