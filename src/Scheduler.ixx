export module Scheduler;

import Util;

import <list>;
import <utility>;

namespace Scheduler
{
	export
	{
		using DriverRunFunc = u64(*)(u64);
		using DriverSuspendFunc = void(*)();
		using EventCallback = void(*)();

		enum class DriverType {
			/* ordered by priority */
			Dma0,
			Dma1,
			Dma2,
			Dma3,
			Cpu
		};

		enum class EventType {
			HBlank,
			HBlankSetFlag,
			IrqChange,
			NewScanline,
			TimerOverflow0,
			TimerOverflow1,
			TimerOverflow2,
			TimerOverflow3
		};

		void AddEvent(EventType type, u64 time_until_fire, EventCallback callback);
		void ChangeEventTime(EventType type, u64 new_time_to_fire);
		void DisengageDriver(DriverType type);
		void EngageDriver(DriverType type, DriverRunFunc run_func, DriverSuspendFunc suspend_func);
		u64 GetGlobalTime();
		void Initialize();
		void RemoveEvent(EventType type);
		void Run();
	}

	uint GetDriverPriority(DriverType type);

	struct alignas(32) Driver
	{
		DriverType type;
		DriverRunFunc run_function;
		DriverSuspendFunc suspend_function;
	};

	struct alignas(32) Event
	{
		EventCallback callback;
		u64 time;
		EventType type;
	};

	u64 global_time;

	std::list<Driver> drivers; /* orderer by priority */
	std::list<Event> events; /* ordered by timestamp */
}