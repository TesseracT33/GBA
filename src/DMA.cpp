module DMA;

import Bus;
import CPU;
import Debug;

namespace DMA
{
	void Initialize()
	{
		for (int i = 0; i < 4; ++i) {
			std::memset(&dma_ch[i], 0, sizeof(DmaChannel));
			dma_ch[i].index = i;
		}
		dma_ch[0].driver = Scheduler::Driver::Dma0;
		dma_ch[1].driver = Scheduler::Driver::Dma1;
		dma_ch[2].driver = Scheduler::Driver::Dma2;
		dma_ch[3].driver = Scheduler::Driver::Dma3;
		dma_ch[0].perform_dma_func = PerformDma<0>;
		dma_ch[1].perform_dma_func = PerformDma<1>;
		dma_ch[2].perform_dma_func = PerformDma<2>;
		dma_ch[3].perform_dma_func = PerformDma<3>;
		dma_ch[0].suspend_dma_func = SuspendDma<0>;
		dma_ch[1].suspend_dma_func = SuspendDma<1>;
		dma_ch[2].suspend_dma_func = SuspendDma<2>;
		dma_ch[3].suspend_dma_func = SuspendDma<3>;
		dma_ch[0].irq_source = IRQ::Source::Dma0;
		dma_ch[1].irq_source = IRQ::Source::Dma1;
		dma_ch[2].irq_source = IRQ::Source::Dma2;
		dma_ch[3].irq_source = IRQ::Source::Dma3;
	}


	void OnHBlank()
	{
		static constexpr u16 hblank_start_timing = 2;
		std::ranges::for_each(dma_ch, [](const DmaChannel& dma) {
			if (dma.control.enable && dma.control.start_timing == hblank_start_timing) {
				dma.NotifyDmaActive();
			}
		});
	}


	void OnVBlank()
	{
		static constexpr u16 vblank_start_timing = 1;
		std::ranges::for_each(dma_ch, [](const DmaChannel& dma) {
			if (dma.control.enable && dma.control.start_timing == vblank_start_timing) {
				dma.NotifyDmaActive();
			}
		});
	}


	/* A free function (not part of DmaChannel struct), so that it works correctly with the scheduler
	   (it stores a pointer to this function, and cannot store member function pointers).
	   Templated, because the scheduler expects a function of the form u64 f(u64).
	   TODO: maybe address? Could use std::function and bind 'this', but it has some overhead */
	template<uint dma_index> 
	u64 PerformDma(u64 max_cycles_to_run)
	{
		DmaChannel& dma = dma_ch[dma_index];

		if (dma.suspended) {
			dma.suspended = false;
		}
		else {
			if (dma.next_copy_is_repeat) {
				dma.ReloadCount();
				if (dma.control.dst_addr_ctrl == 3) {
					dma.ReloadDstAddr();
				}
			}
			auto GetAddrIncr = [&](auto addr_control) {
				switch (addr_control) {
				case 0: case 3: /* increment */
					return 2 + 2 * dma.control.transfer_type;
				case 1: /* decrement */
					return -2 - 2 * dma.control.transfer_type;
				case 2: /* fixed */
					return 0;
				default:
					std::unreachable();
				}
			};
			dma.dst_addr_incr = GetAddrIncr(dma.control.dst_addr_ctrl);
			dma.src_addr_incr = GetAddrIncr(dma.control.src_addr_ctrl);
		}

		u64 cycle = 0;
		auto DoDma = [&] <std::integral Int> {
			while (dma.current_count > 0 && cycle < max_cycles_to_run) {
				Bus::Write(dma.current_dst_addr, Bus::Read<Int>(dma.current_src_addr));
				--dma.current_count;
				dma.current_dst_addr += dma.dst_addr_incr;
				dma.current_src_addr += dma.src_addr_incr;
				cycle += 2; /* TODO: count cycles properly */
			}
		};
		if (dma.control.transfer_type == 0) {
			DoDma.template operator() < u16 > ();
		}
		else {
			DoDma.template operator() < u32 > ();
		}
		if (dma.current_count == 0) {
			dma.NotifyDmaInactive();
			if (dma.control.irq_enable) {
				IRQ::Raise(dma.irq_source);
			}
			if (dma.control.repeat) {
				dma.next_copy_is_repeat = true;
				if constexpr (Debug::enable_asserts) {
					assert(dma.control.transfer_type != 0);
				}
			}
			else {
				dma.control.enable = false;
			}
		}
		return cycle;
	}


