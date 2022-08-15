module CPU;

import Bus;
import Util.Bit;

#define sp (r[13])
#define lr (r[14])
#define pc (r[15])

namespace CPU
{
	void DecodeExecuteTHUMB(u16 opcode)
	{
		switch (opcode >> 12 & 0xF) {
		case 0b0000:
		case 0b0001:
			(opcode & 0x1800) == 0x1800
				? AddSubtract(opcode)
				: Shift(opcode);
			break;

		case 0b0010:
		case 0b0011:
			MoveCompareAddSubtractImm(opcode);
			break;

		case 0b0100:
			if (opcode & 0x800) {
				PcRelativeLoad(opcode);
			}
			else if (opcode & 0x400) {
				HiReg(opcode);
			}
			else {
				using enum ThumbAluInstruction;
				switch (opcode >> 6 & 0xF) {
				case  0: AluOperation<AND>(opcode); break;
				case  1: AluOperation<EOR>(opcode); break;
				case  2: AluOperation<LSL>(opcode); break;
				case  3: AluOperation<LSR>(opcode); break;
				case  4: AluOperation<ASR>(opcode); break;
				case  5: AluOperation<ADC>(opcode); break;
				case  6: AluOperation<SBC>(opcode); break;
				case  7: AluOperation<ROR>(opcode); break;
				case  8: AluOperation<TST>(opcode); break;
				case  9: AluOperation<NEG>(opcode); break;
				case 10: AluOperation<CMP>(opcode); break;
				case 11: AluOperation<CMN>(opcode); break;
				case 12: AluOperation<ORR>(opcode); break;
				case 13: AluOperation<MUL>(opcode); break;
				case 14: AluOperation<BIC>(opcode); break;
				case 15: AluOperation<MVN>(opcode); break;
				default: std::unreachable();
				}
			}
			break;

		case 0b0101:
			opcode & 0x200
				? LoadStoreSignExtendedByteHalfword(opcode)
				: LoadStoreRegOffset(opcode);
			break;

		case 0b0110:
			LoadStoreImmOffset<0>(opcode);
			break;

		case 0b0111:
			LoadStoreImmOffset<1>(opcode);
			break;

		case 0b1000:
			LoadStoreHalfword(opcode);
			break;

		case 0b1001:
			SpRelativeLoadStore(opcode);
			break;

		case 0b1010:
			LoadAddress(opcode);
			break;

		case 0b1011:
			opcode & 0x400
				? PushPopRegisters(opcode)
				: AddOffsetToStackPointer(opcode);
			break;

		case 0b1100:
			MultipleLoadStore(opcode);
			break;

		case 0b1101:
			(opcode & 0xF00) == 0xF00
				? SoftwareInterrupt()
				: ConditionalBranch(opcode);
			break;

		case 0b1110: 
			UnconditionalBranch(opcode);
			break;

		case 0b1111:
			LongBranchWithLink(opcode);
			break;

		default:
			std::unreachable();
		}
	}


	void Shift(u16 opcode) /* Format 1: ASR, LSL, LSR */
	{
		auto rd = opcode & 7;
		auto rs = opcode >> 3 & 7;
		auto offset = opcode >> 6 & 0x1F;
		auto op = opcode >> 11 & 3;

		auto result = [&] {
			switch (op) {
			case 0b00: /* LSL */
				if (offset == 0) {
					return r[rs];
				}
				else {
					cpsr.carry = GetBit(r[rs], 32 - offset);
					return r[rs] << offset;
				}

			case 0b01: /* LSR */
				/* LSR#0 is interpreted as LSR#32 */
				cpsr.carry = 0;
				if (offset == 0) {
					return 0u;
				}
				else {
					return u32(r[rs]) >> offset;
				}

			case 0b10: /* ASR */
				/* ASR#0 is interpreted as ASR#32 */
				cpsr.carry = GetBit(r[rs], 31);
				if (offset == 0) {
					return cpsr.carry ? 0xFFFF'FFFF : 0;
				}
				else {
					return u32(s32(r[rs]) >> offset);
				}

			case 0b11: /* ADD/SUB; already covered by other function */
				std::unreachable();
			}
		}();

		r[rd] = result;
		cpsr.zero = result == 0;
		cpsr.negative = GetBit(result, 31);
	}


