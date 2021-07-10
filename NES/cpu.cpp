#include "cpu.h"
#include "bus.h"
#include <QDebug>
#include <QMessageBox>

CPU::CPU(Bus *bus) : addr_abs(0), addr_rel(0), cycles_wait(0), opcode(0), clock_count(0)
{
    isDebugging = false;
    this->p_ram = bus;
}

void CPU::connectToBus(Bus *bus)
{
    this->p_ram = bus;
}

void CPU::push_stack(quint8 value)
{
    if (reg_sp == 0) {
        QMessageBox::critical(nullptr,
                              QStringLiteral("ERROR"),
                              QStringLiteral("Stack Overflow! Exiting..."));
        abort();
    }
    // 0-255 is Zero Page
    // Stack start from 256, so the offset is 0x100
    p_ram->save(reg_sp + 0x100, value);
    reg_sp--;
}

quint8 CPU::pull_stack()
{
    reg_sp++;
    quint8 res = p_ram->load(reg_sp + 0x100);
    return res;
}

void CPU::reset()
{
    reg_a = 0;
    reg_x = 0;
    reg_y = 0;
    reg_sp = 0xfd;
    reg_sf.set_i(true); // block IRQ
    reg_sf.set_u(true);

    // Little-endian
    quint8 lo8 = p_ram->load(0xFFFC);
    quint8 hi8 = p_ram->load(0xFFFD);
    reg_pc = quint16(hi8 << 8) + lo8;
    addr_abs = 0;
    addr_rel = 0;

    cycles_wait = 8;
}

void CPU::irq()
{
    // if irq is allowed
    if (reg_sf.get_i() == 0) {
        // 1. Protect Scene
        // Push PC and PSW into stack
        push_stack(reg_pc >> 8);
        push_stack(reg_pc & 0xFF);
        reg_sf.set_b(false);
        reg_sf.set_u(true);
        push_stack(reg_sf.data);
        reg_sf.set_i(true); // disable interrupt
        // 2. Load Interrupt handling program
        quint8 lo8 = p_ram->load(0xFFFE);
        quint8 hi8 = p_ram->load(0xFFFF);
        reg_pc = quint16(hi8 << 8) + lo8;
        // 3. extra wait cycles
        cycles_wait = 7;
    }
}

void CPU::nmi()
{
    // 1. Protect Scene
    push_stack(reg_pc >> 8);
    push_stack(reg_pc & 0xFF);
    reg_sf.set_b(false);
    reg_sf.set_u(true);
    push_stack(reg_sf.data);
    reg_sf.set_i(true);
    // 2. Load Interrupt handling program
    quint8 lo8 = p_ram->load(0xFFFA);
    quint8 hi8 = p_ram->load(0xFFFB);
    reg_pc = quint16(hi8 << 8) + lo8;
    // 3. extra wait cycles
    // qDebug() << "NMI, reg_pc = " << reg_pc;
    cycles_wait = 7;
}

// Address Mode
int CPU::IMP()
{
    address_mode = 0;
    return 0;
}

int CPU::IMM()
{
    addr_abs = reg_pc;
    reg_pc++;
    oprand_for_log = p_ram->load(addr_abs);
    address_mode = 1;
    return 0;
}

int CPU::ZP0()
{
    addr_abs = p_ram->load(reg_pc);
    reg_pc++;
    addr_abs &= 0x00FF;
    oprand_for_log = quint16(addr_abs);
    address_mode = 2;
    return 0;
}

int CPU::ZPX()
{
    oprand_for_log = p_ram->load(reg_pc);
    address_mode = 3;
    addr_abs = p_ram->load(reg_pc) + reg_x;
    reg_pc++;
    addr_abs &= 0x00FF;
    return 0;
}

int CPU::ZPY()
{
    oprand_for_log = p_ram->load(reg_pc);
    address_mode = 4;
    addr_abs = p_ram->load(reg_pc) + reg_y;
    reg_pc++;
    addr_abs &= 0x00FF;
    return 0;
}

int CPU::REL()
{
    addr_rel = p_ram->load(reg_pc);
    oprand_for_log = quint16(addr_rel);
    address_mode = 5;
    reg_pc++;
    if (addr_rel & 0x80)    // Check if it's negative
        addr_rel |= 0xFF00; // two's complement of negative
    return 0;
}

