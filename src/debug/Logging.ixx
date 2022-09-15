#include <fstream> /* compiler bug? needed to compile on msvc */
export module Logging;

import UserMessage;
import Util;

import <array>;
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

	std::string FormatCPSR(u32 cpsr)
	{
		static constexpr std::array flag_strs = {
			"nzcv", "nzcV", "nzCv", "nzCV", "nZcv", "nZcV", "nZCv", "nZCV",
			"Nzcv", "NzcV", "NzCv", "NzCV", "NZcv", "NZcV", "NZCv", "NZCV"
		};
		static constexpr std::array ift_strs = {
			"ift", "ifT", "iFt", "iFT", "Ift", "IfT", "IFt", "IFT"
		};
		return std::format("{}/{}/{:02X}", flag_strs[cpsr >> 28], ift_strs[cpsr >> 5 & 7], cpsr & 0x1F);
	}

	std::string FormatRegisters(const std::array<u32, 16>& r)
	{
		return std::format("r0:{:08X} r1:{:08X} r2:{:08X} r3:{:08X} r4:{:08X} r5:{:08X} r6:{:08X} r7:{:08X} "
			"r8:{:08X} r9:{:08X} r10:{:08X} r11:{:08X} r12:{:08X} r13:{:08X} r14:{:08X} r15:{:08X}",
			r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9], r[10], r[11], r[12], r[13], r[14], r[15]);
	}

	export void LogInstruction(u32 pc, u32 opcode, std::string_view cond_str, bool cond, const std::array<u32, 16>& r, u32 cpsr)
	{
		if (!log.is_open()) {
			return;
		}
		log << std::format("{:08X}  ARM:{:08X}  cond:{} ({}) {} cpsr:{}\n", pc, opcode, cond_str, cond, FormatRegisters(r), FormatCPSR(cpsr));
	}

	export void LogInstruction(u32 pc, u16 opcode, const std::array<u32, 16>& r, u32 cpsr)
	{
		if (!log.is_open()) {
			return;
		}
		log << std::format("{:08X}  THUMB:{:04X} {} cpsr:{}\n", pc, opcode, FormatRegisters(r), FormatCPSR(cpsr));
	}

	export void LogInstruction(u32 pc, std::string instr_output, const std::array<u32, 16>& r, u32 cpsr)
	{
		if (!log.is_open()) {
			return;
		}
		log << std::format("{:08X}  {} {} cpsr:{}\n", pc, instr_output, FormatRegisters(r), FormatCPSR(cpsr));
	}
}