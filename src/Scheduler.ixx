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

		enum class Driver {
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
			TimerOverflow3,
		};

		void AddEvent(EventType event_type, u64 time_until_fire, EventCallback callback);
		void ChangeEventTime(EventType event_type, u64 new_time_to_fire);
		void DisengageDriver(Driver driver);
		void EngageDriver(Driver driver, DriverRunFunc run_func, DriverSuspendFunc suspend_func);
		u64 GetGlobalTime();
		void Initialize();
		void RemoveEvent(EventType event_type);
		void Run();
	}

	uint GetDriverPriority(Driver driver);

	struct Driving
	{
		Driver driver;
		DriverRunFunc run_function;
		DriverSuspendFunc suspend_function;
	};

	struct Event
	{
		EventCallback callback;
		u64 time;
		EventType event_type;
	};

	u64 global_time;

	/* TODO: write custom allocators */
	std::list<Driving> drivers; /* orderer by priority */
	std::list<Event> events; /* ordered by timestamp */
}