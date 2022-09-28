module PPU;

import Bus;
import DMA;
import IRQ;
import Scheduler;
import Video;

namespace PPU
{
	void AddInitialEvents()
	{
		Scheduler::AddEvent(Scheduler::EventType::HBlank, cycles_until_hblank, OnHBlank);
	}


	RGB AlphaBlend(RGB target_1, RGB target_2)
	{
		return {
			.r = u8(std::min(31, target_1.r * eva + target_2.r * evb)),
			.g = u8(std::min(31, target_1.g * eva + target_2.g * evb)),
			.b = u8(std::min(31, target_1.b * eva + target_2.b * evb))
		};
	}


	void BlendLayers()
	{
		const bool win0_enable = dispcnt.win0_display;
		const bool win1_enable = dispcnt.win1_display;
		const bool obj_win_enable = dispcnt.obj_win_display;
		const bool scanline_falls_on_win0 = win0_enable && v_counter >= winv_y1[0] && v_counter < winv_y2[0];
		const bool scanline_falls_on_win1 = win1_enable && v_counter >= winv_y1[1] && v_counter < winv_y2[1];
		bool win0_active = false;
		bool win1_active = false;
		bool obj_win_active = false;
		uint bg_skip;

		auto GetNextTopmostOpaqueBgLayer = [&](uint dot) -> int {
			auto it = std::find_if(bg_by_prio.begin() + bg_skip, bg_by_prio.end(), [&](int bg) {
				++bg_skip;
				if (!GetBit(dispcnt.screen_display_bg, bg) || bg_render[bg][dot].transparent) {
					return false;
				}
				if (win0_active) {
					return GetBit(winin.window0_bg_enable, bg);
				}
				if (win1_active) {
					return GetBit(winin.window1_bg_enable, bg);
				}
				if (obj_win_active) {
					return GetBit(winout.obj_window_bg_enable, bg);
				}
				if (win0_enable || win1_enable || obj_win_enable) {
					return GetBit(winout.outside_bg_enable, bg);
				}
				return true;
			});
			return it != bg_by_prio.end() ? *it : 4;
		};

		for (uint dot = 0; dot < dots_per_line; ++dot) {
			bg_skip = 0;
			if (scanline_falls_on_win0) {
				win0_active = dot >= winh_x1[0] && dot < winh_x2[0];
			}
			if (scanline_falls_on_win1) {
				win1_active = dot >= winh_x1[1] && dot < winh_x2[1];
			}
			if (obj_win_enable) {
				obj_win_active = obj_render[dot].obj_mode == obj_mode_obj_window_mask && !obj_render[dot].transparent;
			}
			int topmost_opaque_bg = GetNextTopmostOpaqueBgLayer(dot);
			int fx_1st_target_index;
			RGB rgb_1st_layer;
			if (obj_win_active) {
				/* objects are not displayed */
				/* TODO: are objects displayed if win0 or win1 are also active? */
				if (topmost_opaque_bg < 4) {
					fx_1st_target_index = topmost_opaque_bg;
					rgb_1st_layer = bg_render[topmost_opaque_bg][dot].ToRGB();
				}
				else {
					fx_1st_target_index = fx_backdrop_index;
					rgb_1st_layer = GetBackdropColor().ToRGB();
				}
				bool special_effect_enable = [&] {
					if (bldcnt.color_special_effect == fx_disable_mask) return false;
					if (win0_active) return bool(winin.window0_color_special_effect);
					if (win1_active) return bool(winin.window1_color_special_effect);
					return bool(winout.obj_window_color_special_effect);
				}();
				if (special_effect_enable) {
					const u16 bldcnt_u16 = std::bit_cast<u16>(bldcnt);
					switch (bldcnt.color_special_effect) {
					case fx_alpha_blending_mask: { /* 1st+2nd Target mixed */
						int second_topmost_opaque_bg = GetNextTopmostOpaqueBgLayer(dot);
						int fx_2nd_target_index;
						RGB rgb_2nd_layer;
						if (second_topmost_opaque_bg < 4) {
							fx_2nd_target_index = second_topmost_opaque_bg;
							rgb_2nd_layer = bg_render[second_topmost_opaque_bg][dot].ToRGB();
						}
						else {
							fx_2nd_target_index = fx_backdrop_index;
							rgb_2nd_layer = GetBackdropColor().ToRGB();
						}
						if (GetBit(bldcnt_u16, fx_1st_target_index) &&
							GetBit(bldcnt_u16, fx_2nd_target_index + 8)) {
							rgb_1st_layer = AlphaBlend(rgb_1st_layer, rgb_2nd_layer);
						}
						break;
					}

					case fx_brightness_increase_mask: /* 1st Target becomes whiter */
						if (GetBit(bldcnt_u16, fx_1st_target_index)) {
							rgb_1st_layer = BrightnessIncrease(rgb_1st_layer);
						}
						break;

					case fx_brightness_decrease_mask: /* 1st Target becomes blacker */
						if (GetBit(bldcnt_u16, fx_1st_target_index)) {
							rgb_1st_layer = BrightnessDecrease(rgb_1st_layer);
						}
						break;

					default:
						std::unreachable();
					}
				}
			}
			else {
				/* objects are displayed */
				auto ChooseBgOrObj = [&](int bg_index, bool& obj_chosen, RGB& rgb, int& fx_target_index) -> void {
					if (obj_render[dot].transparent) {
						obj_chosen = false;
						if (bg_index < 4) {
							fx_target_index = bg_index;
							rgb = bg_render[bg_index][dot].ToRGB();
						}
						else {
							fx_target_index = fx_backdrop_index;
							rgb = GetBackdropColor().ToRGB();
						}
					}
					else if (bg_index == 4 || bgcnt[bg_index].bg_priority >= obj_render[dot].priority) {
						obj_chosen = true;
						fx_target_index = fx_obj_index;
						rgb = obj_render[dot].ToRGB();
					}
					else {
						obj_chosen = false;
						fx_target_index = bg_index;
						rgb = bg_render[bg_index][dot].ToRGB();
					}
				};

				bool top_layer_is_obj;
				ChooseBgOrObj(topmost_opaque_bg, top_layer_is_obj, rgb_1st_layer, fx_1st_target_index);

				if (obj_render[dot].obj_mode == obj_mode_semi_transparent_mask) {
					/* OBJs that are defined as 'Semi-Transparent' in OAM memory are always selected as 1st Target (regardless of BLDCNT Bit 4),
						and are always using Alpha Blending mode (regardless of BLDCNT Bit 6-7). */
					int fx_2nd_target_index;
					RGB rgb_2nd_layer;
					if (top_layer_is_obj) {
						if (topmost_opaque_bg < 4) {
							fx_2nd_target_index = topmost_opaque_bg;
							rgb_2nd_layer = bg_render[topmost_opaque_bg][dot].ToRGB();
						}
						else {
							fx_2nd_target_index = fx_backdrop_index;
							rgb_2nd_layer = GetBackdropColor().ToRGB();
						}
						if (GetBit(std::bit_cast<u16>(bldcnt), fx_2nd_target_index)) {
							rgb_1st_layer = AlphaBlend(rgb_1st_layer, rgb_2nd_layer);
						}
					}
					else {
						bool second_layer_is_obj;
						ChooseBgOrObj(GetNextTopmostOpaqueBgLayer(dot), second_layer_is_obj, rgb_2nd_layer, fx_2nd_target_index);
						if (second_layer_is_obj) {
							if (GetBit(std::bit_cast<u16>(bldcnt), fx_1st_target_index + 8)) {
								rgb_2nd_layer = AlphaBlend(rgb_2nd_layer, rgb_1st_layer);
							}
						}
					}
				}
				else {
					bool special_effect_enable = [&] {
						if (bldcnt.color_special_effect == fx_disable_mask) return false;
						if (win0_active)                return bool(winin.window0_color_special_effect);
						if (win1_active)                return bool(winin.window1_color_special_effect);
						if (win0_enable || win1_enable) return bool(winout.outside_color_special_effect);
						return true;
					}();
					if (special_effect_enable) {
						const u16 bldcnt_u16 = std::bit_cast<u16>(bldcnt);
						switch (bldcnt.color_special_effect) {
						case fx_alpha_blending_mask: { /* 1st+2nd Target mixed */
							int fx_2nd_target_index;
							RGB rgb_2nd_layer;
							if (top_layer_is_obj) {
								if (topmost_opaque_bg < 4) {
									fx_2nd_target_index = topmost_opaque_bg;
									rgb_2nd_layer = bg_render[topmost_opaque_bg][dot].ToRGB();
								}
								else {
									fx_2nd_target_index = fx_backdrop_index;
									rgb_2nd_layer = GetBackdropColor().ToRGB();
								}
							}
							else {
								ChooseBgOrObj(GetNextTopmostOpaqueBgLayer(dot), top_layer_is_obj, rgb_2nd_layer, fx_2nd_target_index);
							}
							if (GetBit(bldcnt_u16, fx_1st_target_index) &&
								GetBit(bldcnt_u16, fx_2nd_target_index + 8)) {
								rgb_1st_layer = AlphaBlend(rgb_1st_layer, rgb_2nd_layer);
							}
							break;
						}

						case fx_brightness_increase_mask: /* 1st Target becomes whiter */
							if (GetBit(bldcnt_u16, fx_1st_target_index)) {
								rgb_1st_layer = BrightnessIncrease(rgb_1st_layer);
							}
							break;

						case fx_brightness_decrease_mask: /* 1st Target becomes blacker */
							if (GetBit(bldcnt_u16, fx_1st_target_index)) {
								rgb_1st_layer = BrightnessDecrease(rgb_1st_layer);
							}
							break;

						default:
							std::unreachable();
						}
					}
				}
			}
			PushPixel(Rgb555ToRgb888(rgb_1st_layer));
		}
	}


