#include "ppu.h"

PPU::PPU() {}

PPU::~PPU() {}

void PPU::reset()
{
    frame_complete = false;
    fine_x = 0x00;
    address_latch = false;
    ppu_data_buffer = 0x00;
    scanline = 0;
    cycle = 0;
    bg_next_tile_id = 0x00;
    bg_next_tile_attrib = 0x00;
    bg_next_tile_lsb = 0x00;
    bg_next_tile_msb = 0x00;
    bg_shifter_pattern_lo = 0x0000;
    bg_shifter_pattern_hi = 0x0000;
    bg_shifter_attrib_lo = 0x0000;
    bg_shifter_attrib_hi = 0x0000;
    status.reg = 0x00;
    mask.reg = 0x00;
    control.reg = 0x00;
    vram_addr.reg = 0x0000;
    tram_addr.reg = 0x0000;
    odd_frame = false;

    memset(frame_data, 0, sizeof(quint8) * 256 * 240 * 3);
    memset(tblName, 0, sizeof(quint8) * 1024 * 2);
    memset(tblPalette, 0, sizeof(quint8) * 32);
}

void PPU::ConnectCartridge(Cartridge *cartridge)
{
    this->cart = cartridge;
}

quint8 PPU::get_status()
{
    // Actually the high 3bit is enough
    quint8 data = (status.reg & 0xE0) | (ppu_data_buffer & 0x1F);

    // Clear the vertical blanking flag
    status.vertical_blank = 0;

    // Reset Loopy's Address latch flag
    address_latch = false;

    return data;
}

quint8 PPU::get_oamdata()
{
    return pOAM[oam_addr];
}

quint8 PPU::read_data()
{
    quint8 data = ppu_data_buffer;
    // update the buffer for next time
    ppu_data_buffer = ppuRead(vram_addr.reg);

    if (vram_addr.reg >= 0x3F00)
        data = ppu_data_buffer;

    vram_addr.reg += (control.increment_mode ? 32 : 1);
    return data;
}

void PPU::write_ctrl(quint8 ctrl)
{
    control.reg = ctrl;
    tram_addr.nametable_x = control.nametable_x;
    tram_addr.nametable_y = control.nametable_y;
}

void PPU::write_mask(quint8 mask)
{
    this->mask.reg = mask;
}

void PPU::write_scroll(quint8 scroll)
{
    // if address_latch points to lower bit, write X
    if (address_latch == false) {
        fine_x = scroll & 0x07;
        tram_addr.coarse_x = scroll >> 3;
        address_latch = true;
    } else {
        // else write Y
        tram_addr.fine_y = scroll & 0x07;
        tram_addr.coarse_y = scroll >> 3;
        address_latch = false;
    }
}

void PPU::write_addr(quint8 addr)
{
    if (address_latch == false) {
        // set high 6 bit address
        tram_addr.reg = (quint16)((addr & 0x3F) << 8) | (tram_addr.reg & 0x00FF);
        address_latch = true;
    } else {
        // set low 8 bit address
        tram_addr.reg = (tram_addr.reg & 0xFF00) | addr;
        vram_addr = tram_addr;
        address_latch = false;
    }
}

void PPU::write_data(quint8 data)
{
    ppuWrite(vram_addr.reg, data);
    vram_addr.reg += (control.increment_mode ? 32 : 1);
}

void PPU::write_oamaddr(quint8 addr)
{
    oam_addr = addr;
}

void PPU::write_oamdata(quint8 data)
{
    pOAM[oam_addr] = data;
}

