module APU;

import Audio;
import Bus;

namespace APU
{
	void ApplyNewSampleRate()
	{
		sample_rate = Audio::GetSampleRate();
		t_cycle_sample_counter = 0;
	}


	void DisableAPU()
	{
		ResetAllRegisters();
		apu_enabled = false;
		pulse_ch_1.wave_pos = pulse_ch_2.wave_pos = 0;
		wave_ch.sample_buffer = 0;
		frame_seq_step_counter = 0;
	}


	void EnableAPU()
	{
		frame_seq_step_counter = 0;
		apu_enabled = true;
	}


	void Initialize()
	{
		ResetAllRegisters();
		pulse_ch_1.Initialize();
		pulse_ch_2.Initialize();
		wave_ch.Initialize();
		noise_ch.Initialize();
		//wave_ram_accessible_by_cpu_when_ch3_enabled = true;
		frame_seq_step_counter = 0;
		ApplyNewSampleRate();
	}


	void ResetAllRegisters()
	{
		WriteReg(Bus::ADDR_NR10, 0);
		WriteReg(Bus::ADDR_NR11, 0);
		WriteReg(Bus::ADDR_NR12, 0);
		WriteReg(Bus::ADDR_NR13, 0);
		WriteReg(Bus::ADDR_NR14, 0);
		WriteReg(Bus::ADDR_NR21, 0);
		WriteReg(Bus::ADDR_NR22, 0);
		WriteReg(Bus::ADDR_NR23, 0);
		WriteReg(Bus::ADDR_NR24, 0);
		WriteReg(Bus::ADDR_NR30, 0);
		WriteReg(Bus::ADDR_NR31, 0);
		WriteReg(Bus::ADDR_NR32, 0);
		WriteReg(Bus::ADDR_NR33, 0);
		WriteReg(Bus::ADDR_NR34, 0);
		WriteReg(Bus::ADDR_NR41, 0);
		WriteReg(Bus::ADDR_NR42, 0);
		WriteReg(Bus::ADDR_NR43, 0);
		WriteReg(Bus::ADDR_NR44, 0);
		WriteReg(Bus::ADDR_NR50, 0);
		WriteReg(Bus::ADDR_NR51, 0);
		nr52 = 0; /* Do not use WriteReg, as it will make a call to ResetAllRegisters again when 0 is written to nr52. */
	}


	void Sample()
	{
		f32 right_output = 0.0f, left_output = 0.0f;
		if (nr51 & 0x11) {
			f32 pulse_ch_1_output = pulse_ch_1.GetOutput();
			right_output += pulse_ch_1_output * (nr51 & 1);
			left_output += pulse_ch_1_output * (nr51 >> 4 & 1);
		}
		if (nr51 & 0x22) {
			f32 pulse_ch_2_output = pulse_ch_2.GetOutput();
			right_output += pulse_ch_2_output * (nr51 >> 1 & 1);
			left_output += pulse_ch_2_output * (nr51 >> 5 & 1);
		}
		if (nr51 & 0x44) {
			f32 wave_ch_output = wave_ch.GetOutput();
			right_output += wave_ch_output * (nr51 >> 2 & 1);
			left_output += wave_ch_output * (nr51 >> 6 & 1);
		}
		if (nr51 & 0x88) {
			f32 noise_ch_output = noise_ch.GetOutput();
			right_output += noise_ch_output * (nr51 >> 3 & 1);
			left_output += noise_ch_output * (nr51 >> 7 & 1);
		}
		auto right_vol = nr50 & 7;
		auto left_vol = nr50 >> 4 & 7;
		f32 left_sample = left_vol / 28.0f * left_output;
		f32 right_sample = right_vol / 28.0f * right_output;
		Audio::EnqueueSample(left_sample);
		Audio::EnqueueSample(right_sample);
	}


	void StepFrameSequencer()
	{
		// note: this function is called from the Timer module as DIV increases
		if (frame_seq_step_counter % 2 == 0) {
			pulse_ch_1.length_counter.Clock();
			pulse_ch_2.length_counter.Clock();
			wave_ch.length_counter.Clock();
			noise_ch.length_counter.Clock();
			if (frame_seq_step_counter % 4 == 2) {
				pulse_ch_1.sweep.Clock();
			}
		}
		else if (frame_seq_step_counter == 7) {
			pulse_ch_1.envelope.Clock();
			pulse_ch_2.envelope.Clock();
			noise_ch.envelope.Clock();
		}
		frame_seq_step_counter = (frame_seq_step_counter + 1) & 7;
	}


	void StreamState(SerializationStream& stream)
	{
		/* TODO */
	}


	//u8 ReadWaveRamCpu(u16 addr)
	//{
	//	addr &= 0xF;
	//	// If the wave channel is enabled, accessing any byte from $FF30-$FF3F 
	//	// is equivalent to accessing the current byte selected by the waveform position.
	//	// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
	//	if (apu_enabled && wave_ch.enabled) {
	//		if (wave_ram_accessible_by_cpu_when_ch3_enabled) {
	//			return wave_ram[wave_ch.wave_pos / 2]; /* each byte encodes two samples */
	//		}
	//		else {
	//			return 0xFF;
	//		}
	//	}
	//	else {
	//		return wave_ram[addr];
	//	}
	//}