	void AddSubtract(u16 opcode) /* Format 2: ADD, SUB */
	{
		auto rd = opcode & 7;
		auto rs = opcode >> 3 & 7;
		auto offset = opcode >> 6 & 7;
		bool op = opcode >> 9 & 1;
		bool reg_or_imm_oper = opcode >> 10 & 1; /* 0=Register; 1=Immediate */

		u32 oper1 = r[rs];
		u32 oper2 = reg_or_imm_oper ? offset : r[offset];

		auto result = [&] {
			if (op == 0) { /* ADD */
				s64 sum = s64(oper1) + s64(oper2);
				cpsr.carry = sum > std::numeric_limits<s32>::max();
				return u32(sum);
			}
			else { /* SUB */
				cpsr.carry = oper2 > oper1;
				return oper1 - oper2;
			}
		}();

		r[rd] = result;
		cpsr.overflow = GetBit((oper1 ^ result) & (oper2 ^ result), 31);
		cpsr.zero = result == 0;
		cpsr.negative = GetBit(result, 31);
	}


	void MoveCompareAddSubtractImm(u16 opcode) /* Format 3: ADD, CMP, MOV, SUB */
	{
		u8 immediate = opcode & 0xFF;
		auto rd = opcode >> 8 & 7;
		auto op = opcode >> 11 & 3;

		auto result = [&] {
			switch (op) {
			case 0b00: /* MOV */
				cpsr.negative = 0;
				return u32(immediate);

			case 0b01: { /* CMP */
				u32 result = r[rd] - immediate;
				cpsr.overflow = GetBit((r[rd] ^ result) & (immediate ^ result), 31);
				cpsr.carry = immediate > r[rd];
				cpsr.negative = GetBit(result, 31);
				return result;
			}

			case 0b10: { /* ADD */
				s64 result = s64(r[rd]) + s64(immediate);
				cpsr.overflow = GetBit((r[rd] ^ result) & (immediate ^ result), 31);
				cpsr.carry = result > std::numeric_limits<s32>::max();
				cpsr.negative = GetBit(result, 31);
				return u32(result);
			}

			case 0b11: { /* SUB */
				u32 result = r[rd] - immediate;
				cpsr.overflow = GetBit((r[rd] ^ result) & (immediate ^ result), 31);
				cpsr.carry = immediate > r[rd];
				cpsr.negative = GetBit(result, 31);
				return result;
			}

			default:
				std::unreachable();
			}
		}();

		r[rd] = result;
		cpsr.zero = result == 0;
	}


