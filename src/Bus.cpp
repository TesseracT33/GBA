module Bus;

import APU;
import Bios;
import Cartridge;
import CPU;
import Debug;
import DMA;
import Keypad;
import IRQ;
import PPU;
import Serial;
import Timers;

namespace Bus
{
	void Initialize()
	{
		std::memset(&waitcnt, 0, sizeof(waitcnt));
		board_wram.fill(0);
		chip_wram.fill(0);
	}


	constexpr std::optional<std::string_view> IoAddrToStr(u32 addr)
	{
		switch (addr & ~1) { /* TODO: allow for non-halfword-aligned addresses */
		case ADDR_DISPCNT: return "DISPCNT";
		case ADDR_GREEN_SWAP: return "GREEN_SWAP";
		case ADDR_DISPSTAT: return "DISPSTAT";
		case ADDR_VCOUNT: return "VCOUNT";
		case ADDR_BG0CNT: return "BG0CNT";
		case ADDR_BG1CNT: return "BG1CNT";
		case ADDR_BG2CNT: return "BG2CNT";
		case ADDR_BG3CNT: return "BG3CNT";
		case ADDR_BG0HOFS: return "BG0HOFS";
		case ADDR_BG0VOFS: return "BG0VOFS";
		case ADDR_BG1HOFS: return "BG1HOFS";
		case ADDR_BG1VOFS: return "BG1VOFS";
		case ADDR_BG2HOFS: return "BG2HOFS";
		case ADDR_BG2VOFS: return "BG2VOFS";
		case ADDR_BG3HOFS: return "BG3HOFS";
		case ADDR_BG3VOFS: return "BG3VOFS";
		case ADDR_BG2PA: return "BG2PA";
		case ADDR_BG2PB: return "BG2PB";
		case ADDR_BG2PC: return "BG2PC";
		case ADDR_BG2PD: return "BG2PD";
		case ADDR_BG2X: return "BG2X";
		case ADDR_BG2Y: return "BG2Y";
		case ADDR_BG3PA: return "BG3PA";
		case ADDR_BG3PB: return "BG3PB";
		case ADDR_BG3PC: return "BG3PC";
		case ADDR_BG3PD: return "BG3PD";
		case ADDR_BG3X: return "BG3X";
		case ADDR_BG3Y: return "BG3Y";
		case ADDR_WIN0H: return "WIN0H";
		case ADDR_WIN1H: return "WIN1H";
		case ADDR_WIN0V: return "WIN0V";
		case ADDR_WIN1V: return "WIN1V";
		case ADDR_WININ: return "WININ";
		case ADDR_WINOUT: return "WINOUT";
		case ADDR_MOSAIC: return "MOSAIC";
		case ADDR_BLDCNT: return "BLDCNT";
		case ADDR_BLDALPHA: return "BLDALPHA";
		case ADDR_BLDY: return "BLDY";
		/* Sound */
		case ADDR_SOUND1CNT_L: return "SOUND1CNT_L";
		case ADDR_SOUND1CNT_H: return "SOUND1CNT_H";
		case ADDR_SOUND1CNT_X: return "SOUND1CNT_X";
		case ADDR_SOUND2CNT_L: return "SOUND2CNT_L";
		case ADDR_SOUND2CNT_H: return "SOUND2CNT_H";
		case ADDR_SOUND3CNT_L: return "SOUND3CNT_L";
		case ADDR_SOUND3CNT_H: return "SOUND3CNT_H";
		case ADDR_SOUND3CNT_X: return "SOUND3CNT_X";
		case ADDR_SOUND4CNT_L: return "SOUND4CNT_L";
		case ADDR_SOUND4CNT_H: return "SOUND4CNT_H";
		case ADDR_SOUNDCNT_L: return "SOUNDCNT_L";
		case ADDR_SOUNDCNT_H: return "SOUNDCNT_H";
		case ADDR_SOUNDCNT_X: return "SOUNDCNT_X";
		case ADDR_SOUNDBIAS: return "SOUNDBIAS";
		case ADDR_FIFO_A: return "FIFO_A";
		case ADDR_FIFO_B: return "FIFO_B";
		/* DMA */
		case ADDR_DMA0SAD: return "DMA0SAD";
		case ADDR_DMA0DAD: return "DMA0DAD";
		case ADDR_DMA0CNT_L: return "DMA0CNT_L";
		case ADDR_DMA0CNT_H: return "DMA0CNT_H";
		case ADDR_DMA1SAD: return "DMA1SAD";
		case ADDR_DMA1DAD: return "DMA1DAD";
		case ADDR_DMA1CNT_L: return "DMA1CNT_L";
		case ADDR_DMA1CNT_H: return "DMA1CNT_H";
		case ADDR_DMA2SAD: return "DMA2SAD";
		case ADDR_DMA2DAD: return "DMA2DAD";
		case ADDR_DMA2CNT_L: return "DMA2CNT_L";
		case ADDR_DMA2CNT_H: return "DMA2CNT_H";
		case ADDR_DMA3SAD: return "DMA3SAD";
		case ADDR_DMA3DAD: return "DMA3DAD";
		case ADDR_DMA3CNT_L: return "DMA3CNT_L";
		case ADDR_DMA3CNT_H: return "DMA3CNT_H";
		/* Timers */
		case ADDR_TM0CNT_L: return "TM0CNT_L";
		case ADDR_TM0CNT_H: return "TM0CNT_H";
		case ADDR_TM1CNT_L: return "TM1CNT_L";
		case ADDR_TM1CNT_H: return "TM1CNT_H";
		case ADDR_TM2CNT_L: return "TM2CNT_L";
		case ADDR_TM2CNT_H: return "TM2CNT_H";
		case ADDR_TM3CNT_L: return "TM3CNT_L";
		case ADDR_TM3CNT_H: return "TM3CNT_H";
		/* Serial #1 */
		case ADDR_SIOMULTI0: return "SIOMULTI0";
		case ADDR_SIOMULTI1: return "SIOMULTI1";
		case ADDR_SIOMULTI2: return "SIOMULTI2";
		case ADDR_SIOMULTI3: return "SIOMULTI3";
		case ADDR_SIOCNT: return "SIOCNT";
		case ADDR_SIOMLT_SEND: return "SIOMLT_SEND";
		/* Keypad */
		case ADDR_KEYINPUT: return "KEYINPUT";
		case ADDR_KEYCNT: return "KEYCNT";
		/* Serial #2 */
		case ADDR_RCNT: return "RCNT";
		case ADDR_IR: return "IR";
		case ADDR_JOYCNT: return "JOYCNT";
		case ADDR_JOY_RECV: return "JOY_RECV";
		case ADDR_JOY_TRANS: return "JOY_TRANS";
		case ADDR_JOYSTAT: return "JOYSTAT";
		/* Interrupt; waitstate; power-down */
		case ADDR_IE: return "IE";
		case ADDR_IF: return "IF";
		case ADDR_WAITCNT: return "WAITCNT";
		case ADDR_IME: return "IME";
		case ADDR_POSTFLG: return "POSTFLG";
		case ADDR_HALTCNT: return "HALTCNT";
		default: return {};
		}
	}


