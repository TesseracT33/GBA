module Timers;

namespace Timers
{
	void Initialize()
	{
		for (Timer& t : timer) {
			std::memset(&t, 0, sizeof(Timer));
		}
	}
}