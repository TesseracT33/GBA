import Core;
import Frontend;
import GBA;

import <memory>;

int main(int argc, char** argv) /* Optional CLI arguments: path to rom, path to bios */
{
	std::shared_ptr<Core> core = std::make_shared<GBA>();
	if (!Frontend::Initialize(core)) {
		exit(1);
	}
	bool boot_game_immediately = false;
	if (argc >= 2) {
		auto rom_path = argv[1];
		bool success = Frontend::LoadGame(rom_path);
		boot_game_immediately = success;
		if (argc >= 3) {
			auto bios_path = argv[2];
			Frontend::LoadBios(bios_path);
		}
	}
	Frontend::RunGui(boot_game_immediately);
	Frontend::Shutdown();
	return 0;
}