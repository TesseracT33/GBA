export module Timers;

import Util;

import <array>;
import <concepts>;
import <cstring>;

namespace Timers
{
	export
	{
		void Initialize();
		template<std::integral Int> Int ReadReg(u32 addr);
		void Step(uint cycles);
		void StreamState(SerializationStream& stream);
		template<std::integral Int> void WriteReg(u32 addr, Int data);
	}

	struct Timer
	{
		void WriteControl(u8 data);
		u16 counter;
		u16 reload;
		struct Control
		{
			u8 prescaler : 2;
			u8 timing : 1;
			u8 : 3;
			u8 irq_enable : 1;
			u8 enable : 1;
		} control;
	};

	std::array<Timer, 4> timer;
}