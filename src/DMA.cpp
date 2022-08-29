module DMA;

import Bus;

namespace DMA
{
	void Initialize()
	{
		std::memset(&dma_0, 0, sizeof(dma_0));
		std::memset(&dma_1, 0, sizeof(dma_1));
		std::memset(&dma_2, 0, sizeof(dma_2));
		std::memset(&dma_3, 0, sizeof(dma_3));
	}


	template<std::integral Int>
	Int ReadReg(u32 addr)
	{
		auto ReadByte = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_DMA0CNT_L:
			case Bus::ADDR_DMA0CNT_L + 1: return u8(0);
			case Bus::ADDR_DMA0CNT_H:     return GetByte(dma_0.control, 0);
			case Bus::ADDR_DMA0CNT_H + 1: return GetByte(dma_0.control, 1);
			case Bus::ADDR_DMA1CNT_L:
			case Bus::ADDR_DMA1CNT_L + 1: return u8(0);
			case Bus::ADDR_DMA1CNT_H:     return GetByte(dma_1.control, 0);
			case Bus::ADDR_DMA1CNT_H + 1: return GetByte(dma_1.control, 1);
			case Bus::ADDR_DMA2CNT_L:
			case Bus::ADDR_DMA2CNT_L + 1: return u8(0);
			case Bus::ADDR_DMA2CNT_H:     return GetByte(dma_2.control, 0);
			case Bus::ADDR_DMA2CNT_H + 1: return GetByte(dma_2.control, 1);
			case Bus::ADDR_DMA3CNT_L:
			case Bus::ADDR_DMA3CNT_L + 1: return u8(0);
			case Bus::ADDR_DMA3CNT_H:     return GetByte(dma_3.control, 0);
			case Bus::ADDR_DMA3CNT_H + 1: return GetByte(dma_3.control, 1);
			default:                      return Bus::ReadOpenBus<u8>(addr);
			}
		};

		auto ReadHalf = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_DMA0CNT_L: return u16(0);
			case Bus::ADDR_DMA0CNT_H: return std::bit_cast<u16>(dma_0.control);
			case Bus::ADDR_DMA1CNT_L: return u16(0);
			case Bus::ADDR_DMA1CNT_H: return std::bit_cast<u16>(dma_1.control);
			case Bus::ADDR_DMA2CNT_L: return u16(0);
			case Bus::ADDR_DMA2CNT_H: return std::bit_cast<u16>(dma_2.control);
			case Bus::ADDR_DMA3CNT_L: return u16(0);
			case Bus::ADDR_DMA3CNT_H: return std::bit_cast<u16>(dma_3.control);
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


	template<uint id>
	void DmaChannel<id>::WriteControlLo(u8 data)
	{
		SetByte(control, 0, data);
	}


	template<uint id>
	void DmaChannel<id>::WriteControlHi(u8 data)
	{
		SetByte(control, 1, data);
	}


	template<uint id>
	void DmaChannel<id>::WriteControl(u16 data)
	{
		control = std::bit_cast<Control>(data);
	}


	template<std::integral Int>
	void WriteReg(u32 addr, Int data)
	{
		auto WriteByte = [](u32 addr, u8 data) {
			switch (addr) {
			case Bus::ADDR_DMA0SAD:       SetByte(dma_0.src_addr, 0, data); break;
			case Bus::ADDR_DMA0SAD + 1:   SetByte(dma_0.src_addr, 1, data); break;
			case Bus::ADDR_DMA0SAD + 2:   SetByte(dma_0.src_addr, 2, data); break;
			case Bus::ADDR_DMA0SAD + 3:   SetByte(dma_0.src_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA0DAD:       SetByte(dma_0.dst_addr, 0, data); break;
			case Bus::ADDR_DMA0DAD + 1:   SetByte(dma_0.dst_addr, 1, data); break;
			case Bus::ADDR_DMA0DAD + 2:   SetByte(dma_0.dst_addr, 2, data); break;
			case Bus::ADDR_DMA0DAD + 3:   SetByte(dma_0.dst_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA0CNT_L:     SetByte(dma_0.count, 0, data); break;
			case Bus::ADDR_DMA0CNT_L + 1: SetByte(dma_0.count, 1, data & 0x3F); break;
			case Bus::ADDR_DMA0CNT_H:     dma_0.WriteControlLo(data); break;
			case Bus::ADDR_DMA0CNT_H + 1: dma_0.WriteControlHi(data); break;
			case Bus::ADDR_DMA1SAD:       SetByte(dma_1.src_addr, 0, data); break;
			case Bus::ADDR_DMA1SAD + 1:   SetByte(dma_1.src_addr, 1, data); break;
			case Bus::ADDR_DMA1SAD + 2:   SetByte(dma_1.src_addr, 2, data); break;
			case Bus::ADDR_DMA1SAD + 3:   SetByte(dma_1.src_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA1DAD:       SetByte(dma_1.dst_addr, 0, data); break;
			case Bus::ADDR_DMA1DAD + 1:   SetByte(dma_1.dst_addr, 1, data); break;
			case Bus::ADDR_DMA1DAD + 2:   SetByte(dma_1.dst_addr, 2, data); break;
			case Bus::ADDR_DMA1DAD + 3:   SetByte(dma_1.dst_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA1CNT_L:     SetByte(dma_1.count, 0, data); break;
			case Bus::ADDR_DMA1CNT_L + 1: SetByte(dma_1.count, 1, data & 0x3F); break;
			case Bus::ADDR_DMA1CNT_H:     dma_1.WriteControlLo(data); break;
			case Bus::ADDR_DMA1CNT_H + 1: dma_1.WriteControlHi(data); break;
			case Bus::ADDR_DMA2SAD:       SetByte(dma_2.src_addr, 0, data); break;
			case Bus::ADDR_DMA2SAD + 1:   SetByte(dma_2.src_addr, 1, data); break;
			case Bus::ADDR_DMA2SAD + 2:   SetByte(dma_2.src_addr, 2, data); break;
			case Bus::ADDR_DMA2SAD + 3:   SetByte(dma_2.src_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA2DAD:       SetByte(dma_2.dst_addr, 0, data); break;
			case Bus::ADDR_DMA2DAD + 1:   SetByte(dma_2.dst_addr, 1, data); break;
			case Bus::ADDR_DMA2DAD + 2:   SetByte(dma_2.dst_addr, 2, data); break;
			case Bus::ADDR_DMA2DAD + 3:   SetByte(dma_2.dst_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA2CNT_L:     SetByte(dma_2.count, 0, data); break;
			case Bus::ADDR_DMA2CNT_L + 1: SetByte(dma_2.count, 1, data & 0x3F); break;
			case Bus::ADDR_DMA2CNT_H:     dma_2.WriteControlLo(data); break;
			case Bus::ADDR_DMA2CNT_H + 1: dma_2.WriteControlHi(data); break;
			case Bus::ADDR_DMA3SAD:       SetByte(dma_3.src_addr, 0, data); break;
			case Bus::ADDR_DMA3SAD + 1:   SetByte(dma_3.src_addr, 1, data); break;
			case Bus::ADDR_DMA3SAD + 2:   SetByte(dma_3.src_addr, 2, data); break;
			case Bus::ADDR_DMA3SAD + 3:   SetByte(dma_3.src_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA3DAD:       SetByte(dma_3.dst_addr, 0, data); break;
			case Bus::ADDR_DMA3DAD + 1:   SetByte(dma_3.dst_addr, 1, data); break;
			case Bus::ADDR_DMA3DAD + 2:   SetByte(dma_3.dst_addr, 2, data); break;
			case Bus::ADDR_DMA3DAD + 3:   SetByte(dma_3.dst_addr, 3, data & 0xF); break;
			case Bus::ADDR_DMA3CNT_L:     SetByte(dma_3.count, 0, data); break;
			case Bus::ADDR_DMA3CNT_L + 1: SetByte(dma_3.count, 1, data); break;
			case Bus::ADDR_DMA3CNT_H:     dma_3.WriteControlLo(data); break;
			case Bus::ADDR_DMA3CNT_H + 1: dma_3.WriteControlHi(data); break;
			default: break;
			}
		};

		auto WriteHalf = [](u32 addr, u16 data) {
			switch (addr) {
			case Bus::ADDR_DMA0SAD:     dma_0.src_addr = dma_0.src_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA0SAD + 2: dma_0.src_addr = dma_0.src_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA0DAD:     dma_0.dst_addr = dma_0.dst_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA0DAD + 2: dma_0.dst_addr = dma_0.dst_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA0CNT_L:   dma_0.count = data; break;
			case Bus::ADDR_DMA0CNT_H:   dma_0.WriteControl(data); break;
			case Bus::ADDR_DMA1SAD:     dma_1.src_addr = dma_1.src_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA1SAD + 2: dma_1.src_addr = dma_1.src_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA1DAD:     dma_1.dst_addr = dma_1.dst_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA1DAD + 2: dma_1.dst_addr = dma_1.dst_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA1CNT_L:   dma_1.count = data; break;
			case Bus::ADDR_DMA1CNT_H:   dma_1.WriteControl(data); break;
			case Bus::ADDR_DMA2SAD:     dma_2.src_addr = dma_2.src_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA2SAD + 2: dma_2.src_addr = dma_2.src_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA2DAD:     dma_2.dst_addr = dma_2.dst_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA2DAD + 2: dma_2.dst_addr = dma_2.dst_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA2CNT_L:   dma_2.count = data; break;
			case Bus::ADDR_DMA2CNT_H:   dma_2.WriteControl(data); break;
			case Bus::ADDR_DMA3SAD:     dma_3.src_addr = dma_3.src_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA3SAD + 2: dma_3.src_addr = dma_3.src_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA3DAD:     dma_3.dst_addr = dma_3.dst_addr & 0xFFFF'0000 | data; break;
			case Bus::ADDR_DMA3DAD + 2: dma_3.dst_addr = dma_3.dst_addr & 0xFFFF | data << 16; break;
			case Bus::ADDR_DMA3CNT_L:   dma_3.count = data; break;
			case Bus::ADDR_DMA3CNT_H:   dma_3.WriteControl(data); break;
			default: break;
			}
		};

		auto WriteWord = [&](u32 addr, u32 data) {
			switch (addr) {
			case Bus::ADDR_DMA0SAD: dma_0.src_addr = data; break;
			case Bus::ADDR_DMA0DAD: dma_0.dst_addr = data; break;
			case Bus::ADDR_DMA1SAD: dma_1.src_addr = data; break;
			case Bus::ADDR_DMA1DAD: dma_1.dst_addr = data; break;
			case Bus::ADDR_DMA2SAD: dma_2.src_addr = data; break;
			case Bus::ADDR_DMA2DAD: dma_2.dst_addr = data; break;
			case Bus::ADDR_DMA3SAD: dma_3.src_addr = data; break;
			case Bus::ADDR_DMA3DAD: dma_3.dst_addr = data; break;
			default: {
				WriteHalf(addr + 2, data & 0xFFFF);
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