export module Scheduler;

import Util;

import <list>;

namespace Scheduler
{
	export
	{
		using EventCallback = void(*)();

		enum class EventType {
			LcdRender,
			TimerOverflow0,
			TimerOverflow1,
			TimerOverflow2,
			TimerOverflow3
		};

		void AddEvent(EventType event_type, u64 time_until_fire, EventCallback callback);
		void ChangeEventTime(EventType event_type, u64 new_time_to_fire);
		u64 GetGlobalTime();
		void Initialize();
		void RemoveEvent(EventType event_type);
		void Run();
	}

	struct Event
	{
		EventCallback callback;
		u64 time;
		EventType event_type;
	};

	u64 global_time;

	std::list<Event> events;
}