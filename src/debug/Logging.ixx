#include <fstream> /* compiler bug? needed to compile on msvc */
export module Logging;

import UserMessage;
import Util;

import <format>;
import <fstream>;
import <string>;
import <string_view>;

namespace Logging
{
	std::ofstream log;

	export void Initialize(const auto& log_path)
	{
		if (log.is_open()) {
			log.close();
		}
		log.open(log_path, std::ofstream::out | std::ofstream::binary);
	}

	export void LogInstruction(u32 pc, u32 opcode, std::string_view cond_str, bool cond)
	{
		if (!log.is_open()) {
			return;
		}
		log << std::format("{:08X}  ARM:{:08X}  cond:{} ({})\n", pc, opcode, cond_str, cond);
	}

	export void LogInstruction(u32 pc, u16 opcode)
	{
		if (!log.is_open()) {
			return;
		}
		log << std::format("{:08X}  THUMB:{:04X}\n", pc, opcode);
	}

	export void LogInstruction(u32 pc, std::string instr_output)
	{
		if (!log.is_open()) {
			return;
		}
		log << std::format("{:08X}  {}\n", pc, instr_output);
	}
}