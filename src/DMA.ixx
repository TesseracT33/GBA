export module DMA;

import IRQ;
import Scheduler;
import Util;

import <algorithm>;
import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;
import <utility>;

namespace DMA
{
	export
	{
		void Initialize();
		void OnHBlank();
		void OnVBlank();
		template<std::integral Int> Int ReadReg(u32 addr);
		template<std::integral Int> void WriteReg(u32 addr, Int data);
	}

	struct DmaChannel
	{
		void OnDmaDisable();
		void OnDmaEnable();
		void NotifyDmaActive() const;
		void NotifyDmaInactive() const;
		void ReloadCount();
		void ReloadDstAddr();
		void ReloadSrcAddr();
		void WriteControlLo(u8 data);
		void WriteControlHi(u8 data);
		void WriteControl(u16 data);
		bool next_copy_is_repeat;
		bool suspended;
		uint index;
		u32 current_count;
		u32 current_dst_addr;
		u32 current_src_addr;
		u32 count;
		u32 dst_addr;
		u32 dst_addr_incr;
		u32 src_addr;
		u32 src_addr_incr;
		struct Control
		{
			u16 : 5;
			u16 dst_addr_ctrl : 2;
			u16 src_addr_ctrl : 2;
			u16 repeat : 1;
			u16 transfer_type : 1;
			u16 game_pak_drq : 1;
			u16 start_timing : 2;
			u16 irq_enable : 1;
			u16 enable : 1;
		} control;
		IRQ::Source irq_source;
		Scheduler::Driver driver;
		Scheduler::DriverRunFunc perform_dma_func;
	};

	template<uint dma_index>
	u64 PerformDma(u64 max_cycles_to_run);

	std::array<DmaChannel, 4> dma_ch;
}