	RGB BrightnessDecrease(RGB pixel)
	{
		return {
			.r = u8(std::max(0, pixel.r - pixel.r * evy)),
			.g = u8(std::max(0, pixel.g - pixel.g * evy)),
			.b = u8(std::max(0, pixel.b - pixel.b * evy))
		};
	}


	RGB BrightnessIncrease(RGB pixel)
	{
		return {
			.r = u8(std::min(31, pixel.r + (31 - pixel.r) * evy)),
			.g = u8(std::min(31, pixel.g + (31 - pixel.g) * evy)),
			.b = u8(std::min(31, pixel.b + (31 - pixel.b) * evy))
		};
	}


	BgColorData GetBackdropColor()
	{
		BgColorData col;
		std::memcpy(&col, palette_ram.data(), sizeof(BgColorData));
		col.transparent = false;
		return col;
	}


	void Initialize()
	{
		std::memset(&dispcnt, 0, sizeof(dispcnt));
		std::memset(&green_swap, 0, sizeof(green_swap));
		std::memset(&dispstat, 0, sizeof(dispstat));
		std::memset(&v_counter, 0, sizeof(v_counter));
		for (int i = 0; i < 4; ++i) {
			std::memset(&bgcnt[i], 0, sizeof(bgcnt[i]));
			std::memset(&bghofs[i], 0, sizeof(bghofs[i]));
			std::memset(&bgvofs[i], 0, sizeof(bgvofs[i]));
		}
		for (int i = 0; i < 2; ++i) {
			std::memset(&bgpa[i], 0, sizeof(bgpa[i]));
			std::memset(&bgpb[i], 0, sizeof(bgpb[i]));
			std::memset(&bgpc[i], 0, sizeof(bgpc[i]));
			std::memset(&bgpd[i], 0, sizeof(bgpd[i]));
			std::memset(&bgx[i], 0, sizeof(bgx[i]));
			std::memset(&bgy[i], 0, sizeof(bgy[i]));
			std::memset(&winh_x1[i], 0, sizeof(winh_x1[i]));
			std::memset(&winh_x2[i], 0, sizeof(winh_x2[i]));
			std::memset(&winv_y1[i], 0, sizeof(winv_y1[i]));
			std::memset(&winv_y2[i], 0, sizeof(winv_y2[i]));
		}
		std::memset(&winin, 0, sizeof(winin));
		std::memset(&winout, 0, sizeof(winout));
		std::memset(&mosaic, 0, sizeof(mosaic));
		std::memset(&bldcnt, 0, sizeof(bldcnt));
		std::memset(&eva, 0, sizeof(eva));
		std::memset(&evb, 0, sizeof(evb));
		std::memset(&evy, 0, sizeof(evy));
		in_hblank = in_vblank = false;
		cycle = dot = framebuffer_index = 0;
		oam.fill(0);
		palette_ram.fill(0);
		vram.fill(0);
		objects.clear();
		objects.reserve(128);
		obj_render_jobs.clear();

		framebuffer.resize(framebuffer_width * framebuffer_height * 3, 0);
		Video::SetFramebufferPtr(framebuffer.data());
		Video::SetFramebufferSize(framebuffer_width, framebuffer_height);
		Video::SetPixelFormat(Video::PixelFormat::RGB888);
	}


