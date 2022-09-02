module PPU;

import Bus;
import Scheduler;

namespace PPU
{
	RGB AlphaBlend(RGB target_1, RGB target_2)
	{
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
				u16 bldcnt_u16 = std::bit_cast<u16>(bldcnt);
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
								return bg_color_data_begin_ptrs[layer][dot].ToRGB();
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
		return {
			.r = u8(pixel.r - pixel.r * evy),
			.g = u8(pixel.g - pixel.g * evy),
			.b = u8(pixel.b - pixel.b * evy)
		};
	}


	RGB BrightnessIncrease(RGB pixel)
	{
		return {
			.r = u8(pixel.r + (31 - pixel.r) * evy),
			.g = u8(pixel.g + (31 - pixel.g) * evy),
			.b = u8(pixel.b + (31 - pixel.b) * evy)
		};
	}


	u8 GetObjectWidth(ObjectData obj_data)
	{
		return u8((1 + obj_data.double_size_obj_disable) *
			size_x[obj_data.obj_size][obj_data.obj_shape]);
	}


	void Initialize()
	{
		//objects.resize(sizeof(ObjectData) * max_objects);
		//Scanline();
		//UpdateRotateScalingRegisters();
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


	template<std::integral Int>
	Int ReadReg(u32 addr)
	{
		auto ReadByte = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_DISPCNT:        return GetByte(dispcnt, 0);
			case Bus::ADDR_DISPCNT + 1:    return GetByte(dispcnt, 1);
			case Bus::ADDR_GREEN_SWAP:     return u8(green_swap);
			case Bus::ADDR_GREEN_SWAP + 1: return u8(0);
			case Bus::ADDR_DISPSTAT:       return GetByte(dispstat, 0);
			case Bus::ADDR_DISPSTAT + 1:   return GetByte(dispstat, 1);
			case Bus::ADDR_VCOUNT:         return ly;
			case Bus::ADDR_VCOUNT + 1:     return u8(0);
			case Bus::ADDR_BG0CNT:         return GetByte(bgcnt[0], 0);
			case Bus::ADDR_BG0CNT + 1:     return GetByte(bgcnt[0], 1);
			case Bus::ADDR_BG1CNT:         return GetByte(bgcnt[1], 0);
			case Bus::ADDR_BG1CNT + 1:     return GetByte(bgcnt[1], 1);
			case Bus::ADDR_BG2CNT:         return GetByte(bgcnt[2], 0);
			case Bus::ADDR_BG2CNT + 1:     return GetByte(bgcnt[2], 1);
			case Bus::ADDR_BG3CNT:         return GetByte(bgcnt[3], 0);
			case Bus::ADDR_BG3CNT + 1:     return GetByte(bgcnt[3], 1);
			case Bus::ADDR_WININ:          return GetByte(winin, 0);
			case Bus::ADDR_WININ + 1:      return GetByte(winin, 1);
			case Bus::ADDR_WINOUT:         return GetByte(winout, 0);
			case Bus::ADDR_WINOUT + 1:     return GetByte(winout, 1);
			case Bus::ADDR_BLDCNT:         return GetByte(bldcnt, 0);
			case Bus::ADDR_BLDCNT + 1:     return GetByte(bldcnt, 1);
			case Bus::ADDR_BLDALPHA:       return eva;
			case Bus::ADDR_BLDALPHA + 1:   return evb;
			default:                       return Bus::ReadOpenBus<u8>(addr);
			}
		};

		auto ReadHalf = [](u32 addr) {
			switch (addr) {
			case Bus::ADDR_DISPCNT:    return std::bit_cast<u16>(dispcnt);
			case Bus::ADDR_GREEN_SWAP: return u16(green_swap);
			case Bus::ADDR_DISPSTAT:   return std::bit_cast<u16>(dispstat);
			case Bus::ADDR_VCOUNT:     return u16(ly);
			case Bus::ADDR_BG0CNT:     return std::bit_cast<u16>(bgcnt[0]);
			case Bus::ADDR_BG1CNT:     return std::bit_cast<u16>(bgcnt[1]);
			case Bus::ADDR_BG2CNT:     return std::bit_cast<u16>(bgcnt[2]);
			case Bus::ADDR_BG3CNT:     return std::bit_cast<u16>(bgcnt[3]);
			case Bus::ADDR_WININ:      return std::bit_cast<u16>(winin);
			case Bus::ADDR_WINOUT:     return std::bit_cast<u16>(winout);
			case Bus::ADDR_BLDCNT:     return std::bit_cast<u16>(bldcnt);
			case Bus::ADDR_BLDALPHA:   return u16(eva | evb << 8);
			default:                   return Bus::ReadOpenBus<u16>(addr);
			}
		};

