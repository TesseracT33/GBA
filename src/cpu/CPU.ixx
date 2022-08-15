export module CPU;

import NumericalTypes;
import SerializationStream;
import Util;
import Util.Bit;

import <algorithm>;
import <array>;
import <bit>;
import <cassert>;
import <concepts>;
import <cstring>;
import <limits>;
import <string_view>;
import <utility>;

namespace CPU
{
	export
	{
		void Initialize();
		void Run();
		void SetIrqHigh();
		void SetIrqLow();
		void StreamState(SerializationStream& stream);
	}

	enum class Exception {
		DataAbort, Fiq, Irq, PrefetchAbort, Reset, SoftwareInterrupt, UndefinedInstruction
	};

	enum class ExecutionState {
		ARM = 0, THUMB = 1
	} execution_state = ExecutionState::ARM;

	enum class Mode {
		User, Fiq, Irq, Supervisor, Abort, Undefined, System
	};

	enum class ArmDataProcessingInstruction {
		ADC, ADD, AND, BIC, CMN, CMP, EOR, MOV, MVN, ORR, RSB, RSC, SBC, SUB, TEQ, TST
	};

	enum class OffsetType {
		Register, Immediate
	};

	enum class ThumbAluInstruction {
		ADC, AND, ASR, BIC, CMN, CMP, EOR, LSL, LSR, MUL, MVN, NEG, ORR, ROR, SBC, TST
	};

	using InstrHandler = void(*)(u32);

	bool CheckCondition(u32 cond);
	void DecodeExecute(u32 opcode);
	void DecodeExecuteARM(u32 opcode);
	void DecodeExecuteTHUMB(u16 opcode);
	u32 Fetch();
	void FlushPipeline();
	template<Exception> constexpr uint GetExceptionPriority();
	void HandleDataAbortException();
	void HandleFiqException();
	void HandleIrqException();
	void HandlePrefetchAbortException();
	void HandleResetException();
	void HandleSoftwareInterruptException();
	void HandleUndefinedInstructionException();
	void SetExecutionState(ExecutionState state);
	template<Mode> void SetMode();
	template<Exception> void SignalException();
	void StallPipeline(uint cycles);
	void StepPipeline();

	/* ARM instructions */
	template<bool> void BlockDataTransfer(u32 opcode);
	void Branch(u32 opcode);
	void BranchAndExchange(u32 opcode);
	template<ArmDataProcessingInstruction> void DataProcessing(u32 opcode);
	template<OffsetType> void HalfwordDataTransfer(u32 opcode);
	void MRS(u32 opcode);
	void MSR(u32 opcode);
	void Multiply(u32 opcode);
	void MultiplyLong(u32 opcode);
	void SingleDataSwap(u32 opcode);
	void SingleDataTransfer(u32 opcode);

	/* THUMB instructions */
	void AddOffsetToStackPointer(u16 opcode);
	void AddSubtract(u16 opcode);
	template<ThumbAluInstruction> void AluOperation(u16 opcode);
	void ConditionalBranch(u16 opcode);
	void HiReg(u16 opcode);
	void LoadAddress(u16 opcode);
	void LoadStoreImmOffset(u16 opcode);
	void LoadStoreHalfword(u16 opcode);
	void LoadStoreRegOffset(u16 opcode);
	void LoadStoreSignExtendedByteHalfword(u16 opcode);
	void LongBranchWithLink(u16 opcode);
	void MoveCompareAddSubtractImm(u16 opcode);
	void MultipleLoadStore(u16 opcode);
	void PcRelativeLoad(u16 opcode);
	void PushPopRegisters(u16 opcode);
	void Shift(u16 opcode);
	void SpRelativeLoadStore(u16 opcode);
	void UnconditionalBranch(u16 opcode);

	void SoftwareInterrupt();

	constexpr u32 cpsr_mode_bits_user = 16;
	constexpr u32 cpsr_mode_bits_fiq = 17;
	constexpr u32 cpsr_mode_bits_irq = 18;
	constexpr u32 cpsr_mode_bits_supervisor = 19;
	constexpr u32 cpsr_mode_bits_abort = 23;
	constexpr u32 cpsr_mode_bits_undefined = 27;
	constexpr u32 cpsr_mode_bits_system = 31;
	constexpr u32 exception_vector_reset = 0;
	constexpr u32 exception_vector_undefined_instr = 4;
	constexpr u32 exception_vector_software_int = 8;
	constexpr u32 exception_vector_prefetch_abort = 0xC;
	constexpr u32 exception_vector_data_abort = 0x10;
	constexpr u32 exception_vector_irq = 0x18;
	constexpr u32 exception_vector_fiq = 0x1C;

	constexpr std::array cond_strings = {
		"EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC", "HI", "LS", "GE", "LT", "GT", "LE", "AL", "INVALID"
	};

	struct Pipeline
	{
		std::array<u32, 2> opcode;
		uint opcode_index;
		uint step;
	} pipeline;

	struct CPSR
	{
		u32 mode : 5; /* 16=User, 17=FIQ, 18=IRQ, 19=Supervisor, 23=Abort, 27=Undefined, 31=System */
		u32 state : 1; /* 0=ARM, 1=THUMB */
		u32 fiq_disable : 1;
		u32 irq_disable : 1;
		u32 : 20;
		u32 overflow : 1;
		u32 carry : 1;
		u32 zero : 1;
		u32 negative : 1;
	} cpsr;
	
	std::array<u32, 5> r8_r12_non_fiq; /* R8-R12 */
	std::array<u32, 5> r8_r12_fiq; /* R8-R12 */
	u32 r13_usr, r14_usr; /* also system */
	u32 r13_fiq, r14_fiq;
	u32 r13_svc, r14_svc;
	u32 r13_abt, r14_abt;
	u32 r13_irq, r14_irq;
	u32 r13_und, r14_und;
	u32 spsr_fiq, spsr_svc, spsr_abt, spsr_irq, spsr_und;
	u32 spsr;
	std::array<u32, 16> r; /* currently active registers */

	Exception occurred_exception;
	uint occurred_exception_priority;

	u32 GetSecondOperand(u32 opcode);
}