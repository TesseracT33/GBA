module Scheduler;

import CPU;
import DebugOptions;
import PPU;

namespace Scheduler
{
	void AddEvent(EventType event_type, u64 time_until_fire, EventCallback callback)
	{
		global_time += CPU::GetElapsedCycles();
		u64 event_absolute_time = global_time + time_until_fire;
		for (auto it = events.begin(); it != events.end(); ++it) {
			if (event_absolute_time < it->time) {
				events.emplace(it, callback, event_absolute_time, event_type);
				if (it == events.begin()) {
					CPU::SuspendRun();
				}
				return;
			}
		}
		events.emplace_back(callback, event_absolute_time, event_type);
	}


	void ChangeEventTime(EventType event_type, u64 new_time_to_fire)
	{
		for (auto it = events.begin(); it != events.end(); ++it) {
			if (it->event_type == event_type) {
				EventCallback callback = it->callback;
				events.erase(it);
				AddEvent(event_type, new_time_to_fire, callback);
				break;
			}
		}
	}


	u64 GetGlobalTime()
	{
		global_time += CPU::GetElapsedCycles();
		return global_time;
	}


	void Initialize()
	{
		global_time = 0;
		events.clear();
		AddEvent(EventType::LcdRender, 1000, PPU::Scanline);
	}


	void RemoveEvent(EventType event_type)
	{
		for (auto it = events.begin(); it != events.end(); ) {
			if (it->event_type == event_type) {
				if (it == events.begin()) {
					CPU::SuspendRun();
				}
				events.erase(it);
				return;
			}
			else {
				++it;
			}
		}
	}


	void Run()
	{
		while (true) {
			auto time_until_next_event = events.front().time;
			CPU::Run(time_until_next_event);
			Event top_event = events.front();
			events.pop_front();
			top_event.callback();
		}
	}
}