	//void WriteWaveRamCpu(u16 addr, u8 data)
	//{
	//	addr &= 0xF;
	//	if (apu_enabled && wave_ch.enabled) {
	//		if (wave_ram_accessible_by_cpu_when_ch3_enabled) {
	//			wave_ram[wave_ch.wave_pos / 2] = data; /* each byte encodes two samples */
	//		}
	//	}
	//	else {
	//		wave_ram[addr] = data;
	//	}
	//}


	//void Update()
	//{
	//	// Update() is called each m-cycle, but apu is updated each t-cycle
	//	if (apu_enabled) {
	//		for (int i = 0; i < 4; ++i) {
	//			t_cycle_sample_counter += sample_rate;
	//			if (t_cycle_sample_counter >= System::t_cycles_per_sec_base) {
	//				Sample();
	//				t_cycle_sample_counter -= System::t_cycles_per_sec_base;
	//			}
	//			pulse_ch_1.Step();
	//			pulse_ch_1.Step();
	//			wave_ch.Step();
	//			noise_ch.Step();
	//			// note: the frame sequencer is updated from the Timer module
	//		}
	//	}
	//	else {
	//		t_cycle_sample_counter += 4 * sample_rate;
	//		if (t_cycle_sample_counter >= System::t_cycles_per_sec_base) {
	//			Sample();
	//			t_cycle_sample_counter -= System::t_cycles_per_sec_base;
	//		}
	//	}
	//}


	void Channel::Disable()
	{
		nr52 &= ~(1 << id);
		enabled = false;
	}


	void Channel::Enable()
	{
		nr52 |= 1 << id;
		enabled = true;
	}


	void Envelope::Clock()
	{
		if (period != 0) {
			if (timer > 0) {
				timer--;
			}
			if (timer == 0) {
				timer = period > 0 ? period : 8;
				if (ch->volume < 0xF && direction == Direction::Increasing) {
					ch->volume++;
				}
				else if (ch->volume > 0x0 && direction == Direction::Decreasing) {
					ch->volume--;
				}
				else {
					is_updating = false;
				}
			}
		}
	}


	void Envelope::Enable()
	{
		timer = period;
		is_updating = true;
		ch->volume = initial_volume;
	}


	void Envelope::Initialize()
	{
		is_updating = false;
		initial_volume = period = timer = 0;
		direction = Direction::Decreasing;
	}


	void Envelope::SetParams(u8 data)
	{
		// "Zombie" mode
		// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
		Direction new_direction = data & 8 ? Direction::Increasing : Direction::Decreasing;
		if (ch->enabled) {
			if (period == 0 && is_updating || direction == Direction::Decreasing) {
				ch->volume++;
			}
			if (new_direction != direction) {
				ch->volume = 0x10 - ch->volume;
			}
			ch->volume &= 0xF;
		}
		initial_volume = data >> 4;
		direction = new_direction;
		period = data & 7;
	}


	void LengthCounter::Clock()
	{
		if (enabled && value > 0) {
			if (--value == 0) {
				ch->Disable();
			}
		}
	}


	void LengthCounter::Initialize()
	{
		enabled = false;
		value = length = 0;
	}


	f32 NoiseChannel::GetOutput()
	{
		return enabled * dac_enabled * volume * (~lfsr & 1) / 7.5f - 1.0f;
	}


	void NoiseChannel::Initialize()
	{
		dac_enabled = enabled = false;
		volume = 0;
		output = 0.0f;
		lfsr = 0x7FFF;
		envelope.Initialize();
		length_counter.Initialize();
	}


	void NoiseChannel::Step()
	{
		if (timer == 0) {
			static constexpr std::array divisor_table = {
				8, 16, 32, 48, 64, 80, 96, 112
			};
			auto divisor_code = nr43 & 7;
			auto clock_shift = nr43 >> 4;
			timer = divisor_table[divisor_code] << clock_shift;
			bool xor_result = (lfsr & 1) ^ (lfsr >> 1 & 1);
			lfsr = lfsr >> 1 | xor_result << 14;
			if (nr43 & 8) {
				lfsr &= ~(1 << 6);
				lfsr |= xor_result << 6;
			}
		}
		else {
			--timer;
		}
	}


	void NoiseChannel::Trigger()
	{
		if (dac_enabled) {
			Enable();
		}
		envelope.Enable();
		if (length_counter.value == 0) {
			length_counter.value = 64;
			if (length_counter.enabled && frame_seq_step_counter <= 3) {
				--length_counter.value;
			}
		}
		// Reload period. 
		// The low two bits of the frequency timer are NOT modified.
		// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
		timer = timer & 3 | freq & ~3;
		envelope.timer = envelope.period > 0 ? envelope.period : 8;
		// If a channel is triggered when the frame sequencer's next step will clock the volume envelope, 
		// the envelope's timer is reloaded with one greater than it would have been.
		// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
		if (frame_seq_step_counter == 7) {
			envelope.timer++;
		}
		lfsr = 0x7FFF;
	}


