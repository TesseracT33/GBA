export module DMA;

import Util;

import <bit>;
import <concepts>;
import <cstring>;

namespace DMA
{
	export
	{
		void Initialize();
		template<std::integral Int> Int ReadReg(u32 addr);
		template<std::integral Int> void WriteReg(u32 addr, Int data);
	}


	template<uint index>
	struct DmaChannel
	{
		void WriteControlLo(u8 data);
		void WriteControlHi(u8 data);
		void WriteControl(u16 data);
		u32 dst_addr;
		u32 src_addr;
		u16 count;
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
	};

	DmaChannel<0> dma_0;
	DmaChannel<1> dma_1;
	DmaChannel<2> dma_2;
	DmaChannel<3> dma_3;
}