	void OnHBlank()
	{
		Scheduler::AddEvent(Scheduler::EventType::HBlankSetFlag, cycles_until_set_hblank_flag - cycles_until_hblank, OnHBlankSetFlag);
		in_hblank = true;
		DMA::OnHBlank();
	}


	void OnHBlankSetFlag()
	{
		Scheduler::AddEvent(Scheduler::EventType::NewScanline, cycles_per_line - cycles_until_set_hblank_flag, OnNewScanline);
		dispstat.hblank = 1;
		if (dispstat.hblank_irq_enable) { /* TODO: here or in OnHBlank? */
			IRQ::Raise(IRQ::Source::HBlank);
		}
	}
	

	void OnNewScanline()
	{
		if (v_counter < lines_until_vblank) {
			RenderScanline();
		}
		Scheduler::AddEvent(Scheduler::EventType::HBlank, cycles_until_hblank, OnHBlank);
		dispstat.hblank = in_hblank = false;
		++v_counter;
		if (dispstat.v_counter_irq_enable) {
			bool prev_v_counter_match = dispstat.v_counter_match;
			dispstat.v_counter_match = v_counter == dispstat.v_count_setting;
			if (dispstat.v_counter_match && !prev_v_counter_match) {
				IRQ::Raise(IRQ::Source::VCounter);
			}
		}
		else {
			dispstat.v_counter_match = v_counter == dispstat.v_count_setting;
		}
		if (v_counter < lines_until_vblank) {
			UpdateRotateScalingRegisters();
		}
		else if (v_counter == lines_until_vblank) {
			framebuffer_index = 0;
			Video::NotifyNewGameFrameReady();
			dispstat.vblank = in_vblank = true;
			if (dispstat.vblank_irq_enable) {
				IRQ::Raise(IRQ::Source::VBlank);
			}
			DMA::OnVBlank();
		}
		else if (v_counter == total_num_lines - 1) {
			dispstat.vblank = 0; /* not set in the last line */
			in_vblank = false;
			//bg_rot_coord_x = 0x0FFF'FFFF & std::bit_cast<u64>(bgx);
			//bg_rot_coord_y = 0x0FFF'FFFF & std::bit_cast<u64>(bgy);
		}
		else {
			v_counter %= total_num_lines; /* cheaper than comparison on ly == total_num_lines to set ly = 0? */
		}
	}


	void PrepareObjRenderJobs()
	{
		/* For each object, determine on which dots it should be rendered. */
		obj_render_jobs.clear();
		if (objects.empty()) {
			return;
		}
		if (objects.size() == 1) {
			auto obj_length = std::min((uint)objects[0].size_x, dots_per_line - objects[0].x_coord);
			obj_render_jobs.emplace_back(0, u16(objects[0].x_coord), obj_length);
			return;
		}
		u16 last_obj_end = 0;
		std::stack<size_t> obj_stack{};
		std::vector<u8> obj_handled{};
		obj_handled.resize(objects.size(), false);
		auto ScheduleEntireObj = [&](size_t& idx, u8 obj_start, u8 obj_end) {
			obj_render_jobs.emplace_back(u8(idx), obj_start, obj_end - obj_start);
			last_obj_end = obj_end;
			obj_handled[idx] = true;
			if (obj_stack.empty()) {
				++idx;
			}
			else {
				idx = obj_stack.top();
				obj_stack.pop();
			}
		};
		for (size_t i = 0; i < objects.size(); ) {
			if (obj_handled[i]) {
				++i;
				continue;
			}
			ObjData& obj = objects[i];
			u8 obj_start = std::max(obj.x_coord, last_obj_end) & 0xFF;
			u8 obj_end = std::min(uint(obj.x_coord + obj.size_x), dots_per_line) & 0xFF;
			if (obj_start >= obj_end) { /* object is fully occluded */
				obj_handled[i++] = true;
				continue;
			}
			size_t first_unhandled_obj = std::distance(obj_handled.begin(),
				std::find(obj_handled.begin() + i + 1, obj_handled.end(), false));
			if (first_unhandled_obj == objects.size() || obj_end <= objects[first_unhandled_obj].x_coord) { /* no collisions */
				ScheduleEntireObj(i, obj_start, obj_end);
			}
			else { /* collision(s) */
				bool higher_prio_colliding_obj_found = false;
				for (size_t j = i + first_unhandled_obj; j < objects.size(); ++j) {
					if (obj_end <= objects[j].x_coord) { /* reached the end of colliding objects; no colliding objects with higher prio found */
						break;
					}
					if (obj.oam_index > objects[j].oam_index) { /* colliding object with higher prio found */
						if (obj_start < objects[j].x_coord) {
							obj_render_jobs.emplace_back(i, obj_start, objects[j].x_coord - obj_start);
							last_obj_end = objects[j].x_coord;
						}
						obj_stack.emplace(i);
						i = j;
						higher_prio_colliding_obj_found = true;
						break;
					}
				}
				if (!higher_prio_colliding_obj_found) {
					ScheduleEntireObj(i, obj_start, obj_end);
				}
			}
		}
	}


