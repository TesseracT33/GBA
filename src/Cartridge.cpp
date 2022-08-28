module Cartridge;

namespace Cartridge
{
	bool LoadRom(const std::string& path)
	{
		auto optional_vec = ReadFileIntoVector(path);
		if (optional_vec) {
			rom = optional_vec.value();
			return true;
		}
		else {
			return false;
		}
	}



}