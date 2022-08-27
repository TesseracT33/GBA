export module IRQ;

import Util;

import <utility>;

namespace IRQ
{
	export
	{
		enum class Source : u16 {
			VBlank   = 1 << 0,
			HBlank   = 1 << 1,
			VCounter = 1 << 2,
			Timer0   = 1 << 3,
			Timer1   = 1 << 4,
			Timer2   = 1 << 5,
			Timer3   = 1 << 6,
			Serial   = 1 << 7,
			Dma0     = 1 << 8,
			Dma1     = 1 << 9,
			Dma2     = 1 << 10,
			Dma3     = 1 << 11,
			Keypad   = 1 << 12,
			GamePak  = 1 << 13
		};

		void Initialize();
		void Raise(Source source);
		void StreamState(SerializationStream& stream);

		bool ime;
		bool irq;
		u16 IE;
		u16 IF;
	}
}