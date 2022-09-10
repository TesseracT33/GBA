module IRQ;

import CPU;
import Scheduler;

namespace IRQ
{
	void CheckIrq()
	{
		irq = IE & IF & 0x3FF;
		if (ime) {
			static constexpr int irq_set_cycle_delay = 3;
			Scheduler::AddEvent(Scheduler::EventType::IrqChange, irq_set_cycle_delay, [] { CPU::SetIRQ(irq); });
		}
	}


	void Initialize()
	{
		ime = irq = false;
		IE = IF = 0;
		CPU::SetIRQ(false);
	}


	void Raise(Source source)
	{
		IF |= std::to_underlying(source);
		CheckIrq();
	}


	u8 ReadIE(u8 byte_index)
	{
		return GetByte(IE, byte_index);
	}


	u16 ReadIE()
	{
		return IE;
	}


	u8 ReadIF(u8 byte_index)
	{
		return GetByte(IF, byte_index);
	}


	u16 ReadIF()
	{
		return IF;
	}


	u16 ReadIME()
	{
		return ime;
	}


	void StreamState(SerializationStream& stream)
	{

	}


	void WriteIE(u8 data, u8 byte_index)
	{
		SetByte(IE, byte_index, data);
		CheckIrq();
	}


	void WriteIE(u16 data)
	{
		IE = data;
		CheckIrq();
	}


	void WriteIF(u8 data, u8 byte_index)
	{
		SetByte(IF, byte_index, IF & ~data & 0xFF);
		CheckIrq();
	}


	void WriteIF(u16 data)
	{
		/* Interrupts must be manually acknowledged by writing a "1" to one of the IRQ bits, the IRQ bit will then be cleared. */
		IF &= ~data; /* todo: handle bits 14-15 better? */
		CheckIrq();
	}


	void WriteIME(u16 data)
	{
		bool prev_ime = ime;
		ime = data & 1;
		if (ime ^ prev_ime) {
			CPU::SetIRQ(ime ? irq : false);
		}
	}
}