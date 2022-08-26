module CPU;

import Bus;
import PPU;

#define sp (r[13])
#define lr (r[14])
#define pc (r[15])

namespace CPU
{
	bool CheckCondition(u32 cond)
	{
		switch (cond & 0xF) {
		case  0: return cpsr.zero;
		case  1: return !cpsr.zero;
		case  2: return cpsr.carry;
		case  3: return !cpsr.carry;
		case  4: return cpsr.negative;
		case  5: return !cpsr.negative;
		case  6: return cpsr.overflow;
		case  7: return !cpsr.overflow;
		case  8: return cpsr.carry && !cpsr.zero;
		case  9: return !cpsr.carry || cpsr.zero;
		case 10: return cpsr.negative == cpsr.overflow;
		case 11: return cpsr.negative != cpsr.overflow;
		case 12: return !cpsr.zero && cpsr.negative == cpsr.overflow;
		case 13: return cpsr.zero || cpsr.negative != cpsr.overflow;
		case 14: return true;
		case 15: return false;
		default: std::unreachable();
		}
	}


	void DecodeExecute(u32 opcode)
	{
		if (execution_state == ExecutionState::ARM) {
			u32 cond = opcode >> 28;
			bool execute_instruction = CheckCondition(cond);
			if (execute_instruction) {
				DecodeExecuteARM(opcode);
			}
		}
		else {
			/* In THUMB mode, {cond} can be used only for branch opcodes. */
			DecodeExecuteTHUMB(opcode);
		}
	}


	u32 Fetch()
	{
		if (execution_state == ExecutionState::ARM) {
			u32 opcode = Bus::Read<u32>(pc);
			pc += 4;
			return opcode;
		}
		else {
			u16 opcode = Bus::Read<u16>(pc);
			pc += 2;
			return opcode;
		}
	}


	void FlushPipeline()
	{
		pipeline.index = pipeline.step = 0;
	}


	void Initialize()
	{
		r8_r12_non_fiq.fill(0);
		r8_r12_fiq.fill(0);
		r.fill(0);
		r13_usr = r14_usr = 0;
		r13_fiq = r14_fiq = 0;
		r13_svc = r14_svc = 0;
		r13_abt = r14_abt = 0;
		r13_irq = r14_irq = 0;
		r13_und = r14_und = 0;
		spsr = spsr_fiq = spsr_svc = spsr_abt = spsr_irq = spsr_und = 0;
		cpsr = std::bit_cast<CPSR>(cpsr_mode_bits_system);
		execution_state = ExecutionState::ARM;
	}


	void Run()
	{
		for (uint cycle = 0; cycle < 1000 /* TODO */; ++cycle) {
			StepPipeline();
			//PPU::Step();
		}
	}


	void SetExecutionState(ExecutionState state)
	{
		if (execution_state != state) {
			execution_state = state;
			cpsr.state = std::to_underlying(state);
			FlushPipeline();
		}
	}


	void SetIrqHigh()
	{

	}


	void SetIrqLow()
	{

	}


