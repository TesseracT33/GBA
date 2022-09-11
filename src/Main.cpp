import Core;
import Frontend;
import GBA;

import <memory>;

int main(int argc, char** argv) /* Optional CLI argument: path to rom */
{
	std::shared_ptr<Core> core = std::make_shared<GBA>();
	if (!Frontend::Initialize(core)) {
		exit(1);
	}
	bool boot_game_immediately = false;
	if (argc >= 2) {
		auto rom_path = argv[1];
		Frontend::LoadGame(rom_path);
		boot_game_immediately = true;
	}
	Frontend::RunGui(boot_game_immediately);
	Frontend::Shutdown();
	return 0;
}