	template<bool has_sweep>
	f32 PulseChannel<has_sweep>::GetOutput()
	{
		static constexpr std::array duty_table = {
			0, 0, 0, 0, 0, 0, 0, 1,
			1, 0, 0, 0, 0, 0, 0, 1,
			1, 0, 0, 0, 0, 1, 1, 1,
			0, 1, 1, 1, 1, 1, 1, 0
		};
		return enabled * dac_enabled * volume * duty_table[8 * duty + wave_pos] / 7.5f - 1.0f;
	}


	template<bool has_sweep>
	void PulseChannel<has_sweep>::Initialize()
	{
		dac_enabled = enabled = false;
		volume = duty = wave_pos = 0;
		output = 0.0f;
		envelope.Initialize();
		length_counter.Initialize();
		sweep.Initialize();
	}


	template<bool has_sweep>
	void PulseChannel<has_sweep>::Step()
	{
		if (timer == 0) {
			timer = (2048 - freq) * 4;
			wave_pos = (wave_pos + 1) & 7;
		}
		else {
			--timer;
		}
	}


	template<bool has_sweep>
	void PulseChannel<has_sweep>::Trigger()
	{
		if (dac_enabled) {
			Enable();
		}
		if constexpr (has_sweep) {
			sweep.Enable();
		}
		envelope.Enable();
		// Enabling in first half of length period should clock length
		// dmg_sound 03-trigger test rom
		// TODO 
		if (length_counter.value == 0) {
			length_counter.value = 64;
			if (length_counter.enabled && frame_seq_step_counter <= 3) {
				--length_counter.value;
			}
		}
		// When triggering a square channel, the low two bits of the frequency timer are NOT modified.
		// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
		timer = timer & 3 | freq & ~3;
		envelope.timer = envelope.period > 0 ? envelope.period : 8;
		// If a channel is triggered when the frame sequencer's next step will clock the volume envelope, 
		// the envelope's timer is reloaded with one greater than it would have been.
		// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
		if (frame_seq_step_counter == 7) {
			envelope.timer++;
		}
	}


	void Sweep::Clock()
	{
		if (timer > 0) {
			timer--;
		}
		if (timer == 0) {
			timer = period > 0 ? period : 8;
			if (enabled && period > 0) {
				auto new_freq = ComputeNewFreq();
				if (new_freq < 2048 && shift > 0) {
					// update shadow frequency and CH1 frequency registers with new frequency
					shadow_freq = new_freq;
					nr13 = new_freq & 0xFF;
					nr14 = (new_freq >> 8) & 7;
					ComputeNewFreq();
				}
			}
		}
	}


	uint Sweep::ComputeNewFreq()
	{
		uint new_freq = shadow_freq >> shift;
		if (direction == Direction::Increasing) {
			new_freq = shadow_freq + new_freq;
		}
		else {
			new_freq = shadow_freq - new_freq;
		}
		if (new_freq >= 2048) {
			ch->Disable();
		}
		if (direction == Direction::Decreasing) {
			negate_has_been_used = true;
		}
		return new_freq;
	}


	void Sweep::Enable()
	{
		shadow_freq = ch->freq;
		timer = period > 0 ? period : 8;
		enabled = period != 0 || shift != 0;
		negate_has_been_used = false;
		if (shift > 0) {
			ComputeNewFreq();
		}
	}


	void Sweep::Initialize()
	{
		enabled = negate_has_been_used = false;
		period = shadow_freq = shift = timer = 0;
		direction = Direction::Decreasing;
	}


	f32 WaveChannel::GetOutput()
	{
		if (enabled && dac_enabled) {
			auto sample = sample_buffer;
			if (wave_pos & 1) {
				sample &= 0xF;
			}
			else {
				sample >>= 4;
			}
			static constexpr std::array output_level_shift = { 4, 0, 1, 2 };
			sample >>= output_level_shift[output_level];
			return sample / 7.5f - 1.0f;
		}
		else {
			return 0.0f;
		}
	}


	void WaveChannel::Initialize()
	{
		dac_enabled = enabled = false;
		volume = wave_pos = output_level = sample_buffer = 0;
		output = 0.0f;
		length_counter.Initialize();
	}


	void WaveChannel::Step()
	{
		if (timer == 0) {
			timer = (2048 - freq) * 2;
			wave_pos = (wave_pos + 1) & 0x1F;
			sample_buffer = wave_ram[wave_pos / 2];
			//wave_ram_accessible_by_cpu_when_ch3_enabled = true;
			//t_cycles_since_ch3_read_wave_ram = 0;
		}
		else {
			--timer;
		}
	}


	void WaveChannel::Trigger()
	{
		if (dac_enabled) {
			Enable();
		}
		if (length_counter.value == 0) {
			length_counter.value = 256;
			if (length_counter.enabled && frame_seq_step_counter <= 3) {
				--length_counter.value;
			}
		}
		// Reload period. 
		// The low two bits of the frequency timer are NOT modified.
		// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware#Obscure_Behavior
		timer = timer & 3 | freq & ~3;
		wave_pos = 0;
	}


	template PulseChannel<true>;
	template PulseChannel<false>;
}