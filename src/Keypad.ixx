export module Keypad;

import Util;

import <concepts>;

namespace Keypad
{
	export
	{
		enum class Button {
			A, B, Select, Start, Right, Left, Up, Down, R, L
		};

		void Initialize();
		void NotifyButtonPressed(uint index);
		void NotifyButtonReleased(uint index);
		template<std::integral Int> Int ReadReg(u32 addr);
		void StreamState(SerializationStream& stream);
		template<std::integral Int> void WriteReg(u32 addr, Int data);
	}

	void UpdateIrq();

	u16 keyinput;
	u16 keycnt;
}