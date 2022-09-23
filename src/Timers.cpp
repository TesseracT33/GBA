module Timers;

import Bus;
import Debug;

namespace Timers
{
	void Initialize()
	{
		std::ranges::for_each(timer, [](Timer& t) {
			std::memset(&t, 0, sizeof(Timer));
			t.counter_max_exclusive = 0x10000;
		});
		for (int i = 1; i < 4; ++i) {
			timer[i].prev_timer = &timer[i - 1];
		}
		for (int i = 0; i < 3; ++i) {
			timer[i].next_timer = &timer[i + 1];
		}
		timer[0].prev_timer = nullptr;
		timer[3].next_timer = nullptr;
		timer[0].overflow_callback = OnOverflowWithIrq<0>;
		timer[1].overflow_callback = OnOverflowWithIrq<1>;
		timer[2].overflow_callback = OnOverflowWithIrq<2>;
		timer[3].overflow_callback = OnOverflowWithIrq<3>;
		timer[0].irq_source = IRQ::Source::Timer0;
		timer[1].irq_source = IRQ::Source::Timer1;
		timer[2].irq_source = IRQ::Source::Timer2;
		timer[3].irq_source = IRQ::Source::Timer3;
	}


	template<uint timer_id>
	void OnOverflowWithIrq()
	{
		static_assert(timer_id < 4);
		Timer& t = timer[timer_id];
		IRQ::Raise(t.irq_source);
		Scheduler::AddEvent(t.overflow_event, t.period, OnOverflowWithIrq<timer_id>);
		t.time_until_overflow = t.period;
	}


