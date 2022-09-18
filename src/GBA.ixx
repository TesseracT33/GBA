export module GBA;

import APU;
import Bios;
import Bus;
import Cartridge;
import Core;
import CPU;
import DebugOptions;
import DMA;
import IRQ;
import Keypad;
import Logging;
import PPU;
import Scheduler;
import Timers;
import Util;

import <string_view>;
import <vector>;

export struct GBA : Core
{
	void ApplyNewSampleRate() override
	{

	}

	void Detach() override
	{

	}

	void DisableAudio() override
	{

	}

	void EnableAudio() override
	{

	}

	std::vector<std::string_view> GetActionNames() override
	{
		std::vector<std::string_view> names{};
		names.emplace_back("A");
		names.emplace_back("B");
		names.emplace_back("Select");
		names.emplace_back("Start");
		names.emplace_back("Right");
		names.emplace_back("Left");
		names.emplace_back("Up");
		names.emplace_back("Down");
		names.emplace_back("R");
		names.emplace_back("L");
		return names;
	}

	unsigned GetNumberOfInputs() override
	{
		return 10;
	}

	void Initialize() override
	{
		APU::Initialize();
		Bios::Initialize();
		Bus::Initialize();
		Cartridge::Initialize();
		CPU::Initialize();
		DMA::Initialize();
		IRQ::Initialize();
		Keypad::Initialize();
		PPU::Initialize();
		Scheduler::Initialize();
		Timers::Initialize();
		if constexpr (log_instrs) {
			Logging::Initialize("F:\\gba.log");
		}
	}

	bool LoadBios(const std::string& path) override
	{
		return Bios::Load(path);
	}

	bool LoadRom(const std::string& path) override
	{
		return Cartridge::LoadRom(path);
	}

	void LoadState() override
	{

	}

	void NotifyNewAxisValue(unsigned player_index, unsigned action_index, int new_axis_value) override
	{

	}

	void NotifyButtonPressed(unsigned player_index, unsigned action_index) override
	{

	}

	void NotifyButtonReleased(unsigned player_index, unsigned action_index) override
	{

	}

	void Reset() override
	{

	}

	void Run() override
	{
		Scheduler::Run();
	}

	void SaveState() override
	{

	}
};