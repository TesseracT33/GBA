export module Timers;

import IRQ;
import Scheduler;
import Util;

import <algorithm>;
import <array>;
import <concepts>;
import <cassert>;
import <cstring>;

namespace Timers
{
	export
	{
		void Initialize();
		template<std::integral Int> Int ReadReg(u32 addr);
		void StreamState(SerializationStream& stream);
		template<std::integral Int> void WriteReg(u32 addr, Int data);
	}

	template<uint id> void OnOverflowWithIrq();

	constexpr std::array prescaler_to_period = { 1, 64, 256, 1024 };

	struct Timer
	{
		struct Control;
		void AddToCounter(u64 value);
		u16 GetRealCounter();
		void OnDisable();
		void OnWriteControlTimerEnabled(Control new_control);
		u16 ReadCounter();
		void ReloadCounter();
		void UpdateCounter();
		void UpdatePeriod();
		template<bool start_timer_is_enabled> void UpdateTimerChainOnTimerStatusChange();
		void WriteControl(u8 data);
		void WriteReload(u8 value, u8 byte_index);
		void WriteReload(u16 value);
		struct Control {
			u8 prescaler : 2;
			u8 count_up : 1;
			u8 : 3;
			u8 irq_enable : 1;
			u8 enable : 1;
		} control;
		bool is_counting; /* Timer 0: == control.enable; Others: == control.enable && (!control.count_up || prev_timer->is_counting) */
		u16 reload;
		u16 reload_on_last_overflow;
		u64 counter; /* Let the range be [0, (0x10000 - reload) * prescaler_period - 1] instead of [reload, 0xFFFF] for easier computations */
		u64 counter_max_exclusive;
		u64 period;
		u64 time_last_counter_refresh;
		u64 time_until_overflow;
		IRQ::Source irq_source;
		Scheduler::EventCallback overflow_callback;
		Scheduler::EventType overflow_event;
		Timer* prev_timer;
		Timer* next_timer;
	};

	std::array<Timer, 4> timer;
}