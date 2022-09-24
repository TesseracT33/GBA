export module Bus;

import Scheduler;
import Util;

import <array>;
import <concepts>;
import <cstring>;
import <optional>;
import <string_view>;

namespace Bus
{
	export
	{
		enum IoAddr : u32 {
			/* LCD */
			ADDR_DISPCNT = 0x4000000,
			ADDR_GREEN_SWAP = 0x4000002,
			ADDR_DISPSTAT = 0x4000004,
			ADDR_VCOUNT = 0x4000006,
			ADDR_BG0CNT = 0x4000008,
			ADDR_BG1CNT = 0x400000A,
			ADDR_BG2CNT = 0x400000C,
			ADDR_BG3CNT = 0x400000E,
			ADDR_BG0HOFS = 0x4000010,
			ADDR_BG0VOFS = 0x4000012,
			ADDR_BG1HOFS = 0x4000014,
			ADDR_BG1VOFS = 0x4000016,
			ADDR_BG2HOFS = 0x4000018,
			ADDR_BG2VOFS = 0x400001A,
			ADDR_BG3HOFS = 0x400001C,
			ADDR_BG3VOFS = 0x400001E,
			ADDR_BG2PA = 0x4000020,
			ADDR_BG2PB = 0x4000022,
			ADDR_BG2PC = 0x4000024,
			ADDR_BG2PD = 0x4000026,
			ADDR_BG2X = 0x4000028,
			ADDR_BG2Y = 0x400002C,
			ADDR_BG3PA = 0x4000030,
			ADDR_BG3PB = 0x4000032,
			ADDR_BG3PC = 0x4000034,
			ADDR_BG3PD = 0x4000036,
			ADDR_BG3X = 0x4000038,
			ADDR_BG3Y = 0x400003C,
			ADDR_WIN0H = 0x4000040,
			ADDR_WIN1H = 0x4000042,
			ADDR_WIN0V = 0x4000044,
			ADDR_WIN1V = 0x4000046,
			ADDR_WININ = 0x4000048,
			ADDR_WINOUT = 0x400004A,
			ADDR_MOSAIC = 0x400004C,
			ADDR_BLDCNT = 0x4000050,
			ADDR_BLDALPHA = 0x4000052,
			ADDR_BLDY = 0x4000054,
			/* Sound */
			ADDR_SOUND1CNT_L = 0x4000060,
			ADDR_SOUND1CNT_H = 0x4000062,
			ADDR_SOUND1CNT_X = 0x4000064,
			ADDR_SOUND2CNT_L = 0x4000068,
			ADDR_SOUND2CNT_H = 0x400006C,
			ADDR_SOUND3CNT_L = 0x4000070,
			ADDR_SOUND3CNT_H = 0x4000072,
			ADDR_SOUND3CNT_X = 0x4000074,
			ADDR_SOUND4CNT_L = 0x4000078,
			ADDR_SOUND4CNT_H = 0x400007C,
			ADDR_SOUNDCNT_L = 0x4000080,
			ADDR_SOUNDCNT_H = 0x4000082,
			ADDR_SOUNDCNT_X = 0x4000084,
			ADDR_SOUNDBIAS = 0x4000088,
			ADDR_WAVE_RAM = 0x4000090,
			ADDR_FIFO_A = 0x40000A0,
			ADDR_FIFO_B = 0x40000A4,
			/* DMA */
			ADDR_DMA0SAD = 0x40000B0,
			ADDR_DMA0DAD = 0x40000B4,
			ADDR_DMA0CNT_L = 0x40000B8,
			ADDR_DMA0CNT_H = 0x40000BA,
			ADDR_DMA1SAD = 0x40000BC,
			ADDR_DMA1DAD = 0x40000C0,
			ADDR_DMA1CNT_L = 0x40000C4,
			ADDR_DMA1CNT_H = 0x40000C6,
			ADDR_DMA2SAD = 0x40000C8,
			ADDR_DMA2DAD = 0x40000CC,
			ADDR_DMA2CNT_L = 0x40000D0,
			ADDR_DMA2CNT_H = 0x40000D2,
			ADDR_DMA3SAD = 0x40000D4,
			ADDR_DMA3DAD = 0x40000D8,
			ADDR_DMA3CNT_L = 0x40000DC,
			ADDR_DMA3CNT_H = 0x40000DE,
			/* Timers */
			ADDR_TM0CNT_L = 0x4000100,
			ADDR_TM0CNT_H = 0x4000102,
			ADDR_TM1CNT_L = 0x4000104,
			ADDR_TM1CNT_H = 0x4000106,
			ADDR_TM2CNT_L = 0x4000108,
			ADDR_TM2CNT_H = 0x400010A,
			ADDR_TM3CNT_L = 0x400010C,
			ADDR_TM3CNT_H = 0x400010E,
			/* Serial #1 */
			ADDR_SIOMULTI0 = 0x4000120,
			ADDR_SIOMULTI1 = 0x4000122,
			ADDR_SIOMULTI2 = 0x4000124,
			ADDR_SIOMULTI3 = 0x4000126,
			ADDR_SIOCNT = 0x4000128,
			ADDR_SIOMLT_SEND = 0x400012A,
			/* Keypad */
			ADDR_KEYINPUT = 0x4000130,
			ADDR_KEYCNT = 0x4000132,
			/* Serial #2 */
			ADDR_RCNT = 0x4000134,
			ADDR_IR = 0x4000136,
			ADDR_JOYCNT = 0x4000140,
			ADDR_JOY_RECV = 0x4000150,
			ADDR_JOY_TRANS = 0x4000154,
			ADDR_JOYSTAT = 0x4000158,
			/* Interrupt, waitstate, power-down */
			ADDR_IE = 0x4000200,
			ADDR_IF = 0x4000202,
			ADDR_WAITCNT = 0x4000204,
			ADDR_IME = 0x4000208,
			ADDR_POSTFLG = 0x4000300,
			ADDR_HALTCNT = 0x4000301
		};

		enum class IoOperation {
			Read, Write
		};

		void Initialize();
		constexpr std::optional<std::string_view> IoAddrToStr(u32 addr);
		template<std::integral Int, Scheduler::DriverType driver = Scheduler::DriverType::Cpu> Int Read(u32 addr);
		template<std::integral Int> Int ReadOpenBus(u32 addr);
		template<std::integral Int, Scheduler::DriverType driver = Scheduler::DriverType::Cpu> void Write(u32 addr, Int data);
	}

	template<std::integral Int> Int ReadIo(u32 addr);
	template<std::integral Int> void WriteIo(u32 addr, Int data);
	void WriteWaitcnt(u16 data);
	void WriteWaitcntLo(u8 data);
	void WriteWaitcntHi(u8 data);

	/* First Access (Non-sequential) and Second Access (Sequential) define the waitstates for N and S cycles, the actual access time is 1 clock cycle PLUS the number of waitstates. */
	constexpr std::array cart_wait_1st_access = {5, 4, 3, 9};
	constexpr std::array<std::array<u8, 2>, 3> cart_wait_2nd_access = {{ {3, 2}, {5, 2}, {9, 2} }}; /* wait state * waitcnt setting */
	constexpr std::array sram_wait = {5, 4, 3, 9};

	struct WAITCNT
	{
		bool game_pak_type_flag;
		bool prefetch_buffer_enable;
		u8 cart_wait[3][2]; /* wait state * sequential access */
		u8 phi_terminal_output;
		u8 sram_wait;
		u16 raw;
	} waitcnt;

	u32 next_addr_for_sequential_access;

	std::array<u8, 0x40000> board_wram;
	std::array<u8, 0x8000> chip_wram;
}