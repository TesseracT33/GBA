module Cartridge;

namespace Cartridge
{
	bool LoadRom(const std::string& path)
	{
		auto optional_vec = ReadFileIntoVector(path);
		if (!optional_vec) {
			return false;
		}
		else {
			rom = optional_vec.value();
			return true;
		}
	}



}