	template<std::integral Int>
	Int ReadReg(u32 addr)
	{
		auto ReadByte = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_TM0CNT_L:     return u8(timer[0].ReadCounter() & 0xFF);
			case Bus::ADDR_TM0CNT_L + 1: return u8(timer[0].ReadCounter() >> 8 & 0xFF);
			case Bus::ADDR_TM0CNT_H:     return std::bit_cast<u8>(timer[0].control);
			case Bus::ADDR_TM0CNT_H + 1: return u8(0);
			case Bus::ADDR_TM1CNT_L:     return u8(timer[1].ReadCounter() & 0xFF);
			case Bus::ADDR_TM1CNT_L + 1: return u8(timer[1].ReadCounter() >> 8 & 0xFF);
			case Bus::ADDR_TM1CNT_H:     return std::bit_cast<u8>(timer[1].control);
			case Bus::ADDR_TM1CNT_H + 1: return u8(0);
			case Bus::ADDR_TM2CNT_L:     return u8(timer[2].ReadCounter() & 0xFF);
			case Bus::ADDR_TM2CNT_L + 1: return u8(timer[2].ReadCounter() >> 8 & 0xFF);
			case Bus::ADDR_TM2CNT_H:     return std::bit_cast<u8>(timer[2].control);
			case Bus::ADDR_TM2CNT_H + 1: return u8(0);
			case Bus::ADDR_TM3CNT_L:     return u8(timer[3].ReadCounter() & 0xFF);
			case Bus::ADDR_TM3CNT_L + 1: return u8(timer[3].ReadCounter() >> 8 & 0xFF);
			case Bus::ADDR_TM3CNT_H:     return std::bit_cast<u8>(timer[3].control);
			case Bus::ADDR_TM3CNT_H + 1: return u8(0);
			default:                     return Bus::ReadOpenBus<u8>(addr);
			}
		};

		auto ReadHalf = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_TM0CNT_L: return timer[0].ReadCounter();
			case Bus::ADDR_TM0CNT_H: return u16(std::bit_cast<u8>(timer[0].control));
			case Bus::ADDR_TM1CNT_L: return timer[1].ReadCounter();
			case Bus::ADDR_TM1CNT_H: return u16(std::bit_cast<u8>(timer[1].control));
			case Bus::ADDR_TM2CNT_L: return timer[2].ReadCounter();
			case Bus::ADDR_TM2CNT_H: return u16(std::bit_cast<u8>(timer[2].control));
			case Bus::ADDR_TM3CNT_L: return timer[3].ReadCounter();
			case Bus::ADDR_TM3CNT_H: return u16(std::bit_cast<u8>(timer[3].control));
			default:                 return Bus::ReadOpenBus<u16>(addr);
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


	void StreamState(SerializationStream& stream)
	{

	}


	template<std::integral Int>
	void WriteReg(u32 addr, Int data)
	{
		auto WriteByte = [](u32 addr, u8 data) {
			switch (addr) {
			case Bus::ADDR_TM0CNT_L:     timer[0].WriteReload(data, 0); break;
			case Bus::ADDR_TM0CNT_L + 1: timer[0].WriteReload(data, 1); break;
			case Bus::ADDR_TM0CNT_H:     timer[0].WriteControl(data); break;
			case Bus::ADDR_TM1CNT_L:     timer[1].WriteReload(data, 0); break;
			case Bus::ADDR_TM1CNT_L + 1: timer[1].WriteReload(data, 1); break;
			case Bus::ADDR_TM1CNT_H:     timer[1].WriteControl(data); break;
			case Bus::ADDR_TM2CNT_L:     timer[2].WriteReload(data, 0); break;
			case Bus::ADDR_TM2CNT_L + 1: timer[2].WriteReload(data, 1); break;
			case Bus::ADDR_TM2CNT_H:     timer[2].WriteControl(data); break;
			case Bus::ADDR_TM3CNT_L:     timer[3].WriteReload(data, 0); break;
			case Bus::ADDR_TM3CNT_L + 1: timer[3].WriteReload(data, 1); break;
			case Bus::ADDR_TM3CNT_H:     timer[3].WriteControl(data); break;
			}
		};

		auto WriteHalf = [](u32 addr, u16 data) {
			switch (addr) {
			case Bus::ADDR_TM0CNT_L: timer[0].reload = data; break;
			case Bus::ADDR_TM0CNT_H: timer[0].WriteControl(data & 0xFF); break;
			case Bus::ADDR_TM1CNT_L: timer[1].reload = data; break;
			case Bus::ADDR_TM1CNT_H: timer[1].WriteControl(data & 0xFF); break;
			case Bus::ADDR_TM2CNT_L: timer[2].reload = data; break;
			case Bus::ADDR_TM2CNT_H: timer[2].WriteControl(data & 0xFF); break;
			case Bus::ADDR_TM3CNT_L: timer[3].reload = data; break;
			case Bus::ADDR_TM3CNT_H: timer[3].WriteControl(data & 0xFF); break;
			}
		};

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


	void Timer::AddToCounter(u64 increment)
	{
		if (counter + increment < counter_max_exclusive) {
			counter += increment;
			time_until_overflow = counter_max_exclusive - counter;
		}
		else {
			increment -= counter_max_exclusive - counter;
			counter = increment % counter_max_exclusive;
			time_until_overflow = counter_max_exclusive - counter;
			if (next_timer != nullptr && next_timer->control.enable && next_timer->control.count_up) {
				u64 num_overflows = 1 + increment / counter_max_exclusive;
				next_timer->time_last_counter_refresh = this->time_last_counter_refresh;
				next_timer->AddToCounter(num_overflows);
			}
		}
	}


	u16 Timer::GetRealCounter()
	{
		static constexpr std::array prescaler_shift = { 0, 6, 8, 10 };
		return (reload + (counter >> prescaler_shift[control.prescaler])) & 0xFFFF;
	}


	void Timer::OnDisable()
	{
		Timer* t = this;
		do {
			t->is_counting = false;
			if (t->control.irq_enable) {
				Scheduler::RemoveEvent(t->overflow_event);
			}
		} while (t = t->next_timer, t != nullptr && t->control.enable && t->control.count_up);
	}


	void Timer::OnWriteControlTimerEnabled(u16 real_counter)
	{
		const u64 global_time = Scheduler::GetGlobalTime();
		counter_max_exclusive = 0x10000 - reload;
		if (control.count_up && prev_timer != nullptr) { /* TODO: for timer 0, use prescaler even if count_up is set? */
			period = counter_max_exclusive * prev_timer->period;
			counter = real_counter;
			if (prev_timer->is_counting) {
				Timer* t = prev_timer;
				while (t->control.count_up && t->prev_timer != nullptr) {
					t = t->prev_timer;
				}
				u64 time_elapsed = global_time - t->time_last_counter_refresh;
				t->time_last_counter_refresh = global_time;
				t->AddToCounter(time_elapsed);
				t->UpdateTimerChainOnTimerStatusChange<true>();
			}
			else {
				UpdateTimerChainOnTimerStatusChange<false>();
			}
		}
		else {
			counter_max_exclusive *= prescaler_to_period[control.prescaler];
			period = counter_max_exclusive;
			counter = real_counter * prescaler_to_period[control.prescaler] + (global_time & (prescaler_to_period[control.prescaler] - 1));
			time_until_overflow = counter_max_exclusive - counter;
			time_last_counter_refresh = global_time;
			if (control.irq_enable) {
				Scheduler::AddEvent(overflow_event, time_until_overflow, overflow_callback);
			}
			UpdateTimerChainOnTimerStatusChange<true>();
		}
	}


	u16 Timer::ReadCounter()
	{
		if (is_counting) {
			UpdateCounter();
		}
		return GetRealCounter();
	}


	void Timer::UpdateCounter()
	{ /* precondition: is_counting == true */
		if constexpr (Debug::enable_asserts) {
			assert(is_counting);
		}
		Timer* t = this;
		while (t->control.count_up && t->prev_timer != nullptr) {
			t = t->prev_timer;
		}
		u64 global_time = Scheduler::GetGlobalTime();
		u64 time_elapsed = global_time - t->time_last_counter_refresh;
		t->time_last_counter_refresh = global_time;
		t->AddToCounter(time_elapsed);
	}


	void Timer::UpdatePeriod()
	{
		/* TODO: do we need to update period if we're disabled? */
		counter_max_exclusive = 0x10000 - reload;
		if (!control.count_up) { /* TODO: for timer 0, use prescaler even if count_up is set? */
			counter_max_exclusive *= prescaler_to_period[control.prescaler];
			period = counter_max_exclusive;
		}
		else if (prev_timer != nullptr) {
			period = counter_max_exclusive * prev_timer->period;
		}
		/* Note: changing the period won't affect when this timer will overflow, but it can for other ones. */
		Timer* t = this;
		while (t = t->next_timer, t != nullptr && t->control.count_up) {
			t->period = t->counter_max_exclusive * t->prev_timer->period;
			t->time_until_overflow = t->prev_timer->time_until_overflow + (t->counter_max_exclusive - t->counter - 1) * t->prev_timer->period;
			if (t->control.irq_enable) {
				Scheduler::ChangeEventTime(t->overflow_event, t->time_until_overflow);
			}
		}
	}


	template<bool start_timer_is_enabled>
	void Timer::UpdateTimerChainOnTimerStatusChange()
	{
		is_counting = start_timer_is_enabled;
		Timer* t = this;
		while (t = t->next_timer, t != nullptr && t->control.enable && t->control.count_up) {
			bool prev_is_counting = t->is_counting;
			t->is_counting = start_timer_is_enabled;
			if constexpr (start_timer_is_enabled) {
				t->time_last_counter_refresh = this->time_last_counter_refresh;
				t->period = t->prev_timer->period * t->counter_max_exclusive;
				t->time_until_overflow = t->prev_timer->time_until_overflow + (t->counter_max_exclusive - t->counter - 1) * t->prev_timer->period;
			}
			if (t->control.irq_enable) {
				if constexpr (start_timer_is_enabled) {
					if (prev_is_counting) {
						Scheduler::ChangeEventTime(t->overflow_event, t->time_until_overflow);
					}
					else {
						Scheduler::AddEvent(t->overflow_event, t->time_until_overflow, t->overflow_callback);
					}
				}
				else {
					Scheduler::RemoveEvent(t->overflow_event);
				}
			}
		}
	}


	void Timer::WriteControl(u8 data)
	{
		bool prev_enable = control.enable;
		if (control.enable) {
			u16 real_counter = GetRealCounter();
			control = std::bit_cast<Control>(data);
			OnWriteControlTimerEnabled(real_counter);
		}
		else if (prev_enable) {
			control = std::bit_cast<Control>(data);
			OnDisable();
		}
	}


	void Timer::WriteReload(u8 value, u8 byte_index)
	{
		SetByte(reload, byte_index, value);
		UpdatePeriod();
	}


	void Timer::WriteReload(u16 value)
	{
		reload = value;
		UpdatePeriod();
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