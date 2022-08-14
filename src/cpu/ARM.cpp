module CPU;

import Bus;

import Util.Bit;

#define sp (r[13])
#define lr (r[14])
#define pc (r[15])

namespace CPU
{
	void BlockDataTransfer(u32 opcode)
	{
		auto rn = opcode >> 16 & 0xF;
		bool l = opcode >> 20 & 1;
		bool w = opcode >> 21 & 1;
		bool s = opcode >> 22 & 1;
		bool u = opcode >> 23 & 1;
		bool p = opcode >> 24 & 1;
	}


	void Branch(u32 opcode)
	{
		s32 offset = SignExtend<s32, 24>(opcode & 0xFF'FFFF);
		bool link = opcode & 1 << 24;
		if (link) {
			lr = pc;
		}
		pc += offset;
		pc &= ~3;
		FlushPipeline();
	}


	void BranchAndExchange(u32 opcode)
	{
		auto rn = opcode & 0xF;
		pc = r[rn] & ~3;
		SetExecutionState(static_cast<ExecutionState>(rn & 1));
		FlushPipeline();
	}


	template<DataProcessingInstruction instr>
	void DataProcessing(u32 opcode)
	{
		using enum DataProcessingInstruction;

		static constexpr bool is_arithmetic_instr = instr == ADC || instr == ADD || instr == CMN ||
			instr == CMP || instr == RSB || instr == RSC || instr == SBC || instr == SUB;

		auto rd = opcode >> 12 & 0xF;
		auto rn = opcode >> 16 & 0xF;
		bool set_conds = opcode >> 20 & 1;
		bool immediate_operand = opcode >> 21 & 1;

		u32 oper1 = r[rn];
		u32 oper2 = [&] {
			if (immediate_operand) {
				u32 immediate = opcode & 0xFF;
				auto shift_amount = opcode >> 7 & 0x1E; /* == 2 * (opcode >> 8 & 0xF) */
				cpsr.carry = immediate >> ((shift_amount - 1) & 0x1F) & 1;
				return std::rotr(immediate, shift_amount);
			}
			else {
				return GetSecondOperand(opcode);
			}
		}();

		/* TODO: right now, the carry can be modified through a call to the "GetSecondOperand" function,
			even if the "set condition codes" flag is clear. Is this correct? */
		bool carry{};
		u32 result = [&] {
			if constexpr (instr == ADC) {
				s64 result = s64(oper1) + s64(oper2) + s64(cpsr.carry);
				carry = result > std::numeric_limits<s32>::max();
				return u32(result);
			}
			if constexpr (instr == ADD || instr == CMN) {
				s64 result = s64(oper1) + s64(oper2);
				carry = result > std::numeric_limits<s32>::max();
				return u32(result);
			}
			if constexpr (instr == AND || instr == TST) {
				return oper1 & oper2;
			}
			if constexpr (instr == BIC) {
				return oper1 & ~oper2;
			}
			if constexpr (instr == CMP || instr == SUB) {
				carry = oper2 > oper1;
				return oper1 - oper2;
			}
			if constexpr (instr == EOR || instr == TEQ) {
				return oper1 ^ oper2;
			}
			if constexpr (instr == MOV) {
				return oper2;
			}
			if constexpr (instr == MVN) {
				return ~oper2;
			}
			if constexpr (instr == ORR) {
				return oper1 | oper2;
			}
			if constexpr (instr == RSB) {
				carry = oper1 > oper2;
				return oper2 - oper1;
			}
			if constexpr (instr == RSC) {
				auto result = oper2 - oper1 + cpsr.carry - 1;
				carry = oper1 + 1 - cpsr.carry > oper2;
				return result;
			}
			if constexpr (instr == SBC) {
				auto result = oper1 - oper2 + cpsr.carry - 1;
				carry = oper2 + 1 - cpsr.carry > oper1;
				return result;
			}
		}();

		if constexpr (instr != CMN && instr != CMP && instr != TEQ && instr != TST) {
			r[rd] = result;
		}

		if (set_conds) {
			if (rd == 15) {
				cpsr = std::bit_cast<CPSR, u32>(spsr);
			}
			else {
				cpsr.zero = result == 0;
				cpsr.negative = GetBit(result, 31);
				cpsr.carry = carry;
				if constexpr (is_arithmetic_instr) {
					cpsr.overflow = GetBit((oper1 ^ result) & (oper2 ^ result), 31);
				}
			}
		}
	}


	void DecodeExecuteARM(const u32 opcode)
	{
		if (opcode & 1 << 27) {
			switch (opcode >> 24 & 7) {
			case 0b000:
			case 0b001:
				BlockDataTransfer(opcode);
				break;

			case 0b010:
			case 0b011:
				Branch(opcode);
				break;

			case 0b100:
			case 0b101:
				SignalException<Exception::UndefinedInstruction>();
				break;

			case 0b110:
				SignalException<Exception::UndefinedInstruction>();
				break;

			case 0b111:
				SoftwareInterrupt(opcode);
				break;
			}
		}
		else {
			if (opcode & 1 << 26) {
				(opcode & 0x6000010) == 0x6000010
					? SignalException<Exception::UndefinedInstruction>()
					: SingleDataTransfer(opcode);
			}
			else {
				if ((opcode & 0xF2FFF10) == 0x12FFF10) {
					BranchAndExchange(opcode);
				}
				else if ((opcode & 0xFB00FF0) == 0x1000090) {
					SingleDataSwap(opcode);
				}
				else if ((opcode & 0xFC00090) == 0x90) {
					Multiply(opcode);
				}
				else if ((opcode & 0xF800090) == 0x800090) {
					MultiplyLong(opcode);
				}
				else if ((opcode & 0xF400F90) == 0x90) {
					//HalfwordDataTransferRegisterOffset(opcode);
				}
				else if ((opcode & 0xF400090) == 0x400090) {
					//HalfwordDataTransferImmediateOffset(opcode);
				}
				else {
					using enum DataProcessingInstruction;
					switch (opcode >> 21 & 0xF) {
					case 0b0000: DataProcessing<AND>(opcode); break;
					case 0b0001: DataProcessing<EOR>(opcode); break;
					case 0b0010: DataProcessing<SUB>(opcode); break;
					case 0b0011: DataProcessing<RSB>(opcode); break;
					case 0b0100: DataProcessing<ADD>(opcode); break;
					case 0b0101: DataProcessing<ADC>(opcode); break;
					case 0b0110: DataProcessing<SBC>(opcode); break;
					case 0b0111: DataProcessing<RSC>(opcode); break;
					case 0b1100: DataProcessing<ORR>(opcode); break;
					case 0b1101: DataProcessing<MOV>(opcode); break;
					case 0b1110: DataProcessing<BIC>(opcode); break;
					case 0b1111: DataProcessing<MVN>(opcode); break;

					case 0b1000: 
						if ((opcode & 0xFBF0FFF) == 0x10F0000) {
							MRS(opcode);
						}
						else {
							DataProcessing<TST>(opcode);
						}
						break;

					case 0b1010:
						if ((opcode & 0xFBF0FFF) == 0x10F0000) {
							MRS(opcode);
						}
						else {
							DataProcessing<CMP>(opcode);
						}
						break;

					case 0b1001: 
						if ((opcode & 0xFBFFFF0) == 0x129F000 || (opcode & 0xFBFF000) == 0x329F000) {
							MSR(opcode);
						}
						else {
							DataProcessing<TEQ>(opcode);
						}
						break;

					case 0b1011: 
						if ((opcode & 0xFBFFFF0) == 0x129F000 || (opcode & 0xFBFF000) == 0x329F000) {
							MSR(opcode);
						}
						else {
							DataProcessing<CMN>(opcode);
						}
						break;

					default: 
						std::unreachable();
					}
				}
			}
		}
	}


	u32 GetSecondOperand(u32 opcode)
	{
		auto rm = opcode & 0xF;
		auto shift_type = opcode >> 5 & 3;
		auto shift_amount = [&] {
			if (opcode & 0x10) { /* shift register */
				auto rs = opcode >> 8 & 0xF;
				return r[rs] & 0xFF;
			}
			else { /* shift immediate */
				return opcode >> 7 & 0x1F;
			}
		}();

		if (shift_amount == 0) {
			if (shift_type == 0b11) { /* Special function: rotate right extended */
				u32 result = u32(r[rm]) >> 1 | cpsr.carry << 31;
				cpsr.carry = r[rm] & 1;
				return result;
			}
			else {
				return r[rm];
			}
		}

		switch (shift_type) {
		case 0b00: /* logical left */
			cpsr.carry = GetBit(r[rm], 32 - shift_amount);
			return r[rm] << shift_amount;

		case 0b01: /* logical right */
			cpsr.carry = 0;
			return u32(r[rm]) >> shift_amount;

		case 0b10: /* arithmetic right */
			cpsr.carry = GetBit(r[rm], 31);
			return s32(r[rm]) >> shift_amount;

		case 0b11: /* rotate right */
			cpsr.carry = r[rm] >> ((shift_amount - 1) & 0x1F) & 1;
			return std::rotr(r[rm], shift_amount);

		default:
			std::unreachable();
		}
	}


	template<OffsetType offset_type>
	void HalfwordDataTransfer(u32 opcode)
	{
		auto rm = opcode & 0xF;
		auto immediate_offset = opcode >> 4 & 0xF0 | opcode & 0xF;
		auto sh = opcode >> 5 & 3;
		auto rd = opcode >> 12 & 0xF;
		auto rn = opcode >> 16 & 0xF;
		bool load_or_store = opcode >> 20 & 1;
		bool writeback = opcode >> 21 & 1;
		bool u = opcode >> 23 & 1;
		bool p = opcode >> 24 & 1;

	}


	void MRS(u32 opcode)
	{
		auto rd = opcode >> 12 & 0xF;
		bool psr = opcode >> 22 & 1; /* 0=CPSR; 1=SPSR */
		if (psr == 0) {
			r[rd] = std::bit_cast<u32, decltype(cpsr)>(cpsr);
		}
		else {
			r[rd] = spsr;
		}
	}


	void MSR(u32 opcode)
	{
		//bool dest_psr = opcode >> 22 & 1; /* 0=CPSR; 1=SPSR */
		//bool immediate_operand = opcode >> 25 & 1; /* 0=Register; 1=Immediate */
		//u32 operand = [&] {
		//	if (immediate_operand) {
		//		return GetSecondOperand(opcode); /* TODO: need to specialize this function for this instr? */
		//	}
		//	else {
		//		auto rm = opcode & 0xF;
		//		return r[rm];
		//	}
		//}();
		//if (dest_psr == 0) {
		//	u32 cpsr_u32 = std::bit_cast<u32, CPSR>(cpsr);
		//	cpsr = std::bit_cast<CPSR, u32>(operand & 0xF000'0000 | cpsr_u32 & 0xFFF'FFFF);
		//}
		//else {
		//	spsr = operand | spsr & 0xFFF'FFFF;
		//}
	}


	void Multiply(u32 opcode)
	{
		auto rm = opcode & 0xF;
		auto rs = opcode >> 8 & 0xF;
		auto rd = opcode >> 16 & 0xF;
		bool set_flags = opcode >> 20 & 1;
		bool accumulate = opcode >> 21 & 1;
		auto result = r[rm] * r[rs];
		if (accumulate) {
			auto rn = opcode >> 12 & 0xF;
			result += r[rn];
		}
		r[rd] = result;
		if (set_flags) {
			cpsr.zero = result == 0;
			cpsr.negative = GetBit(result, 31);
		}
		uint cycles_stalled = accumulate + [&] {
			if ((r[rs] & 0xFFFF'FF00) == 0) {
				return 2;
			}
			else if ((r[rs] & 0xFFFF'0000) == 0) {
				return 3;
			}
			else if ((r[rs] & 0xFF00'0000) == 0) {
				return 4;
			}
			else {
				return 5;
			}
		}();
		StallPipeline(cycles_stalled);
	}


	void MultiplyLong(u32 opcode)
	{
		auto rm = opcode & 0xF;
		auto rs = opcode >> 8 & 0xF;
		auto rd_lo = opcode >> 12 & 0xF;
		auto rd_hi = opcode >> 16 & 0xF;
		bool set_flags = opcode >> 20 & 1;
		bool accumulate = opcode >> 21 & 1;
		bool sign = opcode >> 22 & 1; /* 0=unsigned; 1=signed */
		s64 result;
		if (sign == 0) { /* unsigned */
			result = u64(u32(r[rm])) * u64(u32(r[rs]));
		}
		else { /* signed */
			result = s64(r[rm]) * s64(r[rs]);
		}
		if (accumulate) {
			result += r[rd_lo] + (s64(r[rd_hi]) << 32);
		}
		r[rd_lo] = result & 0xFFFF'FFFF;
		r[rd_hi] = result >> 32 & 0xFFFF'FFFF;
		if (set_flags) {
			cpsr.zero = result == 0;
			cpsr.negative = GetBit(result, 63);
		}
		uint cycles_stalled = accumulate + [&] {
			if ((r[rs] & 0xFFFF'FF00) == 0) {
				return 2;
			}
			else if ((r[rs] & 0xFFFF'0000) == 0) {
				return 3;
			}
			else if ((r[rs] & 0xFF00'0000) == 0) {
				return 4;
			}
			else {
				return 5;
			}
		}();
		StallPipeline(cycles_stalled);
	}


	void SingleDataSwap(u32 opcode)
	{
		auto rm = opcode & 0xF;
		auto rd = opcode >> 12 & 0xF;
		auto rn = opcode >> 16 & 0xF;
		bool b = opcode >> 22 & 1;
		auto address = r[rn];
		if (b) {
			auto loaded = Bus::Read<s8>(address);
			Bus::Write<s8>(address, r[rm]);
			r[rd] = loaded;
		}
		else {
			auto loaded = Bus::Read<s32>(address);
			Bus::Write<s32>(address, r[rm]);
			r[rd] = loaded;
		}
		s32 loaded = b ? Bus::Read<s8>(address) : Bus::Read<s32>(address);
	}


	void SingleDataTransfer(u32 opcode)
	{
		auto rd = opcode >> 12 & 0xF;
		auto rn = opcode >> 16 & 0xF;
		bool load_or_store = opcode >> 20 & 1; /* 0 = store to memory; 1 = load from memory */
		bool writeback = opcode >> 21 & 1; /* 0 = no write-back; 1 = write address into base */
		bool byte_or_word = opcode >> 22 & 1; /* 0 = transfer word quanitity; 1 = transfer byte quantity */
		bool u = opcode >> 23 & 1; /* 0 = subtract offset from base; 1 = add offset to base */
		bool p = opcode >> 24 & 1; /* 0 = add offset after transfer; 1 = add offset before transfer */
		bool i = opcode >> 25 & 1; /* 0 = offset is an immediate value; 1 = offset is a register */
		auto base = r[rn];

		s32 offset = [&] {
			auto offset = i ? GetSecondOperand(opcode) : opcode & 0xFFF;
			return u ? s32(offset) : -s32(offset);
		}();

		auto address = base + p * offset;
		if (load_or_store) {
			r[rd] = byte_or_word ? Bus::Read<s8>(address) : Bus::Read<s32>(address);
		}
		else {
			auto value = r[rd];
			/* When R15 is the source register (Rd) of a register store (STR) instruction, the stored
			value will be address of the instruction plus 12. */
			if (rd == 15) {
				value += 8; /* 4 has already been incremented after the instruction was fetched */
			}
			byte_or_word ? Bus::Write<s8>(address, value) : Bus::Write<s32>(address, value);
		}

		r[rn] = base + writeback * offset;
	}
}