quint8 PPU::ppuRead(quint16 addr)
{
    quint8 data = 0x00;
    addr &= 0x3FFF;

    if (addr >= 0x0000 && addr <= 0x1FFF) {
        data = cart->PpuRead(addr);
    } else if (addr >= 0x2000 && addr <= 0x3EFF) {
        addr &= 0x0FFF;

        switch (cart->mapper_ptr->nametable_mirror) {
        case MirrorMode::HORIZONTAL:
            if (addr >= 0x0000 && addr <= 0x03FF)
                data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0400 && addr <= 0x07FF)
                data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                data = tblName[1][addr & 0x03FF];
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                data = tblName[1][addr & 0x03FF];
            break;

        case MirrorMode::VERTICAL:
            if (addr >= 0x0000 && addr <= 0x03FF)
                data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0400 && addr <= 0x07FF)
                data = tblName[1][addr & 0x03FF];
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                data = tblName[0][addr & 0x03FF];
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                data = tblName[1][addr & 0x03FF];
            break;

        case MirrorMode::ONESCREEN_LO:
            data = tblName[0][addr & 0x03FF];
            break;

        case MirrorMode::ONESCREEN_HI:
            data = tblName[1][addr & 0x03FF];
            break;

        default:
            break;
        }
    } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        addr &= 0x001F;
        if (addr == 0x0010)
            addr = 0x0000;
        if (addr == 0x0014)
            addr = 0x0004;
        if (addr == 0x0018)
            addr = 0x0008;
        if (addr == 0x001C)
            addr = 0x000C;
        data = tblPalette[addr] & (mask.grayscale ? 0x30 : 0x3F);
    }

    return data;
}

void PPU::ppuWrite(quint16 addr, quint8 data)
{
    addr &= 0x3FFF;

    if (addr <= 0x1fff) {
        cart->PpuWrite(addr, data);
    } else if (addr >= 0x2000 && addr <= 0x3EFF) {
        addr &= 0x0FFF;
        switch (cart->mapper_ptr->nametable_mirror) {
        case MirrorMode::HORIZONTAL:
            if (addr >= 0x0000 && addr <= 0x03FF)
                tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0400 && addr <= 0x07FF)
                tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                tblName[1][addr & 0x03FF] = data;
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                tblName[1][addr & 0x03FF] = data;
            break;

        case MirrorMode::VERTICAL:
            if (addr >= 0x0000 && addr <= 0x03FF)
                tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0400 && addr <= 0x07FF)
                tblName[1][addr & 0x03FF] = data;
            else if (addr >= 0x0800 && addr <= 0x0BFF)
                tblName[0][addr & 0x03FF] = data;
            else if (addr >= 0x0C00 && addr <= 0x0FFF)
                tblName[1][addr & 0x03FF] = data;
            break;

        case MirrorMode::ONESCREEN_LO:
            tblName[0][addr & 0x03FF] = data;
            break;

        case MirrorMode::ONESCREEN_HI:
            tblName[1][addr & 0x03FF] = data;
            break;

        default:
            break;
        }
    } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
        addr &= 0x001F;
        if (addr == 0x0010)
            addr = 0x0000;
        if (addr == 0x0014)
            addr = 0x0004;
        if (addr == 0x0018)
            addr = 0x0008;
        if (addr == 0x001C)
            addr = 0x000C;
        tblPalette[addr] = data;
    }
}

void PPU::IncrementScrollX()
{
    // The move unit is tile (8*8 pixels)
    // Only if rendering is enabled
    if (mask.render_background || mask.render_sprites) {
        // if coarse_x == 31, meaning that we're crossing into a neighbouring nametable
        if (vram_addr.coarse_x == 31) {
            // back to the very first tile
            vram_addr.coarse_x = 0;
            // Flip target nametable bit
            vram_addr.nametable_x = ~vram_addr.nametable_x;
        } else {
            // Staying in current nametable, so just increment
            vram_addr.coarse_x++;
        }
    }
}