int CPU::ABS()
{
    quint8 lo8 = p_ram->load(reg_pc);
    quint8 hi8 = p_ram->load(reg_pc + 1);
    reg_pc += 2;
    addr_abs = quint16(hi8 << 8) + lo8;
    oprand_for_log = quint16(addr_abs);
    address_mode = 6;
    return 0;
}

int CPU::ABX()
{
    quint8 lo8 = p_ram->load(reg_pc);
    quint8 hi8 = p_ram->load(reg_pc + 1);
    reg_pc += 2;
    addr_abs = quint16(hi8 << 8) + lo8 + reg_x;
    oprand_for_log = quint16((hi8 << 8) + lo8);
    address_mode = 7;
    // change page needs an extra cycle
    if ((hi8 << 8) != (addr_abs & 0xFF00))
        return 1;
    else
        return 0;
}

int CPU::ABY()
{
    quint8 lo8 = p_ram->load(reg_pc);
    quint8 hi8 = p_ram->load(reg_pc + 1);
    reg_pc += 2;
    addr_abs = quint16(hi8 << 8) + lo8 + reg_y;
    oprand_for_log = quint16((hi8 << 8) + lo8);
    address_mode = 8;
    // change page needs an extra cycle
    if ((hi8 << 8) != (addr_abs & 0xFF00))
        return 1;
    else
        return 0;
}

int CPU::IND()
{
    quint8 p_lo8 = p_ram->load(reg_pc);
    quint8 p_hi8 = p_ram->load(reg_pc + 1);
    reg_pc += 2;
    quint16 ptr = quint16(p_hi8 << 8) + p_lo8;
    oprand_for_log = ptr;
    address_mode = 9;

    // this is a hardware bug.
    // when address is xxFF, instead of xx+1 page, it will goto xx00
    // we need to implement this bug
    if (p_lo8 == 0xFF)
        addr_abs = (p_ram->load(ptr & 0xFF00) << 8) + (p_ram->load(ptr));
    else
        addr_abs = (p_ram->load(ptr + 1) << 8) + (p_ram->load(ptr));
    return 0;
}

int CPU::IZX()
{
    quint8 ptr = p_ram->load(reg_pc);
    oprand_for_log = ptr;
    address_mode = 10;
    reg_pc++;
    quint8 lo8 = p_ram->load((ptr + reg_x) & 0x00FF);
    quint8 hi8 = p_ram->load((ptr + reg_x + 1) & 0x00FF);
    addr_abs = (hi8 << 8) + lo8;
    return 0;
}

int CPU::IZY()
{
    quint8 ptr = p_ram->load(reg_pc);
    oprand_for_log = ptr;
    address_mode = 11;
    reg_pc++;
    quint8 lo8 = p_ram->load(ptr & 0x00FF);
    quint8 hi8 = p_ram->load((ptr + 1) & 0x00FF);
    addr_abs = (hi8 << 8) + lo8 + reg_y;
    // change page needs an extra cycle
    if ((hi8 << 8) != (addr_abs & 0xFF00))
        return 1;
    else
        return 0;
}

// Opcode
int CPU::ADC()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // Add. Pay attention to the overflow flag
    quint16 sum = reg_a + operand + reg_sf.get_c();
    reg_sf.set_c(sum >= 256);
    reg_sf.set_v((reg_a ^ sum) & (operand ^ sum) & 0x80);
    reg_sf.set_z((sum & 0xFF) == 0);
    reg_sf.set_n(sum & 0x80);
    reg_a = sum & 0xFF;
    return 1;
}

int CPU::AND()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // And
    reg_a = reg_a & operand;
    reg_sf.set_z(reg_a == 0);
    reg_sf.set_n(reg_a & 0x80);
    return 1;
}

int CPU::ASL()
{
    // IMP gives the data right away
    if (inst_table[opcode].addrmode == &CPU::IMP) {
        // IMP(Accumulator)
        quint16 temp = quint16(reg_a << 1);
        reg_sf.set_c(temp >= 0x100);
        reg_sf.set_z((temp & 0x00FF) == 0);
        reg_sf.set_n(temp & 0x80);
        reg_a = temp & 0x00FF;
    } else {
        // fetch data
        quint8 operand = p_ram->load(addr_abs);
        quint16 temp = quint16(operand << 1);
        reg_sf.set_c(temp >= 0x100);
        reg_sf.set_z((temp & 0x00FF) == 0);
        reg_sf.set_n(temp & 0x80);
        p_ram->save(addr_abs, temp & 0x00FF);
    }
    return 0;
}

