export module APU;

import Util;

import <concepts>;

namespace APU
{
	export
	{
		void Initialize();
		template<std::integral Int> Int ReadReg(u32 addr);
		template<std::integral Int> void WriteReg(u32 addr, Int data); 
	}
}