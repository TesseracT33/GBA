export module Keypad;

import Util;

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
		void StreamState(SerializationStream& stream);

		u16 keyinput;
		u16 keycnt;
	}

	void UpdateIrq();
}