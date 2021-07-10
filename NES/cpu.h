#ifndef CPU_H
#define CPU_H

#include <QDataStream>
#include <QString>

class Bus; // forward declaration

enum StatusFlag {
    C = (1 << 0), // Carry
    Z = (1 << 1), // Zero
    I = (1 << 2), // Disable Interrupts
    D = (1 << 3), // Decimal Mode（No Use）
    B = (1 << 4), // Break
    U = (1 << 5), // Unused(Unknown)
    V = (1 << 6), // Overflow
    N = (1 << 7)  // Negative
};

// PSW
struct PSW
{
public:
    PSW() { this->data = StatusFlag::U; }
    void set_c(bool c) { c ? (data |= StatusFlag::C) : (data &= 0xfe); }
    void set_z(bool z) { z ? (data |= StatusFlag::Z) : (data &= 0xfd); }
    void set_i(bool i) { i ? (data |= StatusFlag::I) : (data &= 0xfb); }
    void set_d(bool d) { d ? (data |= StatusFlag::D) : (data &= 0xf7); }
    void set_b(bool b) { b ? (data |= StatusFlag::B) : (data &= 0xef); }
    void set_u(bool u) { u ? (data |= StatusFlag::U) : (data &= 0xdf); }
    void set_v(bool v) { v ? (data |= StatusFlag::V) : (data &= 0xbf); }
    void set_n(bool n) { n ? (data |= StatusFlag::N) : (data &= 0x7f); }
    bool get_c() const { return (data & StatusFlag::C) ? 1 : 0; }
    bool get_z() const { return (data & StatusFlag::Z) ? 1 : 0; }
    bool get_i() const { return (data & StatusFlag::I) ? 1 : 0; }
    bool get_d() const { return (data & StatusFlag::D) ? 1 : 0; }
    bool get_b() const { return (data & StatusFlag::B) ? 1 : 0; }
    bool get_v() const { return (data & StatusFlag::V) ? 1 : 0; }
    bool get_n() const { return (data & StatusFlag::N) ? 1 : 0; }

public:
    quint8 data;
};

class CPU
{
    friend QDataStream &operator<<(QDataStream &stream, const CPU &Cpu); // Serialize
    friend QDataStream &operator>>(QDataStream &stream, CPU &Cpu);       // Deserialize
public:
    quint8 reg_a;   // Accumulator Register
    quint8 reg_x;   // X Register
    quint8 reg_y;   // Y Register
    quint16 reg_pc; // Program Counter
    quint8 reg_sp;  // Stack Pointer
    PSW reg_sf;      // Status Register

    // ASM instructions
private:
    // 56 Opcodes
    int ADC(); // Add with carry
    int AND(); // bitwise AND with accumulator
    int ASL(); // Arithmetic Shift Left
    int BCC(); // Branch on Carry Clear
    int BCS(); // Branch on Carry Set
    int BEQ(); // Branch on Equal
    int BIT(); // test bits
    int BMI(); // Branch on Minus
    int BNE(); // Branch on Not Equal
    int BPL(); // Branch on Plus
    int BRK(); // Break
    int BVC(); // Branch on Overflow Clear
    int BVS(); // Branch on Overflow Set
    int CLC(); // Clear Carry
    int CLD(); // Clear Decimal(useless)
    int CLI(); // Clear Interrupt
    int CLV(); // Clear Overflow
    int CMP(); // Compare accumulator
    int CPX(); // Compare X register
    int CPY(); // Compare Y register
    int DEC(); // Decrement memory
    int DEX(); // Decrement X
    int DEY(); // Decrement Y
    int EOR(); // bitwise Exclusive OR
    int INC(); // Increment memory
    int INX(); // Increment X
    int INY(); // Increment X
    int JMP(); // Jump
    int JSR(); // Jump to SubRoutine
    int LDA(); // Load Accumulator
    int LDX(); // Load X register
    int LDY(); // Load Y register
    int LSR(); // Logical Shift Right
    int NOP(); // No Operation
    int ORA(); // bitwise OR with Accumulator
    int PHA(); // Push Accumulator
    int PHP(); // Push Processor status
    int PLA(); // Pull Accumulator
    int PLP(); // Pull Processor status
    int ROL(); // Rotate Left
    int ROR(); // Rotate Right
    int RTI(); // Return from Interrupt
    int RTS(); // Return from Subroutine
    int SBC(); // Subtract with Carry
    int SEC(); // Set Carry
    int SED(); // Set Decimal(useless)
    int SEI(); // Set Interrupt
    int STA(); // Store Accumulator
    int STX(); // Store X register
    int STY(); // Store Y register
    int TAX(); // Transfer A to X
    int TAY(); // Transfer A to Y
    int TSX(); // Transfer Stack ptr to X
    int TXA(); // Transfer X to A
    int TXS(); // Transfer X to Stack ptr
    int TYA(); // Transfer Y to A
    int XXX(); // Illegal Opcode

