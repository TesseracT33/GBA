module Bios;

namespace Bios
{
	void Initialize()
	{
		if (bios_ptr == nullptr) {
			SetDefault();
		}
	}


	bool Load(const std::string& path)
	{
		auto opt_bios = ReadFileIntoVector(path);
		if (opt_bios) {
			if (opt_bios.value().size() == 0x4000) {
				custom_bios = opt_bios.value();
				bios_ptr = custom_bios.data();
				return true;
			}
			else {
				UserMessage::Show("BIOS must be 16 KiB large. Reverting to the default BIOS.",
					UserMessage::Type::Warning);
				SetDefault();
				return false;
			}
		}
		else {
			UserMessage::Show("Could not load custom BIOS. Reverting to the default BIOS.",
				UserMessage::Type::Warning);
			SetDefault();
			SetDefault();
			return false;
		}
	}


	template<std::integral Int>
	Int Read(u32 addr)
	{
		Int ret;
		std::memcpy(&ret, bios_ptr + addr, sizeof(Int));
		return ret;
	}


	void SetDefault()
	{
		bios_ptr = default_bios.data();
		custom_bios.clear();
	}


	template s8 Read<s8>(u32);
	template u8 Read<u8>(u32);
	template s16 Read<s16>(u32);
	template u16 Read<u16>(u32);
	template s32 Read<s32>(u32);
	template u32 Read<u32>(u32);
}