	template<ThumbAluInstruction instr>
	void AluOperation(u16 opcode) /* Format 4: ADC, AND, ASR, BIC, CMN, CMP, EOR, LSL, LSR, MUL, MVN, NEG, ORR, ROR, SBC, TST */
	{
		using enum ThumbAluInstruction;

		auto rd = opcode & 7;
		auto rs = opcode >> 3 & 7;
		auto oper1 = r[rd];
		auto oper2 = r[rs];

		/* Affected Flags:
			N,Z,C,V for  ADC,SBC,NEG,CMP,CMN
			N,Z,C   for  LSL,LSR,ASR,ROR (carry flag unchanged if zero shift amount)
			N,Z,C   for  MUL on ARMv4 and below: carry flag destroyed
			N,Z     for  MUL on ARMv5 and above: carry flag unchanged
			N,Z     for  AND,EOR,TST,ORR,BIC,MVN
		*/

		auto result = [&] {
			if constexpr (instr == ADC) {
				s64 result = s64(oper1) + s64(oper2) + s64(cpsr.carry);
				cpsr.carry = result > std::numeric_limits<s32>::max();
				return u32(result);
			}
			else if constexpr (instr == AND || instr == TST) {
				return oper1 & oper2;
			}
			else if constexpr (instr == ASR) {
				auto shift_amount = oper2 & 0xFF;
				if (shift_amount > 0) {
					cpsr.carry = GetBit(oper1, 31);
					return u32(s32(oper1) >> shift_amount);
				}
				else {
					return oper1;
				}
			}
			else if constexpr (instr == BIC) {
				return oper1 & ~oper2;
			}
			else if constexpr (instr == CMN) {
				s64 result = s64(oper1) + s64(oper2);
				cpsr.carry = result > std::numeric_limits<s32>::max();
				return u32(result);
			}
			else if constexpr (instr == CMP) {
				cpsr.carry = oper2 > oper1;
				return oper1 - oper2;
			}
			else if constexpr (instr == EOR) {
				return oper1 ^ oper2;
			}
			else if constexpr (instr == LSL) {
				auto shift_amount = oper2 & 0xFF;
				if (shift_amount > 0) {
					cpsr.carry = shift_amount <= 32 ? GetBit(oper1, 32 - shift_amount) : 0;
					return oper1 << shift_amount;
				}
				else {
					return oper1;
				}
			}
			else if constexpr (instr == LSR) {
				auto shift_amount = oper2 & 0xFF;
				if (shift_amount > 0) {
					cpsr.carry = 0;
					return u32(oper1) >> oper2;
				}
				else {
					return oper1;
				}
			}
			else if constexpr (instr == MUL) {
				cpsr.carry = 0;
				return oper1 * oper2;
			}
			else if constexpr (instr == MVN) {
				return ~oper2;
			}
			else if constexpr (instr == NEG) {
				cpsr.carry = oper2 > 0;
				return -s32(oper2);
			}
			else if constexpr (instr == ORR) {
				return oper1 | oper2;
			}
			else if constexpr (instr == ROR) {
				auto shift_amount = oper2 & 0xFF;
				if (shift_amount > 0) {
					cpsr.carry = oper1 >> ((shift_amount - 1) & 0x1F) & 1;
					return std::rotr(oper1, shift_amount);
				}
				else {
					return oper1;
				}
			}
			else if constexpr (instr == SBC) {
				cpsr.carry = u64(oper2) + u64(!cpsr.carry) > u64(oper1);
				return oper1 - oper2 - !cpsr.carry;
			}
			else {
				static_assert(AlwaysFalse<instr>);
			}
		}();

		if constexpr (instr != CMP && instr != CMN && instr != TST) {
			r[rd] = result;
		}
		cpsr.zero = result == 0;
		cpsr.negative = GetBit(result, 31);
		if constexpr (instr == ADC || instr == CMN || instr == CMP || instr == NEG || instr == SBC) {
			cpsr.overflow = GetBit((oper1 ^ result) & (oper2 ^ result), 31);
		}
	}


	void HiReg(u16 opcode) /* Format 5: ADD, BX, CMP, MOV */
	{
		auto rd = opcode & 7;
		auto rs = opcode >> 3 & 7;
		auto h2 = opcode >> 6 & 1;
		auto h1 = opcode >> 7 & 1;
		auto op = opcode >> 8 & 3;
		/* add 8 to register indeces if h flags are set */
		rd += h1 << 3;
		rs += h2 << 3;
		/* In this group only CMP (Op = 01) sets the CPSR condition codes. */
		switch (op) {
		case 0b00: /* ADD */
			r[rd] += r[rs];
			break;

		case 0b01: { /* CMP */
			auto result = r[rd] - r[rs];
			cpsr.overflow = GetBit((r[rd] ^ result) & (r[rs] ^ result), 31);
			cpsr.carry = r[rs] > r[rd];
			cpsr.zero = result == 0;
			cpsr.negative = GetBit(result, 31);
			break;
		}

		case 0b10: /* MOV */
			r[rd] = r[rs];
			break;

		case 0b11: /* BX */
			pc = r[rs];
			if (rs & 1) {
				SetExecutionState(ExecutionState::THUMB);
				pc &= ~1;
			}
			else {
				SetExecutionState(ExecutionState::ARM);
				pc &= ~3;
			}
			FlushPipeline();
			break;

		default:
			std::unreachable();
		}
	}


	void PcRelativeLoad(u16 opcode) /* Format 6: LDR */
	{
		u32 offset = (opcode & 0xFF) << 2;
		auto rd = opcode >> 8 & 7;
		r[rd] = Bus::Read<u32>((pc & ~2) + offset);
	}