int CPU::BCC()
{
    // Branch on Carry Clear，check if C=0
    quint8 cycles_add = 0;
    if (reg_sf.get_c() == 0) {
        addr_abs = reg_pc + addr_rel;
        // change page needs an extra cycle，
        // branch needs and extra cycle too
        if ((addr_abs & 0xFF00) != (reg_pc & 0xFF00))
            cycles_add = 2;
        else
            cycles_add = 1;
        reg_pc = quint16(addr_abs);
    }
    return -cycles_add;
}

int CPU::BCS()
{
    // Branch on Carry Set，check if C=1
    quint8 cycles_add = 0;
    if (reg_sf.get_c() == 1) {
        addr_abs = reg_pc + addr_rel;
        // change page needs an extra cycle，
        // branch needs and extra cycle too
        if ((addr_abs & 0xFF00) != (reg_pc & 0xFF00))
            cycles_add = 2;
        else
            cycles_add = 1;
        reg_pc = quint16(addr_abs);
    }
    return -cycles_add;
}

int CPU::BEQ()
{
    // Branch on Equal，check if Z=1
    quint8 cycles_add = 0;
    if (reg_sf.get_z() == 1) {
        addr_abs = reg_pc + addr_rel;
        // change page needs an extra cycle，
        // branch needs and extra cycle too
        if ((addr_abs & 0xFF00) != (reg_pc & 0xFF00))
            cycles_add = 2;
        else
            cycles_add = 1;
        reg_pc = quint16(addr_abs);
    }
    return -cycles_add;
}

int CPU::BIT()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);

    reg_sf.set_z((reg_a & operand) == 0);
    reg_sf.set_v(operand & (1 << 6));
    reg_sf.set_n(operand & (1 << 7));
    return 0;
}

int CPU::BMI()
{
    // Branch on Minus，check if N=1
    quint8 cycles_add = 0;
    if (reg_sf.get_n() == 1) {
        addr_abs = reg_pc + addr_rel;
        // change page needs an extra cycle，
        // branch needs and extra cycle too
        if ((addr_abs & 0xFF00) != (reg_pc & 0xFF00))
            cycles_add = 2;
        else
            cycles_add = 1;
        reg_pc = quint16(addr_abs);
    }
    return -cycles_add;
}

int CPU::BNE()
{
    // Branch on not Equal，check if Z=0
    quint8 cycles_add = 0;
    if (reg_sf.get_z() == 0) {
        addr_abs = reg_pc + addr_rel;
        // change page needs an extra cycle，
        // branch needs and extra cycle too
        if ((addr_abs & 0xFF00) != (reg_pc & 0xFF00))
            cycles_add = 2;
        else
            cycles_add = 1;
        reg_pc = quint16(addr_abs);
    }
    return -cycles_add;
}

int CPU::BPL()
{
    // Branch on Plus，check if N=0
    quint8 cycles_add = 0;
    if (reg_sf.get_n() == 0) {
        addr_abs = reg_pc + addr_rel;
        // change page needs an extra cycle，
        // branch needs and extra cycle too
        if ((addr_abs & 0xFF00) != (reg_pc & 0xFF00))
            cycles_add = 2;
        else
            cycles_add = 1;
        reg_pc = quint16(addr_abs);
    }
    return -cycles_add;
}

int CPU::BRK()
{
    reg_pc++;
    // 1. Protect Scene
    push_stack(reg_pc >> 8);
    push_stack(reg_pc & 0xFF);
    reg_sf.set_b(true);
    reg_sf.set_i(true);
    push_stack(reg_sf.data);
    reg_sf.set_b(false);
    // 2. Load Interrupt handling program
    quint8 lo8 = p_ram->load(0xFFFE);
    quint8 hi8 = p_ram->load(0xFFFF);
    reg_pc = quint16(hi8 << 8) + lo8;
    return 0;
}

int CPU::BVC()
{
    // Branch on overflow clear，check if V=0
    quint8 cycles_add = 0;
    if (reg_sf.get_v() == 0) {
        addr_abs = reg_pc + addr_rel;
        // change page needs an extra cycle，
        // branch needs and extra cycle too
        if ((addr_abs & 0xFF00) != (reg_pc & 0xFF00))
            cycles_add = 2;
        else
            cycles_add = 1;
        reg_pc = quint16(addr_abs);
    }
    return -cycles_add;
}

