export module Debug;

import Bus;
import UserMessage;
import Util;

import <array>;
import <concepts>;
import <format>;
import <fstream>;
import <optional>;
import <string>;
import <string_view>;

namespace Debug
{
	export
	{
		constexpr bool enable_asserts = false;
		constexpr bool log_instrs = false;
		constexpr bool log_io_reads = false;
		constexpr bool log_io_writes = false;

		constexpr bool LoggingIsEnabled() { return log_instrs || log_io_reads || log_io_writes; }
		void LogInstruction(u32 pc, u32 opcode, std::string_view cond_str, bool cond, const std::array<u32, 16>& r, u32 cpsr);
		void LogInstruction(u32 pc, u16 opcode, const std::array<u32, 16>& r, u32 cpsr);
		void LogInstruction(u32 pc, std::string instr_output, const std::array<u32, 16>& r, u32 cpsr);
		template<Bus::IoOperation> void LogIoAccess(u32 addr, std::integral auto data);
		void SetLogPath(const std::string& log_path);
	}

	std::string FormatCPSR(u32 cpsr);
	std::string FormatRegisters(const std::array<u32, 16>& r);

	std::ofstream log;
}