	void LoadStoreRegOffset(u16 opcode) /* Format 7: LDR, LDRB, STR, STRB */
	{
		auto rd = opcode & 7;
		auto rb = opcode >> 3 & 7;
		auto ro = opcode >> 6 & 7;
		bool byte_or_word = opcode >> 10 & 1; /* 0: word; 1: byte */
		bool load_or_store = opcode >> 11 & 1; /* 0: store; 1: load */
		auto addr = r[rb] + r[ro];
		if (load_or_store == 0) { /* store */
			byte_or_word ? Bus::Write<u8>(addr, u8(r[rd])) : Bus::Write<u32>(addr, r[rd]);
		}
		else { /* load */
			r[rd] = byte_or_word ? Bus::Read<u8>(addr) : Bus::Read<u32>(addr); 
		}
	}


	void LoadStoreSignExtendedByteHalfword(u16 opcode) /* Format 8: LDSB, LDRH, LDSH, STRH */
	{
		auto rd = opcode & 7;
		auto rb = opcode >> 3 & 7;
		auto ro = opcode >> 6 & 7;
		auto op = opcode >> 10 & 2;
		auto addr = r[rb] + r[ro];
		switch (op) {
		case 0b00: /* Store halfword */
			Bus::Write<s16>(addr, s16(r[rd]));
			break;

		case 0b01: /* Load sign-extended byte */
			r[rd] = Bus::Read<s8>(addr);
			break;

		case 0b10: /* Load zero-extended halfword */
			r[rd] = Bus::Read<u16>(addr);
			break;

		case 0b11: /* Load sign-extended halfword */
			r[rd] = Bus::Read<s16>(addr);
			break;

		default:
			std::unreachable();
		}
	}


	template<bool byte_or_word /* 0: word; 1: byte */ >
	void LoadStoreImmOffset(u16 opcode) /* Format 9: LDR, LDRB, STR, STRB */
	{
		auto rd = opcode & 7;
		auto rb = opcode >> 3 & 7;
		bool load_or_store = opcode >> 11 & 1; /* 0: store; 1: load */
		/* unsigned offset is 0-31 for byte, 0-124 (step 4) for word */
		if constexpr (byte_or_word == 0) { /* word */
			auto offset = opcode >> 4 & 0x7C; /* == (opcode >> 6 & 0x1F) << 2 */
			auto addr = r[rb] + offset;
			if (load_or_store == 0) {
				Bus::Write<u32>(addr, r[rd]);
			}
			else {
				r[rd] = Bus::Read<u32>(addr);
			}
		}
		else { /* byte */
			auto offset = opcode >> 6 & 0x1F;
			auto addr = r[rb] + offset;
			if (load_or_store == 0) {
				Bus::Write<u8>(addr, u8(r[rd]));
			}
			else {
				r[rd] = Bus::Read<u8>(addr);
			}
		}
	}


	void LoadStoreHalfword(u16 opcode) /* Format 10: LDRH, STRH */
	{
		auto rd = opcode & 7;
		auto rb = opcode >> 3 & 7;
		auto offset = opcode >> 5 & 0x3E; /* == (opcode >> 6 & 0x1F) << 1 */
		bool load_or_store = opcode >> 11 & 1; /* 0: store; 1: load */
		auto addr = r[rb] + offset;
		if (load_or_store == 0) { 
			Bus::Write<u16>(addr, r[rd]);
		}
		else {
			r[rd] = Bus::Read<u16>(addr);
		}
	}


	void SpRelativeLoadStore(u16 opcode) /* Format 11: LDR, STR */
	{
		u8 immediate = opcode & 0xFF;
		auto rd = opcode >> 8 & 7;
		bool load_or_store = opcode >> 11 & 1; /* 0: store; 1: load */
		auto addr = sp + (immediate << 2); 
		if (load_or_store == 0) {
			Bus::Write<u32>(addr, r[rd]);
		}
		else {
			r[rd] = Bus::Read<u32>(addr);
		}
	}


	void LoadAddress(u16 opcode) /* Format 12: ADD Rd,PC,#nn; ADD Rd,SP,#nn */
	{
		u8 immediate = opcode & 0xFF;
		auto rd = opcode >> 8 & 7;
		auto src = opcode >> 11 & 1; /* 0: PC (r15); 1: SP (r13) */
		if (src == 0) {
			r[rd] = (pc & ~2) + (immediate << 2);
		}
		else {
			r[rd] = sp + (immediate << 2);
		}
	}