	template<std::integral Int, Scheduler::DriverType driver>
	Int Read(u32 addr)
	{
		static_assert(sizeof(Int) == 1 || sizeof(Int) == 2 || sizeof(Int) == 4);

		bool sequential_access = addr == next_addr_for_sequential_access;
		next_addr_for_sequential_access = addr + sizeof(Int);

		Int val;
		uint cycles;
		if (addr & 0xF000'0000) { /* 1000'0000-FFFF'FFFF   Not used (upper 4bits of address bus unused) */
			val = ReadOpenBus<Int>(addr);
			cycles = 1;
		}
		else {
			switch (addr >> 24 & 0xF) {
			case 0x0: /* 0000'0000-0000'3FFF   BIOS - System ROM */
				val = addr <= 0x3FFF
					? Bios::Read<Int>(addr)
					: ReadOpenBus<Int>(addr);
				cycles = 1;
				break;

			case 0x1: /* not used */
				val = ReadOpenBus<Int>(addr);
				cycles = 1;
				break;

			case 0x2: /* 0200'0000-0203'FFFF   WRAM - On-board Work RAM */
				std::memcpy(&val, board_wram.data() + (addr & 0x3FFFF), sizeof(Int));
				if constexpr (sizeof(Int) == 4) cycles = 6;
				else                            cycles = 3;
				break;

			case 0x3: /* 0300'0000-0300'7FFF   WRAM - On-chip Work RAM */
				std::memcpy(&val, chip_wram.data() + (addr & 0x7FFF), sizeof(Int));
				cycles = 1;
				break;

			case 0x4: /* 0400'0000-0400'03FE   I/O Registers */
				val = ReadIo<Int>(addr);
				cycles = 1;
				break;

			case 0x5: /* 0500'0000-0500'03FF   BG/OBJ Palette RAM */
				val = PPU::ReadPaletteRam<Int>(addr);
				if constexpr (sizeof(Int) == 4) cycles = 2;
				else                            cycles = 1;
				break;

			case 0x6: /* 0600'0000-0601'7FFF   VRAM - Video RAM */
				val = PPU::ReadVram<Int>(addr);
				if constexpr (sizeof(Int) == 4) cycles = 2;
				else                            cycles = 1;
				break;

			case 0x7: /* 0700'0000-0700'03FF   OAM - OBJ Attributes */
				val = PPU::ReadOam<Int>(addr);
				cycles = 1;
				break;

			case 0x8: /* 0800'0000-09FF'FFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 0 */
			case 0x9:
			case 0xA: /* 0A00'0000-0BFF'FFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 1 */
			case 0xB:
			case 0xC: /* 0C00'0000-0DFF'FFFF   Game Pak ROM/FlashROM (max 32MB) - Wait State 2 */
			case 0xD: {
				val = Cartridge::ReadRom<Int>(addr);
				sequential_access &= (addr & 0x1FFFF) != 0; /* The GBA forcefully uses non-sequential timing at the beginning of each 128K-block of gamepak ROM */
				auto wait_state = addr >> 25 & 3;
				cycles = waitcnt.cart_wait[wait_state][sequential_access];
				/* GamePak uses 16bit data bus, so that a 32bit access is split into TWO 16bit accesses (of which, the second fragment is always sequential, even if the first fragment was non-sequential). */
				if constexpr (sizeof(Int) == 4) {
					cycles += waitcnt.cart_wait[wait_state][1];
				}
			} break;

			case 0xE: /* 0E00'0000-0E00'FFFF   Game Pak SRAM    (max 64 KBytes) - 8bit Bus width */
				if constexpr (sizeof(Int) == 1) {
					if (addr <= 0x0E00'FFFF) {
						val = Cartridge::ReadSram(addr);
						cycles = waitcnt.sram_wait;
					}
					else {
						val = ReadOpenBus<Int>(addr);
						cycles = 1;
					}
				}
				else {
					val = ReadOpenBus<Int>(addr); /* TODO: what should happen? */
					cycles = 1;
				}
				break;

			case 0xF:
				val = ReadOpenBus<Int>(addr);
				cycles = 1;
				break;

			default:
				std::unreachable();
			}
		}

		if constexpr (driver == Scheduler::DriverType::Cpu)  CPU::AddCycles(cycles);
		if constexpr (driver == Scheduler::DriverType::Dma0) DMA::AddCycles(cycles, 0);
		if constexpr (driver == Scheduler::DriverType::Dma1) DMA::AddCycles(cycles, 1);
		if constexpr (driver == Scheduler::DriverType::Dma2) DMA::AddCycles(cycles, 2);
		if constexpr (driver == Scheduler::DriverType::Dma3) DMA::AddCycles(cycles, 3);

		return val;
	}


