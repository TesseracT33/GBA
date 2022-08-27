export module Keypad;

import Util;

namespace Keypad
{
	export
	{
		enum class Button {
			A, B, Select, Start, Right, Left, Up, Down, R, L
		};

		void NotifyButtonPressed(uint index);
		void NotifyButtonReleased(uint index);

		u16 keyinput;
		u16 keycnt;
	}

	void UpdateIrq();
}