module CPU;

import Bus;

#define sp (r[13])
#define lr (r[14])
#define pc (r[15])

/* TODO: flush pipeline when r15 is used as a destination */

namespace CPU
{
	constexpr std::string_view ArmDataProcessingInstructionToStr(ArmDataProcessingInstruction instr)
	{
		using enum ArmDataProcessingInstruction;
		switch (instr) {
		case ADC: return "ADC";
		case ADD: return "ADD";
		case AND: return "AND";
		case BIC: return "BIC";
		case CMN: return "CMN";
		case CMP: return "CMP";
		case EOR: return "EOR";
		case MOV: return "MOV";
		case MVN: return "MVN";
		case ORR: return "ORR";
		case RSB: return "RSB";
		case RSC: return "RSC";
		case SBC: return "SBC";
		case SUB: return "SUB";
		case TEQ: return "TEQ";
		case TST: return "TST";
		default: assert(false); return "";
		}
	}


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

		auto StoreReg = [&](u32 reg) {
			if constexpr (pre_or_post == 1) {
				addr += addr_delta;
			}
			Bus::Write<u32>(addr, reg);
			if constexpr (pre_or_post == 0) {
				addr += addr_delta;
			}
		};

		/* The registers are transferred in the order lowest to highest, so R15 (if in the list) will always be transferred last.
		The lowest register also gets transferred to/from the lowest memory address. */
		/* TODO: reduce code duplication */
		if (load_or_store == 0) {
			if (load_psr_and_force_user_mode == 0) {
				if (up_or_down == 0) {
					for (int i = 15; i >= 0; --i) {
						if (reg_list & 1 << i) {
							StoreReg(r[i]);
						}
					}
				}
				else {
					for (int i = 0; i < 16; ++i) {
						if (reg_list & 1 << i) {
							StoreReg(r[i]);
						}
					}
				}
			}
			else {
				/* Transfer user registers */
				if (up_or_down == 0) {
					if (reg_list & 1 << 15) {
						StoreReg(r[15]);
					}
					if (reg_list & 1 << 14) {
						StoreReg(r14_usr);
					}
					if (reg_list & 1 << 13) {
						StoreReg(r13_usr);
					}
					for (int i = 12; i >= 8; --i) {
						if (reg_list & 1 << i) {
							StoreReg(r8_r12_non_fiq[i - 8]);
						}
					}
					for (int i = 7; i >= 0; --i) {
						if (reg_list & 1 << i) {
							StoreReg(r[i]);
						}
					}
				}
				else {
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
		}
		else {
			if (load_psr_and_force_user_mode == 0) {
				if (up_or_down == 0) {
					for (int i = 15; i >= 0; --i) {
						if (reg_list & 1 << i) {
							r[i] = LoadReg();
						}
					}
				}
				else {
					for (int i = 0; i < 16; ++i) {
						if (reg_list & 1 << i) {
							r[i] = LoadReg();
						}
					}
				}
			}
			else {
				if (reg_list & 1 << 15) {
					if (up_or_down == 0) {
						for (int i = 15; i >= 0; --i) {
							if (reg_list & 1 << i) {
								r[i] = LoadReg();
							}
						}
					}
					else {
						for (int i = 0; i < 16; ++i) {
							if (reg_list & 1 << i) {
								r[i] = LoadReg();
							}
						}
					}
					cpsr = std::bit_cast<CPSR>(spsr);
				}
				else {
					/* Transfer user registers */
					if (up_or_down == 0) {
						if (reg_list & 1 << 15) {
							r[15] = LoadReg();
						}
						if (reg_list & 1 << 14) {
							r14_usr = LoadReg();
						}
						if (reg_list & 1 << 13) {
							r13_usr = LoadReg();
						}
						for (int i = 12; i >= 8; --i) {
							if (reg_list & 1 << i) {
								r8_r12_non_fiq[i - 8] = LoadReg();
							}
						}
						for (int i = 7; i >= 0; --i) {
							if (reg_list & 1 << i) {
								r[i] = LoadReg();
							}
						}
					}
					else {
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
		}

		if (write_back) {
			r[rn] = addr;
		}

		// TODO 4.11.6
	}


	void Branch(u32 opcode) /* B */
	{
		s32 offset = SignExtend<s32, 26>((opcode & 0xFF'FFFF) << 2);
		pc += offset;
		FlushPipeline();
	}


	void BranchAndExchange(u32 opcode) /* BX */
	{
		auto rn = opcode & 0xF;
		pc = r[rn];
		if (rn & 1) { /* Bit 0 of rn determines the processor state on entry to the routine */
			SetExecutionState(ExecutionState::THUMB);
			pc &= ~1;
		}
		else {
			SetExecutionState(ExecutionState::ARM);
			pc &= ~3;
		}
		FlushPipeline();
		/* TODO: Switching to THUMB Mode: Set Bit 0 of the value in Rn to 1, program continues then at Rn-1 in THUMB mode. */
	}


	void BranchAndLink(u32 opcode) /* BL */
	{
		s32 offset = SignExtend<s32, 26>((opcode & 0xFF'FFFF) << 2);
		lr = pc - 4; /* The PC value written into R14 is adjusted to allow for the prefetch, and contains the address of the instruction following the BL instruction */
		pc += offset;
		FlushPipeline();
	}


	template<ArmDataProcessingInstruction instr>
	void DataProcessing(u32 opcode) /* ADC, ADD, AND, BIC, CMN, CMP, EOR, MOV, MVN, ORR, RSB, RSC, SBC, SUB, TEQ, TST */
	{
		using enum ArmDataProcessingInstruction;

		static constexpr bool is_arithmetic_instr = instr == ADC || instr == ADD || instr == CMN ||
			instr == CMP || instr == RSB || instr == RSC || instr == SBC || instr == SUB;

		auto rd = opcode >> 12 & 0xF;
		auto rn = opcode >> 16 & 0xF;
		bool set_conds = opcode >> 20 & 1;
		bool reg_or_imm = opcode >> 25 & 1; /* 0 = register; 1 = immediate */

		u32 op1 = r[rn];
		u32 op2 = [&] {
			if (reg_or_imm == 0) { /* register */
				return GetSecondOperand(opcode);
			}
			else { /* immediate */
				auto imm = opcode & 0xFF;
				auto rot = opcode >> 7 & 0x1E; /* == 2 * (opcode >> 8 & 0xF) */
				if (rot == 0) {
					return imm;
				}
				else {
					cpsr.carry = imm >> (rot - 1) & 1;
					return std::rotr(imm, rot);
				}
			}
		}();

		/* TODO: right now, the carry can be modified through a call to the "GetSecondOperand" function,
			even if the "set condition codes" flag is clear. Is this correct? */
		u32 result = [&] {
			if constexpr (instr == ADC)                 return op1 + op2 + cpsr.carry;
			if constexpr (instr == ADD || instr == CMN) return op1 + op2;
			if constexpr (instr == AND || instr == TST) return op1 & op2;
			if constexpr (instr == BIC)                 return op1 & ~op2;
			if constexpr (instr == CMP || instr == SUB) return op1 - op2;
			if constexpr (instr == EOR || instr == TEQ) return op1 ^ op2;
			if constexpr (instr == MOV)                 return op2;
			if constexpr (instr == MVN)                 return ~op2;
			if constexpr (instr == ORR)                 return op1 | op2;
			if constexpr (instr == RSB)                 return op2 - op1;
			if constexpr (instr == RSC)                 return op2 - op1 + cpsr.carry - 1;
			if constexpr (instr == SBC)                 return op1 - op2 + cpsr.carry - 1;
		}();

		if constexpr (instr != CMN && instr != CMP && instr != TEQ && instr != TST) {
			r[rd] = result;
			if (rd == 15) {
				FlushPipeline();
			}
		}

		if (set_conds) {
			if (rd == 15) {
				if (cpsr.mode != cpsr_mode_bits_user && cpsr.mode != cpsr_mode_bits_system) {
					switch (spsr & 0x1F) {
					case cpsr_mode_bits_user:       SetMode<Mode::User>(); break;
					case cpsr_mode_bits_fiq:        SetMode<Mode::Fiq>(); break;
					case cpsr_mode_bits_irq:        SetMode<Mode::Irq>(); break;
					case cpsr_mode_bits_supervisor: SetMode<Mode::Supervisor>(); break;
					case cpsr_mode_bits_abort:      SetMode<Mode::Abort>(); break;
					case cpsr_mode_bits_undefined:  SetMode<Mode::Undefined>(); break;
					case cpsr_mode_bits_system:     SetMode<Mode::System>(); break;
					default: assert(false); break;
					}
					SetExecutionState(static_cast<ExecutionState>(GetBit(spsr, 5)));
					cpsr = std::bit_cast<CPSR>(spsr);
				}
			}
			else {
				cpsr.zero = result == 0;
				cpsr.negative = GetBit(result, 31);
				if constexpr (is_arithmetic_instr)          cpsr.overflow = GetBit((op1 ^ result) & (op2 ^ result), 31);
				if constexpr (instr == ADC)                 cpsr.carry = u64(op1) + u64(op2) + u64(cpsr.carry) > std::numeric_limits<u32>::max();
				if constexpr (instr == ADD || instr == CMN) cpsr.carry = std::numeric_limits<u32>::max() - u32(op1) < u32(op2);
				if constexpr (instr == CMP || instr == SUB) cpsr.carry = op2 <= op1; /* this is not borrow */
				if constexpr (instr == RSB)                 cpsr.carry = op1 <= op2;
				if constexpr (instr == RSC)                 cpsr.carry = u64(op1) - u64(cpsr.carry) + u64(1) <= u64(op2);
				if constexpr (instr == SBC)                 cpsr.carry = u64(op2) - u64(cpsr.carry) + u64(1) <= u64(op1);
			}
		}
	}


	void DecodeExecuteARM(u32 opcode)
	{
		if (opcode & 1 << 27) {
			switch (opcode >> 24 & 7) {
			case 0b000:
				BlockDataTransfer<0>(opcode);
				break;

			case 0b001:
				BlockDataTransfer<1>(opcode);
				break;

			case 0b010:
				Branch(opcode);
				break;

			case 0b011:
				BranchAndLink(opcode);
				break;

			case 0b100:
			case 0b101:
			case 0b110:
				SignalException<Exception::UndefinedInstruction>();
				break;

			case 0b111:
				SoftwareInterrupt();
				break;

			default:
				std::unreachable();
			}
		}
		else if (opcode & 1 << 26) {
			SingleDataTransfer(opcode);
		}
		else if ((opcode & 0xFFF'FFF0) == 0x12F'FF10) {
			BranchAndExchange(opcode);
		}
		else if ((opcode & 0xFB0'0FF0) == 0x100'0090) {
			SingleDataSwap(opcode);
		}
		else if ((opcode & 0xFC0'00F0) == 0x90) {
			Multiply(opcode);
		}
		else if ((opcode & 0xF80'00F0) == 0x80'0090) {
			MultiplyLong(opcode);
		}
		else if ((opcode & 0xE40'0F90) == 0x90) {
			HalfwordDataTransfer<OffsetType::Register>(opcode);
		}
		else if ((opcode & 0xE40'0090) == 0x40'0090) {
			HalfwordDataTransfer<OffsetType::Immediate>(opcode);
		}
		else {
			using enum ArmDataProcessingInstruction;
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
				(opcode & 0xFFF'0FFF) == 0x10F'0000
					? MRS<0>(opcode)
					: DataProcessing<TST>(opcode);
				break;

			case 0b1010:
				(opcode & 0xFFF'0FFF) == 0x14F'0000
					? MRS<1>(opcode)
					: DataProcessing<CMP>(opcode);
				break;

			case 0b1001:
				(opcode & 0xFF0'FFF0) == 0x120'F000 || (opcode & 0xFF0'F000) == 0x320'F000
					? MSR<0>(opcode)
					: DataProcessing<TEQ>(opcode);
				break;

			case 0b1011:
				(opcode & 0xFF0'FFF0) == 0x160'F000 || (opcode & 0xFF0'F000) == 0x360'F000
					? MSR<1>(opcode)
					: DataProcessing<CMN>(opcode);
				break;

			default:
				std::unreachable();
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

		switch (shift_type) {
		case 0b00: /* logical left */
			if (shift_amount == 0) {
				/* LSL#0: No shift performed, ie. directly Op2=Rm, the C flag is NOT affected. */
				return r[rm];
			}
			else {
				cpsr.carry = GetBit(r[rm], 32 - shift_amount);
				return r[rm] << shift_amount;
			}

		case 0b01: /* logical right */
			if (shift_amount == 0) {
				/* LSR#0: Interpreted as LSR#32, ie. Op2 becomes zero, C becomes Bit 31 of Rm */
				cpsr.carry = GetBit(r[rm], 31);
				return 0;
			}
			else {
				cpsr.carry = GetBit(r[rm], shift_amount - 1);
				return u32(r[rm]) >> shift_amount;
			}

		case 0b10: /* arithmetic right */
			if (shift_amount == 0) {
				/* ASR#0: Interpreted as ASR#32, ie. Op2 and C are filled by Bit 31 of Rm. */
				cpsr.carry = GetBit(r[rm], 31);
				return cpsr.carry ? 0xFFFF'FFFF : 0;
			}
			else {
				cpsr.carry = GetBit(r[rm], shift_amount - 1);
				return s32(r[rm]) >> shift_amount;
			}

		case 0b11: /* rotate right */
			if (shift_amount == 0) {
				/* ROR#0: Interpreted as RRX#1 (RCR), like ROR#1, but Op2 Bit 31 set to old C. */
				auto prev_carry = cpsr.carry;
				cpsr.carry = r[rm] & 1;
				return u32(r[rm]) >> 1 | prev_carry << 31;
			}
			else {
				cpsr.carry = r[rm] >> ((shift_amount - 1) & 0x1F) & 1;
				return std::rotr(r[rm], shift_amount);
			}

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

		default: /* 0b00: Single Data Swap */
			std::unreachable();
		}
		if (w) {
			addr += !p * offset;
			r[rn] = addr;
		}
	}


	template<bool psr /* 0=CPSR; 1=SPSR */ >
	void MRS(u32 opcode) /* MRS */
	{
		auto rd = opcode >> 12 & 0xF;
		if constexpr (psr == 0) {
			r[rd] = std::bit_cast<u32>(cpsr);
		}
		else {
			r[rd] = spsr;  /* TODO: read from spsr in user/system modes? */
		}
	}


	template<bool psr /* 0=CPSR; 1=SPSR */ >
	void MSR(u32 opcode) /* MSR */
	{
		auto mode = cpsr.mode;
		if constexpr (psr == 1) {
			if (mode == cpsr_mode_bits_user || mode == cpsr_mode_bits_system) {
				return; /* User/System modes do not have a SPSR */
			}
		}

		bool reg_or_imm = opcode >> 25 & 1; /* 0=Register; 1=Immediate */
		u32 oper = [&] {
			if (reg_or_imm == 0) { /* register */
				auto rm = opcode & 0xF;
				return r[rm]; /* seems to not shifted/rotated, unlike in data processing instrs */
			}
			else { /* (rotated) immediate */
				auto imm = opcode & 0xFF;
				auto rot = opcode >> 7 & 0x1E; /* == 2 * (opcode >> 8 & 0xF) */
				return std::rotr(imm, rot); /* seems to not set carry, unlike in data processing instrs */
			}
		}();

		u32 mask{};
		if (mode == cpsr_mode_bits_user) {
			mask |= 0xF0000000 * GetBit(opcode, 19); /* User mode can only change the flag bits. TODO: bits 24-27? */
			if constexpr (psr == 0) { /* cpsr */
				u32 prev_cpsr = std::bit_cast<u32>(cpsr);
				cpsr = std::bit_cast<CPSR>(oper & mask | prev_cpsr & ~mask);
			}
			else { /* spsr */
				spsr = oper & mask | spsr & ~mask;
			}
		}
		else {
			mask |= 0xFF000000 * GetBit(opcode, 19);
			mask |= 0x00FF0000 * GetBit(opcode, 18);
			mask |= 0x0000FF00 * GetBit(opcode, 17);
			mask |= 0x000000FF * GetBit(opcode, 16);
			if constexpr (psr == 0) { /* cpsr */
				if (mask & 0xFF) {
					oper |= 0x10; /* bit 4 is forced to 1 */
					switch (oper & 0x1F) {
					case cpsr_mode_bits_user: SetMode<Mode::User>(); break;
					case cpsr_mode_bits_fiq: SetMode<Mode::Fiq>(); break;
					case cpsr_mode_bits_irq: SetMode<Mode::Irq>(); break;
					case cpsr_mode_bits_supervisor: SetMode<Mode::Supervisor>(); break;
					case cpsr_mode_bits_abort: SetMode<Mode::Abort>(); break;
					case cpsr_mode_bits_undefined: SetMode<Mode::Undefined>(); break;
					case cpsr_mode_bits_system: SetMode<Mode::System>(); break;
					default: assert(false); break;
					}
					SetExecutionState(static_cast<ExecutionState>(GetBit(oper, 5)));
				}
				u32 prev_cpsr = std::bit_cast<u32>(cpsr);
				cpsr = std::bit_cast<CPSR>(oper & mask | prev_cpsr & ~mask);
			}
			else { /* spsr */
				spsr = oper & mask | spsr & ~mask;
			}
		}
	}


	void Multiply(u32 opcode) /* MUL, MLA */
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


	void MultiplyLong(u32 opcode) /* MULL, MLAL */
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
			result = s64(s32(r[rm])) * s64(s32(r[rs]));
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