void PPU::IncrementScrollY()
{
    // The bottom two rows of tiles in nametalbe are in fact not tiles at all, they
    // contain the "attribute" information for the entire table.

    // In addition, the NES doesn't scroll a whole tile, but a pixel using fine_y
    // only when fine_y == 7 will need to move to the next tile

    // Only if rendering is enabled
    if (mask.render_background || mask.render_sprites) {
        // If possible, just increment the fine y offset
        if (vram_addr.fine_y < 7) {
            vram_addr.fine_y++;
        } else {
            // reset fine_y
            vram_addr.fine_y = 0;

            // if we need to switch nametable
            if (vram_addr.coarse_y == 29) {
                vram_addr.coarse_y = 0;
                vram_addr.nametable_y = ~vram_addr.nametable_y;
            } else if (vram_addr.coarse_y == 31) {
                // if pointing to the attribute table
                vram_addr.coarse_y = 0;
            } else {
                // just increment the coarse y offset
                vram_addr.coarse_y++;
            }
        }
    }
}

void PPU::TransferAddressX()
{
    // Only if rendering is enabled
    if (mask.render_background || mask.render_sprites) {
        vram_addr.nametable_x = tram_addr.nametable_x;
        vram_addr.coarse_x = tram_addr.coarse_x;
    }
}

void PPU::TransferAddressY()
{
    // Only if rendering is enabled
    if (mask.render_background || mask.render_sprites) {
        vram_addr.fine_y = tram_addr.fine_y;
        vram_addr.nametable_y = tram_addr.nametable_y;
        vram_addr.coarse_y = tram_addr.coarse_y;
    }
}

void PPU::LoadBackgroundShifters()
{
    // Each PPU update we calculate one pixel. These shifters shift 1 bit along
    // feeding the pixel compositor with the binary information it needs. Its
    // 16 bits wide, because the top 8 bits are the current 8 pixels being drawn
    // and the bottom 8 bits are the next 8 pixels to be drawn. Naturally this means
    // the required bit is always the MSB of the shifter. However, "fine x" scrolling
    // plays a part in this too, whcih is seen later, so in fact we can choose
    // any one of the top 8 bits.
    bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
    bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;

    // Attribute info isn't changed based on pixels. In fact every 8 pixels
    // have exact same attribute
    bg_shifter_attrib_lo = (bg_shifter_attrib_lo & 0xFF00)
                           | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
    bg_shifter_attrib_hi = (bg_shifter_attrib_hi & 0xFF00)
                           | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
}

void PPU::UpdateShifters()
{
    // if a bgpixel has rendered, pop it out
    if (mask.render_background) {
        bg_shifter_pattern_lo <<= 1;
        bg_shifter_pattern_hi <<= 1;

        bg_shifter_attrib_lo <<= 1;
        bg_shifter_attrib_hi <<= 1;
    }

    // if a sprite pixel has rendered, pop it out
    // if it's not done, x--
    if (mask.render_sprites && cycle >= 1 && cycle < 258) {
        for (int i = 0; i < sprite_count; i++) {
            if (spriteScanline[i].x > 0) {
                spriteScanline[i].x--;
            } else {
                sprite_shifter_pattern_lo[i] <<= 1;
                sprite_shifter_pattern_hi[i] <<= 1;
            }
        }
    }
}

// I recommend you to read at https://github.com/OneLoneCoder/olcNES
// for a very detailed annotation