	template<std::integral Int>
	Int ReadIo(u32 addr)
	{
		/* Reads are aligned, and all regions start at word-aligned addresses, so there cannot be a cross-region read. */
		/* TODO: measure if jump table will be faster */
		Int ret = [&] {
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
				return Serial::ReadReg<Int>(addr);
			}
			else if (addr < 0x400'0134) {
				return Keypad::ReadReg<Int>(addr);
			}
			else if (addr < 0x400'0200) {
				return Serial::ReadReg<Int>(addr);
			}
			else {
				auto ReadByte = [](u32 addr) {
					switch (addr) {
					case ADDR_IE:          return IRQ::ReadIE(0);
					case ADDR_IE + 1:      return IRQ::ReadIE(1);
					case ADDR_IF:          return IRQ::ReadIF(0);
					case ADDR_IF + 1:      return IRQ::ReadIF(1);
					case ADDR_IME:         return u8(IRQ::ReadIME());
					case ADDR_IME + 1:     u8(0);
					case ADDR_WAITCNT:     return GetByte(waitcnt.raw, 0);
					case ADDR_WAITCNT + 1: return GetByte(waitcnt.raw, 1);
					default: return ReadOpenBus<u8>(addr);
					}
				};
				auto ReadHalf = [](u32 addr) {
					switch (addr) {
					case ADDR_IE:      return IRQ::ReadIE();
					case ADDR_IF:      return IRQ::ReadIF();
					case ADDR_IME:     return IRQ::ReadIME();
					case ADDR_WAITCNT: return waitcnt.raw;
					default: return ReadOpenBus<u16>(addr);
					}
				};
				if constexpr (sizeof(Int) == 1) {
					return Int(ReadByte(addr));
				}
				if constexpr (sizeof(Int) == 2) {
					return Int(ReadHalf(addr));
				}
				if constexpr (sizeof(Int) == 4) {
					u16 lo = ReadHalf(addr);
					u16 hi = ReadHalf(addr + 2);
					return Int(lo | hi << 16);
				}
			}
		}();

