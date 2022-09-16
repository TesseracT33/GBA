module CPU;

#define sp (r[13])
#define lr (r[14])
#define pc (r[15])

namespace CPU
{
	constexpr std::string_view ExceptionToStr(Exception exc)
	{
		switch (exc) {
		case Exception::DataAbort: return "Data abort";
		case Exception::Fiq: return "FIQ";
		case Exception::Irq: return "IRQ";
		case Exception::PrefetchAbort: return "Prefetch abort";
		case Exception::Reset: return "Reset";
		case Exception::SoftwareInterrupt: return "Software interrupt";
		case Exception::UndefinedInstruction: return "Undefined instruction";
		default: assert(false); return "";
		}
	}


	template<Exception exception>
	ExceptionHandler GetExceptionHandler()
	{
		if constexpr (exception == Exception::Reset)                return HandleResetException;
		if constexpr (exception == Exception::DataAbort)            return HandleDataAbortException;
		if constexpr (exception == Exception::Fiq)                  return HandleFiqException;
		if constexpr (exception == Exception::Irq)                  return HandleIrqException;
		if constexpr (exception == Exception::PrefetchAbort)        return HandlePrefetchAbortException;
		if constexpr (exception == Exception::SoftwareInterrupt)    return HandleSoftwareInterruptException;
		if constexpr (exception == Exception::UndefinedInstruction) return HandleUndefinedInstructionException;
	}


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
		pc = exception_vector_data_abort;
		FlushPipeline();
		spsr_abt = std::bit_cast<u32>(cpsr);
		cpsr.irq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Abort>();
	}


	void HandleFiqException()
	{
		r14_fiq = pc;
		pc = exception_vector_fiq;
		FlushPipeline();
		spsr_fiq = std::bit_cast<u32>(cpsr);
		cpsr.irq_disable = cpsr.fiq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Fiq>();
	}


	void HandleIrqException()
	{
		r14_irq = pc;
		pc = exception_vector_irq;
		FlushPipeline();
		spsr_irq = std::bit_cast<u32>(cpsr);
		cpsr.irq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Irq>();
	}


	void HandlePrefetchAbortException()
	{
		r14_abt = pc;
		pc = exception_vector_prefetch_abort;
		FlushPipeline();
		spsr_abt = std::bit_cast<u32>(cpsr);
		cpsr.irq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Abort>();
	}


	void HandleResetException()
	{
		r14_svc = pc;
		pc = exception_vector_reset;
		FlushPipeline();
		spsr_svc = std::bit_cast<u32>(cpsr);
		cpsr.irq_disable = cpsr.fiq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Supervisor>();
	}


	void HandleSoftwareInterruptException()
	{
		r14_svc = pc;
		pc = exception_vector_software_int;
		FlushPipeline();
		spsr_svc = std::bit_cast<u32>(cpsr);
		cpsr.irq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Supervisor>();
	}


	void HandleUndefinedInstructionException()
	{
		r14_und = pc;
		pc = exception_vector_undefined_instr;
		FlushPipeline();
		spsr_und = std::bit_cast<u32>(cpsr);
		cpsr.irq_disable = 1;
		SetExecutionState(ExecutionState::ARM);
		SetMode<Mode::Undefined>();
	}


	template<Exception exception>
	void SignalException()
	{
		constexpr static auto priority = GetExceptionPriority<exception>();
		if (!exception_has_occurred || priority < occurred_exception_priority) {
			exception_has_occurred = true;
			occurred_exception_priority = priority;
			exception_handler = GetExceptionHandler<exception>();
		}
	}


	template void SignalException<Exception::DataAbort>();
	template void SignalException<Exception::Fiq>();
	template void SignalException<Exception::Irq>();
	template void SignalException<Exception::PrefetchAbort>();
	template void SignalException<Exception::Reset>();
	template void SignalException<Exception::SoftwareInterrupt>();
	template void SignalException<Exception::UndefinedInstruction>();
}