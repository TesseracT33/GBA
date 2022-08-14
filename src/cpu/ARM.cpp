module CPU;

import Bus;

import Util.Bit;

#define sp (r[13])
#define lr (r[14])
#define pc (r[15])

namespace CPU
{
	template<bool pre_or_post /* 0 = post (add offset after transfer); 1 = pre (add offset before transfer) */ >
	void BlockDataTransfer(u32 opcode) /* LDM, STM */
	{
		auto reg_list = opcode & 0xFFFF;
		auto rn = opcode >> 16 & 0xF;
		bool load_or_store = opcode >> 20 & 1; /* 0 = store to memory; 1 = load from memory */
		bool write_back = opcode >> 21 & 1; /* 0 = no write-back; 1 = write address into base */
		bool load_psr_and_force_user_mode = opcode >> 22 & 1; /* 0 = do not load PSR or force user mode; 1 = load PSR or force user mode */
		bool up_or_down = opcode >> 23 & 1; /* 0 = down (subtract offset from base); 1 = up (add offset to base) */

		auto addr = r[rn];
		s32 addr_delta = up_or_down ? 4 : -4;

		auto LoadReg = [&] {
			if constexpr (pre_or_post == 1) {
				addr += addr_delta;
			}
			u32 ret = Bus::Read<u32>(addr);
			if constexpr (pre_or_post == 0) {
				addr += addr_delta;
			}
			return ret;
		};

		auto StoreReg = [&](u32 word) {
			if constexpr (pre_or_post == 1) {
				addr += addr_delta;
			}
			Bus::Write<u32>(addr, word);
			if constexpr (pre_or_post == 0) {
				addr += addr_delta;
			}
		};

		if (load_or_store == 0) {
			if (load_psr_and_force_user_mode == 0) {
				for (int i = 0; i < 16; ++i) {
					if (reg_list & 1 << i) {
						StoreReg(r[i]);
					}
				}
			}
			else {
				/* Transfer user registers */
				for (int i = 0; i < 8; ++i) {
					if (reg_list & 1 << i) {
						StoreReg(r[i]);
					}
				}
				for (int i = 8; i < 13; ++i) {
					if (reg_list & 1 << i) {
						StoreReg(r8_r12_non_fiq[i - 8]);
					}
				}
				if (reg_list & 1 << 13) {
					StoreReg(r13_usr);
				}
				if (reg_list & 1 << 14) {
					StoreReg(r14_usr);
				}
				if (reg_list & 1 << 15) {
					StoreReg(r[15]);
				}
			}
		}
		else {
			if (load_psr_and_force_user_mode == 0) {
				for (int i = 0; i < 16; ++i) {
					if (reg_list & 1 << i) {
						r[i] = LoadReg();
					}
				}
			}
			else {
				if (reg_list & 1 << 15) {
					for (int i = 0; i < 16; ++i) {
						if (reg_list & 1 << i) {
							r[i] = LoadReg();
						}
					}
					cpsr = std::bit_cast<CPSR, u32>(spsr);
				}
				else {
					/* Transfer user registers */
					for (int i = 0; i < 8; ++i) {
						if (reg_list & 1 << i) {
							r[i] = LoadReg();
						}
					}
					for (int i = 8; i < 13; ++i) {
						if (reg_list & 1 << i) {
							r8_r12_non_fiq[i - 8] = LoadReg();
						}
					}
					if (reg_list & 1 << 13) {
						r13_usr = LoadReg();
					}
					if (reg_list & 1 << 14) {
						r14_usr = LoadReg();
					}
					if (reg_list & 1 << 15) {
						r[15] = LoadReg();
					}
				}
			}
		}

		if (write_back) {
			r[rn] = addr;
		}

		// TODO 4.11.6
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
				opcode & 1 << 24
					? BlockDataTransfer<1>(opcode)
					: BlockDataTransfer<0>(opcode);
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
	void HalfwordDataTransfer(u32 opcode) /* LDRH, STRH, LDRSB, LDRSH */
	{
		auto sh = opcode >> 5 & 3;
		auto rd = opcode >> 12 & 0xF;
		auto rn = opcode >> 16 & 0xF;
		bool l = opcode >> 20 & 1; /* 0 = store to memory; 1 = load from memory */
		bool w = opcode >> 21 & 1; /* 0 = no write-back; 1 = write address into base */
		bool u = opcode >> 23 & 1; /* 0 = down (subtract offset from base); 1 = up (add offset to base) */
		bool p = opcode >> 24 & 1; /* 0 = add/subtract offset after transfer; 0 = add/subtract offset before transfer */

		s32 offset = [&] {
			if constexpr (offset_type == OffsetType::Register) {
				auto rm = opcode & 0xF;
				return r[rm];
			}
			else { /* immediate */
				return opcode >> 4 & 0xF0 | opcode & 0xF;
			}
		}();
		if (u == 0) {
			offset = -offset;
		}
		auto addr = r[rn] + p * offset;
		switch (sh) {
		case 0b01: /* unsigned halfword */
			if (l) {
				r[rd] = Bus::Read<u16>(addr);
			}
			else {
				Bus::Write<u16>(addr, u16(r[rd]));
			}
			break;

		case 0b10: /* signed byte */
			if (l) {
				r[rd] = Bus::Read<s8>(addr);
			}
			else {
				Bus::Write<s8>(addr, s8(r[rd]));
			}
			break;

		case 0b11: /* signed halfword */
			if (l) {
				r[rd] = Bus::Read<s16>(addr);
			}
			else {
				Bus::Write<s16>(addr, s16(r[rd]));
			}
			break;

		default:
			std::unreachable();
		}
		if (w) {
			addr += !p * offset;
			r[rn] = addr;
		}
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


	void SingleDataSwap(u32 opcode) /* SWP */
	{
		auto rm = opcode & 0xF;
		auto rd = opcode >> 12 & 0xF;
		auto rn = opcode >> 16 & 0xF;
		bool byte_or_word = opcode >> 22 & 1; /* 0 = word; 1 = byte */
		auto addr = r[rn];
		if (byte_or_word == 0) {
			auto mem = Bus::Read<u32>(addr);
			Bus::Write<u32>(addr, r[rm]);
			r[rd] = mem;
		}
		else {
			auto mem = Bus::Read<u8>(addr);
			Bus::Write<u8>(addr, u8(r[rm]));
			r[rd] = mem;
		}
	}


	void SingleDataTransfer(u32 opcode) /* LDR, STR */
	{
		auto rd = opcode >> 12 & 0xF;
		auto rn = opcode >> 16 & 0xF;
		bool load_or_store = opcode >> 20 & 1; /* 0 = store to memory; 1 = load from memory */
		bool writeback = opcode >> 21 & 1; /* 0 = no write-back; 1 = write address into base */
		bool byte_or_word = opcode >> 22 & 1; /* 0 = transfer word quanitity; 1 = transfer byte quantity */
		bool up_or_down = opcode >> 23 & 1; /* 0 = subtract offset from base; 1 = add offset to base */
		bool p = opcode >> 24 & 1; /* 0 = add offset after transfer; 1 = add offset before transfer */
		bool reg_or_imm = opcode >> 25 & 1; /* 0 = offset is an immediate value; 1 = offset is a register */

		s32 offset = [&] {
			auto offset = reg_or_imm ? GetSecondOperand(opcode) : opcode & 0xFFF;
			return up_or_down ? s32(offset) : -s32(offset);
		}();
		auto addr = r[rn] + p * offset;
		if (load_or_store) {
			r[rd] = byte_or_word ? Bus::Read<u8>(addr) : Bus::Read<u32>(addr);
		}
		else {
			byte_or_word ? Bus::Write<u8>(addr, r[rd]) : Bus::Write<u32>(addr, r[rd]);
		}
		if (writeback) {
			addr += !p * offset;
			r[rn] = addr;
		}
	}
}