int CPU::BVS()
{
    // Branch on overflow set，check if V=1
    quint8 cycles_add = 0;
    if (reg_sf.get_v() == 1) {
        addr_abs = reg_pc + addr_rel;
        // change page needs an extra cycle，
        // branch needs and extra cycle too
        if ((addr_abs & 0xFF00) != (reg_pc & 0xFF00))
            cycles_add = 2;
        else
            cycles_add = 1;
        reg_pc = quint16(addr_abs);
    }
    return -cycles_add;
}
int CPU::CLC()
{
    // Clear Carry
    reg_sf.set_c(false);
    return 0;
}

int CPU::CLD()
{
    // Clear Decimal
    reg_sf.set_d(false);
    return 0;
}

int CPU::CLI()
{
    // Clear Interrupt
    reg_sf.set_i(false);
    return 0;
}

int CPU::CLV()
{
    // Clear Overflow
    reg_sf.set_v(false);
    return 0;
}

int CPU::CMP()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // compare with Accumulator
    quint16 temp = reg_a - operand;
    reg_sf.set_c(reg_a >= operand);
    reg_sf.set_z((temp & 0x00FF) == 0);
    reg_sf.set_n(bool(temp & 0x0080));
    return 1;
}

int CPU::CPX()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // compare with X
    quint16 temp = reg_x - operand;
    reg_sf.set_c(reg_x >= operand);
    reg_sf.set_z((temp & 0x00FF) == 0);
    reg_sf.set_n(bool(temp & 0x0080));
    return 0;
}

int CPU::CPY()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // compare with Y
    quint16 temp = reg_y - operand;
    reg_sf.set_c(reg_y >= operand);
    reg_sf.set_z((temp & 0x00FF) == 0);
    reg_sf.set_n(bool(temp & 0x0080)); //
    return 0;
}

int CPU::DEC()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // Decrement memory
    quint16 res = operand - 1;
    p_ram->save(addr_abs, res & 0x00FF);
    reg_sf.set_z((res & 0x00FF) == 0);
    reg_sf.set_n(bool(res & 0x0080));
    return 0;
}

int CPU::DEX()
{
    // X--
    reg_x--;
    reg_sf.set_z(reg_x == 0);
    reg_sf.set_n(bool(reg_x & 0x0080));
    return 0;
}

int CPU::DEY()
{
    // Y--
    reg_y--;
    reg_sf.set_z(reg_y == 0);
    reg_sf.set_n(bool(reg_y & 0x0080));
    return 0;
}

int CPU::EOR()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // xor
    reg_a = reg_a ^ operand;
    reg_sf.set_z(reg_a == 0);
    reg_sf.set_n(bool(reg_a & 0x0080));
    return 1;
}

int CPU::INC()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // Increment Memory
    quint16 res = operand + 1;
    p_ram->save(addr_abs, res & 0x00FF);
    reg_sf.set_z((res & 0x00FF) == 0);
    reg_sf.set_n(bool(res & 0x0080));
    return 0;
}

int CPU::INX()
{
    // X++
    reg_x++;
    reg_sf.set_z(reg_x == 0);
    reg_sf.set_n(bool(reg_x & 0x0080));
    return 0;
}

int CPU::INY()
{
    // Y++
    reg_y++;
    reg_sf.set_z(reg_y == 0);
    reg_sf.set_n(bool(reg_y & 0x0080));
    return 0;
}

int CPU::JMP()
{
    // jmp
    reg_pc = quint16(addr_abs);
    return 0;
}

int CPU::JSR()
{
    // save current PC
    push_stack((reg_pc - 1) >> 8);
    push_stack((reg_pc - 1) & 0xFF);
    // Jump to subroutine
    reg_pc = quint16(addr_abs);
    return 0;
}

int CPU::LDA()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // Load Accumulator
    reg_a = operand;
    reg_sf.set_z(reg_a == 0);
    reg_sf.set_n(bool(reg_a & 0x0080));
    return 1;
}

int CPU::LDX()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // Load X
    reg_x = operand;
    reg_sf.set_z(reg_x == 0);
    reg_sf.set_n(bool(reg_x & 0x0080));
    return 1;
}

