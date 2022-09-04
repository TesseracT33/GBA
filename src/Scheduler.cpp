module Scheduler;

import CPU;
import DebugOptions;
import DMA;
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
					drivers.front().suspend_function();
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


	void DisengageDriver(Driver driver)
	{
		for (auto it = drivers.begin(); it != drivers.end(); ++it) {
			if (it->driver == driver) {
				drivers.erase(it);
				break;
			}
		}
	}


	void EngageDriver(Driver driver, DriverRunFunc run_func, DriverSuspendFunc suspend_func)
	{
		for (auto it = drivers.begin(); it != drivers.end(); ++it) {
			if (GetDriverPriority(it->driver) > GetDriverPriority(driver)) {
				drivers.emplace(it, driver, run_func, suspend_func);
				if (it == drivers.begin()) {
					(++it)->suspend_function();
				}
				return;
			}
		}
		drivers.emplace_back(driver, run_func, suspend_func);
	}


	uint GetDriverPriority(Driver driver)
	{
		return std::to_underlying(driver);
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
		PPU::AddInitialEvents();
		EngageDriver(Driver::Cpu, CPU::Run, CPU::SuspendRun);
	}


	void RemoveEvent(EventType event_type)
	{
		for (auto it = events.begin(); it != events.end(); ) {
			if (it->event_type == event_type) {
				if (it == events.begin()) {
					drivers.front().suspend_function();
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
			while (global_time < events.front().time) {
				global_time += drivers.front().run_function(events.front().time - global_time);
			}
			Event top_event = events.front();
			events.pop_front();
			global_time = top_event.time; /* just in case we ran for longer than we should have */
			top_event.callback();
		}
	}
}