	void PushPixel(auto color_data)
	{
		if (color_data.transparent) {
			std::memset(framebuffer.data() + framebuffer_index, 0xFF, 3);
			framebuffer_index += 3;
		}
		else {
			framebuffer[framebuffer_index++] = color_data.r;
			framebuffer[framebuffer_index++] = color_data.g;
			framebuffer[framebuffer_index++] = color_data.b;
		}
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
	Int ReadOam(u32 addr)
	{
		if (dispcnt.forced_blank || in_vblank || in_hblank && dispcnt.hblank_interval_free) {
			Int ret;
			std::memcpy(&ret, oam.data() + (addr & 0x3FF), sizeof(Int));
			return ret;
		}
		else {
			return Int(-1);
		}
	}


	template<std::integral Int>
	Int ReadPaletteRam(u32 addr)
	{
		if (dispcnt.forced_blank || in_vblank || in_hblank) {
			Int ret;
			std::memcpy(&ret, palette_ram.data() + (addr & 0x3FF), sizeof(Int));
			return ret;
		}
		else {
			return Int(-1);
		}
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
			case Bus::ADDR_VCOUNT:         return v_counter;
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
			case Bus::ADDR_VCOUNT:     return u16(v_counter);
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

		if constexpr (sizeof(Int) == 1) {
			return ReadByte(addr);
		}
		if constexpr (sizeof(Int) == 2) {
			return ReadHalf(addr);
		}
		if constexpr (sizeof(Int) == 4) {
			u16 lo = ReadHalf(addr);
			u16 hi = ReadHalf(addr + 2);
			return lo | hi << 16;
		}
	}


	template<std::integral Int>
	Int ReadVram(u32 addr)
	{
		if (dispcnt.forced_blank || in_vblank || in_hblank) {
			Int ret;
			std::memcpy(&ret, vram.data() + (addr % 0x18000), sizeof(Int));
			return ret;
		}
		else {
			return Int(-1);
		}
	}


	template<void(*RenderFun)(), bool vertical_mosaic>
	void RenderBackground(uint bg)
	{
		if (GetBit(dispcnt.screen_display_bg, bg)) {
			if (!vertical_mosaic || !bgcnt[bg].mosaic_enable) {
				RenderFun();
			}
		}
		else {
			RenderTransparentBackground(bg);
		}
	}


	template<void(*RenderFun)(uint), bool vertical_mosaic>
	void RenderBackground(uint bg)
	{
		if (GetBit(dispcnt.screen_display_bg, bg)) {
			if (!vertical_mosaic || !bgcnt[bg].mosaic_enable) {
				RenderFun(bg);
			}
		}
		else {
			RenderTransparentBackground(bg);
		}
	}


	template<bool vertical_mosaic>
	void RenderBackgrounds()
	{
		switch (dispcnt.bg_mode) {
		case 0:
			for (uint bg = 0; bg < 4; ++bg) {
				RenderBackground<ScanlineBackgroundTextMode, vertical_mosaic>(bg);
			}
			break;

		case 1:
			RenderBackground<ScanlineBackgroundTextMode, vertical_mosaic>(0);
			RenderBackground<ScanlineBackgroundTextMode, vertical_mosaic>(1);
			RenderBackground<ScanlineBackgroundRotateScaleMode, vertical_mosaic>(2);
			RenderTransparentBackground(3);
			break;

		case 2:
			RenderTransparentBackground(0);
			RenderTransparentBackground(1);
			RenderBackground<ScanlineBackgroundRotateScaleMode, vertical_mosaic>(2);
			RenderBackground<ScanlineBackgroundRotateScaleMode, vertical_mosaic>(3);
			break;

		case 3:
			RenderTransparentBackground(0);
			RenderTransparentBackground(1);
			RenderTransparentBackground(3);
			RenderBackground<ScanlineBackgroundBitmapMode3, vertical_mosaic>(2);
			break;

		case 4:
			RenderTransparentBackground(0);
			RenderTransparentBackground(1);
			RenderTransparentBackground(3);
			RenderBackground<ScanlineBackgroundBitmapMode4, vertical_mosaic>(2);
			break;

		case 5:
			RenderTransparentBackground(0);
			RenderTransparentBackground(1);
			RenderTransparentBackground(3);
			RenderBackground<ScanlineBackgroundBitmapMode5, vertical_mosaic>(2);
			break;

		case 6:
		case 7:
			break;

		default:
			std::unreachable();
		}
	}


	void RenderScanline()
	{
		if (dispcnt.forced_blank) {
			/* output only white pixels */
			std::memset(framebuffer.data() + framebuffer_index, 0xFF, framebuffer_pitch);
			framebuffer_index += framebuffer_pitch;
			return;
		}

		if (mosaic.bg_v_size == 0) {
			RenderBackgrounds<false>();
		}
		else {
			if (mosaic_v_counter++ == 0) {
				RenderBackgrounds<false>();
			}
			else {
				RenderBackgrounds<true>();
			}
			if (mosaic_v_counter == mosaic.bg_v_size - 1) {
				mosaic_v_counter = 0;
			}
		}

		if (dispcnt.screen_display_obj) {
			ScanOam();
			PrepareObjRenderJobs();
			ScanlineObjects();
		}
		else {
			obj_render.fill(transparent_obj_pixel);
		}

		BlendLayers();
	}


	RGB Rgb555ToRgb888(RGB rgb)
	{
		/* Convert each 5-bit channel to 8-bit channels (https://github.com/mattcurrie/dmg-acid2) */
		return {
			.r = u8(rgb.r << 3 | rgb.r >> 2),
			.g = u8(rgb.g << 3 | rgb.g >> 2),
			.b = u8(rgb.b << 3 | rgb.b >> 2)
		};
	}


	void RenderTransparentBackground(uint bg)
	{
		for (uint dot = 0; dot < dots_per_line; ++dot) {
			bg_render[bg][dot] = transparent_bg_pixel;
		}
	}


	void ScanlineBackgroundTextMode(uint bg)
	{
		constexpr static uint tile_size = 8;
		constexpr static uint map_entry_size = 2;
		const uint bg_width = 256 << (bgcnt[bg].screen_size & 1); /* 0 => 256; 1 => 512; 2 => 256; 3 = 512 */
		const uint tile_map_addr_base = [&] { /* already takes into account which vertical tile we're on (it's constant), but not horizontal */
			constexpr static uint bytes_per_bg_map_area_row = 256 / tile_size * map_entry_size;
			const uint bg_height = 256 << (bgcnt[bg].screen_size >> 1); /* 0 => 256; 1 => 256; 2 => 512; 3 => 512 */
			const uint bg_tile_index_y = ((bgvofs[bg] + v_counter) & 255) / tile_size;
			uint tile_map_addr_base = bgcnt[bg].screen_base_block;
			if (bg_height == 512 && ((bgvofs[bg] + v_counter) & 511) > 255) {
				tile_map_addr_base += 1 + (bg_width == 512); /* BG width == 256: SC0 => SC1; BG width == 512: SC0/SC1 => SC2/SC3 */
			}
			tile_map_addr_base *= 0x800;
			return tile_map_addr_base + bg_tile_index_y * bytes_per_bg_map_area_row;
		}();
		const uint mosaic_incr = bgcnt[bg].mosaic_enable ? mosaic.bg_h_size + 1 : 1; /* TODO */
		uint bg_tile_index_x = (bghofs[bg] & (bg_width - 1)) / tile_size; /* note: possibly 0-63, but masked to 0-31 when needed */
		uint dot = 0;

		auto FetchPushTile = [&](uint pixels_to_ignore_left, uint pixels_to_ignore_right) {
			uint tile_map_addr = tile_map_addr_base + map_entry_size * (bg_tile_index_x & 31);
			if (bg_width == 512 && bg_tile_index_x > 31) {
				tile_map_addr += 0x800; /* SC0/SC2 => SC1/SC3 */
			}
			/* VRAM BG Screen Data Format (BG Map)
				0-9   Tile Number     (0-1023) (a bit less in 256 color mode, because there'd be otherwise no room for the bg map)
				10    Horizontal Flip (0=Normal, 1=Mirrored)
				11    Vertical Flip   (0=Normal, 1=Mirrored)
				12-15 Palette Number  (0-15)    (Not used in 256 color/1 palette mode) */
			constexpr static uint col_size = 2;
			const uint tile_num = vram[tile_map_addr] | vram[tile_map_addr + 1] << 8 & 0x300;
			const bool flip_x = vram[tile_map_addr + 1] & 4;
			const bool flip_y = vram[tile_map_addr + 1] & 8;
			const uint tile_offset_y = flip_y ? 7 - (v_counter + bgvofs[bg]) % 8 : (v_counter + bgvofs[bg]) % 8;
			uint tile_data_addr = bgcnt[bg].char_base_block * 0x4000;
			/* 4-bit depth (16 colors, 16 palettes). Each tile occupies 32 bytes of memory, the first 4 bytes for the topmost row of the tile, and so on.
				Each byte representing two dots, the lower 4 bits define the color for the left dot, the upper 4 bits the color for the right dot. */
			if (bgcnt[bg].palette_mode == 0) {
				tile_data_addr += 32 * tile_num + 4 * tile_offset_y;
				u8 palette_num = vram[tile_map_addr + 1] >> 4;
				const u8* palette_start_ptr = palette_ram.data() + 16 * col_size * palette_num;
				uint col_shift = (pixels_to_ignore_left & 1) ? 4 : 0; /* access lower nibble of byte if tile pixel index is even, else higher nibble. */
				auto FetchPushPixel = [&](uint pixel_index) {
					u8 col_id = vram[tile_data_addr + pixel_index / 2] >> col_shift & 0xF;
					BgColorData col;
					std::memcpy(&col, palette_start_ptr + col_size * col_id, col_size);
					col.transparent = col_id == 0;
					bg_render[bg][dot++] = col;
				};
				if (flip_x) {
					for (int i = 7 - pixels_to_ignore_right; i >= (int)pixels_to_ignore_left; --i, col_shift ^= 4) {
						FetchPushPixel(i);
					}
				}
				else {
					for (int i = pixels_to_ignore_left; i < 8 - (int)pixels_to_ignore_right; ++i, col_shift ^= 4) {
						FetchPushPixel(i);
					}
				}
			}
			/* 8-bit depth (256 colors, 1 palette). Each tile occupies 64 bytes of memory, the first 8 bytes for the topmost row of the tile, etc..
				Each byte selects the palette entry for each dot. */
			else {
				tile_data_addr += 64 * tile_num + 8 * tile_offset_y; /* TODO: tile_num can't be as high as 1023 in 256 col mode */
				auto FetchPushPixel = [&](uint pixel_index) {
					u8 col_id = vram[tile_data_addr + pixel_index];
					BgColorData col;
					std::memcpy(&col, palette_ram.data() + col_size * col_id, col_size);
					col.transparent = col_id == 0;
					bg_render[bg][dot++] = col;
				};
				if (flip_x) {
					for (int i = 7 - pixels_to_ignore_right; i >= (int)pixels_to_ignore_left; --i) {
						FetchPushPixel(i);
					}
				}
				else {
					for (int i = pixels_to_ignore_left; i < 8 - (int)pixels_to_ignore_right; ++i) {
						FetchPushPixel(i);
					}
				}
			}
			bg_tile_index_x = (bg_tile_index_x + 1) & ((bg_width - 1) / 8);
		};
		/* The LCD being 240 pixels wide means 30 tiles to fetch if bghofs lands perfectly at the beginning of a tile. Else: 31 tiles. */
		if (bghofs[bg] & 7) {
			FetchPushTile(bghofs[bg] & 7, 0);
			for (int i = 0; i < 29; ++i) {
				FetchPushTile(0, 0);
			}
			FetchPushTile(0, 8 - (bghofs[bg] & 7));
		}
		else {
			for (int i = 0; i < 30; ++i) {
				FetchPushTile(0, 0);
			}
		}
	}


	void ScanlineBackgroundRotateScaleMode(uint bg)
	{

	}


	void ScanlineBackgroundBitmapMode3()
	{
		constexpr static uint col_size = 2;
		constexpr static uint bytes_per_scanline = dots_per_line * col_size;
		const uint mosaic_incr = bgcnt[2].mosaic_enable ? mosaic.bg_h_size + 1 : 1;
		for (uint dot = 0; dot < dots_per_line; ) {
			BgColorData col;
			std::memcpy(&col, vram.data() + v_counter * bytes_per_scanline + dot * col_size, col_size);
			col.transparent = false; /* The two bytes directly define one of the 32768 colors (without using palette data, and thus not supporting a 'transparent' BG color) */
			for (uint i = 0; i < mosaic_incr; ++i) {
				bg_render[2][dot++] = col;
			}
		}
	}


	void ScanlineBackgroundBitmapMode4()
	{
		constexpr static uint col_size = 2;
		constexpr static uint bytes_per_scanline = dots_per_line;
		const uint mosaic_incr = bgcnt[2].mosaic_enable ? mosaic.bg_h_size + 1 : 1;
		const uint vram_frame_offset = dispcnt.display_frame_select ? 0xA000 : 0;
		for (uint dot = 0; dot < dots_per_line; ) {
			u8 palette_index = vram[vram_frame_offset + v_counter * bytes_per_scanline + dot];
			BgColorData col;
			std::memcpy(&col, palette_ram.data() + palette_index * col_size, col_size);
			col.transparent = palette_index == 0;
			for (uint i = 0; i < mosaic_incr; ++i) {
				bg_render[2][dot++] = col;
			}
		}
	}


	void ScanlineBackgroundBitmapMode5()
	{
		/* TODO: can objects be drawn around the background? Probably yes */
		constexpr static uint col_size = 2;
		constexpr static uint bytes_per_scanline = dots_per_line * col_size;
		const uint mosaic_incr = bgcnt[2].mosaic_enable ? mosaic.bg_h_size + 1 : 1;
		const uint vram_frame_offset = dispcnt.display_frame_select ? 0xA000 : 0;
		uint dot = 0;
		while (dot < 40) {
			bg_render[2][dot++] = transparent_bg_pixel;
		}
		while (dot < 200) {
			BgColorData col;
			std::memcpy(&col, vram.data() + vram_frame_offset + v_counter * bytes_per_scanline + dot * col_size, col_size);
			col.transparent = false; /* The two bytes directly define one of the 32768 colors (without using palette data, and thus not supporting a 'transparent' BG color) */
			for (uint i = 0; i < mosaic_incr; ++i) {
				bg_render[2][dot++] = col;
			}
		}
		while (dot < 240) {
			bg_render[2][dot++] = transparent_bg_pixel;
		}
	}


	void ScanlineObjects()
	{
		if (objects.empty()) {
			obj_render.fill(transparent_obj_pixel);
			return;
		}
		static constexpr uint col_size = 2;
		const bool char_vram_mapping = dispcnt.obj_char_vram_mapping; /* 0: 2D; 1: 1D */
		const uint vram_base_addr = dispcnt.bg_mode < 3 ? 0x10000 : 0x14000;
		const uint vram_addr_mask = 0x17FFF - vram_base_addr;

		uint dot = 0;

		auto RenderObject = [&](const ObjData& obj, u8 num_dots) {
			if (!obj.rotate_scale) {
				const bool flip_x = obj.rot_scale_param & 8;
				const bool flip_y = obj.rot_scale_param & 16;
				const auto tile_offset_x = (dot - obj.x_coord) / 8;
				const auto tile_offset_y = (v_counter - obj.y_coord) / 8;
				auto tile_pixel_offset_x = (dot - obj.x_coord) % 8;
				auto tile_pixel_offset_y = (v_counter - obj.y_coord) % 8;
				if (flip_x) {
					tile_pixel_offset_x = 7 - tile_pixel_offset_x;
				}
				if (flip_y) {
					tile_pixel_offset_y = 7 - tile_pixel_offset_y;
				}
				auto RenderObject = [&] <bool palette_mode> {
					/* Palette Mode 0: 4-bit depth (16 colors, 16 palettes). Each tile occupies 32 bytes of memory, the first 4 bytes for the topmost row of the tile, and so on.
						Each byte representing two dots, the lower 4 bits define the color for the left dot, the upper 4 bits the color for the right dot. 
						Palette Mode 1: 8-bit depth (256 colors, 1 palette). Each tile occupies 64 bytes of memory, the first 8 bytes for the topmost row of the tile, etc..
						Each byte selects the palette entry for each dot. */
					static constexpr uint tile_size = [&] {
						if constexpr (palette_mode == 0) return 32;
						else                             return 64;
					}();
					static constexpr uint tile_row_size = tile_size / 8;
					/* When using the 256 Colors/1 Palette mode, only each second tile may be used, the lower bit
						of the tile number should be zero (in 2-dimensional mapping mode, the bit is completely ignored). */
					u32 base_tile_num = obj.tile_num;
					if constexpr (palette_mode == 1) {
						base_tile_num &= ~1;
					}
					u32 tile_data_addr_offset;
					auto FetchPushTile = [&](uint pixels_to_ignore_left, uint pixels_to_ignore_right) {
						u32 tile_data_addr = vram_base_addr + (tile_data_addr_offset & vram_addr_mask);
						uint col_shift; 
						auto FetchPushPixel = [&](uint pixel_index) {
							ObjColorData col;
							u8 col_id;
							const u8* palette_start_ptr;
							if constexpr (palette_mode == 0) {
								col_id = vram[tile_data_addr + pixel_index / 2] >> col_shift & 0xF; 
								palette_start_ptr = palette_ram.data() + 16 * col_size * obj.palette_num;
							}
							else {
								col_id = vram[tile_data_addr + pixel_index];
								palette_start_ptr = palette_ram.data();
							}
							std::memcpy(&col, palette_start_ptr + col_size * col_id, col_size);
							col.transparent = col_id == 0;
							col.obj_mode = obj.obj_mode;
							col.priority = obj.priority;
							obj_render[dot++] = col;
						};
						if (flip_x) {
							col_shift = (pixels_to_ignore_right & 1) ? 0 : 4; /* access lower nibble of byte if tile pixel index is even, else higher nibble. */
							for (int i = 7 - pixels_to_ignore_right; i >= (int)pixels_to_ignore_left; --i, col_shift ^= 4) {
								FetchPushPixel(i);
							}
						}
						else {
							col_shift = (pixels_to_ignore_left & 1) ? 4 : 0;
							for (int i = pixels_to_ignore_left; i < 8 - (int)pixels_to_ignore_right; ++i, col_shift ^= 4) {
								FetchPushPixel(i);
							}
						}
					};
					/* Char VRAM Mapping = 0: The 1024 OBJ tiles are arranged as a matrix of 32x32 tiles / 256x256 pixels (In 256 color mode: 16x32 tiles / 128x256 pixels)
						E.g., when displaying a 16x16 pixel OBJ with tile number 04h, the upper row of the OBJ will consist of tile 04h and 05h, the next row of 24h and 25h. (In 256 color mode: 04h and 06h, 24h and 26h.)
						Char VRAM Mapping = 1: Tiles are mapped each after each other from 00h-3FFh. Using the same example as above, the upper row of the OBJ
							will consist of tile 04h and 05h, the next row of tile 06h and 07h. (In 256 color mode: 04h and 06h, 08h and 0Ah.)*/
					const auto base_rel_tile_num = tile_offset_x + tile_offset_y * (char_vram_mapping == 0 ? 32 : obj.size_x / 8);
					tile_data_addr_offset = 32 * (base_tile_num + base_rel_tile_num) + tile_row_size * tile_pixel_offset_y;
					auto pixels_to_ignore_left = tile_pixel_offset_x;
					auto pixels_to_ignore_right = std::max(0, 8 - int(tile_pixel_offset_x) - int(num_dots));
					FetchPushTile(pixels_to_ignore_left, pixels_to_ignore_right);
					num_dots -= 8 - pixels_to_ignore_left - pixels_to_ignore_right;
					for (int tile = 0; tile < num_dots / 8; ++tile) {
						tile_data_addr_offset += tile_size;
						FetchPushTile(0, 0);
					}
					if (num_dots % 8) {
						tile_data_addr_offset += tile_size;
						FetchPushTile(0, 8 - num_dots % 8);
					}
				};

				if (obj.palette_mode == 0) RenderObject.template operator() < 0 > ();
				else                       RenderObject.template operator() < 1 > ();
			}
		};

		std::ranges::for_each(obj_render_jobs, [&](ObjRenderJob job) {
			while (dot < job.dot_start) {
				obj_render[dot++] = transparent_obj_pixel;
			}
			RenderObject(objects[job.obj_index], job.length);
		});
		while (dot < dots_per_line) {
			obj_render[dot++] = transparent_obj_pixel;
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
		objects.clear();
		for (uint oam_addr = 0; oam_addr < oam.size(); oam_addr += 8) {
			bool rotate_scale = oam[oam_addr + 1] & 1;
			bool double_size_obj_disable = oam[oam_addr + 1] & 2;
			if (!rotate_scale && double_size_obj_disable) {
				continue;
			}
			u8 y_coord = oam[oam_addr];
			if (y_coord > v_counter) {
				continue;
			}
			u16 x_coord = oam[oam_addr + 2] | (oam[oam_addr + 3] & 1) << 8;
			if (x_coord >= dots_per_line) {
				continue;
			}
			u8 obj_shape = oam[oam_addr + 1] >> 6 & 3;
			u8 obj_size = oam[oam_addr + 3] >> 6 & 3;
			static constexpr u8 obj_height_table[4][4] = { /* OBJ Size (0-3) * OBJ Shape (0-2 (3 prohibited)) */
				8, 8, 16, 8,
				16, 8, 32, 16,
				32, 16, 32, 32,
				64, 32, 64, 64
			};
			static constexpr u8 obj_width_table[4][4] = {
				8, 16, 8, 8,
				16, 32, 8, 16,
				32, 32, 16, 32,
				64, 64, 32, 64
			};
			u8 obj_height = obj_height_table[obj_size][obj_shape] << (double_size_obj_disable & rotate_scale);
			if (y_coord + obj_height <= v_counter) {
				continue;
			}
			ObjData obj_data;
			std::memcpy(&obj_data, oam.data() + oam_addr, 6);
			obj_data.size_x = obj_width_table[obj_size][obj_shape] << (double_size_obj_disable & rotate_scale);
			obj_data.oam_index = oam_addr / 8;
			/* Sort the objects so that the first object in the vector is the left-most object to be rendered on the current scanline.
				If two objects have the same x-coordinates, the one with the smaller oam index has priority. */
			auto it = std::ranges::find_if(objects, [&](const ObjData& obj) {
				return obj_data.x_coord < obj.x_coord;
			});
			objects.insert(it, obj_data);
		}
	}


	void SortBackgroundsAfterPriority()
	{
		/* TODO: also take into account if a bg is enabled? */
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 3; ++j) {
				if (bgcnt[bg_by_prio[j]].bg_priority > bgcnt[bg_by_prio[j + 1]].bg_priority ||
					bgcnt[bg_by_prio[j]].bg_priority == bgcnt[bg_by_prio[j + 1]].bg_priority &&
					bg_by_prio[j] > bg_by_prio[j + 1]) {
					std::swap(bg_by_prio[j], bg_by_prio[j + 1]);
				}
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
	void WriteOam(u32 addr, Int data)
	{
		std::memcpy(oam.data() + (addr & 0x3FF), &data, sizeof(Int));
		//if (dispcnt.forced_blank || in_vblank || in_hblank && dispcnt.hblank_interval_free) {
		//	std::memcpy(oam.data() + (addr & 0x3FF), &data, sizeof(Int));
		//}
	}


	template<std::integral Int>
	void WritePaletteRam(u32 addr, Int data)
	{
		std::memcpy(palette_ram.data() + (addr & 0x3FF), &data, sizeof(Int));
		//if (dispcnt.forced_blank || in_vblank || in_hblank) {
		//	std::memcpy(palette_ram.data() + (addr & 0x3FF), &data, sizeof(Int));
		//}
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
			case Bus::ADDR_BG0CNT:       SetByte(bgcnt[0], 0, data); SortBackgroundsAfterPriority(); break;
			case Bus::ADDR_BG0CNT + 1:   SetByte(bgcnt[0], 1, data); break;
			case Bus::ADDR_BG1CNT:       SetByte(bgcnt[1], 0, data); SortBackgroundsAfterPriority(); break;
			case Bus::ADDR_BG1CNT + 1:   SetByte(bgcnt[1], 1, data); break;
			case Bus::ADDR_BG2CNT:       SetByte(bgcnt[2], 0, data); SortBackgroundsAfterPriority(); break;
			case Bus::ADDR_BG2CNT + 1:   SetByte(bgcnt[2], 1, data); break;
			case Bus::ADDR_BG3CNT:       SetByte(bgcnt[3], 0, data); SortBackgroundsAfterPriority(); break;
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
			case Bus::ADDR_WIN0V:        winv_y2[0] = data; break;
			case Bus::ADDR_WIN0V + 1:    winv_y1[0] = data; break;
			case Bus::ADDR_WIN1V:        winv_y2[1] = data; break;
			case Bus::ADDR_WIN1V + 1:    winv_y1[1] = data; break;
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
			}
		};

		auto WriteHalf = [](u32 addr, u16 data) {
			switch (addr) {
			case Bus::ADDR_DISPCNT:    dispcnt = std::bit_cast<DISPCNT>(data); break;
			case Bus::ADDR_GREEN_SWAP: green_swap = data & 1; break;
			case Bus::ADDR_DISPSTAT:   dispstat = std::bit_cast<DISPSTAT>(data); break;
			case Bus::ADDR_BG0CNT:     bgcnt[0] = std::bit_cast<BGCNT>(data); SortBackgroundsAfterPriority(); break;
			case Bus::ADDR_BG1CNT:     bgcnt[1] = std::bit_cast<BGCNT>(data); SortBackgroundsAfterPriority(); break;
			case Bus::ADDR_BG2CNT:     bgcnt[2] = std::bit_cast<BGCNT>(data); SortBackgroundsAfterPriority(); break;
			case Bus::ADDR_BG3CNT:     bgcnt[3] = std::bit_cast<BGCNT>(data); SortBackgroundsAfterPriority(); break;
			case Bus::ADDR_BG0HOFS:    bghofs[0] = data & 0x1FF; break;
			case Bus::ADDR_BG0VOFS:    bgvofs[0] = data & 0x1FF; break;
			case Bus::ADDR_BG1HOFS:    bghofs[1] = data & 0x1FF; break;
			case Bus::ADDR_BG1VOFS:    bgvofs[1] = data & 0x1FF; break;
			case Bus::ADDR_BG2HOFS:    bghofs[2] = data & 0x1FF; break;
			case Bus::ADDR_BG2VOFS:    bgvofs[2] = data & 0x1FF; break;
			case Bus::ADDR_BG3HOFS:    bghofs[3] = data & 0x1FF; break;
			case Bus::ADDR_BG3VOFS:    bgvofs[3] = data & 0x1FF; break;
			case Bus::ADDR_BG2PA:      bgpa[0] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG2PB:      bgpb[0] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG2PC:      bgpc[0] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG2PD:      bgpd[0] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG2X:       SetByte(bgx[0], 0, data & 0xFF); SetByte(bgx[0], 1, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG2X + 2:   SetByte(bgx[0], 2, data & 0xFF); SetByte(bgx[0], 3, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG2Y:       SetByte(bgy[0], 0, data & 0xFF); SetByte(bgy[0], 1, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG2Y + 2:   SetByte(bgy[0], 2, data & 0xFF); SetByte(bgy[0], 3, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG3PA:      bgpa[1] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG3PB:      bgpb[1] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG3PC:      bgpc[1] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG3PD:      bgpd[1] = std::bit_cast<BGP>(data); break;
			case Bus::ADDR_BG3X:       SetByte(bgx[1], 0, data & 0xFF); SetByte(bgx[1], 1, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG3X + 2:   SetByte(bgx[1], 2, data & 0xFF); SetByte(bgx[1], 3, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG3Y:       SetByte(bgy[1], 0, data & 0xFF); SetByte(bgy[1], 1, data >> 8 & 0xFF); break;
			case Bus::ADDR_BG3Y + 2:   SetByte(bgy[1], 2, data & 0xFF); SetByte(bgy[1], 3, data >> 8 & 0xFF); break;
			case Bus::ADDR_WIN0H:      winh_x2[0] = data & 0xFF; winh_x1[0] = data >> 8 & 0xFF; break;
			case Bus::ADDR_WIN1H:      winh_x2[1] = data & 0xFF; winh_x1[1] = data >> 8 & 0xFF; break;
			case Bus::ADDR_WIN0V:      winv_y2[0] = data & 0xFF; winv_y1[0] = data >> 8 & 0xFF; break;
			case Bus::ADDR_WIN1V:      winv_y2[1] = data & 0xFF; winv_y1[1] = data >> 8 & 0xFF; break;
			case Bus::ADDR_WININ:      winin = std::bit_cast<WININ>(data); break;
			case Bus::ADDR_WINOUT:     winout = std::bit_cast<WINOUT>(data); break;
			case Bus::ADDR_MOSAIC:     mosaic = std::bit_cast<MOSAIC>(data); break;
			case Bus::ADDR_BLDCNT:     bldcnt = std::bit_cast<BLDCNT>(data); break;
			case Bus::ADDR_BLDALPHA:   eva = data & 0x1F; evb = data >> 8 & 0x1F; break;
			case Bus::ADDR_BLDY:       evy = data & 0x1F; break;
			}
		};

		if constexpr (sizeof(Int) == 1) {
			WriteByte(addr, data);
		}
		if constexpr (sizeof(Int) == 2) {
			WriteHalf(addr, data);
		}
		if constexpr (sizeof(Int) == 4) {
			WriteHalf(addr, data & 0xFFFF);
			WriteHalf(addr + 2, data >> 16 & 0xFFFF);
		}
	}


	template<std::integral Int>
	void WriteVram(u32 addr, Int data)
	{
		std::memcpy(vram.data() + (addr % 0x18000), &data, sizeof(Int));
		//if (dispcnt.forced_blank || in_vblank || in_hblank) {
		//	std::memcpy(vram.data() + (addr % 0x18000), &data, sizeof(Int));
		//}
	}


	template u8 ReadOam<u8>(u32);
	template s8 ReadOam<s8>(u32);
	template u16 ReadOam<u16>(u32);
	template s16 ReadOam<s16>(u32);
	template u32 ReadOam<u32>(u32);
	template s32 ReadOam<s32>(u32);
	template void WriteOam<u8>(u32, u8);
	template void WriteOam<s8>(u32, s8);
	template void WriteOam<u16>(u32, u16);
	template void WriteOam<s16>(u32, s16);
	template void WriteOam<u32>(u32, u32);
	template void WriteOam<s32>(u32, s32);

	template u8 ReadPaletteRam<u8>(u32);
	template s8 ReadPaletteRam<s8>(u32);
	template u16 ReadPaletteRam<u16>(u32);
	template s16 ReadPaletteRam<s16>(u32);
	template u32 ReadPaletteRam<u32>(u32);
	template s32 ReadPaletteRam<s32>(u32);
	template void WritePaletteRam<u8>(u32, u8);
	template void WritePaletteRam<s8>(u32, s8);
	template void WritePaletteRam<u16>(u32, u16);
	template void WritePaletteRam<s16>(u32, s16);
	template void WritePaletteRam<u32>(u32, u32);
	template void WritePaletteRam<s32>(u32, s32);

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

	template u8 ReadVram<u8>(u32);
	template s8 ReadVram<s8>(u32);
	template u16 ReadVram<u16>(u32);
	template s16 ReadVram<s16>(u32);
	template u32 ReadVram<u32>(u32);
	template s32 ReadVram<s32>(u32);
	template void WriteVram<u8>(u32, u8);
	template void WriteVram<s8>(u32, s8);
	template void WriteVram<u16>(u32, u16);
	template void WriteVram<s16>(u32, s16);
	template void WriteVram<u32>(u32, u32);
	template void WriteVram<s32>(u32, s32);
}