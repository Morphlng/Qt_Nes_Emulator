#ifndef PPU2_H
#define PPU2_H

#include "cartridge.h"
#include "palette.h"
#include <QDataStream>

class PPU
{
    friend QDataStream &operator<<(QDataStream &stream, const PPU &Ppu); // Serialize
    friend QDataStream &operator>>(QDataStream &stream, PPU &Ppu);       // Deserialize
public:
    PPU();
    ~PPU();

private:
    quint8 tblName[2][1024]; // NameTable (attribute table included)
    quint8 tblPalette[32];   // palette (8 in all, 4 for background, 4 for sprites)

public:
    quint8 frame_data[256][240][3]; // save the RGB for each pixels
    bool frame_complete = false;    // flag indicate a frame has done

private:
    union PPUSTATUS {
        struct
        {
            quint8 unused : 5;
            quint8 sprite_overflow : 1;
            quint8 sprite_zero_hit : 1;
            quint8 vertical_blank : 1;
        };

        quint8 reg;
    } status;

    union PPUMASK {
        struct
        {
            quint8 grayscale : 1;
            quint8 render_background_left : 1; // whether show the left most 8 bgpixels
            quint8 render_sprites_left : 1;    // whether show the left most 8 sprite pixels
            quint8 render_background : 1;      // render bg or not
            quint8 render_sprites : 1;         // render sprite or not
            quint8 enhance_red : 1;            // These three bit are useless for most games
            quint8 enhance_green : 1;
            quint8 enhance_blue : 1;
        };

        quint8 reg;
    } mask;

    union PPUCTRL {
        struct
        {
            quint8 nametable_x : 1;
            quint8 nametable_y : 1;
            quint8 increment_mode : 1; // for ppu_address increment；+32 when scanline changes，or +1 to simply move to next pixel
            quint8 pattern_sprite : 1;
            quint8 pattern_background : 1;
            quint8 sprite_size : 1; // sprite could be 8x8 or 8x16(tiles)
            quint8 slave_mode : 1;  // unused
            quint8 enable_nmi : 1;
        };

        quint8 reg;
    } control;

    // For more details about the loopy_register:
    // http://nesdev.com/loopyppu.zip
    union loopy_register {
        struct
        {
            // coarse_y and coarse_x represent the offset of current tile in nametable
            // in other word: y*width+x
            quint16 coarse_x : 5;
            quint16 coarse_y : 5;

            // some game could at most address 4 nametable
            // so we need 2bit to tag them, 00、01、10、11
            quint16 nametable_x : 1;
            quint16 nametable_y : 1;

            // a tile is 8*8 pixels
            // we need 3bit to confirm the accurate offset within one tile
            // fine_y is for verical purpose, fine_x is outside the structure
            quint16 fine_y : 3;
            quint16 unused : 1;
        };

        // According to the loopy_register, while rendering each pixel:
        // 1. first, fine_x++ to move to the next pixel
        // 2. when fine_x>7, coarse_x++ to move to the next tile horizontally
        // 3. similarly fine_y++ to move to next line
        // 4. when fine_y>7, coarse_y++ to move to the next tile vertically
        // 5. we might need to switch nametable from time to time
        // 6. that's when you flip nametable_x(y)

        quint16 reg = 0x0000;
    };

    // The reason why you need 2 loopy_register：http://wiki.nesdev.com/w/index.php/PPU_scrolling

    // Active "pointer" address into nametable to extract background tile info
    loopy_register vram_addr;

    // Temporary store of information to be "transferred" into "pointer" at various times
    loopy_register tram_addr;

    // Pixel offset horizontally
    quint8 fine_x = 0x00;

    bool address_latch = false;

    // most of the time, CPU needs an extra cycle to read from PPU
    // so we need to buffer things
    quint8 ppu_data_buffer = 0x00;

    int16_t scanline = 0;   // you can see it as y
    int16_t cycle = 0;      // you can see it as x
    bool odd_frame = false; // on odd frame there would be a skip frame

    // Background rendering =========================================
    quint8 bg_next_tile_id = 0x00;
    quint8 bg_next_tile_attrib = 0x00;

    // 4 colors available for 1 pixel, we only need 2bit to represent that
    // we can further divide that 2bit into lsb(least significant bit) and msb(most significant bit)
    quint8 bg_next_tile_lsb = 0x00;
    quint8 bg_next_tile_msb = 0x00;

    // shifter will save 8 pixels currently rendering and 8 pixels about to render
    quint16 bg_shifter_pattern_lo = 0x0000;
    quint16 bg_shifter_pattern_hi = 0x0000;
    quint16 bg_shifter_attrib_lo = 0x0000;
    quint16 bg_shifter_attrib_hi = 0x0000;

    // Foreground "Sprite" rendering ================================
    // The OAM is an additional memory internal to the PPU. It is
    // not connected via the any bus. It stores the locations of
    // 64 of 8x8 (or 8x16) tiles to be drawn on the next frame.
    struct sObjectAttributeEntry
    {
        quint8 y;         // Y position of sprite
        quint8 id;        // ID of tile from pattern memory
        quint8 attribute; // Flags define how sprite should be rendered
        quint8 x;         // X position of sprite
    } OAM[64];

    // A register to store the address when the CPU manually communicates
    // with OAM via PPU registers. This is not commonly used because it
    // is very slow, and instead a 256-Byte DMA transfer is used. See
    // the Bus header for a description of this.
    quint8 oam_addr = 0x00;

    // Only load 8 sprites each line
    sObjectAttributeEntry spriteScanline[8];
    quint8 sprite_count;                 // if sprite_count > 8, will flag overflow in PPU_STATUS
    quint8 sprite_shifter_pattern_lo[8]; // save the information of sprites we're about to render
    quint8 sprite_shifter_pattern_hi[8];

    // Sprite Zero Collision Flags
    bool bSpriteZeroHitPossible = false;
    bool bSpriteZeroBeingRendered = false;

public:
    // 开放给CPU读取OAM的入口
    quint8 *pOAM = (quint8 *) OAM;

private:
    // 渲染阶段相关函数（用于clock）
    void IncrementScrollX();       // make the tile "pointer" increse horizontally
    void IncrementScrollY();       // make the tile "pointer" increse vertically
    void TransferAddressX();       // copy X info from tram to vram
    void TransferAddressY();       // copy Y info from tram to vram
    void LoadBackgroundShifters(); // Initialize shfiter
    void UpdateShifters(); // left shift the shifter, means that a pixel has already rendered

public:
    // CPU relevant functions
    // read
    quint8 get_status();
    quint8 get_oamdata();
    quint8 read_data();

    // write
    void write_ctrl(quint8 ctrl);
    void write_mask(quint8 mask);
    void write_scroll(quint8 scroll);
    void write_addr(quint8 addr);
    void write_data(quint8 data);

    void write_oamaddr(quint8 addr);
    void write_oamdata(quint8 data);

    // PPU_bus relevant functions
    quint8 ppuRead(quint16 addr);
    void ppuWrite(quint16 addr, quint8 data);

private:
    // Connect the cartridge to PPU_bus
    Cartridge *cart;

public:
    // For Bus to call
    void ConnectCartridge(Cartridge *cartridge);
    void clock();
    void reset();
    bool nmi = false;
};

#endif // PPU2_H
