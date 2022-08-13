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
		switch (opcode >> 12 & 0xF)
		{
		case 0b0000:
		case 0b0001:
			(opcode & 0x180) == 0x180
				? AddSubtract(opcode)
				: Shift(opcode);
			break;

		case 0b0010:
		case 0b0011:
			MoveCompareAddSubtractImm(opcode);
			break;

		case 0b0100:
			if (opcode & 0x400) {
				HiReg(opcode);
			}
			else {
				using enum ThumbAluInstruction;
				switch (opcode >> 6 & 0xF) {
				case 0b0000: Alu<AND>(opcode); break;
				case 0b0001: Alu<EOR>(opcode); break;
				case 0b0010: Alu<LSL>(opcode); break;
				case 0b0011: Alu<LSR>(opcode); break;
				case 0b0100: Alu<ASR>(opcode); break;
				case 0b0101: Alu<ADC>(opcode); break;
				case 0b0110: Alu<SBC>(opcode); break;
				case 0b0111: Alu<ROR>(opcode); break;
				case 0b1000: Alu<TST>(opcode); break;
				case 0b1001: Alu<NEG>(opcode); break;
				case 0b1010: Alu<CMP>(opcode); break;
				case 0b1011: Alu<CMN>(opcode); break;
				case 0b1100: Alu<ORR>(opcode); break;
				case 0b1101: Alu<MUL>(opcode); break;
				case 0b1110: Alu<BIC>(opcode); break;
				case 0b1111: Alu<MVN>(opcode); break;
				default: std::unreachable();
				}
			}
			break;

		case 0b0101:
			opcode & 0x200
				? LoadStoreSignExtendedByteHalfword(opcode)
				: LoadStoreWithRegisterOffset(opcode);
			break;

		case 0b0110:
		case 0b0111:
			LoadStoreImmediateOffset(opcode); /* TODO: can also select 'B' here given the different cases */
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

		case 0b1101: // TODO: check {cond} on all branch instrs
			(opcode & 0xF0) == 0xF0
				? SoftwareInterrupt(opcode)
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


	void Shift(u16 opcode)
	{
		auto rd = opcode & 7;
		auto rs = opcode >> 3 & 7;
		auto offset = opcode >> 6 & 0x1F;
		auto op = opcode >> 11 & 3;

		auto result = [&] {
			switch (op) {
			case 0b00: /* LSL */
				if (offset > 0) {
					cpsr.carry = GetBit(r[rs], 31);
				}
				return r[rs] << offset;

			case 0b01: /* LSR */
				if (offset > 0) {
					cpsr.carry = 0;
				}
				return u32(r[rs]) >> offset;

			case 0b10: /* ASR */
				if (offset > 0) {
					cpsr.carry = GetBit(r[rs], 31);
				}
				return u32(s32(r[rs]) >> offset);

			case 0b11: /* ADD/SUB; already covered by other function */
				std::unreachable();
			}
		}();

		r[rd] = result;
		cpsr.zero = result == 0;
		cpsr.negative = GetBit(result, 31);
	}


	void AddSubtract(u16 opcode)
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


	void MoveCompareAddSubtractImm(u16 opcode)
	{
		u8 immediate = opcode & 0xFF; /* TODO: signed? */
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
			}
		}();

		r[rd] = result;
		cpsr.zero = result == 0;
	}


	template<ThumbAluInstruction instr>
	void Alu(u16 opcode)
	{
		using enum ThumbAluInstruction;

		auto rd = opcode & 7;
		auto rs = opcode >> 3 & 7;

		auto oper1 = r[rd];
		auto oper2 = r[rs];

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
				if (oper2 > 0) {
					cpsr.carry = GetBit(oper1, 31);
				}
				return s32(oper1) >> oper2;
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
				if (oper2 > 0) {
					cpsr.carry = oper2 <= 32 ? GetBit(oper1, 32 - oper2) : 0;
				}
				return oper1 << oper2;
			}
			else if constexpr (instr == LSR) {
				if (oper2 > 0) {
					cpsr.carry = 0;
				}
				return u32(oper1) >> oper2;
			}
			else if constexpr (instr == MUL) {
				/* TODO: carry */
				return oper1 * oper2;
			}
			else if constexpr (instr == MVN) {
				return ~oper2;
			}
			else if constexpr (instr == NEG) {
				return -s32(oper2);
			}
			else if constexpr (instr == ORR) {
				return oper1 | oper2;
			}
			else if constexpr (instr == ROR) {
				if (oper2 > 0) {
					cpsr.carry = oper1 >> ((oper2 - 1) & 0x1F) & 1;
				}
				return std::rotr(oper1, oper2);
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
		if constexpr (instr == ADC || instr == CMN || instr == CMP || instr == SBC) {
			cpsr.overflow = GetBit((oper1 ^ result) & (oper2 ^ result), 31);
		}
	}


	void HiReg(u16 opcode)
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
			SetExecutionState(static_cast<ExecutionState>(r[rs] & 1));
			break;
		}
	}


	void PcRelativeLoad(u16 opcode)
	{
		u32 offset = (opcode & 0xFF) << 2; /* unsigned */
		auto rd = opcode >> 8 & 7;
		r[rd] = Bus::Read<s32>(pc + offset);
	}


	void LoadStoreWithRegisterOffset(u16 opcode)
	{
		auto rd = opcode & 7;
		auto rb = opcode >> 3 & 7;
		auto ro = opcode >> 6 & 7;
		bool byte_or_word = opcode >> 10 & 1; /* 0: word; 1: byte */
		bool load_or_store = opcode >> 11 & 1; /* 0: store; 1: load */
		auto addr = r[rb] + r[ro];
		if (load_or_store == 0) {
			byte_or_word ? Bus::Write<u8>(addr, r[rd]) : Bus::Write<u32>(addr, r[rd]);
		}
		else {
			r[rd] = byte_or_word ? Bus::Read<u8>(addr) : Bus::Read<u32>(addr); /* TODO: sign-extension? */
		}
	}


	void LoadStoreSignExtendedByteHalfword(u16 opcode)
	{
		auto rd = opcode & 7;
		auto rb = opcode >> 3 & 7;
		auto ro = opcode >> 6 & 7;
		auto op = opcode >> 10 & 2;
		auto addr = r[rb] + r[ro];
		switch (op) {
		case 0b00: /* Store halfword */
			Bus::Write<s16>(addr, s16(r[rd] & 0xFFFF));
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
		}
	}


	void LoadStoreImmediateOffset(u16 opcode)
	{
		auto rd = opcode & 7;
		auto rb = opcode >> 3 & 7;
		bool load_or_store = opcode >> 11 & 1; /* 0: store; 1: load */
		bool byte_or_word = opcode >> 12 & 1; /* 0: word; 1: byte */
		if (byte_or_word == 0) {
			auto offset = opcode >> 4 & 0x7C; /* == (opcode >> 6 & 0x1F) << 2 */ /* TODO: signed? */
			auto addr = r[rb] + offset;
			if (load_or_store == 0) {
				Bus::Write<u32>(addr, r[rd]);
			}
			else {
				r[rd] = Bus::Read<u32>(addr);
			}
		}
		else {
			auto offset = opcode >> 6 & 0x1F; /* TODO: signed? */
			auto addr = r[rb] + offset;
			if (load_or_store == 0) {
				Bus::Write<u8>(addr, u8(r[rd] & 0xFF));
			}
			else {
				r[rd] = Bus::Read<u8>(addr); /* TODO: zero or sign extension? */
			}
		}
	}


	void LoadStoreHalfword(u16 opcode)
	{
		auto rd = opcode & 7;
		auto rb = opcode >> 3 & 7;
		auto offset = opcode >> 5 & 0x3E; /* == (opcode >> 6 & 0x1F) << 1 */  /* TODO: signed? */
		bool load_or_store = opcode >> 11 & 1; /* 0: store; 1: load */
		auto addr = r[rb] + offset;
		if (load_or_store == 0) {
			Bus::Write<u16>(addr, r[rd]);
		}
		else {
			r[rd] = Bus::Read<u16>(addr); /* TODO: sign-extension? */
		}
	}


	void SpRelativeLoadStore(u16 opcode)
	{
		u8 immediate = opcode & 0xFF; /* unsigned */
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


	void LoadAddress(u16 opcode)
	{
		u8 immediate = opcode & 0xFF; /* TODO: signed? */
		auto rd = opcode >> 8 & 7;
		auto src = opcode >> 11 & 1; /* 0: PC (r15); 1: SP (r13) */
		auto addr = r[15 - 2 * src] + (immediate << 2);
		r[rd] = Bus::Read<u32>(addr);
	}


	void AddOffsetToStackPointer(u16 opcode)
	{
		s16 offset = (opcode & 0x7F) << 2;
		bool sign = opcode >> 7 & 1; /* 0: positive offset; 1: negative offset */
		if (sign == 1) {
			offset = -offset; /* [-508, 508] */
		}
		sp += offset;
	}


	void PushPopRegisters(u16 opcode)
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
				pc = Bus::Read<u32>(sp++);
			}
		}
	}


	void MultipleLoadStore(u16 opcode)
	{
		auto reg_list = opcode & 0xFF;
		auto rb = opcode >> 8 & 7;
		bool load_or_store = opcode >> 11 & 1; /* 0: store; 1: load */
		auto addr = r[rb];
		if (load_or_store) {
			for (int i = 0; i < 8; ++i) {
				if (reg_list & 1 << i) {
					r[i] = Bus::Read<u32>(addr);
					addr += 4;
				}
			}
		}
		else {
			for (int i = 0; i < 8; ++i) {
				if (reg_list & 1 << i) {
					Bus::Write<u32>(addr, r[i]);
					addr += 4;
				}
			}
		}
		r[rb] = addr;
	}


	void ConditionalBranch(u16 opcode)
	{
		auto cond = opcode >> 8 & 0xF;
		bool branch = CheckCondition(cond);
		if (branch) {
			s32 offset = SignExtend<s32, 9>(opcode << 1 & 0x1FE);
			pc += offset;
		}
	}


	void SoftwareInterrupt(u16 opcode)
	{
		SignalException<Exception::SoftwareInterrupt>();
	}


	void UnconditionalBranch(u16 opcode)
	{
		s32 offset = SignExtend<s32, 12>(opcode << 1 & 0xFFE);
		pc += offset;
	}


	void LongBranchWithLink(u16 opcode)
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
			s32 prev_pc = pc;
			pc = lr;
			lr = prev_pc | 1;
		}
	}
}