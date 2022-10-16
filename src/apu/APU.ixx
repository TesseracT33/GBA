export module APU;

import Util;

import <array>;
import <concepts>;

namespace APU
{
	export
	{
		void Initialize();
		template<std::integral Int> Int ReadReg(u32 addr);
		template<std::integral Int> void WriteReg(u32 addr, Int data); 
	}

	enum class Direction {
		Decreasing, Increasing /* Sweep and envelope */
	};

	struct Channel
	{
	protected:
		explicit Channel(uint id) : id(id) {};

	public:
		void Disable();
		void Enable();

		const uint id;

		bool dac_enabled;
		bool enabled;
		uint freq;
		uint timer;
		uint volume;
		f32 output;
	};

	struct Envelope
	{
		explicit Envelope(Channel* ch) : ch(ch) {}

		void Clock();
		void Enable();
		void Initialize();
		void SetParams(u8 data);

		bool is_updating;
		uint initial_volume;
		uint period;
		uint timer;
		Direction direction;
		Channel* const ch;
	};

	struct LengthCounter
	{
		explicit LengthCounter(Channel* ch) : ch(ch) {}

		void Clock();
		void Initialize();

		bool enabled;
		uint value;
		uint length;
		Channel* const ch;
	};

	struct Sweep
	{
		explicit Sweep(Channel* ch) : ch(ch) {}

		void Clock();
		uint ComputeNewFreq();
		void Enable();
		void Initialize();

		bool enabled;
		bool negate_has_been_used;
		uint period;
		uint shadow_freq;
		uint shift;
		uint timer;
		Direction direction;
		Channel* const ch;
	};

	template<bool has_sweep>
	struct PulseChannel : Channel
	{
		PulseChannel(uint id) : Channel(id) {}

		f32 GetOutput();
		void Initialize();
		void Step();
		void Trigger();

		uint duty;
		uint wave_pos;
		Envelope envelope{ this };
		LengthCounter length_counter{ this };
		Sweep sweep{ this };
	};

	PulseChannel<true> pulse_ch_1{0};
	PulseChannel<false> pulse_ch_2{1};

	struct WaveChannel : Channel
	{
		WaveChannel() : Channel(2) {}

		f32 GetOutput();
		void Initialize();
		void Step();
		void Trigger();

		uint output_level;
		uint wave_pos;
		u8 sample_buffer;
		LengthCounter length_counter{ this };
	} wave_ch;

	struct NoiseChannel : Channel
	{
		NoiseChannel() : Channel(3) {}

		f32 GetOutput();
		void Initialize();
		void Step();
		void Trigger();

		u16 lfsr;
		Envelope envelope{ this };
		LengthCounter length_counter{ this };
	} noise_ch;

	void DisableAPU();
	void EnableAPU();
	void ResetAllRegisters();
	void Sample();

	bool apu_enabled;

	u8 nr10, nr11, nr12, nr13, nr14, nr21, nr22, nr23, nr24,
		nr30, nr31, nr32, nr33, nr34, nr41, nr42, nr43, nr44,
		nr50, nr51, nr52;

	uint frame_seq_step_counter;
	uint sample_rate;
	uint t_cycle_sample_counter;

	std::array<u8, 0x10> wave_ram;
}