	void AddOffsetToStackPointer(u16 opcode) /* Format 13: ADD SP,#nn */
	{
		s16 offset = (opcode & 0x7F) << 2;
		bool sign = opcode >> 7 & 1; /* 0: positive offset; 1: negative offset */
		if (sign == 1) {
			offset = -offset; /* [-508, 508] in steps of 4 */
		}
		sp += offset;
	}


	void PushPopRegisters(u16 opcode) /* Format 14: PUSH, POP */
	{
		auto reg_list = opcode & 0xFF;
		bool transfer_lr_pc = opcode >> 8 & 1; /* 0: Do not store LR / load PC; 1: Store LR / Load PC */
		bool load_or_store = opcode >> 11 & 1; /* 0: store; 1: load */
		if (load_or_store == 0) {
			for (int i = 0; i < 8; ++i) {
				if (reg_list & 1 << i) {
					Bus::Write<u32>(--sp, r[i]);
				}
			}
			if (transfer_lr_pc) {
				Bus::Write<u32>(--sp, lr);
			}
		}
		else {
			for (int i = 0; i < 8; ++i) {
				if (reg_list & 1 << i) {
					r[i] = Bus::Read<u32>(sp++);
				}
			}
			if (transfer_lr_pc) {
				pc = Bus::Read<u32>(sp++) & ~1;
			}
		}
	}


	void MultipleLoadStore(u16 opcode) /* Format 15: LDMIA, STMIA */
	{
		auto reg_list = opcode & 0xFF;
		auto rb = opcode >> 8 & 7;
		bool load_or_store = opcode >> 11 & 1; /* 0: store; 1: load */
		/* Strange Effects on Invalid Rlist's
			* Empty Rlist: R15 loaded/stored (ARMv4 only), and Rb=Rb+40h (ARMv4-v5).
			* Writeback with Rb included in Rlist: Store OLD base if Rb is FIRST entry in Rlist,
			otherwise store NEW base (STM/ARMv4). Always store OLD base (STM/ARMv5), no writeback (LDM/ARMv4/ARMv5).
			TODO: emulate 2nd point
		*/
		if (load_or_store) {
			if (reg_list == 0) {
				pc = Bus::Read<u32>(r[rb]);
				r[rb] += 0x40;
			}
			else {
				for (int i = 0; i < 8; ++i) {
					if (reg_list & 1 << i) {
						r[i] = Bus::Read<u32>(r[rb]);
						r[rb] += 4;
					}
				}
			}
		}
		else {
			if (reg_list == 0) {
				pc = Bus::Read<u32>(r[rb]);
				r[rb] += 0x40;
			}
			else {
				for (int i = 0; i < 8; ++i) {
					if (reg_list & 1 << i) {
						Bus::Write<u32>(r[rb], r[i]);
						r[rb] += 4;
					}
				}
			}
		}
		
	}


	void ConditionalBranch(u16 opcode) /* Format 16: BEQ, BNE, BCS, BCC, BMI, BPL, BVS, BVC, BHI, BLS, BGE, BLT, BGT, BLE */
	{
		auto cond = opcode >> 8 & 0xF;
		bool branch = CheckCondition(cond);
		if (branch) {
			s32 offset = SignExtend<s32, 9>(opcode << 1 & 0x1FE); /* [-256, 254] in steps of 2 */
			pc += offset;
		}
	}


	void SoftwareInterrupt() /* Format 17: SWI */
	{
		SignalException<Exception::SoftwareInterrupt>();
	}


	void UnconditionalBranch(u16 opcode) /* Format 18: B */
	{
		s32 offset = SignExtend<s32, 12>(opcode << 1 & 0xFFE); /* [-2048, 2046] in steps of 2 */
		pc += offset;
	}


	void LongBranchWithLink(u16 opcode) /* Format 19: BL */
	{
		auto immediate = opcode & 0x7FF;
		bool low_or_high_offset = opcode >> 11 & 1; /* 0: offset high; 1: offset low */
		if (low_or_high_offset == 0) {
			s32 offset = SignExtend<s32, 23>(immediate << 12);
			lr = pc + offset;
		}
		else {
			s32 offset = immediate << 1;
			lr += offset;
			std::swap(pc, lr);
			lr |= 1;
		}
	}
}