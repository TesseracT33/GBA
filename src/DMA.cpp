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


	template<std::integral Int>
	void WriteReg(u32 addr, Int data)
	{
		auto WriteByte = [](u32 addr, u8 data) {
			switch (addr) {
			case Bus::ADDR_DMA0SAD:       break;
			case Bus::ADDR_DMA0SAD + 1:   break;
			case Bus::ADDR_DMA0SAD + 2:   break;
			case Bus::ADDR_DMA0SAD + 3:   break;
			case Bus::ADDR_DMA0DAD:       break;
			case Bus::ADDR_DMA0DAD + 1:   break;
			case Bus::ADDR_DMA0DAD + 2:   break;
			case Bus::ADDR_DMA0DAD + 3:   break;
			case Bus::ADDR_DMA0CNT_L:     break;
			case Bus::ADDR_DMA0CNT_L + 1: break;
			case Bus::ADDR_DMA0CNT_H:     break;
			case Bus::ADDR_DMA0CNT_H + 1: break;
			case Bus::ADDR_DMA1SAD:       break;
			case Bus::ADDR_DMA1SAD + 1:   break;
			case Bus::ADDR_DMA1SAD + 2:   break;
			case Bus::ADDR_DMA1SAD + 3:   break;
			case Bus::ADDR_DMA1DAD:       break;
			case Bus::ADDR_DMA1DAD + 1:   break;
			case Bus::ADDR_DMA1DAD + 2:   break;
			case Bus::ADDR_DMA1DAD + 3:   break;
			case Bus::ADDR_DMA1CNT_L:     break;
			case Bus::ADDR_DMA1CNT_L + 1: break;
			case Bus::ADDR_DMA1CNT_H:     break;
			case Bus::ADDR_DMA1CNT_H + 1: break;
			case Bus::ADDR_DMA2SAD:       break;
			case Bus::ADDR_DMA2SAD + 1:   break;
			case Bus::ADDR_DMA2SAD + 2:   break;
			case Bus::ADDR_DMA2SAD + 3:   break;
			case Bus::ADDR_DMA2DAD:       break;
			case Bus::ADDR_DMA2DAD + 1:   break;
			case Bus::ADDR_DMA2DAD + 2:   break;
			case Bus::ADDR_DMA2DAD + 3:   break;
			case Bus::ADDR_DMA2CNT_L:     break;
			case Bus::ADDR_DMA2CNT_L + 1: break;
			case Bus::ADDR_DMA2CNT_H:     break;
			case Bus::ADDR_DMA2CNT_H + 1: break;
			case Bus::ADDR_DMA3SAD:       break;
			case Bus::ADDR_DMA3SAD + 1:   break;
			case Bus::ADDR_DMA3SAD + 2:   break;
			case Bus::ADDR_DMA3SAD + 3:   break;
			case Bus::ADDR_DMA3DAD:       break;
			case Bus::ADDR_DMA3DAD + 1:   break;
			case Bus::ADDR_DMA3DAD + 2:   break;
			case Bus::ADDR_DMA3DAD + 3:   break;
			case Bus::ADDR_DMA3CNT_L:     break;
			case Bus::ADDR_DMA3CNT_L + 1: break;
			case Bus::ADDR_DMA3CNT_H:     break;
			case Bus::ADDR_DMA3CNT_H + 1: break;
			default: break;
			}
		};

		auto WriteHalf = [](u32 addr, u16 data) {
			switch (addr) {

			case Bus::ADDR_DMA0SAD:     break;
			case Bus::ADDR_DMA0SAD + 2: break;
			case Bus::ADDR_DMA0DAD:     break;
			case Bus::ADDR_DMA0DAD + 2: break;
			case Bus::ADDR_DMA0CNT_L:   break;
			case Bus::ADDR_DMA0CNT_H:   break;
			case Bus::ADDR_DMA1SAD:     break;
			case Bus::ADDR_DMA1SAD + 2: break;
			case Bus::ADDR_DMA1DAD:     break;
			case Bus::ADDR_DMA1DAD + 2: break;
			case Bus::ADDR_DMA1CNT_L:   break;
			case Bus::ADDR_DMA1CNT_H:   break;
			case Bus::ADDR_DMA2SAD:     break;
			case Bus::ADDR_DMA2SAD + 2: break;
			case Bus::ADDR_DMA2DAD:     break;
			case Bus::ADDR_DMA2DAD + 2: break;
			case Bus::ADDR_DMA2CNT_L:   break;
			case Bus::ADDR_DMA2CNT_H:   break;
			case Bus::ADDR_DMA3SAD:     break;
			case Bus::ADDR_DMA3SAD + 2: break;
			case Bus::ADDR_DMA3DAD:     break;
			case Bus::ADDR_DMA3DAD + 2: break;
			case Bus::ADDR_DMA3CNT_L:   break;
			case Bus::ADDR_DMA3CNT_H:   break;
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