int CPU::LDY()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // Load Y
    reg_y = operand;
    reg_sf.set_z(reg_y == 0);
    reg_sf.set_n(bool(reg_y & 0x0080));
    return 1;
}

int CPU::LSR()
{
    // IMP gives the data right away
    if (inst_table[opcode].addrmode == &CPU::IMP) {
        //IMP(Accumulator)
        quint16 temp = quint16(reg_a >> 1);
        reg_sf.set_c(reg_a & 0x0001);
        reg_sf.set_z((temp & 0x00FF) == 0);
        reg_sf.set_n(temp & 0x80);
        reg_a = temp & 0x00FF;
    } else {
        // fetch data
        quint8 operand = p_ram->load(addr_abs);
        quint16 temp = quint16(operand >> 1);
        reg_sf.set_c(operand & 0x0001);
        reg_sf.set_z((temp & 0x00FF) == 0);
        reg_sf.set_n(temp & 0x80);
        p_ram->save(addr_abs, temp & 0x00FF);
    }
    return 0;
}

int CPU::NOP()
{
    // No Op
    switch (opcode) {
    case 0x1C:
    case 0x3C:
    case 0x5C:
    case 0x7C:
    case 0xDC:
    case 0xFC:
        return 1;
        break;
    }
    return 0;
}

int CPU::ORA()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // Or
    reg_a = reg_a | operand;
    reg_sf.set_z(reg_a == 0);
    reg_sf.set_n(bool(reg_a & 0x0080));
    return 1;
}

int CPU::PHA()
{
    // Push Accumulator
    push_stack(reg_a);
    return 0;
}

int CPU::PHP()
{
    // Push Status
    push_stack(reg_sf.data | (1 << 4) | (1 << 5));
    reg_sf.set_b(false);
    reg_sf.set_u(false);
    return 0;
}

int CPU::PLA()
{
    // Pull Accumulator
    reg_a = pull_stack();
    reg_sf.set_z(reg_a == 0);
    reg_sf.set_n(bool(reg_a & 0x0080));
    return 0;
}

int CPU::PLP()
{
    // Pull Status
    reg_sf.data = pull_stack();
    reg_sf.set_u(true);
    return 0;
}

int CPU::ROL()
{
    // IMP gives the data right away
    if (inst_table[opcode].addrmode == &CPU::IMP) {
        // IMP(Accumulator)
        quint16 temp = quint16(reg_a << 1) | reg_sf.get_c(); //
        reg_sf.set_c(temp >= 0x100);
        reg_sf.set_z((temp & 0x00FF) == 0);
        reg_sf.set_n(temp & 0x80);
        reg_a = temp & 0x00FF;
    } else {
        // fetch data
        quint8 operand = p_ram->load(addr_abs);
        quint16 temp = quint16(operand << 1) | reg_sf.get_c();
        reg_sf.set_c(temp >= 0x100);
        reg_sf.set_z((temp & 0x00FF) == 0);
        reg_sf.set_n(temp & 0x80);
        p_ram->save(addr_abs, temp & 0x00FF);
    }
    return 0;
}

int CPU::ROR()
{
    // IMP gives the data right away
    if (inst_table[opcode].addrmode == &CPU::IMP) {
        // IMP(Accumulator)
        quint16 temp = quint16(reg_a >> 1) | quint16(reg_sf.get_c() << 7); //
        reg_sf.set_c(reg_a & 0x0001);
        reg_sf.set_z((temp & 0x00FF) == 0);
        reg_sf.set_n(temp & 0x80);
        reg_a = temp & 0x00FF;
    } else {
        // fetch data
        quint8 operand = p_ram->load(addr_abs);
        quint16 temp = quint16(operand >> 1) | quint16(reg_sf.get_c() << 7);
        reg_sf.set_c(operand & 0x0001);
        reg_sf.set_z((temp & 0x00FF) == 0);
        reg_sf.set_n(temp & 0x80);
        p_ram->save(addr_abs, temp & 0x00FF);
    }
    return 0;
}

int CPU::RTI()
{
    // Return from interrupt
    reg_sf.data = pull_stack();
    reg_sf.set_b(false);
    reg_sf.set_u(false);
    quint8 pc_lo8 = pull_stack();
    quint8 pc_hi8 = pull_stack();
    reg_pc = quint16(pc_hi8 << 8) + pc_lo8;
    return 0;
}