		auto ReadWord = [&](u32 addr) {
			u16 lo = ReadHalf(addr);
			u16 hi = ReadHalf(addr + 2);
			return lo | hi << 16;
		};

		if constexpr (sizeof(Int) == 1) {
			return ReadByte(addr);
		}
		if constexpr (sizeof(Int) == 2) {
			return ReadHalf(addr);
		}
		if constexpr (sizeof(Int) == 4) {
			return ReadWord(addr);
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

		Scheduler::AddEvent(Scheduler::EventType::LcdRender, 1000, Scanline);
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
					current_scanline_bg_layers[bg][dot++] = std::bit_cast<ColorData>(col);
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
		/* OBJ Attribute 0 (R/W)
			  Bit   Expl.
			  0-7   Y-Coordinate           (0-255)
			  8     Rotation/Scaling Flag  (0=Off, 1=On)
			  When Rotation/Scaling used (Attribute 0, bit 8 set):
				9     Double-Size Flag     (0=Normal, 1=Double)
			  When Rotation/Scaling not used (Attribute 0, bit 8 cleared):
				9     OBJ Disable          (0=Normal, 1=Not displayed)
			  10-11 OBJ Mode  (0=Normal, 1=Semi-Transparent, 2=OBJ Window, 3=Prohibited)
			  12    OBJ Mosaic             (0=Off, 1=On)
			  13    Colors/Palettes        (0=16/16, 1=256/1)
			  14-15 OBJ Shape              (0=Square,1=Horizontal,2=Vertical,3=Prohibited)

			OBJ Attribute 1 (R/W)
			  Bit   Expl.
			  0-8   X-Coordinate           (0-511)
			  When Rotation/Scaling used (Attribute 0, bit 8 set):
				9-13  Rotation/Scaling Parameter Selection (0-31)
					  (Selects one of the 32 Rotation/Scaling Parameters that
					  can be defined in OAM, for details read next chapter.)
			  When Rotation/Scaling not used (Attribute 0, bit 8 cleared):
				9-11  Not used
				12    Horizontal Flip      (0=Normal, 1=Mirrored)
				13    Vertical Flip        (0=Normal, 1=Mirrored)
			  14-15 OBJ Size               (0..3, depends on OBJ Shape, see Attr 0)
					  Size  Square   Horizontal  Vertical
					  0     8x8      16x8        8x16
					  1     16x16    32x8        8x32
					  2     32x32    32x16       16x32
					  3     64x64    64x32       32x64

			OBJ Attribute 2 (R/W)
			  Bit   Expl.
			  0-9   Character Name          (0-1023=Tile Number)
			  10-11 Priority relative to BG (0-3; 0=Highest)
			  12-15 Palette Number   (0-15) (Not used in 256 color/1 palette mode)
		*/
		for (uint oam_addr = 0; oam_addr < oam.size(); oam_addr += 8) {
			bool rotate_scale = oam[oam_addr + 1] & 1;
			bool double_size_obj_disable = oam[oam_addr + 1] & 2;
			if (!rotate_scale && double_size_obj_disable) {
				continue;
			}
			u8 y_coord = oam[oam_addr] & 0xFF;
			if (y_coord <= ly) {
				u8 obj_shape = oam[oam_addr + 1] >> 6 & 3;
				u8 obj_size = oam[oam_addr + 5] >> 6 & 3;
				static constexpr u8 obj_size_y[4][4] = {
					8, 8, 16, 8,
					16, 8, 32, 16,
					32, 16, 32, 32,
					64, 32, 64, 64
				};
				u8 obj_height = (1 + double_size_obj_disable) * obj_size_y[obj_size][obj_shape];
				if (y_coord + obj_height > ly) {
					ObjectData obj_data;
					std::memcpy(&obj_data, oam.data() + oam_addr, 6);
					objects.push_back(obj_data);
				}
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


	template<std::integral Int>
	void WriteReg(u32 addr, Int data)
	{
		auto WriteByte = [](u32 addr, u8 data) {
			switch (addr) {
			case Bus::ADDR_DISPCNT:      SetByte(dispcnt, 0, data); break;
			case Bus::ADDR_DISPCNT + 1:  SetByte(dispcnt, 1, data); break;
			case Bus::ADDR_GREEN_SWAP:   green_swap = data & 1; break;
			case Bus::ADDR_DISPSTAT:     SetByte(dispstat, 0, data); break;
			case Bus::ADDR_DISPSTAT + 1: SetByte(dispstat, 1, data); break;
			case Bus::ADDR_BG0CNT:       SetByte(bgcnt[0], 0, data); break;
			case Bus::ADDR_BG0CNT + 1:   SetByte(bgcnt[0], 1, data); break;
			case Bus::ADDR_BG1CNT:       SetByte(bgcnt[1], 0, data); break;
			case Bus::ADDR_BG1CNT + 1:   SetByte(bgcnt[1], 1, data); break;
			case Bus::ADDR_BG2CNT:       SetByte(bgcnt[2], 0, data); break;
			case Bus::ADDR_BG2CNT + 1:   SetByte(bgcnt[2], 1, data); break;
			case Bus::ADDR_BG3CNT:       SetByte(bgcnt[3], 0, data); break;
			case Bus::ADDR_BG3CNT + 1:   SetByte(bgcnt[3], 1, data); break;
			case Bus::ADDR_BG0HOFS:      SetByte(bghofs[0], 0, data); break;
			case Bus::ADDR_BG0HOFS + 1:  SetByte(bghofs[0], 1, data & 1); break;
			case Bus::ADDR_BG0VOFS:      SetByte(bgvofs[0], 0, data); break;
			case Bus::ADDR_BG0VOFS + 1:  SetByte(bgvofs[0], 1, data & 1); break;
			case Bus::ADDR_BG1HOFS:      SetByte(bghofs[1], 0, data); break;
			case Bus::ADDR_BG1HOFS + 1:  SetByte(bghofs[1], 1, data & 1); break;
			case Bus::ADDR_BG1VOFS:      SetByte(bgvofs[1], 0, data); break;
			case Bus::ADDR_BG1VOFS + 1:  SetByte(bgvofs[1], 1, data & 1); break;
			case Bus::ADDR_BG2HOFS:      SetByte(bghofs[2], 0, data); break;
			case Bus::ADDR_BG2HOFS + 1:  SetByte(bghofs[2], 1, data & 1); break;
			case Bus::ADDR_BG2VOFS:      SetByte(bgvofs[2], 0, data); break;
			case Bus::ADDR_BG2VOFS + 1:  SetByte(bgvofs[2], 1, data & 1); break;
			case Bus::ADDR_BG3HOFS:      SetByte(bghofs[3], 0, data); break;
			case Bus::ADDR_BG3HOFS + 1:  SetByte(bghofs[3], 1, data & 1); break;
			case Bus::ADDR_BG3VOFS:      SetByte(bgvofs[3], 0, data); break;
			case Bus::ADDR_BG3VOFS + 1:  SetByte(bgvofs[3], 1, data & 1); break;
			case Bus::ADDR_BG2PA:        SetByte(bgpa[0], 0, data); break;
			case Bus::ADDR_BG2PA + 1:    SetByte(bgpa[0], 1, data); break;
			case Bus::ADDR_BG2PB:        SetByte(bgpb[0], 0, data); break;
			case Bus::ADDR_BG2PB + 1:    SetByte(bgpb[0], 1, data); break;
			case Bus::ADDR_BG2PC:        SetByte(bgpc[0], 0, data); break;
			case Bus::ADDR_BG2PC + 1:    SetByte(bgpc[0], 1, data); break;
			case Bus::ADDR_BG2PD:        SetByte(bgpd[0], 0, data); break;
			case Bus::ADDR_BG2PD + 1:    SetByte(bgpd[0], 1, data); break;
			case Bus::ADDR_BG2X:         SetByte(bgx[0], 0, data); break;
			case Bus::ADDR_BG2X + 1:     SetByte(bgx[0], 1, data); break;
			case Bus::ADDR_BG2X + 2:     SetByte(bgx[0], 2, data); break;
			case Bus::ADDR_BG2X + 3:     SetByte(bgx[0], 3, data); break;
			case Bus::ADDR_BG2Y:         SetByte(bgy[0], 0, data); break;
			case Bus::ADDR_BG2Y + 1:     SetByte(bgy[0], 1, data); break;
			case Bus::ADDR_BG2Y + 2:     SetByte(bgy[0], 2, data); break;
			case Bus::ADDR_BG2Y + 3:     SetByte(bgy[0], 3, data); break;
			case Bus::ADDR_BG3PA:        SetByte(bgpa[1], 0, data); break;
			case Bus::ADDR_BG3PA + 1:    SetByte(bgpa[1], 1, data); break;
			case Bus::ADDR_BG3PB:        SetByte(bgpb[1], 0, data); break;
			case Bus::ADDR_BG3PB + 1:    SetByte(bgpb[1], 1, data); break;
			case Bus::ADDR_BG3PC:        SetByte(bgpc[1], 0, data); break;
			case Bus::ADDR_BG3PC + 1:    SetByte(bgpc[1], 1, data); break;
			case Bus::ADDR_BG3PD:        SetByte(bgpd[1], 0, data); break;
			case Bus::ADDR_BG3PD + 1:    SetByte(bgpd[1], 1, data); break;
			case Bus::ADDR_BG3X:         SetByte(bgx[1], 0, data); break;
			case Bus::ADDR_BG3X + 1:     SetByte(bgx[1], 1, data); break;
			case Bus::ADDR_BG3X + 2:     SetByte(bgx[1], 2, data); break;
			case Bus::ADDR_BG3X + 3:     SetByte(bgx[1], 3, data); break;
			case Bus::ADDR_BG3Y:         SetByte(bgy[1], 0, data); break;
			case Bus::ADDR_BG3Y + 1:     SetByte(bgy[1], 1, data); break;
			case Bus::ADDR_BG3Y + 2:     SetByte(bgy[1], 2, data); break;
			case Bus::ADDR_BG3Y + 3:     SetByte(bgy[1], 3, data); break;
			case Bus::ADDR_WIN0H:        winh_x2[0] = data; break;
			case Bus::ADDR_WIN0H + 1:    winh_x1[0] = data; break;
			case Bus::ADDR_WIN1H:        winh_x2[1] = data; break;
			case Bus::ADDR_WIN1H + 1:    winh_x1[1] = data; break;
			case Bus::ADDR_WIN0V:        winv_x2[0] = data; break;
			case Bus::ADDR_WIN0V + 1:    winv_x1[0] = data; break;
			case Bus::ADDR_WIN1V:        winv_x2[1] = data; break;
			case Bus::ADDR_WIN1V + 1:    winv_x1[1] = data; break;
			case Bus::ADDR_WININ:        SetByte(winin, 0, data); break;
			case Bus::ADDR_WININ + 1:    SetByte(winin, 1, data); break;
			case Bus::ADDR_WINOUT:       SetByte(winout, 0, data); break;
			case Bus::ADDR_WINOUT + 1:   SetByte(winout, 1, data); break;
			case Bus::ADDR_MOSAIC:       SetByte(mosaic, 0, data); break;
			case Bus::ADDR_MOSAIC + 1:   SetByte(mosaic, 1, data); break;
			case Bus::ADDR_BLDCNT:       SetByte(bldcnt, 0, data); break;
			case Bus::ADDR_BLDCNT + 1:   SetByte(bldcnt, 1, data); break;
			case Bus::ADDR_BLDALPHA:     eva = data & 0x1F; break;
			case Bus::ADDR_BLDALPHA + 1: evb = data & 0x1F; break;
			case Bus::ADDR_BLDY:         evy = data & 0x1F; break;
			default: break;
			}
		};

		auto WriteHalf = [](u32 addr, u16 data) {
			switch (addr) {
			case Bus::ADDR_DISPCNT:      dispcnt = std::bit_cast<DISPCNT>(data); break;
			case Bus::ADDR_GREEN_SWAP:   green_swap = data & 1; break;
			case Bus::ADDR_DISPSTAT:     dispstat = std::bit_cast<DISPSTAT>(data); break;
			case Bus::ADDR_BG0CNT:       bgcnt[0] = std::bit_cast<BGCNT>(data); break;
			case Bus::ADDR_BG1CNT:       bgcnt[1] = std::bit_cast<BGCNT>(data); break;
			case Bus::ADDR_BG2CNT:       bgcnt[2] = std::bit_cast<BGCNT>(data); break;
			case Bus::ADDR_BG3CNT:       bgcnt[3] = std::bit_cast<BGCNT>(data); break;
			case Bus::ADDR_BG0HOFS:      bghofs[0] = data & 0x1FF; break;
			case Bus::ADDR_BG0VOFS:      bgvofs[0] = data & 0x1FF; break;
			case Bus::ADDR_BG1HOFS:      bghofs[1] = data & 0x1FF; break;
			case Bus::ADDR_BG1VOFS:      bgvofs[1] = data & 0x1FF; break;
			case Bus::ADDR_BG2HOFS:      bghofs[2] = data & 0x1FF; break;
			case Bus::ADDR_BG2VOFS:      bgvofs[2] = data & 0x1FF; break;
			case Bus::ADDR_BG3HOFS:      bghofs[3] = data & 0x1FF; break;
			case Bus::ADDR_BG3VOFS:      bgvofs[3] = data & 0x1FF; break;
			case Bus::ADDR_BG2PA:        bgpa[0] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG2PB:        bgpb[0] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG2PC:        bgpc[0] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG2PD:        bgpd[0] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG2X:         SetByte(bgx[0], 0, data & 0xFF); SetByte(bgx[0], 1, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG2X + 2:     SetByte(bgx[0], 2, data & 0xFF); SetByte(bgx[0], 3, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG2Y:         SetByte(bgy[0], 0, data & 0xFF); SetByte(bgy[0], 1, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG2Y + 2:     SetByte(bgy[0], 2, data & 0xFF); SetByte(bgy[0], 3, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG3PA:        bgpa[1] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG3PB:        bgpb[1] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG3PC:        bgpc[1] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG3PD:        bgpd[1] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG3X:         SetByte(bgx[1], 0, data & 0xFF); SetByte(bgx[1], 1, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG3X + 2:     SetByte(bgx[1], 2, data & 0xFF); SetByte(bgx[1], 3, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG3Y:         SetByte(bgy[1], 0, data & 0xFF); SetByte(bgy[1], 1, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG3Y + 2:     SetByte(bgy[1], 2, data & 0xFF); SetByte(bgy[1], 3, data >> 8 & 0xFF); break;
			case Bus::ADDR_WIN0H:        winh_x2[0] = data & 0xFF; winh_x1[0] = data >> 8 & 0xFF; break;
			case Bus::ADDR_WIN1H:        winh_x2[1] = data & 0xFF; winh_x1[1] = data >> 8 & 0xFF; break;
			case Bus::ADDR_WIN0V:        winv_x2[0] = data & 0xFF; winv_x1[0] = data >> 8 & 0xFF; break;
			case Bus::ADDR_WIN1V:        winv_x2[1] = data & 0xFF; winv_x1[1] = data >> 8 & 0xFF; break;
			case Bus::ADDR_WININ:        winin = std::bit_cast<WININ>(data); break;
			case Bus::ADDR_WINOUT:       winout = std::bit_cast<WINOUT>(data); break;
			case Bus::ADDR_MOSAIC:       mosaic = std::bit_cast<MOSAIC>(data); break;
			case Bus::ADDR_BLDCNT:       bldcnt = std::bit_cast<BLDCNT>(data); break;
			case Bus::ADDR_BLDALPHA:     eva = data & 0x1F; evb = data >> 8 & 0x1F; break;
			case Bus::ADDR_BLDY:         evy = data & 0x1F; break;
			default: break;
			}
		};

		auto WriteWord = [&](u32 addr, u32 data) {
			WriteHalf(addr, data & 0xFFFF);
			WriteHalf(addr + 2, data >> 16 & 0xFFFF);
		};

		if constexpr (sizeof(Int) == 1) {
			WriteByte(addr, data);
		}
		if constexpr (sizeof(Int) == 2) {
			WriteHalf(addr, data);
		}
		if constexpr (sizeof(Int) == 4) {
			WriteWord(addr, data);
		}
	}


	template u8 ReadReg<u8>(u32);
	template s8 ReadReg<s8>(u32);
	template u16 ReadReg<u16>(u32);
	template s16 ReadReg<s16>(u32);
	template u32 ReadReg<u32>(u32);
	template s32 ReadReg<s32>(u32);
	template void WriteReg<u8>(u32, u8);
	template void WriteReg<s8>(u32, s8);
	template void WriteReg<u16>(u32, u16);
	template void WriteReg<s16>(u32, s16);
	template void WriteReg<u32>(u32, u32);
	template void WriteReg<s32>(u32, s32);
}