		if constexpr (Debug::log_io_reads) {
			Debug::LogIoAccess<IoOperation::Read>(addr, ret);
		}
		return ret;
	}


	template<std::integral Int>
	Int ReadOpenBus(u32 addr)
	{
		return 0;
	}


	template<std::integral Int, Scheduler::DriverType driver>
	void Write(u32 addr, Int data)
	{
		static_assert(sizeof(Int) == 1 || sizeof(Int) == 2 || sizeof(Int) == 4);

		bool sequential_access = addr == next_addr_for_sequential_access;
		next_addr_for_sequential_access = addr + sizeof(Int);

		uint cycles;
		if (addr & 0xF000'0000) { /* 1000'0000-FFFF'FFFF   Not used (upper 4bits of address bus unused) */
			cycles = 1;
		}
		else {
			switch (addr >> 24 & 0xF) {
			case 0x2: /* 0200'0000-0203'FFFF   WRAM - On-board Work RAM */
				std::memcpy(board_wram.data() + (addr & 0x3FFFF), &data, sizeof(Int));
				if constexpr (sizeof(Int) == 4) cycles = 6;
				else                            cycles = 3;
				break;

			case 0x3: /* 0300'0000-0300'7FFF   WRAM - On-chip Work RAM */
				std::memcpy(chip_wram.data() + (addr & 0x7FFF), &data, sizeof(Int));
				cycles = 1;
				break;

			case 0x4: /* 0400'0000-0400'03FE   I/O Registers */
				WriteIo<Int>(addr, data);
				cycles = 1;
				break;

			case 0x5: /* 0500'0000-0500'03FF   BG/OBJ Palette RAM */
				if constexpr (sizeof(Int) == 1) {
					cycles = 1;
				}
				if constexpr (sizeof(Int) == 2) {
					PPU::WritePaletteRam<Int>(addr, data);
					cycles = 1;
				}
				if constexpr (sizeof(Int) == 4) {
					PPU::WritePaletteRam<Int>(addr, data);
					cycles = 2;
				}
				break;

			case 0x6: /* 0600'0000-0601'7FFF   VRAM - Video RAM */
				if constexpr (sizeof(Int) == 1) {
					cycles = 1;
				}
				if constexpr (sizeof(Int) == 2) {
					PPU::WriteVram<Int>(addr, data);
					cycles = 1;
				}
				if constexpr (sizeof(Int) == 4) {
					PPU::WriteVram<Int>(addr, data);
					cycles = 2;
				}
				break;

			case 0x7: /* 0700'0000-0700'03FF   OAM - OBJ Attributes */
				if (sizeof(Int) != 1) {
					PPU::WriteOam<Int>(addr, data);
				}
				cycles = 1;
				break;

			case 0xE: /* 0E00'0000-0E00'FFFF   Game Pak SRAM    (max 64 KBytes) - 8bit Bus width */
				if constexpr (sizeof(Int) == 1) {
					Cartridge::WriteSram(addr, data);
					cycles = waitcnt.sram_wait;
				}
				else {
					cycles = 1;
				}
				break;

			default:
				cycles = 1;
			}
		}

		if constexpr (driver == Scheduler::DriverType::Cpu)  CPU::AddCycles(cycles);
		if constexpr (driver == Scheduler::DriverType::Dma0) DMA::AddCycles(cycles, 0);
		if constexpr (driver == Scheduler::DriverType::Dma1) DMA::AddCycles(cycles, 1);
		if constexpr (driver == Scheduler::DriverType::Dma2) DMA::AddCycles(cycles, 2);
		if constexpr (driver == Scheduler::DriverType::Dma3) DMA::AddCycles(cycles, 3);
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
			Serial::WriteReg(addr, data);
		}
		else if (addr < 0x400'0134) {
			Keypad::WriteReg(addr, data);
		}
		else if (addr < 0x400'0200) {
			Serial::WriteReg(addr, data);
		}
		else {
			auto WriteByte = [](u32 addr, u8 data) {
				switch (addr) {
				case ADDR_IE:          IRQ::WriteIE(data, 0); break;
				case ADDR_IE + 1:      IRQ::WriteIE(data, 1); break;
				case ADDR_IF:          IRQ::WriteIF(data, 0); break;
				case ADDR_IF + 1:      IRQ::WriteIF(data, 1); break;
				case ADDR_IME:         IRQ::WriteIME(data); break;
				case ADDR_WAITCNT:     WriteWaitcntLo(data); break;
				case ADDR_WAITCNT + 1: WriteWaitcntHi(data); break;
				}
			};
			auto WriteHalf = [](u32 addr, u16 data) {
				switch (addr) {
				case ADDR_IE:      IRQ::WriteIE(data); break;
				case ADDR_IF:      IRQ::WriteIF(data); break;
				case ADDR_IME:     IRQ::WriteIME(data); break;
				case ADDR_WAITCNT: WriteWaitcnt(data); break;
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

		if constexpr (Debug::log_io_writes) {
			Debug::LogIoAccess<IoOperation::Write>(addr, data);
		}
	}


	void WriteWaitcnt(u16 data)
	{
		waitcnt.raw = waitcnt.raw & 0x8000 | data & 0x7FFF;
		waitcnt.sram_wait = sram_wait[data & 3];
		waitcnt.cart_wait[0][0] = cart_wait_1st_access   [data >> 2 & 3];
		waitcnt.cart_wait[0][1] = cart_wait_2nd_access[0][data >> 4 & 1];
		waitcnt.cart_wait[1][0] = cart_wait_1st_access   [data >> 5 & 3];
		waitcnt.cart_wait[1][1] = cart_wait_2nd_access[1][data >> 7 & 1];
		waitcnt.cart_wait[2][0] = cart_wait_1st_access   [data >> 8 & 3];
		waitcnt.cart_wait[2][1] = cart_wait_2nd_access[2][data >> 10 & 1];
		waitcnt.phi_terminal_output = data >> 11 & 3;
		waitcnt.prefetch_buffer_enable = data & 0x4000;
		waitcnt.game_pak_type_flag = data & 0x8000;
	}


	void WriteWaitcntLo(u8 data)
	{
		waitcnt.raw = waitcnt.raw & 0xFF00 | data;
		waitcnt.sram_wait = sram_wait[data & 3];
		waitcnt.cart_wait[0][0] = cart_wait_1st_access   [data >> 2 & 3];
		waitcnt.cart_wait[0][1] = cart_wait_2nd_access[0][data >> 4 & 1];
		waitcnt.cart_wait[1][0] = cart_wait_1st_access   [data >> 5 & 3];
		waitcnt.cart_wait[1][1] = cart_wait_2nd_access[1][data >> 7 & 1];
	}


	void WriteWaitcntHi(u8 data)
	{
		waitcnt.raw = waitcnt.raw & 0x80FF | (data & 0x7F) << 8;
		waitcnt.cart_wait[2][0] = cart_wait_1st_access   [data & 3];
		waitcnt.cart_wait[2][1] = cart_wait_2nd_access[2][data >> 2 & 1];
		waitcnt.phi_terminal_output = data >> 3 & 3;
		waitcnt.prefetch_buffer_enable = data & 0x40;
		waitcnt.game_pak_type_flag = data & 0x80;
	}

#define ENUM_READ_WRITE_TEMPL_SPEC(DRIVER)      \
	template s8 Read<s8, DRIVER>(u32);          \
	template u8 Read<u8, DRIVER>(u32);          \
	template s16 Read<s16, DRIVER>(u32);        \
	template u16 Read<u16, DRIVER>(u32);        \
	template s32 Read<s32, DRIVER>(u32);        \
	template u32 Read<u32, DRIVER>(u32);        \
	template void Write<s8, DRIVER>(u32, s8);   \
	template void Write<u8, DRIVER>(u32, u8);   \
	template void Write<s16, DRIVER>(u32, s16); \
	template void Write<u16, DRIVER>(u32, u16); \
	template void Write<s32, DRIVER>(u32, s32); \
	template void Write<u32, DRIVER>(u32, u32);

	ENUM_READ_WRITE_TEMPL_SPEC(Scheduler::DriverType::Cpu);
	ENUM_READ_WRITE_TEMPL_SPEC(Scheduler::DriverType::Dma0);
	ENUM_READ_WRITE_TEMPL_SPEC(Scheduler::DriverType::Dma1);
	ENUM_READ_WRITE_TEMPL_SPEC(Scheduler::DriverType::Dma2);
	ENUM_READ_WRITE_TEMPL_SPEC(Scheduler::DriverType::Dma3);
}