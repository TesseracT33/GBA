module Bus;

namespace Bus
{
	template<std::integral Int>
	Int Read(s32 addr)
	{
		static_assert(sizeof(Int) == 1 || sizeof(Int) == 2 || sizeof(Int) == 4);
		/* A byte load expects the data on data bus inputs 7 through 0 if the supplied
		address is on a word boundary, on data bus inputs 15 through 8 if it is a word address
		plus one byte, and so on. The selected byte is placed in the bottom 8 bits of the
		destination register, and the remaining bits of the register are filled with zeros.

		A word load will normally use a word aligned address. However, an address offset
		from a word boundary will cause the data to be rotated into the register so that
		the addressed byte occupies bits 0 to 7. This means that half-words accessed at
		offsets 0 and 2 from the word boundary will be correctly loaded into bits 0 through 15
		of the register. Two shift operations are then required to clear or to sign extend the
		upper 16 bits.
		*/

		if constexpr (sizeof(Int) == 2) {

		}
		if constexpr (sizeof(Int) == 4) {

		}

		auto page = addr >> 12 & 0xFFFF;
		u8* mem_ptr = page_table_read[page];
		if (mem_ptr != nullptr) {
			Int ret;
			std::memcpy(&ret, mem_ptr + (addr & 0xFFF), sizeof(Int));
			return ret;
		}
		else {
			return 0;
		}
	}

	template<std::integral Int>
	void Write(s32 addr, Int data)
	{
		static_assert(sizeof(Int) == 1 || sizeof(Int) == 2 || sizeof(Int) == 4);
		/* A byte store repeats the bottom 8 bits of the source register four times across
		data bus outputs 31 through 0.
		
		A halfword store repeats the bottom 16 bits of the source register twice across
		the data bus outputs 31 through to 0. Note that the address must be
		halfword aligned; if bit 0 of the address is HIGH this will cause unpredictable
		behaviour.

		A word store should generate a word aligned address. The word presented to the
		data bus is not affected if the address is not word aligned. That is, bit 31 of the
		register being stored always appears on data bus output 31.
		*/

		if constexpr (sizeof(Int) == 1) {
			addr &= ~3;
		}
		if constexpr (sizeof(Int) == 2) {

		}
		if constexpr (sizeof(Int) == 4) {
			addr &= ~3;
		}

		auto page = addr >> 12 & 0xFFFF;
		u8* mem_ptr = page_table_write[page];
		if (mem_ptr != nullptr) {
			std::memcpy(mem_ptr + (addr & 0xFFF) , &data, sizeof(Int));
		}
		else {

		}
	}
}