	template<std::integral Int>
	Int ReadReg(u32 addr)
	{
		auto ReadByte = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_DMA0CNT_L:
			case Bus::ADDR_DMA0CNT_L + 1: return u8(0);
			case Bus::ADDR_DMA0CNT_H:     return GetByte(dma_ch[0].control, 0);
			case Bus::ADDR_DMA0CNT_H + 1: return GetByte(dma_ch[0].control, 1);
			case Bus::ADDR_DMA1CNT_L:
			case Bus::ADDR_DMA1CNT_L + 1: return u8(0);
			case Bus::ADDR_DMA1CNT_H:     return GetByte(dma_ch[1].control, 0);
			case Bus::ADDR_DMA1CNT_H + 1: return GetByte(dma_ch[1].control, 1);
			case Bus::ADDR_DMA2CNT_L:
			case Bus::ADDR_DMA2CNT_L + 1: return u8(0);
			case Bus::ADDR_DMA2CNT_H:     return GetByte(dma_ch[2].control, 0);
			case Bus::ADDR_DMA2CNT_H + 1: return GetByte(dma_ch[2].control, 1);
			case Bus::ADDR_DMA3CNT_L:
			case Bus::ADDR_DMA3CNT_L + 1: return u8(0);
			case Bus::ADDR_DMA3CNT_H:     return GetByte(dma_ch[3].control, 0);
			case Bus::ADDR_DMA3CNT_H + 1: return GetByte(dma_ch[3].control, 1);
			default:                      return Bus::ReadOpenBus<u8>(addr);
			}
		};

		auto ReadHalf = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_DMA0CNT_L: return u16(0);
			case Bus::ADDR_DMA0CNT_H: return std::bit_cast<u16>(dma_ch[0].control);
			case Bus::ADDR_DMA1CNT_L: return u16(0);
			case Bus::ADDR_DMA1CNT_H: return std::bit_cast<u16>(dma_ch[1].control);
			case Bus::ADDR_DMA2CNT_L: return u16(0);
			case Bus::ADDR_DMA2CNT_H: return std::bit_cast<u16>(dma_ch[2].control);
			case Bus::ADDR_DMA3CNT_L: return u16(0);
			case Bus::ADDR_DMA3CNT_H: return std::bit_cast<u16>(dma_ch[3].control);
			default:                  return Bus::ReadOpenBus<u16>(addr);
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

	template<uint dma_index>
	void SuspendDma()
	{
		dma_ch[dma_index].suspended = true;
	}


	template<std::integral Int>
	void WriteReg(u32 addr, Int data)
	{
		auto WriteByte = [](u32 addr, u8 data) {
			switch (addr) {
			case Bus::ADDR_DMA0SAD:       SetByte(dma_ch[0].src_addr, 0, data); break;
			case Bus::ADDR_DMA0SAD + 1:   SetByte(dma_ch[0].src_addr, 1, data); break;
			case Bus::ADDR_DMA0SAD + 2:   SetByte(dma_ch[0].src_addr, 2, data); break;
			case Bus::ADDR_DMA0SAD + 3:   SetByte(dma_ch[0].src_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA0DAD:       SetByte(dma_ch[0].dst_addr, 0, data); break;
			case Bus::ADDR_DMA0DAD + 1:   SetByte(dma_ch[0].dst_addr, 1, data); break;
			case Bus::ADDR_DMA0DAD + 2:   SetByte(dma_ch[0].dst_addr, 2, data); break;
			case Bus::ADDR_DMA0DAD + 3:   SetByte(dma_ch[0].dst_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA0CNT_L:     SetByte(dma_ch[0].count, 0, data); break;
			case Bus::ADDR_DMA0CNT_L + 1: SetByte(dma_ch[0].count, 1, data & 0x3F); break;
			case Bus::ADDR_DMA0CNT_H:     dma_ch[0].WriteControlLo(data); break;
			case Bus::ADDR_DMA0CNT_H + 1: dma_ch[0].WriteControlHi(data); break;
			case Bus::ADDR_DMA1SAD:       SetByte(dma_ch[1].src_addr, 0, data); break;
			case Bus::ADDR_DMA1SAD + 1:   SetByte(dma_ch[1].src_addr, 1, data); break;
			case Bus::ADDR_DMA1SAD + 2:   SetByte(dma_ch[1].src_addr, 2, data); break;
			case Bus::ADDR_DMA1SAD + 3:   SetByte(dma_ch[1].src_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA1DAD:       SetByte(dma_ch[1].dst_addr, 0, data); break;
			case Bus::ADDR_DMA1DAD + 1:   SetByte(dma_ch[1].dst_addr, 1, data); break;
			case Bus::ADDR_DMA1DAD + 2:   SetByte(dma_ch[1].dst_addr, 2, data); break;
			case Bus::ADDR_DMA1DAD + 3:   SetByte(dma_ch[1].dst_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA1CNT_L:     SetByte(dma_ch[1].count, 0, data); break;
			case Bus::ADDR_DMA1CNT_L + 1: SetByte(dma_ch[1].count, 1, data & 0x3F); break;
			case Bus::ADDR_DMA1CNT_H:     dma_ch[1].WriteControlLo(data); break;
			case Bus::ADDR_DMA1CNT_H + 1: dma_ch[1].WriteControlHi(data); break;
			case Bus::ADDR_DMA2SAD:       SetByte(dma_ch[2].src_addr, 0, data); break;
			case Bus::ADDR_DMA2SAD + 1:   SetByte(dma_ch[2].src_addr, 1, data); break;
			case Bus::ADDR_DMA2SAD + 2:   SetByte(dma_ch[2].src_addr, 2, data); break;
			case Bus::ADDR_DMA2SAD + 3:   SetByte(dma_ch[2].src_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA2DAD:       SetByte(dma_ch[2].dst_addr, 0, data); break;
			case Bus::ADDR_DMA2DAD + 1:   SetByte(dma_ch[2].dst_addr, 1, data); break;
			case Bus::ADDR_DMA2DAD + 2:   SetByte(dma_ch[2].dst_addr, 2, data); break;
			case Bus::ADDR_DMA2DAD + 3:   SetByte(dma_ch[2].dst_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA2CNT_L:     SetByte(dma_ch[2].count, 0, data); break;
			case Bus::ADDR_DMA2CNT_L + 1: SetByte(dma_ch[2].count, 1, data & 0x3F); break;
			case Bus::ADDR_DMA2CNT_H:     dma_ch[2].WriteControlLo(data); break;
			case Bus::ADDR_DMA2CNT_H + 1: dma_ch[2].WriteControlHi(data); break;
			case Bus::ADDR_DMA3SAD:       SetByte(dma_ch[3].src_addr, 0, data); break;
			case Bus::ADDR_DMA3SAD + 1:   SetByte(dma_ch[3].src_addr, 1, data); break;
			case Bus::ADDR_DMA3SAD + 2:   SetByte(dma_ch[3].src_addr, 2, data); break;
			case Bus::ADDR_DMA3SAD + 3:   SetByte(dma_ch[3].src_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA3DAD:       SetByte(dma_ch[3].dst_addr, 0, data); break;
			case Bus::ADDR_DMA3DAD + 1:   SetByte(dma_ch[3].dst_addr, 1, data); break;
			case Bus::ADDR_DMA3DAD + 2:   SetByte(dma_ch[3].dst_addr, 2, data); break;
			case Bus::ADDR_DMA3DAD + 3:   SetByte(dma_ch[3].dst_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA3CNT_L:     SetByte(dma_ch[3].count, 0, data); break;
			case Bus::ADDR_DMA3CNT_L + 1: SetByte(dma_ch[3].count, 1, data); break;
			case Bus::ADDR_DMA3CNT_H:     dma_ch[3].WriteControlLo(data); break;
			case Bus::ADDR_DMA3CNT_H + 1: dma_ch[3].WriteControlHi(data); break;
			default: break;
			}
		};

		auto WriteHalf = [](u32 addr, u16 data) {
			switch (addr) {
			case Bus::ADDR_DMA0SAD:     dma_ch[0].src_addr = dma_ch[0].src_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA0SAD + 2: dma_ch[0].src_addr = dma_ch[0].src_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA0DAD:     dma_ch[0].dst_addr = dma_ch[0].dst_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA0DAD + 2: dma_ch[0].dst_addr = dma_ch[0].dst_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA0CNT_L:   dma_ch[0].count = data; break;
			case Bus::ADDR_DMA0CNT_H:   dma_ch[0].WriteControl(data); break;
			case Bus::ADDR_DMA1SAD:     dma_ch[1].src_addr = dma_ch[1].src_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA1SAD + 2: dma_ch[1].src_addr = dma_ch[1].src_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA1DAD:     dma_ch[1].dst_addr = dma_ch[1].dst_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA1DAD + 2: dma_ch[1].dst_addr = dma_ch[1].dst_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA1CNT_L:   dma_ch[1].count = data; break;
			case Bus::ADDR_DMA1CNT_H:   dma_ch[1].WriteControl(data); break;
			case Bus::ADDR_DMA2SAD:     dma_ch[2].src_addr = dma_ch[2].src_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA2SAD + 2: dma_ch[2].src_addr = dma_ch[2].src_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA2DAD:     dma_ch[2].dst_addr = dma_ch[2].dst_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA2DAD + 2: dma_ch[2].dst_addr = dma_ch[2].dst_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA2CNT_L:   dma_ch[2].count = data; break;
			case Bus::ADDR_DMA2CNT_H:   dma_ch[2].WriteControl(data); break;
			case Bus::ADDR_DMA3SAD:     dma_ch[3].src_addr = dma_ch[3].src_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA3SAD + 2: dma_ch[3].src_addr = dma_ch[3].src_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA3DAD:     dma_ch[3].dst_addr = dma_ch[3].dst_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA3DAD + 2: dma_ch[3].dst_addr = dma_ch[3].dst_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA3CNT_L:   dma_ch[3].count = data; break;
			case Bus::ADDR_DMA3CNT_H:   dma_ch[3].WriteControl(data); break;
			default: break;
			}
		};

		auto WriteWord = [&](u32 addr, u32 data) {
			switch (addr) {
			case Bus::ADDR_DMA0SAD: dma_ch[0].src_addr = data; break;
			case Bus::ADDR_DMA0DAD: dma_ch[0].dst_addr = data; break;
			case Bus::ADDR_DMA1SAD: dma_ch[1].src_addr = data; break;
			case Bus::ADDR_DMA1DAD: dma_ch[1].dst_addr = data; break;
			case Bus::ADDR_DMA2SAD: dma_ch[2].src_addr = data; break;
			case Bus::ADDR_DMA2DAD: dma_ch[2].dst_addr = data; break;
			case Bus::ADDR_DMA3SAD: dma_ch[3].src_addr = data; break;
			case Bus::ADDR_DMA3DAD: dma_ch[3].dst_addr = data; break;
			default: {
				WriteHalf(addr, data & 0xFFFF);
				WriteHalf(addr + 2, data >> 16 & 0xFFFF);
				break;
			}
			}
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


	void DmaChannel::OnDmaDisable()
	{
		NotifyDmaInactive();
	}


	void DmaChannel::OnDmaEnable()
	{
		next_copy_is_repeat = suspended = false;
		ReloadCount();
		ReloadDstAddr();
		ReloadSrcAddr();
		if (control.start_timing == 0) {
			NotifyDmaActive();
		}
	}


	void DmaChannel::NotifyDmaActive() const
	{
		Scheduler::EngageDriver(driver, perform_dma_func, suspend_dma_func);
	}


	void DmaChannel::NotifyDmaInactive() const
	{
		Scheduler::DisengageDriver(driver);
	}


	void DmaChannel::ReloadCount()
	{
		if (count == 0) {
			current_count = index == 3 ? 0x10000 : 0x4000;
		}
		else {
			current_count = count;
		}
	}


	void DmaChannel::ReloadDstAddr()
	{
		current_dst_addr = dst_addr;
	}


	void DmaChannel::ReloadSrcAddr()
	{
		current_src_addr = src_addr;
	}


	void DmaChannel::WriteControlLo(u8 data)
	{
		SetByte(control, 0, data);
	}


	void DmaChannel::WriteControlHi(u8 data)
	{
		Control prev_control = control;
		SetByte(control, 1, data);
		if (control.enable ^ prev_control.enable) {
			control.enable ? OnDmaEnable() : OnDmaDisable();
		}
	}


	void DmaChannel::WriteControl(u16 data)
	{
		Control prev_control = control;
		control = std::bit_cast<Control>(data);
		if (control.enable ^ prev_control.enable) {
			control.enable ? OnDmaEnable() : OnDmaDisable();
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