int CPU::RTS()
{
    // Return from subroutine
    quint8 pc_lo8 = pull_stack();
    quint8 pc_hi8 = pull_stack();
    reg_pc = quint16(pc_hi8 << 8) + pc_lo8;
    reg_pc++;
    return 0;
}

int CPU::SBC()
{
    // fetch data
    quint8 operand = p_ram->load(addr_abs);
    // subtraction. Pay attention to the overflow flag
    quint16 sub = reg_a - operand - (!reg_sf.get_c());
    reg_sf.set_c(!(sub & 0x100));
    reg_sf.set_v((reg_a ^ sub) & ((~operand) ^ sub) & 0x80);
    reg_sf.set_z((sub & 0xFF) == 0);
    reg_sf.set_n(sub & 0x80);
    reg_a = sub & 0x00FF;
    return 1;
}

int CPU::SEC()
{
    // Set Carry
    reg_sf.set_c(true);
    return 0;
}

int CPU::SED()
{
    // Set Decimal
    reg_sf.set_d(true);
    return 0;
}

int CPU::SEI()
{
    // Set Interrupt
    reg_sf.set_i(true);
    return 0;
}

int CPU::STA()
{
    // Store Accumulator
    p_ram->save(addr_abs, reg_a);
    return 0;
}

int CPU::STX()
{
    // Store X
    p_ram->save(addr_abs, reg_x);
    return 0;
}

int CPU::STY()
{
    // Store Y
    p_ram->save(addr_abs, reg_y);
    return 0;
}

int CPU::TAX()
{
    // Transfer A to X
    reg_x = reg_a;
    reg_sf.set_z(reg_x == 0);
    reg_sf.set_n(reg_x & 0x0080);
    return 0;
}

int CPU::TAY()
{
    // Transfer A to Y
    reg_y = reg_a;
    reg_sf.set_z(reg_y == 0);
    reg_sf.set_n(reg_y & 0x0080);
    return 0;
}

int CPU::TSX()
{
    // Transfer SP to X
    reg_x = reg_sp;
    reg_sf.set_z(reg_x == 0);
    reg_sf.set_n(reg_x & 0x0080);
    return 0;
}

int CPU::TXA()
{
    // Transfer X to A
    reg_a = reg_x;
    reg_sf.set_z(reg_a == 0);
    reg_sf.set_n(reg_a & 0x0080);
    return 0;
}

int CPU::TXS()
{
    // Transfer X to SP
    reg_sp = reg_x;
    return 0;
}

int CPU::TYA()
{
    // Transfer Y to A
    reg_a = reg_y;
    reg_sf.set_z(reg_a == 0);
    reg_sf.set_n(reg_a & 0x0080);
    return 0;
}

int CPU::XXX()
{
    // Illegal Opcode
    QMessageBox::critical(nullptr,
                          QStringLiteral("ERROR"),
                          QStringLiteral("CPU executed an unknown instruction! Exiting..."));
    abort();
    return 0;
}

void CPU::clock()
{
    // Only fetch another instruction after last one is done
    if (cycles_wait == 0) {
        // 1. fetch instruction
        opcode = p_ram->load(reg_pc);
        reg_pc++;
        reg_sf.set_u(true);
        // 2. extra cycles
        int cycles_add_by_addrmode = (this->*inst_table[opcode].addrmode)();
        int cycles_add_by_operate = (this->*inst_table[opcode].operate)();

        if (isDebugging)
            update_curr_instruction();

        // 3. calculate total cycles needed
        cycles_wait = this->inst_table[opcode].cycle_cnt;
        if (cycles_add_by_operate < 0)
            cycles_wait += (-cycles_add_by_operate);
        else
            cycles_wait += (cycles_add_by_operate & cycles_add_by_addrmode);
        reg_sf.set_u(true);
    }

    cycles_wait--;
    clock_count++;
}

