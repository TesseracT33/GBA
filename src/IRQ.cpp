module IRQ;

import CPU;

namespace IRQ
{
	void Initialize()
	{
		ime = irq = false;
		IE = IF = 0;
	}


	void Raise(Source source)
	{
		IF |= std::to_underlying(source);
		irq = IE & IF & 0x3FF;
		if (ime) {
			CPU::SetIRQ(irq);
		}
	}


	void StreamState(SerializationStream& stream)
	{

	}
}