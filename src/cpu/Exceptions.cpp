module CPU;

#define sp (r[13])
#define lr (r[14])
#define pc (r[15])

namespace CPU
{
	template<Exception exception>
	constexpr uint GetExceptionPriority()
	{
		if constexpr (exception == Exception::Reset)                return 1;
		if constexpr (exception == Exception::DataAbort)            return 2;
		if constexpr (exception == Exception::Fiq)                  return 3;
		if constexpr (exception == Exception::Irq)                  return 4;
		if constexpr (exception == Exception::PrefetchAbort)        return 5;
		if constexpr (exception == Exception::SoftwareInterrupt)    return 6;
		if constexpr (exception == Exception::UndefinedInstruction) return 7;
	}


	void HandleDataAbortException()
	{
		r14_abt = pc;
		pc = 0x10;
		spsr_abt = std::bit_cast<u32, CPSR>(cpsr);
		cpsr.irq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Abort>();
	}


	void HandleFiqException()
	{
		r_fiq[6] = pc;
		pc = 0x1C;
		spsr_fiq = std::bit_cast<u32, CPSR>(cpsr);
		cpsr.irq_disable = cpsr.fiq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Fiq>();
	}


	void HandleIrqException()
	{
		r14_irq = pc;
		pc = 0x18;
		spsr_irq = std::bit_cast<u32, CPSR>(cpsr);
		cpsr.irq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Irq>();
	}


	void HandlePrefetchAbortException()
	{
		r14_abt = pc;
		pc = 0xC;
		spsr_abt = std::bit_cast<u32, CPSR>(cpsr);
		cpsr.irq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Abort>();
	}


	void HandleResetException()
	{
		r14_svc = pc;
		pc = 0;
		spsr_svc = std::bit_cast<u32, CPSR>(cpsr);
		cpsr.irq_disable = cpsr.fiq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Supervisor>();
	}


	void HandleSoftwareInterruptException()
	{
		r14_svc = pc;
		pc = 8;
		spsr_svc = std::bit_cast<u32, CPSR>(cpsr);
		cpsr.irq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Supervisor>();
	}


	void HandleUndefinedInstructionException()
	{
		r14_und = pc;
		pc = 4;
		spsr_und = std::bit_cast<u32, CPSR>(cpsr);
		cpsr.irq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Undefined>();
	}


	template<Exception exception>
	void SignalException()
	{
		constexpr static auto priority = GetExceptionPriority<exception>();
		if (priority < occurred_exception_priority) {
			occurred_exception = exception;
			occurred_exception_priority = priority;
		}
	}
}