// TODO: Known bug around the "Odd Frame" skip thing
void PPU::clock()
{
    // The pre-render scanline at -1
    // is used to configure the "shifters" for the first visible scanline, 0.
    if (scanline >= -1 && scanline < 240) {
        // Background Rendering ======================================================
        if (scanline == 0 && cycle == 0 && odd_frame
            && (mask.render_background || mask.render_sprites)) {
            // "Odd Frame" cycle skip
            cycle = 1;
        }

        if (scanline == -1 && cycle == 1) {
            // start of a new frame, so clear all flags
            status.vertical_blank = 0;
            status.sprite_overflow = 0;
            status.sprite_zero_hit = 0;

            // clear Shifters
            memset(sprite_shifter_pattern_lo, 0, sizeof(quint8) * 8);
            memset(sprite_shifter_pattern_hi, 0, sizeof(quint8) * 8);
        }

        if ((cycle >= 2 && cycle < 258) || (cycle >= 321 && cycle < 338)) {
            UpdateShifters();

            switch ((cycle - 1) % 8) {
                // 0-2 cycles，fetch nametable id
            case 0:
                // Load the current background tile pattern and attributes into the "shifter"
                LoadBackgroundShifters();

                // Fetch the next background tile ID
                // "(vram_addr.reg & 0x0FFF)" : Mask to 12 bits that are relevant
                // "| 0x2000"                 : Offset into nametable space on PPU address bus
                bg_next_tile_id = ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
                break;

                // 2-4 cycles，fetch attribute id
            case 2:
                // (vram_addr.nametable_y << 11) | (vram_addr.nametable_x << 10)
                // | ((vram_addr.coarse_y >> 2) << 3) | (vram_addr.coarse_x >> 2)
                // Result: YX00 00yy yxxx
                bg_next_tile_attrib = ppuRead(
                    0x23C0 | (vram_addr.nametable_y << 11) | (vram_addr.nametable_x << 10)
                    | ((vram_addr.coarse_y >> 2) << 3) | (vram_addr.coarse_x >> 2));

                // further more, we need to confirm the palette info
                // if coarse_y % 4 < 2, then we at top; bottom otherwise
                // if coarse_x % 4 < 2, then we at left; right otherwise
                if (vram_addr.coarse_y & 0x02)
                    bg_next_tile_attrib >>= 4;
                if (vram_addr.coarse_x & 0x02)
                    bg_next_tile_attrib >>= 2;
                bg_next_tile_attrib &= 0x03;
                break;

                // 4-6 cycles，fetch bgtile LSB
            case 4:
                bg_next_tile_lsb = ppuRead((control.pattern_background << 12)
                                           + ((quint16) bg_next_tile_id << 4) + (vram_addr.fine_y)
                                           + 0);

                break;

                // 6-7 cycles，fetch bgtile MSB
            case 6:
                // mind that 8bytes offset
                bg_next_tile_msb = ppuRead((control.pattern_background << 12)
                                           + ((quint16) bg_next_tile_id << 4) + (vram_addr.fine_y)
                                           + 8);
                break;

                // 7-8 cycles，Increment the background tile "pointer"
            case 7:
                IncrementScrollX();
                break;
            }
        }

        // End of a visible scanline, increment downwards
        if (cycle == 256) {
            IncrementScrollY();
        }

        // reset the x position(because we moved downwards)
        if (cycle == 257) {
            LoadBackgroundShifters();
            TransferAddressX();
        }

        // Superfluous reads of tile id at end of scanline
        if (cycle == 338 || cycle == 340) {
            bg_next_tile_id = ppuRead(0x2000 | (vram_addr.reg & 0x0FFF));
        }

        if (scanline == -1 && cycle >= 280 && cycle < 305) {
            // End of vertical blank period so reset the Y address ready for rendering
            TransferAddressY();
        }

        // Foreground Rendering ========================================================
        // Didn't follow the Ntsc_timing, cause we're actually preparing for the next
        // scanline. We prepare all things in one cycle to simplify the process

        if (cycle == 257 && scanline >= 0) {
            // We're reaching the end of a visible scanline. It is now time to determine
            // which sprites are visible on the next scanline

            // clear out the sprite memory
            memset(spriteScanline, 0xFF, 8 * sizeof(sObjectAttributeEntry));

            // clear sprite_count
            sprite_count = 0;

            // clear Shifter
            memset(sprite_shifter_pattern_lo, 0, sizeof(quint8) * 8);
            memset(sprite_shifter_pattern_hi, 0, sizeof(quint8) * 8);

            // prepare to count how many sprite are we gonna render
            // next line
            quint8 nOAMEntry = 0;

            // New set of sprites.
            // Sprite zero may not exist in the new set, so clear this
            bSpriteZeroHitPossible = false;

            while (nOAMEntry < 64 && sprite_count < 9) {
                int16_t diff = ((int16_t) scanline - (int16_t) OAM[nOAMEntry].y);

                // diff between [0,sprite height] will be rendered
                if (diff >= 0 && diff < (control.sprite_size ? 16 : 8) && sprite_count < 8) {
                    if (sprite_count < 8) {
                        // if it's sprite0, then it might trigger Sprite0_hit
                        if (nOAMEntry == 0) {
                            bSpriteZeroHitPossible = true;
                        }

                        memcpy(&spriteScanline[sprite_count],
                               &OAM[nOAMEntry],
                               sizeof(sObjectAttributeEntry));
                    }
                    sprite_count++;
                }
                nOAMEntry++;
            }

            // if sprite_count > 8, then set overflow flag
            status.sprite_overflow = (sprite_count >= 8);
        }

        // at the end of the scanline, we set those Shifters
        if (cycle == 340) {
            for (quint8 i = 0; i < sprite_count; i++) {
                quint8 sprite_pattern_bits_lo, sprite_pattern_bits_hi;
                quint16 sprite_pattern_addr_lo, sprite_pattern_addr_hi;

                // 8x8 Sprite Mode
                if (!control.sprite_size) {
                    // Sprite is NOT flipped vertically
                    if (!(spriteScanline[i].attribute & 0x80)) {
                        sprite_pattern_addr_lo
                            = (control.pattern_sprite
                               << 12) // Which Pattern Table? 0KB or 4KB offset
                              | (spriteScanline[i].id
                                 << 4) // Which Cell? Tile ID * 16 (16 bytes per tile)
                              | (scanline - spriteScanline[i].y); // Which Row in cell? (0->7)
                    } else {
                        // Sprite is flipped vertically
                        sprite_pattern_addr_lo
                            = (control.pattern_sprite
                               << 12) // Which Pattern Table? 0KB or 4KB offset
                              | (spriteScanline[i].id
                                 << 4) // Which Cell? Tile ID * 16 (16 bytes per tile)
                              | (7 - (scanline - spriteScanline[i].y)); // Which Row in cell? (0->7)
                    }
                } else {
                    // 8x16 Sprite Mode
                    // Sprite is NOT flipped vertically
                    if (!(spriteScanline[i].attribute & 0x80)) {
                        // 8x16相当于由两个瓦片组合而成，因此我们需要判断从哪一个瓦片读取
                        // 8x16 equals to Two Tiles combined, so we need to judge which one
                        if (scanline - spriteScanline[i].y < 8) {
                            // top half tile
                            sprite_pattern_addr_lo
                                = ((spriteScanline[i].id & 0x01)
                                   << 12) // Which Pattern Table? 0KB or 4KB offset
                                  | ((spriteScanline[i].id & 0xFE)
                                     << 4) // Which Cell? Tile ID * 16 (16 bytes per tile)
                                  | ((scanline - spriteScanline[i].y)
                                     & 0x07); // Which Row in cell? (0->7)
                        } else {
                            // bottom half tile
                            sprite_pattern_addr_lo
                                = ((spriteScanline[i].id & 0x01)
                                   << 12) // Which Pattern Table? 0KB or 4KB offset
                                  | (((spriteScanline[i].id & 0xFE) + 1)
                                     << 4) // Which Cell? Tile ID * 16 (16 bytes per tile)
                                  | ((scanline - spriteScanline[i].y)
                                     & 0x07); // Which Row in cell? (0->7)
                        }
                    } else {
                        // Sprite is flipped vertically
                        if (scanline - spriteScanline[i].y < 8) {
                            // top half tile
                            sprite_pattern_addr_lo
                                = ((spriteScanline[i].id & 0x01)
                                   << 12) // Which Pattern Table? 0KB or 4KB offset
                                  | (((spriteScanline[i].id & 0xFE) + 1)
                                     << 4) // Which Cell? Tile ID * 16 (16 bytes per tile)
                                  | (7 - (scanline - spriteScanline[i].y)
                                     & 0x07); // Which Row in cell? (0->7)
                        } else {
                            // bottom half tile
                            sprite_pattern_addr_lo
                                = ((spriteScanline[i].id & 0x01)
                                   << 12) // Which Pattern Table? 0KB or 4KB offset
                                  | ((spriteScanline[i].id & 0xFE)
                                     << 4) // Which Cell? Tile ID * 16 (16 bytes per tile)
                                  | (7 - (scanline - spriteScanline[i].y)
                                     & 0x07); // Which Row in cell? (0->7)
                        }
                    }
                }

                // Hibit plane equivalent is always offset by 8 bytes from lobit plane
                sprite_pattern_addr_hi = sprite_pattern_addr_lo + 8;

                // read those sprite patterns
                sprite_pattern_bits_lo = ppuRead(sprite_pattern_addr_lo);
                sprite_pattern_bits_hi = ppuRead(sprite_pattern_addr_hi);

                // If the sprite is flipped horizontally, we need to flip the
                // pattern bytes.
                if (spriteScanline[i].attribute & 0x40) {
                    // This little lambda function "flips" a byte
                    // so 0b11100000 becomes 0b00000111.
                    // https://stackoverflow.com/a/2602885
                    auto flipbyte = [](quint8 b) {
                        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
                        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
                        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
                        return b;
                    };

                    // Flip Patterns Horizontally
                    sprite_pattern_bits_lo = flipbyte(sprite_pattern_bits_lo);
                    sprite_pattern_bits_hi = flipbyte(sprite_pattern_bits_hi);
                }

                // load them into Shifters, ready for rendering on the next scanline
                sprite_shifter_pattern_lo[i] = sprite_pattern_bits_lo;
                sprite_shifter_pattern_hi[i] = sprite_pattern_bits_hi;
            }
        }
    }

    if (scanline == 240) {
        // Post Render Scanline - Do Nothing!
    }

    if (scanline >= 241 && scanline < 261) {
        if (scanline == 241 && cycle == 1) {
            // Effectively end of frame, so set vertical blank flag
            status.vertical_blank = 1;

            // emit a NMI signal to CPU
            if (control.enable_nmi)
                nmi = true;
        }
    }

    // Composition - We now have background & foreground pixel information for this cycle
    // Background =============================================================
    quint8 bg_pixel = 0x00;   // The 2-bit pixel to be rendered
    quint8 bg_palette = 0x00; // The 3-bit index of the palette the pixel indexes

    // Only if rendering is enabled
    if (mask.render_background) {
        if (mask.render_background_left || (cycle >= 9)) {
            // choose the right value from shifter according to fine_x
            quint16 bit_mux = 0x8000 >> fine_x; // 1000 0000 >> fine_x

            // Select Plane pixels by extracting from the shifter
            // at the required location.
            quint8 p0_pixel = (bg_shifter_pattern_lo & bit_mux) > 0;
            quint8 p1_pixel = (bg_shifter_pattern_hi & bit_mux) > 0;

            // Combine to form pixel index
            bg_pixel = (p1_pixel << 1) | p0_pixel;

            // Get palette
            quint8 bg_pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
            quint8 bg_pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
            bg_palette = (bg_pal1 << 1) | bg_pal0;
        }
    }

    // Foreground =============================================================
    quint8 fg_pixel = 0x00;    // The 2-bit pixel to be rendered
    quint8 fg_palette = 0x00;  // The 3-bit index of the palette the pixel indexes
    quint8 fg_priority = 0x00; // A bit of the sprite attribute indicates if its

    // Only if rendering is enabled
    if (mask.render_sprites) {
        if (mask.render_sprites_left || (cycle >= 9)) {
            bSpriteZeroBeingRendered = false;

            // The priority of sprite descend from 0-63
            // Choose the first none transparent sprite
            for (quint8 i = 0; i < sprite_count; i++) {
                // when x==0, should render this sprite
                if (spriteScanline[i].x == 0) {
                    // Determine the pixel value
                    quint8 fg_pixel_lo = (sprite_shifter_pattern_lo[i] & 0x80) > 0;
                    quint8 fg_pixel_hi = (sprite_shifter_pattern_hi[i] & 0x80) > 0;
                    fg_pixel = (fg_pixel_hi << 1) | fg_pixel_lo;

                    // Extract the palette from the bottom two bits
                    fg_palette = (spriteScanline[i].attribute & 0x03) + 0x04;

                    // get the priority
                    fg_priority = (spriteScanline[i].attribute & 0x20) == 0;

                    // If pixel is not transparent, we render it and break
                    if (fg_pixel != 0) {
                        // if it's sprite 0, then tag it
                        // and prepare to check for sprite0hit
                        if (i == 0) {
                            bSpriteZeroBeingRendered = true;
                        }

                        break;
                    }
                }
            }
        }
    }

    // we have a background pixel and a foreground pixel
    // decide to render which based on their priority
    quint8 pixel = 0x00;   // The FINAL Pixel
    quint8 palette = 0x00; // The FINAL Palette

    if (bg_pixel == 0 && fg_pixel == 0) {
        // The background pixel is transparent
        // The foreground pixel is transparent
        // No winner, draw "background" colour
        pixel = 0x00;
        palette = 0x00;
    } else if (bg_pixel == 0 && fg_pixel > 0) {
        // The background pixel is transparent
        // The foreground pixel is visible
        // Foreground wins
        pixel = fg_pixel;
        palette = fg_palette;
    } else if (bg_pixel > 0 && fg_pixel == 0) {
        // The background pixel is visible
        // The foreground pixel is transparent
        // Background wins
        pixel = bg_pixel;
        palette = bg_palette;
    } else if (bg_pixel > 0 && fg_pixel > 0) {
        // The background pixel is visible
        // The foreground pixel is visible
        // we need to look at the sprite priority
        if (fg_priority) {
            // sprite win
            pixel = fg_pixel;
            palette = fg_palette;
        } else {
            // background win
            pixel = bg_pixel;
            palette = bg_palette;
        }

        // Sprite Zero Hit detection
        if (bSpriteZeroHitPossible && bSpriteZeroBeingRendered) {
            // Sprite zero is a collision between foreground and background
            // so they must both be enabled
            if (mask.render_background & mask.render_sprites) {
                // The left edge of the screen has specific switches to control
                // its appearance. This is used to smooth inconsistencies when
                // scrolling (since sprites x coord must be >= 0)
                if (!(mask.render_background_left | mask.render_sprites_left)) {
                    if (cycle >= 9 && cycle < 258) {
                        status.sprite_zero_hit = 1;
                    }
                } else {
                    if (cycle >= 1 && cycle < 258) {
                        status.sprite_zero_hit = 1;
                    }
                }
            }
        }
    }

    // Finally，save the pixel value into frame_data
    int x = cycle - 1, y = scanline;
    if (x >= 0 && x < 256 && y >= 0 && y < 240) {
        quint8 palette_addr = ppuRead(0x3F00 + (palette << 2) + pixel) & 0x3F;
        frame_data[x][y][0] = RGBColorMap[palette_addr][0];
        frame_data[x][y][1] = RGBColorMap[palette_addr][1];
        frame_data[x][y][2] = RGBColorMap[palette_addr][2];
    }

    // Advance renderer - it never stops, it's relentless
    cycle++;
    if (mask.render_background || mask.render_sprites)
        if (cycle == 260 && scanline < 240) {
            cart->mapper_ptr->scanline();
        }

    if (cycle >= 341) {
        cycle = 0;
        scanline++;
        // prepare for next frame
        if (scanline >= 261) {
            scanline = -1;
            frame_complete = true;
            odd_frame = !odd_frame;
        }
    }
}

