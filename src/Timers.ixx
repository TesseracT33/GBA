export module Timers;

import Util;

import <array>;
import <cstring>;

export namespace Timers
{
	void Initialize();

	struct Timer
	{
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