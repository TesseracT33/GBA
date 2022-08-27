module Keypad;

namespace Keypad
{
	void NotifyButtonPressed(uint index)
	{
		keyinput |= 1 << index;
		UpdateIrq();
	}


	void NotifyButtonReleased(uint index)
	{
		keyinput &= ~(1 << index);
		UpdateIrq();
	}


	void UpdateIrq()
	{
		static constexpr u16 irq_enable_mask = 1 << 14;
		static constexpr u16 irq_cond_mask = 1 << 15;
		if (keycnt & irq_enable_mask) {
			if (keycnt & irq_cond_mask) { /* logical AND mode */
				if ((keyinput & keycnt & 0x3FF) == keycnt) {
					/* TODO */
				}
				else {

				}
			}
			else { /* logical OR mode */
				if (keyinput & keycnt & 0x3FF) {

				}
				else {

				}
			}
		}
	}
}