	template<Mode mode>
	void SetMode()
	{
		/* Store banked registers */
		switch (cpsr.mode) {
		case cpsr_mode_bits_system:
		case cpsr_mode_bits_user:
			std::copy(r.begin() + 8, r.end() - 3, r8_r12_non_fiq.begin());
			r13_usr = r[13];
			r14_usr = r[14];
			break;

		case cpsr_mode_bits_irq:
			std::copy(r.begin() + 8, r.end() - 3, r8_r12_non_fiq.begin());
			r13_irq = r[13];
			r14_irq = r[14];
			spsr_irq = spsr;
			break;

		case cpsr_mode_bits_supervisor:
			std::copy(r.begin() + 8, r.end() - 3, r8_r12_non_fiq.begin());
			r13_svc = r[13];
			r14_svc = r[14];
			spsr_svc = spsr;
			break;

		case cpsr_mode_bits_abort:
			std::copy(r.begin() + 8, r.end() - 3, r8_r12_non_fiq.begin());
			r13_abt = r[13];
			r14_abt = r[14];
			spsr_abt = spsr;
			break;

		case cpsr_mode_bits_undefined:
			std::copy(r.begin() + 8, r.end() - 3, r8_r12_non_fiq.begin());
			r13_und = r[13];
			r14_und = r[14];
			spsr_und = spsr;
			break;

		case cpsr_mode_bits_fiq:
			std::copy(r.begin() + 8, r.end() - 3, r8_r12_fiq.begin());
			r13_fiq = r[13];
			r14_fiq = r[14];
			spsr_fiq = spsr;
			break;

		default:
			assert(false);
			break;
		}

		/* Load banked registers */
		if constexpr (mode == Mode::System) {
			cpsr.mode = cpsr_mode_bits_system;
			std::copy(r8_r12_non_fiq.begin(), r8_r12_non_fiq.end(), r.begin() + 8);
			r[13] = r13_usr;
			r[14] = r14_usr;
		}
		if constexpr (mode == Mode::User) {
			cpsr.mode = cpsr_mode_bits_user;
			std::copy(r8_r12_non_fiq.begin(), r8_r12_non_fiq.end(), r.begin() + 8);
			r[13] = r13_usr;
			r[14] = r14_usr;
		}
		if constexpr (mode == Mode::Irq) {
			cpsr.mode = cpsr_mode_bits_irq;
			spsr = spsr_irq;
			std::copy(r8_r12_non_fiq.begin(), r8_r12_non_fiq.end(), r.begin() + 8);
			r[13] = r13_irq;
			r[14] = r14_irq;
		}
		if constexpr (mode == Mode::Supervisor) {
			cpsr.mode = cpsr_mode_bits_supervisor;
			spsr = spsr_svc;
			std::copy(r8_r12_non_fiq.begin(), r8_r12_non_fiq.end(), r.begin() + 8);
			r[13] = r13_svc;
			r[14] = r14_svc;
		}
		if constexpr (mode == Mode::Abort) {
			cpsr.mode = cpsr_mode_bits_abort;
			spsr = spsr_abt;
			std::copy(r8_r12_non_fiq.begin(), r8_r12_non_fiq.end(), r.begin() + 8);
			r[13] = r13_abt;
			r[14] = r14_abt;
		}
		if constexpr (mode == Mode::Undefined) {
			cpsr.mode = cpsr_mode_bits_undefined;
			spsr = spsr_und;
			std::copy(r8_r12_non_fiq.begin(), r8_r12_non_fiq.end(), r.begin() + 8);
			r[13] = r13_und;
			r[14] = r14_und;
		}
		if constexpr (mode == Mode::Fiq) {
			cpsr.mode = cpsr_mode_bits_fiq;
			spsr = spsr_fiq;
			std::copy(r8_r12_fiq.begin(), r8_r12_fiq.end(), r.begin() + 8);
			r[13] = r13_fiq;
			r[14] = r14_fiq;
		}
	}


	void StallPipeline(uint cycles)
	{
		for (uint i = 0; i < cycles; ++i) {
			PPU::Step();
		}
	}


	void StepPipeline()
	{
		if (pipeline.step >= 2) {
			auto opcode = pipeline.opcode[pipeline.index];
			DecodeExecute(opcode);
			pipeline.opcode[pipeline.index] = Fetch();
			pipeline.index ^= 1;
		}
		else {
			pipeline.opcode[pipeline.index] = Fetch();
			pipeline.index ^= 1;
			pipeline.step++;
		}
	}


	void StreamState(SerializationStream& stream)
	{

	}


	template void SetMode<Mode::User>();
	template void SetMode<Mode::Fiq>();
	template void SetMode<Mode::Irq>();
	template void SetMode<Mode::Supervisor>();
	template void SetMode<Mode::Abort>();
	template void SetMode<Mode::Undefined>();
	template void SetMode<Mode::System>();
}