void CPU::update_curr_instruction()
{
    QString addr;
    switch (address_mode) {
    case 0:
        break;
    case 1:
        addr = QString(" #%1H").arg(oprand_for_log, 2, 16, QLatin1Char('0'));
        break;
    case 2:
        addr = QString(" %1H").arg(oprand_for_log, 2, 16, QLatin1Char('0'));
        break;
    case 3:
        addr = QString(" %1H, X").arg(oprand_for_log, 2, 16, QLatin1Char('0'));
        break;
    case 4:
        addr = QString(" %1H, Y").arg(oprand_for_log, 2, 16, QLatin1Char('0'));
        break;
    case 5:
    case 6:
        addr = QString(" %1H").arg(oprand_for_log, 4, 16, QLatin1Char('0'));
        break;
    case 7:
        addr = QString(" %1H, X").arg(oprand_for_log, 4, 16, QLatin1Char('0'));
        break;
    case 8:
        addr = QString(" %1H, Y").arg(oprand_for_log, 4, 16, QLatin1Char('0'));
        break;
    case 9:
        addr = QString(" (%1H)").arg(oprand_for_log, 4, 16, QLatin1Char('0'));
        break;
    case 10:
        addr = QString(" (%1H, X)").arg(oprand_for_log, 4, 16, QLatin1Char('0'));
        break;
    case 11:
        addr = QString(" (%1H, Y)").arg(oprand_for_log, 4, 16, QLatin1Char('0'));
        break;
    default:
        break;
    }

    curr_instruction = inst_table[opcode].name + addr;
}

void CPU::print_log() const
{
    char addr_str[20] = {0};
    if (inst_table[opcode].addrmode == &CPU::IMP) {
        // do nothing.
    } else if (inst_table[opcode].addrmode == &CPU::IMM) {
        sprintf(addr_str, " #%02xH", oprand_for_log);
    } else if (inst_table[opcode].addrmode == &CPU::ZP0) {
        sprintf(addr_str, " %02xH", oprand_for_log);
    } else if (inst_table[opcode].addrmode == &CPU::ZPX) {
        sprintf(addr_str, " %02xH, X", oprand_for_log);
    } else if (inst_table[opcode].addrmode == &CPU::ZPY) {
        sprintf(addr_str, " %02xH, Y", oprand_for_log);
    } else if (inst_table[opcode].addrmode == &CPU::REL) {
        sprintf(addr_str, " %04xH", oprand_for_log);
    } else if (inst_table[opcode].addrmode == &CPU::ABS) {
        sprintf(addr_str, " %04xH", oprand_for_log);
    } else if (inst_table[opcode].addrmode == &CPU::ABX) {
        sprintf(addr_str, " %04xH, X", oprand_for_log);
    } else if (inst_table[opcode].addrmode == &CPU::ABY) {
        sprintf(addr_str, " %04xH, Y", oprand_for_log);
    } else if (inst_table[opcode].addrmode == &CPU::IND) {
        sprintf(addr_str, " (%04xH)", oprand_for_log);
    } else if (inst_table[opcode].addrmode == &CPU::IZX) {
        sprintf(addr_str, " (%04xH, X)", oprand_for_log);
    } else if (inst_table[opcode].addrmode == &CPU::IZY) {
        sprintf(addr_str, " (%04xH), Y", oprand_for_log);
    }
    qDebug() << "Program Counter = " << QString::number(reg_pc, 16) << ", opcode = " << opcode
             << ", code = " << this->inst_table[opcode].name << addr_str << ", A = " << this->reg_a
             << ", X = " << this->reg_x << ", Y = " << this->reg_y << ", stack pointer = " << reg_sp
             << ", NVDIZC = " << int(reg_sf.get_n()) << int(reg_sf.get_v()) << int(reg_sf.get_d())
             << int(reg_sf.get_i()) << int(reg_sf.get_z()) << int(reg_sf.get_c());
}

QDataStream &operator<<(QDataStream &stream, const CPU &Cpu)
{
    stream << Cpu.reg_a;
    stream << Cpu.reg_x;
    stream << Cpu.reg_y;
    stream << Cpu.reg_pc;
    stream << Cpu.reg_sp;
    stream << Cpu.reg_sf.data;

    stream << Cpu.addr_abs;
    stream << Cpu.addr_rel;
    stream << Cpu.cycles_wait;
    stream << Cpu.opcode;
    stream << Cpu.clock_count;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, CPU &Cpu)
{
    stream >> Cpu.reg_a;
    stream >> Cpu.reg_x;
    stream >> Cpu.reg_y;
    stream >> Cpu.reg_pc;
    stream >> Cpu.reg_sp;
    stream >> Cpu.reg_sf.data;

    stream >> Cpu.addr_abs;
    stream >> Cpu.addr_rel;
    stream >> Cpu.cycles_wait;
    stream >> Cpu.opcode;
    stream >> Cpu.clock_count;

    return stream;
}