QDataStream &operator<<(QDataStream &stream, const PPU &Ppu)
{
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 1024; j++)
            stream << Ppu.tblName[i][j];

    for (int i = 0; i < 32; i++)
        stream << Ppu.tblPalette[i];

    stream << Ppu.status.reg;
    stream << Ppu.mask.reg;
    stream << Ppu.control.reg;
    stream << Ppu.vram_addr.reg;
    stream << Ppu.tram_addr.reg;

    stream << Ppu.fine_x;
    stream << Ppu.address_latch;

    stream << Ppu.ppu_data_buffer;

    stream << Ppu.scanline;
    stream << Ppu.cycle;
    stream << Ppu.odd_frame;

    stream << Ppu.bg_next_tile_id;
    stream << Ppu.bg_next_tile_attrib;

    stream << Ppu.bg_next_tile_lsb;
    stream << Ppu.bg_next_tile_msb;

    stream << Ppu.bg_shifter_pattern_lo;
    stream << Ppu.bg_shifter_pattern_hi;
    stream << Ppu.bg_shifter_attrib_lo;
    stream << Ppu.bg_shifter_attrib_hi;

    for (int i = 0; i < 64; i++) {
        stream << Ppu.OAM[i].y;
        stream << Ppu.OAM[i].id;
        stream << Ppu.OAM[i].attribute;
        stream << Ppu.OAM[i].x;
    }

    stream << Ppu.oam_addr;
    stream << Ppu.sprite_count;
    for (int i = 0; i < 8; i++) {
        stream << Ppu.spriteScanline[i].y;
        stream << Ppu.spriteScanline[i].id;
        stream << Ppu.spriteScanline[i].attribute;
        stream << Ppu.spriteScanline[i].x;

        stream << Ppu.sprite_shifter_pattern_lo[i];
        stream << Ppu.sprite_shifter_pattern_hi[i];
    }

    stream << Ppu.bSpriteZeroHitPossible;
    stream << Ppu.bSpriteZeroBeingRendered;
    stream << Ppu.nmi;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, PPU &Ppu)
{
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 1024; j++)
            stream >> Ppu.tblName[i][j];

    for (int i = 0; i < 32; i++)
        stream >> Ppu.tblPalette[i];

    stream >> Ppu.status.reg;
    stream >> Ppu.mask.reg;
    stream >> Ppu.control.reg;
    stream >> Ppu.vram_addr.reg;
    stream >> Ppu.tram_addr.reg;

    stream >> Ppu.fine_x;
    stream >> Ppu.address_latch;

    stream >> Ppu.ppu_data_buffer;

    stream >> Ppu.scanline;
    stream >> Ppu.cycle;
    stream >> Ppu.odd_frame;

    stream >> Ppu.bg_next_tile_id;
    stream >> Ppu.bg_next_tile_attrib;

    stream >> Ppu.bg_next_tile_lsb;
    stream >> Ppu.bg_next_tile_msb;

    stream >> Ppu.bg_shifter_pattern_lo;
    stream >> Ppu.bg_shifter_pattern_hi;
    stream >> Ppu.bg_shifter_attrib_lo;
    stream >> Ppu.bg_shifter_attrib_hi;

    for (int i = 0; i < 64; i++) {
        stream >> Ppu.OAM[i].y;
        stream >> Ppu.OAM[i].id;
        stream >> Ppu.OAM[i].attribute;
        stream >> Ppu.OAM[i].x;
    }

    stream >> Ppu.oam_addr;
    stream >> Ppu.sprite_count;
    for (int i = 0; i < 8; i++) {
        stream >> Ppu.spriteScanline[i].y;
        stream >> Ppu.spriteScanline[i].id;
        stream >> Ppu.spriteScanline[i].attribute;
        stream >> Ppu.spriteScanline[i].x;

        stream >> Ppu.sprite_shifter_pattern_lo[i];
        stream >> Ppu.sprite_shifter_pattern_hi[i];
    }

    stream >> Ppu.bSpriteZeroHitPossible;
    stream >> Ppu.bSpriteZeroBeingRendered;
    stream >> Ppu.nmi;

    return stream;
}
