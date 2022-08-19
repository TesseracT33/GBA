export module GBA;

import PPU;

import Util;

import <string>;

export namespace GBA
{
	void ApplyNewSampleRate()
	{

	}


	bool AssociatesWithRomExtension(const std::string& ext)
	{
		return ext.compare("gba") == 0 || ext.compare("GBA") == 0;
	}


	void Detach()
	{

	}


	void DisableAudio()
	{
		// TODO
	}


	void EnableAudio()
	{
		// TODO
	}


	uint GetNumberOfInputs()
	{
		return 10;
	}


	void Initialize()
	{
		//CPU::Initialize();
		//PPU::Initialize();
	}


	bool LoadBios(const std::string& path)
	{
		return true;
	}


	bool LoadRom(const std::string& path)
	{
		return true;
	}


	void NotifyNewAxisValue(uint player_index, uint input_action_index, int axis_value)
	{
		/* no axes */
	}


	void NotifyButtonPressed(uint player_index, uint input_action_index)
	{
		
	}


	void NotifyButtonReleased(uint player_index, uint input_action_index)
	{
		
	}


	void Reset()
	{ // TODO: reset vs initialize
		//CPU::Initialize();
		//PPU::Initialize();
	}


	void Run()
	{
		//CPU::Run();
	}


	void StreamState(SerializationStream& stream)
	{
		
	}
}