    // 12 Addressing mode
    int IMP(); // Impilicit
    int IMM(); // Immediate
    int ZP0(); // Zero Page
    int ZPX(); // Zero Page X
    int ZPY(); // Zero Page Y
    int REL(); // Relative
    int ABS(); // Absolute
    int ABX(); // Absolute X
    int ABY(); // Absolute Y
    int IND(); // Indirect（Only for jmp）
    int IZX(); // Indirect X
    int IZY(); // Indirect Y

    struct Instruction
    {
        QString name;               // instruction name
        int (CPU::*operate)(void);  // opcode
        int (CPU::*addrmode)(void); // address mode
        quint8 cycle_cnt;           // cycles needed
    };

    // Instruction set. 256 in total, 105 of them are undefined
    const Instruction inst_table[256] = {
        {"BRK", &CPU::BRK, &CPU::IMM, 7}, {"ORA", &CPU::ORA, &CPU::IZX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::ZP0, 3}, {"ORA", &CPU::ORA, &CPU::ZP0, 3},
        {"ASL", &CPU::ASL, &CPU::ZP0, 5}, {"???", &CPU::XXX, &CPU::IMP, 5},
        {"PHP", &CPU::PHP, &CPU::IMP, 3}, {"ORA", &CPU::ORA, &CPU::IMM, 2},
        {"ASL", &CPU::ASL, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 2},
        {"???", &CPU::NOP, &CPU::ABS, 4}, {"ORA", &CPU::ORA, &CPU::ABS, 4},
        {"ASL", &CPU::ASL, &CPU::ABS, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BPL", &CPU::BPL, &CPU::REL, 2}, {"ORA", &CPU::ORA, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::ZPX, 4}, {"ORA", &CPU::ORA, &CPU::ZPX, 4},
        {"ASL", &CPU::ASL, &CPU::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"CLC", &CPU::CLC, &CPU::IMP, 2}, {"ORA", &CPU::ORA, &CPU::ABY, 4},
        {"???", &CPU::NOP, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::ABX, 4}, {"ORA", &CPU::ORA, &CPU::ABX, 4},
        {"ASL", &CPU::ASL, &CPU::ABX, 7}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"JSR", &CPU::JSR, &CPU::ABS, 6}, {"AND", &CPU::AND, &CPU::IZX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"BIT", &CPU::BIT, &CPU::ZP0, 3}, {"AND", &CPU::AND, &CPU::ZP0, 3},
        {"ROL", &CPU::ROL, &CPU::ZP0, 5}, {"???", &CPU::XXX, &CPU::IMP, 5},
        {"PLP", &CPU::PLP, &CPU::IMP, 4}, {"AND", &CPU::AND, &CPU::IMM, 2},
        {"ROL", &CPU::ROL, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 2},
        {"BIT", &CPU::BIT, &CPU::ABS, 4}, {"AND", &CPU::AND, &CPU::ABS, 4},
        {"ROL", &CPU::ROL, &CPU::ABS, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BMI", &CPU::BMI, &CPU::REL, 2}, {"AND", &CPU::AND, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::ZPX, 4}, {"AND", &CPU::AND, &CPU::ZPX, 4},
        {"ROL", &CPU::ROL, &CPU::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"SEC", &CPU::SEC, &CPU::IMP, 2}, {"AND", &CPU::AND, &CPU::ABY, 4},
        {"???", &CPU::NOP, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::ABX, 4}, {"AND", &CPU::AND, &CPU::ABX, 4},
        {"ROL", &CPU::ROL, &CPU::ABX, 7}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"RTI", &CPU::RTI, &CPU::IMP, 6}, {"EOR", &CPU::EOR, &CPU::IZX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::ZP0, 3}, {"EOR", &CPU::EOR, &CPU::ZP0, 3},
        {"LSR", &CPU::LSR, &CPU::ZP0, 5}, {"???", &CPU::XXX, &CPU::IMP, 5},
        {"PHA", &CPU::PHA, &CPU::IMP, 3}, {"EOR", &CPU::EOR, &CPU::IMM, 2},
        {"LSR", &CPU::LSR, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 2},
        {"JMP", &CPU::JMP, &CPU::ABS, 3}, {"EOR", &CPU::EOR, &CPU::ABS, 4},
        {"LSR", &CPU::LSR, &CPU::ABS, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BVC", &CPU::BVC, &CPU::REL, 2}, {"EOR", &CPU::EOR, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::ZPX, 4}, {"EOR", &CPU::EOR, &CPU::ZPX, 4},
        {"LSR", &CPU::LSR, &CPU::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"CLI", &CPU::CLI, &CPU::IMP, 2}, {"EOR", &CPU::EOR, &CPU::ABY, 4},
        {"???", &CPU::NOP, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::ABX, 4}, {"EOR", &CPU::EOR, &CPU::ABX, 4},
        {"LSR", &CPU::LSR, &CPU::ABX, 7}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"RTS", &CPU::RTS, &CPU::IMP, 6}, {"ADC", &CPU::ADC, &CPU::IZX, 6},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::ZP0, 3}, {"ADC", &CPU::ADC, &CPU::ZP0, 3},
        {"ROR", &CPU::ROR, &CPU::ZP0, 5}, {"???", &CPU::XXX, &CPU::IMP, 5},
        {"PLA", &CPU::PLA, &CPU::IMP, 4}, {"ADC", &CPU::ADC, &CPU::IMM, 2},
        {"ROR", &CPU::ROR, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 2},
        {"JMP", &CPU::JMP, &CPU::IND, 5}, {"ADC", &CPU::ADC, &CPU::ABS, 4},
        {"ROR", &CPU::ROR, &CPU::ABS, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BVS", &CPU::BVS, &CPU::REL, 2}, {"ADC", &CPU::ADC, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::ZPX, 4}, {"ADC", &CPU::ADC, &CPU::ZPX, 4},
        {"ROR", &CPU::ROR, &CPU::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"SEI", &CPU::SEI, &CPU::IMP, 2}, {"ADC", &CPU::ADC, &CPU::ABY, 4},
        {"???", &CPU::NOP, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::ABX, 4}, {"ADC", &CPU::ADC, &CPU::ABX, 4},
        {"ROR", &CPU::ROR, &CPU::ABX, 7}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::IMM, 2}, {"STA", &CPU::STA, &CPU::IZX, 6},
        {"???", &CPU::NOP, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"STY", &CPU::STY, &CPU::ZP0, 3}, {"STA", &CPU::STA, &CPU::ZP0, 3},
        {"STX", &CPU::STX, &CPU::ZP0, 3}, {"???", &CPU::XXX, &CPU::IMP, 3},
        {"DEY", &CPU::DEY, &CPU::IMP, 2}, {"???", &CPU::NOP, &CPU::IMP, 2},
        {"TXA", &CPU::TXA, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 2},
        {"STY", &CPU::STY, &CPU::ABS, 4}, {"STA", &CPU::STA, &CPU::ABS, 4},
        {"STX", &CPU::STX, &CPU::ABS, 4}, {"???", &CPU::XXX, &CPU::IMP, 4},
        {"BCC", &CPU::BCC, &CPU::REL, 2}, {"STA", &CPU::STA, &CPU::IZY, 6},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"STY", &CPU::STY, &CPU::ZPX, 4}, {"STA", &CPU::STA, &CPU::ZPX, 4},
        {"STX", &CPU::STX, &CPU::ZPY, 4}, {"???", &CPU::XXX, &CPU::IMP, 4},
        {"TYA", &CPU::TYA, &CPU::IMP, 2}, {"STA", &CPU::STA, &CPU::ABY, 5},
        {"TXS", &CPU::TXS, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 5},
        {"???", &CPU::NOP, &CPU::IMP, 5}, {"STA", &CPU::STA, &CPU::ABX, 5},
        {"???", &CPU::XXX, &CPU::IMP, 5}, {"???", &CPU::XXX, &CPU::IMP, 5},
        {"LDY", &CPU::LDY, &CPU::IMM, 2}, {"LDA", &CPU::LDA, &CPU::IZX, 6},
        {"LDX", &CPU::LDX, &CPU::IMM, 2}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"LDY", &CPU::LDY, &CPU::ZP0, 3}, {"LDA", &CPU::LDA, &CPU::ZP0, 3},
        {"LDX", &CPU::LDX, &CPU::ZP0, 3}, {"???", &CPU::XXX, &CPU::IMP, 3},
        {"TAY", &CPU::TAY, &CPU::IMP, 2}, {"LDA", &CPU::LDA, &CPU::IMM, 2},
        {"TAX", &CPU::TAX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 2},
        {"LDY", &CPU::LDY, &CPU::ABS, 4}, {"LDA", &CPU::LDA, &CPU::ABS, 4},
        {"LDX", &CPU::LDX, &CPU::ABS, 4}, {"???", &CPU::XXX, &CPU::IMP, 4},
        {"BCS", &CPU::BCS, &CPU::REL, 2}, {"LDA", &CPU::LDA, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 5},
        {"LDY", &CPU::LDY, &CPU::ZPX, 4}, {"LDA", &CPU::LDA, &CPU::ZPX, 4},
        {"LDX", &CPU::LDX, &CPU::ZPY, 4}, {"???", &CPU::XXX, &CPU::IMP, 4},
        {"CLV", &CPU::CLV, &CPU::IMP, 2}, {"LDA", &CPU::LDA, &CPU::ABY, 4},
        {"TSX", &CPU::TSX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 4},
        {"LDY", &CPU::LDY, &CPU::ABX, 4}, {"LDA", &CPU::LDA, &CPU::ABX, 4},
        {"LDX", &CPU::LDX, &CPU::ABY, 4}, {"???", &CPU::XXX, &CPU::IMP, 4},
        {"CPY", &CPU::CPY, &CPU::IMM, 2}, {"CMP", &CPU::CMP, &CPU::IZX, 6},
        {"???", &CPU::NOP, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"CPY", &CPU::CPY, &CPU::ZP0, 3}, {"CMP", &CPU::CMP, &CPU::ZP0, 3},
        {"DEC", &CPU::DEC, &CPU::ZP0, 5}, {"???", &CPU::XXX, &CPU::IMP, 5},
        {"INY", &CPU::INY, &CPU::IMP, 2}, {"CMP", &CPU::CMP, &CPU::IMM, 2},
        {"DEX", &CPU::DEX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 2},
        {"CPY", &CPU::CPY, &CPU::ABS, 4}, {"CMP", &CPU::CMP, &CPU::ABS, 4},
        {"DEC", &CPU::DEC, &CPU::ABS, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BNE", &CPU::BNE, &CPU::REL, 2}, {"CMP", &CPU::CMP, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::ZPX, 4}, {"CMP", &CPU::CMP, &CPU::ZPX, 4},
        {"DEC", &CPU::DEC, &CPU::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"CLD", &CPU::CLD, &CPU::IMP, 2}, {"CMP", &CPU::CMP, &CPU::ABY, 4},
        {"NOP", &CPU::NOP, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::ABX, 4}, {"CMP", &CPU::CMP, &CPU::ABX, 4},
        {"DEC", &CPU::DEC, &CPU::ABX, 7}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"CPX", &CPU::CPX, &CPU::IMM, 2}, {"SBC", &CPU::SBC, &CPU::IZX, 6},
        {"???", &CPU::NOP, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"CPX", &CPU::CPX, &CPU::ZP0, 3}, {"SBC", &CPU::SBC, &CPU::ZP0, 3},
        {"INC", &CPU::INC, &CPU::ZP0, 5}, {"???", &CPU::XXX, &CPU::IMP, 5},
        {"INX", &CPU::INX, &CPU::IMP, 2}, {"SBC", &CPU::SBC, &CPU::IMM, 2},
        {"NOP", &CPU::NOP, &CPU::IMP, 2}, {"???", &CPU::SBC, &CPU::IMP, 2},
        {"CPX", &CPU::CPX, &CPU::ABS, 4}, {"SBC", &CPU::SBC, &CPU::ABS, 4},
        {"INC", &CPU::INC, &CPU::ABS, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"BEQ", &CPU::BEQ, &CPU::REL, 2}, {"SBC", &CPU::SBC, &CPU::IZY, 5},
        {"???", &CPU::XXX, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 8},
        {"???", &CPU::NOP, &CPU::ZPX, 4}, {"SBC", &CPU::SBC, &CPU::ZPX, 4},
        {"INC", &CPU::INC, &CPU::ZPX, 6}, {"???", &CPU::XXX, &CPU::IMP, 6},
        {"SED", &CPU::SED, &CPU::IMP, 2}, {"SBC", &CPU::SBC, &CPU::ABY, 4},
        {"NOP", &CPU::NOP, &CPU::IMP, 2}, {"???", &CPU::XXX, &CPU::IMP, 7},
        {"???", &CPU::NOP, &CPU::ABX, 4}, {"SBC", &CPU::SBC, &CPU::ABX, 4},
        {"INC", &CPU::INC, &CPU::ABX, 7}, {"???", &CPU::XXX, &CPU::IMP, 7},
    };

public:
    CPU(Bus * = nullptr);
    void connectToBus(Bus *);
    void reset();                   // Reset
    void irq();                     // Interrupt Request
    void nmi();                     // Non-Maskable Interrupt
    void push_stack(quint8 value);
    quint8 pull_stack();
    void clock(); // run 1 cycle
    void print_log() const;
    void update_curr_instruction(); // this is for debugger

    quint16 addr_abs; // absolute address
    quint16 addr_rel; // relative address
    Bus *p_ram;
    quint8 cycles_wait; // cycles left for current instruction
    quint8 opcode;      // current opcode

    // debug
    bool isDebugging;
    uint64_t clock_count;     // For debugger(unused now)
    quint16 oprand_for_log;   // data used by current instruction
    quint8 address_mode;      // address mode of current instruction
    QString curr_instruction; // current instruction, example: LDA 2002H
};

#endif // CPU_H
