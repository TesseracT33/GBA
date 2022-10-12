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
}