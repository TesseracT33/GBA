module APU;

import Bus;

namespace APU
{
	template<std::integral Int>
	Int ReadReg(u32 addr)
	{
		auto ReadByte = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_NR10: return u8(nr10 | 0x80);
			case Bus::ADDR_NR11: return u8(nr11 | 0x3F);
			case Bus::ADDR_NR12: return nr12;
			case Bus::ADDR_NR13: return nr13;
			case Bus::ADDR_NR14: return u8(nr14 | 0xBF);
			case Bus::ADDR_NR21: return u8(nr21 | 0x3F);
			case Bus::ADDR_NR22: return nr22;
			case Bus::ADDR_NR23: return nr23;
			case Bus::ADDR_NR24: return u8(nr24 | 0xBF);
			case Bus::ADDR_NR30: return u8(nr30 | 0x7F);
			case Bus::ADDR_NR31: return nr31;
			case Bus::ADDR_NR32: return u8(nr32 | 0x9F);
			case Bus::ADDR_NR33: return nr33;
			case Bus::ADDR_NR34: return u8(nr34 | 0xBF);
			case Bus::ADDR_NR41: return nr41;
			case Bus::ADDR_NR42: return nr42;
			case Bus::ADDR_NR43: return nr43;
			case Bus::ADDR_NR44: return u8(nr44 | 0xBF);
			case Bus::ADDR_NR50: return nr50;
			case Bus::ADDR_NR51: return nr51;
			case Bus::ADDR_DMA_SOUND_CTRL: return u8(0); // TODO
			case Bus::ADDR_NR52: return u8(nr52 | 0x70);
			case Bus::ADDR_SOUNDBIAS: return u8(0); // TODO
			case Bus::ADDR_FIFO_A: return u8(0); // TODO
			case Bus::ADDR_FIFO_B: return u8(0); // TODO
				// TODO; wave ram
			default: return Bus::ReadOpenBus<u8>(addr);
			}
		};

		auto ReadHalf = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_NR10: return u16(nr10 | 0x80);
			case Bus::ADDR_NR11: return u16(nr11 | nr12 | 0xFF3F);
			case Bus::ADDR_NR13: return u16(nr13 | nr14 | 0xBFFF);
			case Bus::ADDR_NR21: return u16(nr21 | nr22 | 0xFF3F);
			case Bus::ADDR_NR23: return u16(nr23 | nr24 | 0xBFFF);
			case Bus::ADDR_NR30: return u16(nr30 | 0x7F);
			case Bus::ADDR_NR31: return u16(nr31 | nr32 | 0x9FFF);
			case Bus::ADDR_NR33: return u16(nr33 | nr34 | 0xBFFF);
			case Bus::ADDR_NR41: return u16(nr41 | nr42);
			case Bus::ADDR_NR43: return u16(nr43 | nr44 | 0xBFFF);
			case Bus::ADDR_NR50: return u16(nr50 | nr51);
			case Bus::ADDR_DMA_SOUND_CTRL: return u16(0); // TODO
			case Bus::ADDR_NR52: return u16(nr52 | 0x70);
			case Bus::ADDR_SOUNDBIAS: return u16(0); // TODO
			case Bus::ADDR_FIFO_A: return u16(0); // TODO
			case Bus::ADDR_FIFO_B: return u16(0); // TODO
			default: return Bus::ReadOpenBus<u16>(addr);
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


	template<std::integral Int>
	void WriteReg(u32 addr, Int data)
	{
		auto WriteNR10 = [&](u8 data) {
			nr10 = data;
			pulse_ch_1.sweep.period = nr10 >> 4 & 7;
			pulse_ch_1.sweep.direction = nr10 & 8 ? Direction::Decreasing : Direction::Increasing;
			pulse_ch_1.sweep.shift = nr10 & 7;
			if (pulse_ch_1.sweep.direction == Direction::Increasing && pulse_ch_1.sweep.negate_has_been_used) {
				pulse_ch_1.Disable();
			}
		};

		auto WriteNR11 = [&](u8 data) {
			nr11 = data;
			pulse_ch_1.duty = data >> 6;
			pulse_ch_1.length_counter.length = data & 0x3F;
			pulse_ch_1.length_counter.value = 64 - pulse_ch_1.length_counter.length;
		};

		auto WriteNR12 = [&](u8 data) {
			nr12 = data;
			pulse_ch_1.envelope.SetParams(data);
			pulse_ch_1.dac_enabled = data & 0xF8;
			if (!pulse_ch_1.dac_enabled) {
				pulse_ch_1.Disable();
			}
		};

		auto WriteNR13 = [&](u8 data) {
			nr13 = data;
			pulse_ch_1.freq &= 0x700;
			pulse_ch_1.freq |= data;
		};

		auto WriteNR14 = [&](u8 data) {
			nr14 = data;
			pulse_ch_1.freq &= 0xFF;
			pulse_ch_1.freq |= (data & 7) << 8;
			// Extra length clocking occurs when writing to NRx4 when the frame sequencer's next step 
			// is one that doesn't clock the length counter. In this case, if the length counter was 
			// PREVIOUSLY disabled and now enabled and the length counter is not zero, it is decremented. 
			// If this decrement makes it zero and trigger is clear, the channel is disabled.
			// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
			bool enable_length = data & 0x40;
			bool trigger = data & 0x80;
			if (enable_length && !pulse_ch_1.length_counter.enabled && pulse_ch_1.length_counter.value > 0 && frame_seq_step_counter & 1 && !trigger) {
				if (--pulse_ch_1.length_counter.value == 0) {
					pulse_ch_1.Disable();
				}
			}
			pulse_ch_1.length_counter.enabled = enable_length;
			if (trigger) {
				pulse_ch_1.Trigger();
			}
		};

		auto WriteNR21 = [&](u8 data) {
			nr21 = data;
			pulse_ch_2.duty = data >> 6;
			pulse_ch_2.length_counter.length = data & 0x3F;
			pulse_ch_2.length_counter.value = 64 - pulse_ch_2.length_counter.length;
		};

		auto WriteNR22 = [&](u8 data) {
			nr22 = data;
			pulse_ch_2.envelope.SetParams(data);
			pulse_ch_2.dac_enabled = data & 0xF8;
			if (!pulse_ch_2.dac_enabled) {
				pulse_ch_2.Disable();
			}
		};

		auto WriteNR23 = [&](u8 data) {
			nr23 = data;
			pulse_ch_2.freq &= 0x700;
			pulse_ch_2.freq |= data;
		};

		auto WriteNR24 = [&](u8 data) {
			nr24 = data;
			pulse_ch_2.freq &= 0xFF;
			pulse_ch_2.freq |= (data & 7) << 8;
			bool enable_length = data & 0x40;
			bool trigger = data & 0x80;
			if (enable_length && !pulse_ch_2.length_counter.enabled && pulse_ch_2.length_counter.value > 0 && frame_seq_step_counter & 1 && !trigger) {
				if (--pulse_ch_2.length_counter.value == 0) {
					pulse_ch_2.Disable();
				}
			}
			pulse_ch_2.length_counter.enabled = enable_length;
			if (trigger) {
				pulse_ch_2.Trigger();
			}
		};

		auto WriteNR30 = [&](u8 data) {
			nr30 = data;
			wave_ch.dac_enabled = data & 0x80;
			if (!wave_ch.dac_enabled) {
				wave_ch.Disable();
			}
		};

		auto WriteNR31 = [&](u8 data) {
			nr31 = data;
			wave_ch.length_counter.length = data & 0x3F;
			wave_ch.length_counter.value = 256 - wave_ch.length_counter.length;
		};

		auto WriteNR32 = [&](u8 data) {
			nr32 = data;
			wave_ch.output_level = data >> 5 & 3;
		};

		auto WriteNR33 = [&](u8 data) {
			nr33 = data;
			wave_ch.freq &= 0x700;
			wave_ch.freq |= data;
		};

		auto WriteNR34 = [&](u8 data) {
			nr34 = data;
			wave_ch.freq &= 0xFF;
			wave_ch.freq |= (data & 7) << 8;
			bool enable_length = data & 0x40;
			bool trigger = data & 0x80;
			if (enable_length && !wave_ch.length_counter.enabled && wave_ch.length_counter.value > 0 && frame_seq_step_counter & 1 && !trigger) {
				if (--wave_ch.length_counter.value == 0) {
					wave_ch.Disable();
				}
			}
			wave_ch.length_counter.enabled = enable_length;
			if (trigger) {
				wave_ch.Trigger();
			}
		};

		auto WriteNR41 = [&](u8 data) {
			nr41 = data;
			noise_ch.length_counter.length = data & 0x3F;
			noise_ch.length_counter.value = 64 - noise_ch.length_counter.length;
		};

		auto WriteNR42 = [&](u8 data) {
			nr42 = data;
			noise_ch.envelope.SetParams(data);
			noise_ch.dac_enabled = data & 0xF8;
			if (!noise_ch.dac_enabled) {
				noise_ch.Disable();
			}
		};

		auto WriteNR43 = [&](u8 data) {
			nr43 = data;
		};

		auto WriteNR44 = [&](u8 data) {
			nr44 = data;
			bool enable_length = data & 0x40;
			bool trigger = data & 0x80;
			if (enable_length && !noise_ch.length_counter.enabled && noise_ch.length_counter.value > 0 && frame_seq_step_counter & 1 && !trigger) {
				if (--noise_ch.length_counter.value == 0) {
					noise_ch.Disable();
				}
			}
			noise_ch.length_counter.enabled = enable_length;
			if (trigger) {
				noise_ch.Trigger();
			}
		};

		auto WriteNR50 = [&](u8 data) {
			nr50 = data;
		};

		auto WriteNR51 = [&](u8 data) {
			nr51 = data;
		};

		auto WriteNR52 = [&](u8 data) {
			// If bit 7 is reset, then all of the sound system is immediately shut off, and all audio regs are cleared
			nr52 = data;
			nr52 & 0x80 ? EnableAPU() : DisableAPU();
		};

		auto WriteByte = [&](u32 addr, u8 data) {
			switch (addr) {
			case Bus::ADDR_NR10: WriteNR10(data); break;
			case Bus::ADDR_NR11: WriteNR11(data); break;
			case Bus::ADDR_NR12: WriteNR12(data); break;
			case Bus::ADDR_NR13: WriteNR13(data); break;
			case Bus::ADDR_NR14: WriteNR14(data); break;
			case Bus::ADDR_NR21: WriteNR21(data); break;
			case Bus::ADDR_NR22: WriteNR22(data); break;
			case Bus::ADDR_NR23: WriteNR23(data); break;
			case Bus::ADDR_NR24: WriteNR24(data); break;
			case Bus::ADDR_NR30: WriteNR30(data); break;
			case Bus::ADDR_NR31: WriteNR31(data); break;
			case Bus::ADDR_NR32: WriteNR32(data); break;
			case Bus::ADDR_NR33: WriteNR33(data); break;
			case Bus::ADDR_NR34: WriteNR34(data); break;
			case Bus::ADDR_NR41: WriteNR41(data); break;
			case Bus::ADDR_NR42: WriteNR42(data); break;
			case Bus::ADDR_NR43: WriteNR43(data); break;
			case Bus::ADDR_NR44: WriteNR44(data); break;
			case Bus::ADDR_NR50: WriteNR50(data); break;
			case Bus::ADDR_NR51: WriteNR51(data); break;
			case Bus::ADDR_DMA_SOUND_CTRL: break; // TODO
			case Bus::ADDR_NR52: WriteNR52(data); break;
			case Bus::ADDR_SOUNDBIAS: break; // TODO
			case Bus::ADDR_FIFO_A: break; // TODO
			case Bus::ADDR_FIFO_B: break; // TODO
			case Bus::ADDR_WAVE_RAM:
			case Bus::ADDR_WAVE_RAM + 1:
			case Bus::ADDR_WAVE_RAM + 2:
			case Bus::ADDR_WAVE_RAM + 3:
			case Bus::ADDR_WAVE_RAM + 4:
			case Bus::ADDR_WAVE_RAM + 5:
			case Bus::ADDR_WAVE_RAM + 6:
			case Bus::ADDR_WAVE_RAM + 7:
			case Bus::ADDR_WAVE_RAM + 8:
			case Bus::ADDR_WAVE_RAM + 9:
			case Bus::ADDR_WAVE_RAM + 10:
			case Bus::ADDR_WAVE_RAM + 11:
			case Bus::ADDR_WAVE_RAM + 12:
			case Bus::ADDR_WAVE_RAM + 13:
			case Bus::ADDR_WAVE_RAM + 14:
			case Bus::ADDR_WAVE_RAM + 15:
				break;
			}
		};

		auto WriteHalf = [&](u32 addr, u16 data) {
			switch (addr) {
			case Bus::ADDR_NR10: WriteNR10(data & 0xFF); break;
			case Bus::ADDR_NR11: WriteNR11(data & 0xFF); WriteNR12(data >> 8 & 0xFF); break;
			case Bus::ADDR_NR13: WriteNR13(data & 0xFF); WriteNR14(data >> 8 & 0xFF); break;
			case Bus::ADDR_NR21: WriteNR21(data & 0xFF); WriteNR22(data >> 8 & 0xFF); break;
			case Bus::ADDR_NR23: WriteNR23(data & 0xFF); WriteNR24(data >> 8 & 0xFF); break;
			case Bus::ADDR_NR30: WriteNR30(data & 0xFF); break;
			case Bus::ADDR_NR31: WriteNR31(data & 0xFF); WriteNR32(data >> 8 & 0xFF); break;
			case Bus::ADDR_NR33: WriteNR33(data & 0xFF); WriteNR34(data >> 8 & 0xFF); break;
			case Bus::ADDR_NR41: WriteNR41(data & 0xFF); WriteNR42(data >> 8 & 0xFF); break;
			case Bus::ADDR_NR43: WriteNR43(data & 0xFF); WriteNR44(data >> 8 & 0xFF); break;
			case Bus::ADDR_NR50: WriteNR50(data & 0xFF); WriteNR51(data >> 8 & 0xFF); break;
			case Bus::ADDR_DMA_SOUND_CTRL: break; // TODO
			case Bus::ADDR_NR52: WriteNR52(data & 0xFF); break;
			case Bus::ADDR_SOUNDBIAS: break; // TODO
			case Bus::ADDR_FIFO_A: break; // TODO
			case Bus::ADDR_FIFO_B: break; // TODO
			case Bus::ADDR_WAVE_RAM:
			case Bus::ADDR_WAVE_RAM + 2:
			case Bus::ADDR_WAVE_RAM + 4:
			case Bus::ADDR_WAVE_RAM + 6:
			case Bus::ADDR_WAVE_RAM + 8:
			case Bus::ADDR_WAVE_RAM + 10:
			case Bus::ADDR_WAVE_RAM + 12:
			case Bus::ADDR_WAVE_RAM + 14:
				break;
			}
		};

		if (apu_enabled) {
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
		else {
			// TODO
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