module PPU;

import Util.Bit;

namespace PPU
{
	RGB AlphaBlend(RGB target_1, RGB target_2)
	{
		auto eva = bldalpha.eva_coeff;
		auto evb = bldalpha.evb_coeff;
		return {
			.r = u8(std::min(31, target_1.r * eva + target_2.r * evb)),
			.g = u8(std::min(31, target_1.g * eva + target_2.g * evb)),
			.b = u8(std::min(31, target_1.b * eva + target_2.b * evb))
		};
	}


	void BlendBackgrounds()
	{
		/* Sort backgrounds after priority */
		static constexpr std::array<ColorData*, 4> bg_color_data_begin_ptrs_default = {
			current_scanline_bg_layers[0].data(),
			current_scanline_bg_layers[1].data(),
			current_scanline_bg_layers[2].data(),
			current_scanline_bg_layers[3].data()
		};
		std::array<ColorData*, 4> bg_color_data_begin_ptrs = bg_color_data_begin_ptrs_default;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 3; ++j) {
				if (bgcnt[j].bg_priority < bgcnt[j + 1].bg_priority) {
					std::swap(bg_color_data_begin_ptrs[j], bg_color_data_begin_ptrs[j + 1]);
				}
			}
		}
		/* Blend backgrounds, pixel by pixel */
		for (uint dot = 0; dot < dots_per_line; ++dot) {
			/* Find the non-transparent layer with highest priority */
			int layer = 0;
			for (; layer < 4; ++layer) {
				if (!bg_color_data_begin_ptrs[layer][dot].transparent) {
					break;
				}
			}
			if (layer == 4) {
				/* No non-transparent layer found */
				PushPixel(0xFF, 0xFF, 0xFF);
				continue;
			}
			int topmost_layer = layer;
			RGB rgb = bg_color_data_begin_ptrs[topmost_layer][dot].ToRGB();
			if (bldcnt.color_special_effect != 0) {
				u16 bldcnt_u16 = std::bit_cast<u16, decltype(bldcnt)>(bldcnt);
				switch (bldcnt.color_special_effect) {
				case 1: /* Alpha Blending (1st+2nd Target mixed) */
					if (bldcnt_u16 & (1 << topmost_layer | 1 << (topmost_layer + 8))) {
						/* Find the non-transparent layer with 2nd highest priority */
						int layer = 0;
						for (; layer < 4; ++layer) {
							if (layer != topmost_layer && !bg_color_data_begin_ptrs[layer][dot].transparent) {
								break;
							}
						}
						RGB second_rgb = [&] {
							if (layer == 4) {
								// TODO 
							}
							else {
								return bg_color_data_begin_ptrs[layer][dot].ToRGB();
							}
						}();
						rgb = AlphaBlend(rgb, second_rgb);
					}
					break;

				case 2: /* Brightness Increase (1st Target becomes whiter) */
					if (bldcnt_u16 & 1 << topmost_layer) {
						rgb = BrightnessIncrease(rgb);
					}
					break;

				case 3: /* Brightness Decrease (1st Target becomes blacker) */
					if (bldcnt_u16 & 1 << topmost_layer) {
						rgb = BrightnessDecrease(rgb);
					}
					break;

				default:
					std::unreachable();
				}
			}
			PushPixel(rgb);
		}
	}


	RGB BrightnessDecrease(RGB pixel)
	{
		auto evy = bldy.evy_coeff;
		return {
			.r = u8(pixel.r - pixel.r * evy),
			.g = u8(pixel.g - pixel.g * evy),
			.b = u8(pixel.b - pixel.b * evy)
		};
	}


	RGB BrightnessIncrease(RGB pixel)
	{
		auto evy = bldy.evy_coeff;
		return {
			.r = u8(pixel.r + (31 - pixel.r) * evy),
			.g = u8(pixel.g + (31 - pixel.g) * evy),
			.b = u8(pixel.b + (31 - pixel.b) * evy)
		};
	}


	u8 GetObjectHeight(ObjectData obj_data)
	{
		return (1 + obj_data.double_size_obj_disable) *
			size_y[obj_data.obj_size][obj_data.obj_shape];
	}


	u8 GetObjectWidth(ObjectData obj_data)
	{
		return (1 + obj_data.double_size_obj_disable) *
			size_x[obj_data.obj_size][obj_data.obj_shape];
	}


	void Initialize()
	{
		objects.resize(sizeof(ObjectData) * max_objects);
		Scanline();
		UpdateRotateScalingRegisters();
	}


	void PushPixel(ColorData color_data)
	{
		framebuffer[framebuffer_index++] = color_data.r;
		framebuffer[framebuffer_index++] = color_data.g;
		framebuffer[framebuffer_index++] = color_data.b;
	}


	void PushPixel(RGB rgb)
	{
		framebuffer[framebuffer_index++] = rgb.r;
		framebuffer[framebuffer_index++] = rgb.g;
		framebuffer[framebuffer_index++] = rgb.b;
	}


	void PushPixel(u8 r, u8 g, u8 b)
	{
		framebuffer[framebuffer_index++] = r;
		framebuffer[framebuffer_index++] = g;
		framebuffer[framebuffer_index++] = b;
	}


	u8 ReadOam(u32 addr)
	{
		if ((in_hblank && dispcnt.hblank_interval_free) || in_vblank || dispcnt.forced_blank) {
			addr &= 0x3FF;
			return oam[addr & 0x3FF];
		}
		else {
			return 0xFF;
		}
	}


	u8 ReadPaletteRam(u32 addr)
	{
		if (in_hblank || in_vblank || dispcnt.forced_blank) {
			return palette_ram[addr & 0x3FF];
		}
		else {
			return 0xFF;
		}
	}


	u8 ReadVram(u32 addr)
	{
		if (in_hblank || in_vblank || dispcnt.forced_blank) {
			return vram[addr & 0x17FFF];
		}
		else {
			return 0xFF;
		}
	}


	void Scanline()
	{
		if (dispcnt.forced_blank) {
			/* output all white pixels */
			std::memset(framebuffer.data() + framebuffer_index, 0xFF, framebuffer_pitch);
			framebuffer_index += framebuffer_pitch;
			return;
		}

		switch (dispcnt.bg_mode) {
		case 0:
			if (dispcnt.screen_display_bg0) {
				ScanlineBackgroundTextMode(0);
			}
			if (dispcnt.screen_display_bg1) {
				ScanlineBackgroundTextMode(1);
			}
			if (dispcnt.screen_display_bg2) {
				ScanlineBackgroundTextMode(2);
			}
			if (dispcnt.screen_display_bg3) {
				ScanlineBackgroundTextMode(3);
			}
			break;

		case 1:
			if (dispcnt.screen_display_bg0) {
				ScanlineBackgroundTextMode(0);
			}
			if (dispcnt.screen_display_bg1) {
				ScanlineBackgroundTextMode(1);
			}
			if (dispcnt.screen_display_bg2) {
				ScanlineBackgroundRotateScaleMode(2);
			}
			break;

		case 2:
			if (dispcnt.screen_display_bg2) {
				ScanlineBackgroundRotateScaleMode(2);
			}
			if (dispcnt.screen_display_bg3) {
				ScanlineBackgroundRotateScaleMode(3);
			}
			break;

		case 3:
			if (dispcnt.screen_display_bg2) {
				ScanlineBackgroundBitmapMode3();
			}
			break;

		case 4:
			if (dispcnt.screen_display_bg2) {
				ScanlineBackgroundBitmapMode4();
			}
			break;

		case 5:
			if (dispcnt.screen_display_bg2) {
				ScanlineBackgroundBitmapMode5();
			}
			break;

		case 6:
		case 7:
			break;

		default:
			std::unreachable();
		}

		if (dispcnt.screen_display_obj) {
			ScanlineObjects();
		}

		BlendBackgrounds();
	}


	void ScanlineBackgroundRotateScaleMode(uint bg)
	{

	}


	void ScanlineBackgroundTextMode(uint bg)
	{
		static constexpr std::array<uint, 4> screen_size_to_bg_width_text_mode = { 256, 512, 256, 512 };
		static constexpr std::array<uint, 4> screen_size_to_bg_height_text_mode = { 256, 256, 512, 512 };

		const uint bg_width = screen_size_to_bg_width_text_mode[bgcnt[bg].screen_size];
		const uint bg_height = screen_size_to_bg_height_text_mode[bgcnt[bg].screen_size];
		const uint bg_tile_index_y = ((bgvofs[bg] + ly) & (bg_height - 1)) / 8;
		const uint bg_map_base_addr = bgcnt[bg].screen_base_block * 0x800 + bg_height / 4 * bg_tile_index_y;
		uint bg_tile_index_x = (bghofs[bg] & (bg_width - 1)) / 8;
		uint dot = 0;

		enum class TilePosition {
			LeftEdge, Middle, RightEdge
		};

		auto FetchTile = [&] <TilePosition tile_pos> {
			static constexpr int pixels_to_ignore_left = [&] {
				if constexpr (tile_pos == TilePosition::LeftEdge) {
					return bghofs[bg].offset % 8;
				}
				else {
					return 0;
				}
			}();
			static constexpr int pixels_to_ignore_right = [&] {
				if constexpr (tile_pos == TilePosition::RightEdge) {
					return 8 - pixels_to_ignore_left;
				}
				else {
					return 0;
				}
			}();
			if constexpr (tile_pos == TilePosition::RightEdge && pixels_to_ignore_right == 8) {
				return;
			}
			const uint bg_map_addr = bg_map_base_addr + 2 * bg_tile_index_x;
			const uint tile_num = (vram[bg_map_addr] | vram[bg_map_addr + 1] << 8) & 0x3FF;
			const bool flip_x = vram[bg_map_addr + 1] & 4;
			const uint delta_y = [&] {
				const bool flip_y = vram[bg_map_addr + 1] & 8;
				u8 delta_y = (ly + bgvofs[bg].offset) % 8;
				if (flip_y) {
					delta_y = 7 - delta_y;
				}
				return delta_y;
			}();
			uint tile_data_addr = bgcnt[bg].char_base_block * 0x4000; /* 0-0xC000 */
			/* 4-bit depth (16 colors, 16 palettes). Each tile occupies 32 bytes of memory, the first 4 bytes for the topmost row of the tile, and so on.
			Each byte representing two dots, the lower 4 bits define the color for the left (!) dot, the upper 4 bits the color for the right dot. */
			if (bgcnt[bg].palette_mode == 0) {
				tile_data_addr += 32 * tile_num + 4 * delta_y;
				u8 palette_number = vram[bg_map_addr + 1] >> 4;
				const u8* base_palette_start = palette_ram.data() + 16 * palette_number;
				for (int i = 0; i < 4; ++i, ++tile_data_addr) {
					u8 right_col_id = vram[tile_data_addr] & 0xF;
					u8 left_col_id = vram[tile_data_addr] >> 4;
					u16 right_col, left_col;
					std::memcpy(&right_col, base_palette_start + 2 * right_col_id, 2);
					std::memcpy(&left_col, base_palette_start + 2 * left_col_id, 2);
					current_scanline_bg_layers[bg][dot++] = left_col;
					current_scanline_bg_layers[bg][dot++] = right_col;
				}
			}
			/* 8-bit depth (256 colors, 1 palette). Each tile occupies 64 bytes of memory, the first 8 bytes for the topmost row of the tile, and so on.
			Each byte selects the palette entry for each dot. */
			else {
				tile_data_addr += 64 * tile_num + 8 * delta_y;
				auto PushPixel = [&](uint pixel_index) {
					u8 palette_number = vram[tile_data_addr + pixel_index];
					u16 col = palette_ram[palette_number] | palette_ram[palette_number + 1] << 8;
					col &= 0x7FFF;
					col |= (palette_number == 0) << 15;
					current_scanline_bg_layers[bg][dot++] = std::bit_cast<ColorData, u16>(col);
				};
				if (flip_x) {
					for (int i = 7 - pixels_to_ignore_right; i >= pixels_to_ignore_left; --i) {
						PushPixel(i);
					}
				}
				else {
					for (int i = pixels_to_ignore_left; i < 8 - pixels_to_ignore_right; ++i) {
						PushPixel(i);
					}
				}
			}
			bg_tile_index_x++;
			bg_tile_index_x &= (bg_width - 1) / 8;
		};

		//FetchTile.template operator() < TilePosition::LeftEdge > ();
		//for (int i = 0; i <= 28; ++i) {
		//	FetchTile.template operator() < TilePosition::Middle > ();
		//}
		//FetchTile.template operator() < TilePosition::RightEdge > ();
	}


	void ScanlineBackgroundBitmapMode3()
	{
		for (uint dot = 0; dot < dots_per_line; ++dot) {
			uint color_index = 2 * dot;
			ColorData color_data;
			std::memcpy(&color_data, vram.data() + 480 * ly + color_index, 2);
			PushPixel(color_data);
		}
	}


	void ScanlineBackgroundBitmapMode4()
	{
		uint vram_frame_offset = dispcnt.display_frame_select ? 0x9600 : 0;
		for (uint dot = 0; dot < dots_per_line; ++dot) {
			uint color_index = dot;
			uint palette_index = vram[vram_frame_offset + color_index];
			ColorData color_data;
			std::memcpy(&color_data, palette_ram.data() + 2 * palette_index, 2);
			PushPixel(color_data);
		}
	}


	void ScanlineBackgroundBitmapMode5()
	{
		uint vram_frame_offset = dispcnt.display_frame_select ? 0xA000 : 0;
		uint dot = 0;
		for (; dot < 40; ++dot) { /* TODO: don't know about this */
			PushPixel(0xFF, 0xFF, 0xFF);
		}
		for (; dot < 200; ++dot) {
			uint color_index = 2 * dot;
			ColorData color_data;
			std::memcpy(&color_data, vram.data() + 480 * ly + color_index, 2);
			PushPixel(color_data);
		}
		for (; dot < dots_per_line; ++dot) {
			PushPixel(0xFF, 0xFF, 0xFF);
		}
	}


	void ScanlineObjects()
	{
		uint current_obj_oam_index = 0;

		auto FetchSprite = [&](ObjectData obj_data) {

		};

		for (uint dot = 0; dot < dots_per_line; ++dot) {
			/* Find sprite. The lower the oam index, the higher the priority. */
			for (ObjectData obj_data : objects) {
				if (obj_data.x_coord <= dot) {
					auto obj_width = GetObjectWidth(obj_data);
					if (obj_data.x_coord + obj_width > dot) {

					}
				}
			}
		}
	}


	void ScanOam()
	{
		for (uint oam_addr = 0; oam_addr < oam.size(); oam_addr += 8) {
			bool rotate_scale = oam[oam_addr + 1] & 1;
			if (!rotate_scale) {
				bool obj_disable = oam[oam_addr + 1] & 2;
				if (obj_disable) {
					continue;
				}
			}
			u8 y_coord = oam[oam_addr] & 0xFF;
			if (y_coord <= ly) {
				//u8 obj_height = GetObjectHeight();
				//if (y_coord + obj_height > ly) {
				//	ObjectData obj_data;
				//	std::memcpy(&obj_data, oam.data() + oam_addr, 3);
				//	objects.push_back(obj_data);
				//}
			}
		}
	}


	void Step()
	{
		++cycle;
		if (cycle == cycles_until_hblank) {
			in_hblank = true;
		}
		else if (cycle == cycles_until_set_hblank_flag) {
			dispstat.hblank = 1;
			if (!in_vblank && dispstat.hblank_irq_enable) {
				/* TODO IRQ */
			}
		}
		else if (cycle == cycles_per_line) {
			cycle = 0;
			dispstat.hblank = 0;
			in_hblank = false;

			++ly;
			bool prev_v_counter = dispstat.v_counter;
			dispstat.v_counter = ly == dispstat.v_count_setting; /* TODO: 0..227? */
			if (dispstat.v_counter && !prev_v_counter) {
				if (dispstat.v_counter_irq_enable) {
					/* TODO IRQ */
				}
			}
			if (ly < lines_until_vblank) {
				Scanline();
				UpdateRotateScalingRegisters();
			}
			else if (ly == lines_until_vblank) {
				dispstat.vblank = 1;
				in_vblank = true;
				if (dispstat.vblank_irq_enable) {
					/* TODO IRQ */
				}
				framebuffer_index = 0;
			}
			else if (ly == total_num_lines - 1) {
				dispstat.vblank = 0;
				in_vblank = false;
				//bg_rot_coord_x = 0x0FFF'FFFF & std::bit_cast<u64, decltype(bgx)>(bgx);
				//bg_rot_coord_y = 0x0FFF'FFFF & std::bit_cast<u64, decltype(bgy)>(bgy);
			}
			else {
				ly %= total_num_lines; /* cheaper than branch on ly == total_num_lines to set ly = 0? */
			}
		}
	}


	void StreamState(SerializationStream& stream)
	{

	}


	void UpdateRotateScalingRegisters()
	{
		//s16 dmx = 0;
		//bg_rot_coord_x = (bg_rot_coord_x + dmx) & 0x0FFF'FFF;
		//s16 dmy = 0;
		//bg_rot_coord_y = (bg_rot_coord_y + dmy) & 0x0FFF'FFF;
	}


	void WriteOam(u32 addr)
	{

	}


	void WritePaletteRam(u32 addr)
	{

	}


	void WriteVram(u32 addr)
	{

	}
}