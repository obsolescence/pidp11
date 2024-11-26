/* realcons_pdp11_70.c : Logic for the specific 11/70 panel

 Copyright (c) 2012-2018, Joerg Hoppe
 j_hoppe@t-online.de, www.retrocmp.com

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 JOERG HOPPE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 21-Mar-2018    JH	    merged with current SimH
 10-Mar-2018    JH      lot of fixes to match the tests at LCM "Miss Piggy"
 03-Feb-2018    JH      fixed SUPER-USER-KERNEL encoding
 04-Jul-2017    MH      fixed 16-BIT mode and console register access
 22-Apr-2016    JH      added POWER and PANEL_LOCK logic
 01-Apr-2016    JH      added ADRESSING 16/18/22, ADDR SELECT knob VIRTUAL positions
 18-Mar-2016    JH      access to SimHs I/O page and CPU registers (18->22bit addr)
 21-Feb-2016    JH      added PANEL_MODE_POWERLESS
 Dec-2015   JH      migration from SimH 3.82 to 4.x
 CPU extensions moved to pdp11_cpu.c
 25-Mar-2012    JH      created


 Inputs:
 - cpu state (PDP11 as set by SimH)
 - blinkenlight API panel input (switches)

 Outputs:
 - cpu state (as checked by SimH, example: HALT state)
 - blinkenlight API panel outputs (lamps)
 - SimH command string

 Class hierarchy:
 base panel = generic machine with properties requiered by SimH
 everthing accessed in scp.c is "base"

 inherited pdp11_70 (or other architecure)
 an actual Panel has an actual machine state.

 (For instance, a 11/05 has almost the same machine state,
 but quite a different panel)

 */
#ifdef USE_REALCONS


#include <assert.h>


// ------------------------------------------------- 20180515
//#include <unistd.h>	// for sleep()
#include <string.h>
// ----------------------------------------------------------



#include "sim_defs.h"
#include "realcons.h"
#include "realcons_console_pdp11_70.h"

// indexes of used general timers
#define TIMER_TEST	0

#define TIME_TEST_MS		3000	// self test terminates after 3 seconds
// the RUN LED behaves a bit difficult, so distinguish these states:
#define RUN_STATE_HALT	0
#define RUN_STATE_RESET	1
#define RUN_STATE_WAIT	2
#define RUN_STATE_RUN	3

// encoding of addr select knob positions
#define ADDR_SELECT_VALUE_USER_I    0
#define ADDR_SELECT_VALUE_USER_D    1
#define ADDR_SELECT_VALUE_SUPER_I   2
#define ADDR_SELECT_VALUE_SUPER_D   3
#define ADDR_SELECT_VALUE_CONS_PHY  4
#define ADDR_SELECT_VALUE_KERNEL_I  5
#define ADDR_SELECT_VALUE_KERNEL_D  6
#define ADDR_SELECT_VALUE_PROG_PHY  7

// encoding of data select knob positions
#define DATA_SELECT_VALUE_uADDR_FPU_CPU    0
#define DATA_SELECT_VALUE_DISPLAY_REGISTER  1
#define DATA_SELECT_VALUE_DATA_PATH 2
#define DATA_SELECT_VALUE_BUS_REG   3

// Table with const uAdr-patterns
// display const pattern, from LCM2018 script "Unixv7 idle"
// 100% 100% 50% 10% 100% 50% 80% 100% 10% 50%
// bits 16..8 = FPP (always 11?) bits 7..0 = microprogram address
#define MICROADDRPATTERN_RUN_LEN    5
static unsigned MicroAddrPatternRUN[MICROADDRPATTERN_RUN_LEN] =
{ 0x3bf, // 1 1  1 0 1 1  1 1 1 1
        0x32a, // 1 1  0 0 1 0  1 1 0 0
        0x3bd, // 1 1  1 0 1 1  1 1 0 1
        0x32c, // 1 1  0 0 1 0  1 1 0 0
        0x3f5 // 1 1  1 1 1 1  0 1 0 1
        };

// see FPMS, "FLOW 14", COM.00: "DISPLAY ADDRESS & DATA". uADDRS = 170,167
#define MICROADDRPATTERN_CON_LEN   2
static unsigned MicroAddrPatternCON[MICROADDRPATTERN_CON_LEN] =
    { 0170, 0167 };

// count bits in a value
    static int bitcount(unsigned val) {
    unsigned n=0 ;
    while (val) {
        if (val & 1) n++ ;
        val >>= 1 ;
    }
    return n ;
}


// SHORTER macros for signal access
// assume "realcons_console_logic_pdp11_70_t *_this" in context
#define SIGNAL_SET(signal,value) REALCONS_SIGNAL_SET(_this,signal,value)
#define SIGNAL_GET(signal) REALCONS_SIGNAL_GET(_this,signal)


/*
 * mount a 22 bit LOAD ADRS from SWR and SR
 */
    static t_value loaded_address_22bit(realcons_console_logic_pdp11_70_t *_this) {
        return
            (_this->loaded_address_15_00_datapathSR & 0xffff) // bits 15:00
            | (_this->loaded_address_datapathSWR & 0x3f0000) ; // bits 21:00
    }



/*
 * SimH gives not the CPU registers, if their IO page addresses are exam/deposit
 * So convert addr -> reg, and revers in the *_operator_reg_exam_deposit()
 * result: error == 1: error stepping into register space
 */
static t_addr increment_address(t_addr addr, int *addr_inc_error)
{
    *addr_inc_error = 0;
    if (addr == 017777676) {
        // do not switch from "+2 increment" into "+1 increment" of register file
        // see LCM2018, script 9
        *addr_inc_error = 1;
    } else if (addr >= 017777700 && addr <= 017777717) {
        addr += 1; // inc to next register
        addr &= 017777717; // wrap around to start of registers after 017777717
    } else
        addr += 2; // inc by word
    addr &= 017777777; // trunc to 22 bit
    return addr;
}

// generate SimH expression for panel address
// - result string to be used in "sim>" exam or deposit cmd
// - translate memory addr to register, if CPU is accessed
// - trunc to 16 bit and add MMU space prefixes, if ADDR SELECT knob is in virtual position
// TODO: the "-d" option means "data space" and also "decimal ???
// Tests indicated that -d -o could be a solution. To be investigated!
static char *realcons_console_pdp11_70_addr_panel2simh(t_addr panel_addr, uint64_t addr_select)
{
    static char buff[40];

    switch (addr_select) {

    case ADDR_SELECT_VALUE_USER_I:
        panel_addr &= 0177777; // 16 bit virtual "user mode instruction" space
        sprintf(buff, "-v -u -o %o", panel_addr);
        return buff;
    case ADDR_SELECT_VALUE_USER_D:
        panel_addr &= 0177777; // 16 bit virtual "user mode data" space
        sprintf(buff, "-v -u -d -o %o", panel_addr);
        return buff;
    case ADDR_SELECT_VALUE_SUPER_I:
        panel_addr &= 0177777; // 16 bit virtual "super mode instruction" space
        sprintf(buff, "-v -s -o %o", panel_addr);
        return buff;
    case ADDR_SELECT_VALUE_SUPER_D:
        panel_addr &= 0177777; // 16 bit virtual "super mode data" space
        sprintf(buff, "-v -s -d -o %o", panel_addr);
        return buff;
    case ADDR_SELECT_VALUE_KERNEL_I:
        panel_addr &= 0177777; // 16 bit virtual "kernel mode instruction" space
        sprintf(buff, "-v -k -o %o", panel_addr);
        return buff;
    case ADDR_SELECT_VALUE_KERNEL_D:
        panel_addr &= 0177777; // 16 bit virtual "kernel mode data" space
        sprintf(buff, "-v -k -d -o %o", panel_addr);
        return buff;
    default:
        // physical 22 bit address
        switch (panel_addr) {
        case 017777700:
            return "r00";
        case 017777701:
            return "r01";
        case 017777702:
            return "r02";
        case 017777703:
            return "r03";
        case 017777704:
            return "r04";
        case 017777705:
            return "r05";
        case 017777706:
            return "ksp";
        case 017777707:
            return "pc";
        case 017777710:
            return "r10";
        case 017777711:
            return "r11";
        case 017777712:
            return "r12";
        case 017777713:
            return "r13";
        case 017777714:
            return "r14";
        case 017777715:
            return "r15";
        case 017777716:
            return "ssp";
        case 017777717:
            return "usp";
        default:
            panel_addr &= ~1; // clear LSB, according to DEC doc
            sprintf(buff, "%o", panel_addr);
            return buff;
        }
    }
}


// set virtual and physical address
// use MMU
static void set_memory_address_va_pa(realcons_console_logic_pdp11_70_t *_this, int32 va) {
    int32 realcons_reloc(int32 addr);
    // use something like relocC(), but feed CPU I/D & mode into it!
    SIGNAL_SET(cpusignal_memory_address_virt_register, va & 0xffff);
    SIGNAL_SET(cpusignal_memory_address_phys_register, realcons_reloc(va));
}


/*
 * constructor / destructor
 */
realcons_console_logic_pdp11_70_t *realcons_console_pdp11_70_constructor(realcons_t *realcons)
{
    realcons_console_logic_pdp11_70_t *_this;

    _this = (realcons_console_logic_pdp11_70_t *) malloc(sizeof(realcons_console_logic_pdp11_70_t));
    _this->realcons = realcons; // connect to parent
    _this->run_state = 0;
    return _this;
}

void realcons_console_pdp11_70_destructor(realcons_console_logic_pdp11_70_t *_this)
{
    free(_this);
}

/*
 * Interface to external simulation:
 * signaled by SimH for state changes of CPU
 *
 * CPU conditions:
 * HALT instruction has R(00)
 * RESET instruction has R(00)
 * WAIT instruction has R(IR)
 * SINGLE STEP and HALT switch has Processor Status (PS)
 *
 * Console switches:
 * LOAD ADRS - the transferred Switch register address.
 * DEP - the Switch register data just deposited.
 * EXAM - the information from the address examined.
 * HALT - displays the current Processor Status (PS) word.
 *
 * Called high speed in simulator loop!
 * return 0, if not accepted
 */


void realcons_console_pdp11_70_event_connect(realcons_console_logic_pdp11_70_t *_this)
{
    // set panel mode to "powerless". all lights go off,
    // On Java panels the power switch should flip to the ON position
    realcons_power_mode(_this->realcons, 1);
}

void realcons_console_pdp11_70_event_disconnect(realcons_console_logic_pdp11_70_t *_this)
{
    // set panel mode to "powerless". all lights go off,
    // On Java panels the power switch should flip to the OFF position
    realcons_power_mode(_this->realcons, 0);
}

void realcons_console_pdp11_70__event_opcode_any(realcons_console_logic_pdp11_70_t *_this)
{
    // other opcodes executed by processor
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_opcode_any\n");
    // after any opcode: ADDR shows PC, DATA shows IR = opcode
    set_memory_address_va_pa(_this, SIGNAL_GET(cpusignal_PC));
    // opcode fetch: ALU_output already set
  //  SIGNAL_SET(cpusignal_memory_data_register, SIGNAL_GET(cpusignal_instruction_register));

    SIGNAL_SET(cpusignal_memory_write_access, 0) ; // "fetch" = read

    // SIGNAL_SET(cpusignal_bus_register, SIGNAL_GET(cpusignal_instruction_register));
    _this->led_PAUSE->value = 0; // see 1.3.4
    _this->run_state = RUN_STATE_RUN;
}

void realcons_console_pdp11_70__event_opcode_halt(realcons_console_logic_pdp11_70_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_opcode_halt\n");
    set_memory_address_va_pa(_this, SIGNAL_GET(cpusignal_PC));
    // micro step FLOWS 12, BRK.00 : PC -> SR
    _this->loaded_address_15_00_datapathSR = SIGNAL_GET(cpusignal_PC);

    SIGNAL_SET(cpusignal_DATAPATH_shifter, SIGNAL_GET(cpusignal_R0)); // HALT displays R0
    _this->led_PAUSE->value = 0; // see 1.3.4
    _this->run_state = RUN_STATE_HALT;
}

void realcons_console_pdp11_70__event_opcode_reset(realcons_console_logic_pdp11_70_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_opcode_reset\n");
    set_memory_address_va_pa(_this, SIGNAL_GET(cpusignal_PC));
    // what is in the unibus interface on RESET? IR == RESET Opcode ?
    SIGNAL_SET(cpusignal_DATAPATH_shifter, SIGNAL_GET(cpusignal_R0)); // RESET displays R0
    _this->led_PAUSE->value = 0; // see 1.3.4
    _this->run_state = RUN_STATE_RESET;
    // RESET lasts 10ms on 11/70
    realcons_ms_sleep(_this->realcons, 10); // keep realcons logic active

}


void realcons_console_pdp11_70__event_opcode_wait(realcons_console_logic_pdp11_70_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_opcode_wait\n");
    set_memory_address_va_pa(_this, SIGNAL_GET(cpusignal_PC));

//    SIGNAL_SET(cpusignal_memory_data_register, SIGNAL_GET(cpusignal_instruction_register));
    SIGNAL_SET(cpusignal_DATAPATH_shifter, SIGNAL_GET(cpusignal_R0)); // WAIT displays R0
    _this->led_PAUSE->value = 0; // see 1.3.4
    _this->run_state = RUN_STATE_WAIT; // RUN led off
}

void realcons_console_pdp11_70__event_run_start(realcons_console_logic_pdp11_70_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_run_start\n");
    if (_this->switch_HALT->value)
        return; // do not accept RUN command
    // set property RUNMODE, so SimH can read it back
    _this->led_PAUSE->value = 0; // see 1.3.4
    _this->run_state = RUN_STATE_RUN;
}

void realcons_console_pdp11_70__event_step_start(realcons_console_logic_pdp11_70_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_step_start\n");
    set_memory_address_va_pa(_this, SIGNAL_GET(cpusignal_PC));
    // micro step  FLOWS 12, BRK.00: PC -> SR
    _this->loaded_address_15_00_datapathSR = SIGNAL_GET(cpusignal_PC);

    _this->led_PAUSE->value = 0; // see 1.3.4
}

void realcons_console_pdp11_70__event_operator_halt(realcons_console_logic_pdp11_70_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_operator_halt\n");
    set_memory_address_va_pa(_this, SIGNAL_GET(cpusignal_PC));

    // micro step FLOWS12, BRK.00: PC -> SR
    _this->loaded_address_15_00_datapathSR = SIGNAL_GET(cpusignal_PC);

    // SIGNAL_SET(cpusignal_DATAPATH_shifter, SIGNAL_GET(cpusignal_PSW));
    _this->led_PAUSE->value = 0; // see 1.3.4
    _this->run_state = RUN_STATE_HALT;
}

void realcons_console_pdp11_70__event_step_halt(realcons_console_logic_pdp11_70_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_step_halt\n");
    set_memory_address_va_pa(_this, SIGNAL_GET(cpusignal_PC));
    // micro step FLOWS 12, BRK.00: PC -> SR
    _this->loaded_address_15_00_datapathSR = SIGNAL_GET(cpusignal_PC); // update only lower bits
//    _this->loaded_address_SWR2116
    // SIGNAL_SET(cpusignal_DATAPATH_shifter, SIGNAL_GET(cpusignal_PSW));
    _this->led_PAUSE->value = 0; // see 1.3.4
    _this->run_state = RUN_STATE_HALT;
}

// This is called, when "exam" or "deposit" is entered over the
// sim> command prompt by user or as result of an EXAM or DEPOSIT panel switch.
// if ADDR SELECT knob is VIRTUAL, the virtual address for the
// SimH physical address should be displayed.
// This is impossible: not every physical addr has a virtual representation
// for given address space (KERNEL,SUPER,USER, I/D)
// So simpy DO NOT UPDATE panel register if KNBOB is virtual,
// then at least the key switch values remain in ADRR LEDs
void realcons_console_pdp11_70__event_operator_exam_deposit(
        realcons_console_logic_pdp11_70_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_operator_exam\n");
    // exam on SimH console sets also console address (like LOAD ADR),
    // if not virtual (see above)
    if ((_this->switch_ADDR_SELECT->value == ADDR_SELECT_VALUE_CONS_PHY)
            || (_this->switch_ADDR_SELECT->value == ADDR_SELECT_VALUE_PROG_PHY)) {
        _this->loaded_address_15_00_datapathSR = SIGNAL_GET(cpusignal_memory_address_phys_register) & 0xffff;
        // exam over simh> works like switch settings
        _this->loaded_address_datapathSWR = SIGNAL_GET(cpusignal_memory_address_phys_register);
    }
    SIGNAL_SET(cpusignal_DATAPATH_shifter, SIGNAL_GET(cpusignal_memory_data_register));
    _this->led_PAUSE->value = 0; // see 1.3.4
}

// Called if Realcons or a user typed "examine <regname>" or "deposit <regname> value".
// Realcons accesses the CPU registers only by name, so only these are recognized here
// see realcons_console_pdp11_70_addr_panel2simh() !
void realcons_console_pdp11_70__event_operator_reg_exam_deposit(
        realcons_console_logic_pdp11_70_t *_this)
{
    char *regname = SIGNAL_GET(cpusignal_register_name); // alias
    t_addr addr = 0;

    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_operator_reg_exam_deposit\n");
    // exam on SimH console sets also console address (like LOAD ADR)
    // convert register into UNIBUS address
    if ((!strcasecmp(regname, "r00")) || (!strcasecmp(regname, "r0")))
        addr = 017777700;
    else if ((!strcasecmp(regname, "r01")) || (!strcasecmp(regname, "r1")))
        addr = 017777701;
    else if ((!strcasecmp(regname, "r02")) || (!strcasecmp(regname, "r2")))
        addr = 017777702;
    else if ((!strcasecmp(regname, "r03")) || (!strcasecmp(regname, "r3")))
        addr = 017777703;
    else if ((!strcasecmp(regname, "r04")) || (!strcasecmp(regname, "r4")))
        addr = 017777704;
    else if ((!strcasecmp(regname, "r05")) || (!strcasecmp(regname, "r5")))
        addr = 017777705;
    else if ((!strcasecmp(regname, "ksp")) || (!strcasecmp(regname, "sp")))
        addr = 017777706;
    else if (!strcasecmp(regname, "pc"))
        addr = 017777707;
    else if (!strcasecmp(regname, "r10"))
        addr = 017777710;
    else if (!strcasecmp(regname, "r11"))
        addr = 017777711;
    else if (!strcasecmp(regname, "r12"))
        addr = 017777712;
    else if (!strcasecmp(regname, "r13"))
        addr = 017777713;
    else if (!strcasecmp(regname, "r14"))
        addr = 017777714;
    else if (!strcasecmp(regname, "r15"))
        addr = 017777715;
    else if (!strcasecmp(regname, "ssp"))
        addr = 017777716;
    else if (!strcasecmp(regname, "usp"))
        addr = 017777717;

    if (addr) {
        SIGNAL_SET(cpusignal_memory_address_phys_register, addr);
        realcons_console_pdp11_70__event_operator_exam_deposit(_this);
    } // other register accesses are ignored by the panel
}


// Called after CPU has been reinitialized
void realcons_console_pdp11_70__event_cpu_reset(
        realcons_console_logic_pdp11_70_t *_this) {
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_70__event_cpu_reset\n");

    // load PC, show PC in ADDR
    SIGNAL_SET(cpusignal_PC, loaded_address_22bit(_this) & 0xffff) ;
    set_memory_address_va_pa(_this, SIGNAL_GET(cpusignal_PC));
}



void realcons_console_pdp11_70_interface_connect(realcons_console_logic_pdp11_70_t *_this,
        realcons_console_controller_interface_t *intf, char *panel_name)
{
    intf->name = panel_name;
    intf->constructor_func =
            (console_controller_constructor_func_t) realcons_console_pdp11_70_constructor;
    intf->destructor_func =
            (console_controller_destructor_func_t) realcons_console_pdp11_70_destructor;
    intf->reset_func = (console_controller_reset_func_t) realcons_console_pdp11_70_reset;
    intf->service_func = (console_controller_service_func_t) realcons_console_pdp11_70_service;
    intf->test_func = (console_controller_test_func_t) realcons_console_pdp11_70_test;

    intf->event_connect = (console_controller_event_func_t) realcons_console_pdp11_70_event_connect;
    intf->event_disconnect =
            (console_controller_event_func_t) realcons_console_pdp11_70_event_disconnect;

    // connect pdp11 cpu signals end events to simulator and realcons state variables
    {
        // REALCONS extension in scp.c
        extern t_addr realcons_memory_address_phys_register; // REALCONS extension in scp.c
        extern t_addr realcons_memory_address_virt_register; // REALCONS extension in scp.c

        extern t_value realcons_memory_data_register; // REALCONS extension in scp.c
        extern int realcons_memory_write_access ; // 1: last mem access was WRITE
        extern t_stat realcons_memory_status; // error of last memory access
        extern char *realcons_register_name; // pseudo: name of last accessed register
        extern int realcons_console_halt; // 1: CPU halted by realcons console
        extern volatile t_bool sim_is_running; // global in scp.c

        extern int32 R[8]; // working registers, global in pdp11_cpu.c
        extern int32 saved_PC;
        extern int32 SR, DR; // switch/display register, global in pdp11_cpu_mod.c
        extern int32 cm; // cpu mode, global in pdp11_cpu.c. MD:KRN_MD, MD_SUP,MD_USR,MD_UND
        extern int32 MMR0, MMR3; // MMU control registers
        extern int realcons_bus_ID_mode; // 1 = DATA space access, 0 = instruction space access
        extern t_value realcons_DATAPATH_shifter; // output of ALU
        extern t_value realcons_IR; // buffer for instruction register (opcode)
        extern t_value realcons_PSW; // buffer for program status word

        realcons_console_halt = 0;

        // from scp.c
        _this->cpusignal_memory_address_phys_register = &realcons_memory_address_phys_register;
        _this->cpusignal_memory_address_virt_register = &realcons_memory_address_virt_register;
        _this->cpusignal_register_name = &realcons_register_name; // pseudo: name of last accessed register
        _this->cpusignal_memory_data_register = &realcons_memory_data_register;
		_this->cpusignal_memory_write_access = &realcons_memory_write_access ;
        _this->cpusignal_memory_status = &realcons_memory_status;
        _this->cpusignal_console_halt = &realcons_console_halt;

        // from pdp11_cpu.c
        // is "sim_is_running" indeed identical with our "cpu_is_running" ?
        // may cpu stops, but some device are still serviced?
        _this->cpusignal_run = &(sim_is_running);

        _this->cpusignal_DATAPATH_shifter = &realcons_DATAPATH_shifter; // not used
        _this->cpusignal_instruction_register = &realcons_IR;
        _this->cpusignal_PSW = &realcons_PSW;
        _this->cpusignal_bus_ID_mode = &realcons_bus_ID_mode;
        _this->cpusignal_cpu_mode = (t_value*)&cm; // MD_SUP,MD_
        _this->cpusignal_MMR0 = &MMR0; // MMU register 17 777 572
        _this->cpusignal_MMR3 = &MMR3; // MMU register 17 772 516
        _this->cpusignal_R0 = (t_value*)&(R[0]); // R: global of pdp11_cpu.c. "Working Register Set"!
        _this->cpusignal_PC = &saved_PC; // R[7] not valid in console mode
        _this->cpusignal_switch_register = (t_value*)&SR; // see pdp11_cpumod.SR_rd()
        _this->cpusignal_display_register = (t_value*)&DR;
    }

    /*** link handler to cpu/device events ***/
    {
        // scp.c
        extern console_controller_event_func_t realcons_event_run_start;
        extern console_controller_event_func_t realcons_event_operator_halt;
        extern console_controller_event_func_t realcons_event_step_start;
        extern console_controller_event_func_t realcons_event_step_halt;
        extern console_controller_event_func_t realcons_event_operator_exam;
        extern console_controller_event_func_t realcons_event_operator_deposit;
        extern console_controller_event_func_t realcons_event_operator_reg_exam;
        extern console_controller_event_func_t realcons_event_operator_reg_deposit;
        extern console_controller_event_func_t realcons_event_cpu_reset;
        // pdp11_cpu.c
        extern console_controller_event_func_t realcons_event_opcode_any; // triggered after any opcode execution
        extern console_controller_event_func_t realcons_event_opcode_halt;
        extern console_controller_event_func_t realcons_event_opcode_reset; // triggered after execution of RESET
        extern console_controller_event_func_t realcons_event_opcode_wait; // triggered after execution of WAIT

        realcons_event_run_start =
                (console_controller_event_func_t) realcons_console_pdp11_70__event_run_start;
        realcons_event_step_start =
                (console_controller_event_func_t) realcons_console_pdp11_70__event_step_start;
        realcons_event_operator_halt =
                (console_controller_event_func_t) realcons_console_pdp11_70__event_operator_halt;
        realcons_event_step_halt =
                (console_controller_event_func_t) realcons_console_pdp11_70__event_step_halt;
        realcons_event_operator_exam =
                realcons_event_operator_deposit =
                        (console_controller_event_func_t) realcons_console_pdp11_70__event_operator_exam_deposit;
        realcons_event_operator_reg_exam =
        realcons_event_cpu_reset =
			(console_controller_event_func_t) realcons_console_pdp11_70__event_cpu_reset;

        realcons_event_operator_reg_exam =
                realcons_event_operator_reg_deposit =
                        (console_controller_event_func_t) realcons_console_pdp11_70__event_operator_reg_exam_deposit;

        realcons_event_opcode_any =
                (console_controller_event_func_t) realcons_console_pdp11_70__event_opcode_any;
        realcons_event_opcode_halt =
                (console_controller_event_func_t) realcons_console_pdp11_70__event_opcode_halt;
        realcons_event_opcode_reset =
                (console_controller_event_func_t) realcons_console_pdp11_70__event_opcode_reset;
        realcons_event_opcode_wait =
                (console_controller_event_func_t) realcons_console_pdp11_70__event_opcode_wait;
    }
}

// setup first state
t_stat realcons_console_pdp11_70_reset(realcons_console_logic_pdp11_70_t *_this)
{
    _this->realcons->simh_cmd_buffer[0] = '\0';
    _this->loaded_address_15_00_datapathSR = 0;
    _this->loaded_address_datapathSWR = 0;
    SIGNAL_SET(cpusignal_DATAPATH_shifter, 0); // else DATA trash is shown before first EXAM
    _this->autoinc_addr_action_switch = NULL; // not active
    _this->last_memory_status = SCPE_OK;

    /*
     * direct links to all required controls.
     * Also check of config file
     */
    if (!(_this->keyswitch_power = realcons_console_get_input_control(_this->realcons, "POWER")))
        return SCPE_NOATT;
    if (!(_this->keyswitch_panel_lock = realcons_console_get_input_control(_this->realcons,
            "PANEL_LOCK")))
        return SCPE_NOATT;

    if (!(_this->switch_SR = realcons_console_get_input_control(_this->realcons, "SR")))
        return SCPE_NOATT;
    if (!(_this->switch_LOAD_ADRS = realcons_console_get_input_control(_this->realcons, "LOAD_ADRS")))
        return SCPE_NOATT;
    if (!(_this->switch_EXAM = realcons_console_get_input_control(_this->realcons, "EXAM")))
        return SCPE_NOATT;
    if (!(_this->switch_DEPOSIT = realcons_console_get_input_control(_this->realcons, "DEPOSIT")))
        return SCPE_NOATT;
    if (!(_this->switch_CONT = realcons_console_get_input_control(_this->realcons, "CONT")))
        return SCPE_NOATT;
    if (!(_this->switch_HALT = realcons_console_get_input_control(_this->realcons, "HALT")))
        return SCPE_NOATT;
    if (!(_this->switch_S_BUS_CYCLE = realcons_console_get_input_control(_this->realcons,
            "S_BUS_CYCLE")))
        return SCPE_NOATT;
    if (!(_this->switch_START = realcons_console_get_input_control(_this->realcons, "START")))
        return SCPE_NOATT;
    if (!(_this->switch_DATA_SELECT = realcons_console_get_input_control(_this->realcons,
            "DATA_SELECT")))
        return SCPE_NOATT;
    if (!(_this->switch_ADDR_SELECT = realcons_console_get_input_control(_this->realcons,
            "ADDR_SELECT")))
        return SCPE_NOATT;
    if (!(_this->switch_PANEL_LOCK = realcons_console_get_input_control(_this->realcons,
            "PANEL_LOCK")))
        return SCPE_NOATT;

    if (!(_this->leds_ADDRESS = realcons_console_get_output_control(_this->realcons, "ADDRESS")))
        return SCPE_NOATT;
    if (!(_this->leds_DATA = realcons_console_get_output_control(_this->realcons, "DATA")))
        return SCPE_NOATT;
    if (!(_this->led_PARITY_HIGH = realcons_console_get_output_control(_this->realcons,
            "PARITY_HIGH")))
        return SCPE_NOATT;
    if (!(_this->led_PARITY_LOW = realcons_console_get_output_control(_this->realcons,
            "PARITY_LOW")))
        return SCPE_NOATT;
    if (!(_this->led_PAR_ERR = realcons_console_get_output_control(_this->realcons, "PAR_ERR")))
        return SCPE_NOATT;
    if (!(_this->led_ADRS_ERR = realcons_console_get_output_control(_this->realcons, "ADRS_ERR")))
        return SCPE_NOATT;
    if (!(_this->led_RUN = realcons_console_get_output_control(_this->realcons, "RUN")))
        return SCPE_NOATT;
    if (!(_this->led_PAUSE = realcons_console_get_output_control(_this->realcons, "PAUSE")))
        return SCPE_NOATT;
    if (!(_this->led_MASTER = realcons_console_get_output_control(_this->realcons, "MASTER")))
        return SCPE_NOATT;
    if (!(_this->leds_MMR0_MODE = realcons_console_get_output_control(_this->realcons, "MMR0_MODE")))
        return SCPE_NOATT;
    if (!(_this->led_DATA_SPACE = realcons_console_get_output_control(_this->realcons, "DATA_SPACE")))
        return SCPE_NOATT;
    if (!(_this->led_ADDRESSING_16 = realcons_console_get_output_control(_this->realcons,
            "ADDRESSING_16")))
        return SCPE_NOATT;
    if (!(_this->led_ADDRESSING_18 = realcons_console_get_output_control(_this->realcons,
            "ADDRESSING_18")))
        return SCPE_NOATT;
    if (!(_this->led_ADDRESSING_22 = realcons_console_get_output_control(_this->realcons,
            "ADDRESSING_22")))
        return SCPE_NOATT;
    return SCPE_OK;
}

#ifdef USED
/*
 * Translate the console address register (LOAD ADDR) to a physical 22bit address
 * This is necessary, if the ADDR SLECT KNOB is in one of the VIRTUAL positions
 * DEC : "VIRTUAL - Six positions: KERNEL, SUPER and USER I space and KERNEL, SUPER and
 * USER D space. The address displayed is a 16-bit virtual address; bits
 * 21 -16 are always off. During Console DEP or EXAM operations, bits
 * 15:00 of the Switch Register are considered to be a Virtual Address.
 * If Memory Management is enabled, this Virtual Address is relocated.
 * The set of PAR/PDRs indicated by the switch position is used."
 */
static int32 console_addr_register_physical(realcons_console_logic_pdp11_70_t *_this)
{
    t_addr pa, va;
    extern int32 relocC(int32 va, int32 sw);

    va = SIGNAL_GET(cpusignal_console_address_register) & 0xffff; // if virtual mode
    // interpret addr as virtual and translate to physical

    // add address space to virtual address
    // bit 18,17 = cm = mode: MD_SUP, MD_KER,MD_USR,MD_UND, see calc_is()
    // MD_KER=0, MD_SUP =1, MD_USR= 3
    // bit 16 = I/D space flag, see calc_ds(). 1 = data, 0 = instruction
    // encoding of addr select knob positions
    switch (_this->switch_ADDR_SELECT->value) {
        case ADDR_SELECT_VALUE_USER_I:
        va |= (3 << 17) | (0 << 16);
        pa = (t_addr) relocC((int32) va);
        break;
        case ADDR_SELECT_VALUE_USER_D:
        va |= (3 << 17) | (1 << 16);
        pa = (t_addr) relocC((int32) va);
        break;
        case ADDR_SELECT_VALUE_SUPER_I:
        va |= (1 << 17) | (0 << 16);
        pa = (t_addr) relocC((int32) va);
        break;
        case ADDR_SELECT_VALUE_SUPER_D:
        va |= (1 << 17) | (1 << 16);
        pa = (t_addr) relocC((int32) va);
        break;
        case ADDR_SELECT_VALUE_KERNEL_I:
        va |= (0 << 17) | (0 << 16);
        pa = (t_addr) relocC((int32) va);
        break;
        case ADDR_SELECT_VALUE_KERNEL_D:
        va |= (0 << 17) | (1 << 16);
        pa = (t_addr) relocC((int32) va);
        break;
        default:
        pa = SIGNAL_GET(cpusignal_console_address_register);
    }
    return pa;
}
#endif

// process panel state.
// operates on Blinkenlight_API panel structs,
// but RPC communication is done external
t_stat realcons_console_pdp11_70_service(realcons_console_logic_pdp11_70_t *_this)
{
    blinkenlight_panel_t *p = _this->realcons->console_model; // alias
    int panel_lock;
    int console_mode;
    int user_mode;
    int addr_inc_error = 0 ; // 1 = error while auto incrementing EXAM/DEPOSIT address

    blinkenlight_control_t *action_switch; // current action switch

    if (_this->keyswitch_power->value == 0) {
//------------- PiDP change: commented out code
//        SIGNAL_SET(cpusignal_console_halt, 1); // stop execution
//-------------
        if (_this->keyswitch_power->value_previous == 1) {
            // Power switch transition to POWER OFF: terminate SimH
            // This is drastic, but will teach users not to twiddle with the power switch.
            // when panel is disconnected, panel mode goes to POWERLESS and power switch goes OFF.
            // But shutdown sequence is not initiated, because we're disconnected then.
//------------- PiDP inserted code
SIGNAL_SET(cpusignal_console_halt, 1); // stop execution - Joerg, shouldn't it be here, instead of above??

FILE *bootfil;
bootfil=fopen("/run/pidp11/tmpsimhcommand.txt", "r");
fscanf(bootfil, "%[^\n]s", _this->realcons->simh_cmd_buffer);
strcat(_this->realcons->simh_cmd_buffer, "\n");
fclose(bootfil);
//-------------
        }
        // do nothing, if power is off. else cpusignal_console_halt may be deactivate by HALT switch
        return SCPE_OK;
    }

    /*
     LOCK - Same as POWER, except that the LOAD ADRS, EXAM, DEP, CONT, ENABLE/HALT, S
     INST/S BUS CYCLE and START switches are disabled. All other switches are
     operational.
     */
    panel_lock = (_this->keyswitch_panel_lock->value != 0);

    /* test time expired? */

    // STOP by activating HALT?
    if (!panel_lock && _this->switch_HALT->value && !SIGNAL_GET(cpusignal_console_halt)) {
        // must be done by SimH.pdp11_cpu.c!
    }

    // run mode KERNEL, SUPERVISOR, USER?
    user_mode = ( SIGNAL_GET(cpusignal_cpu_mode) == REALCONS_CPU_PDP11_CPUMODE_USER);

    // CONSOLE: processor accepts commands from console panel when HALTed
    console_mode = !SIGNAL_GET(cpusignal_run) || SIGNAL_GET(cpusignal_console_halt);

    /*************************************************************
     * eval switches
     * react on changed action switches
     */

    // fetch switch register
    SIGNAL_SET(cpusignal_switch_register, (t_value )_this->switch_SR->value);

    // fetch HALT mode, must be sensed by simulated processor to produce state OPERATOR_HALT
    if (panel_lock)
        SIGNAL_SET(cpusignal_console_halt,0);
    else
        SIGNAL_SET(cpusignal_console_halt, (t_value )_this->switch_HALT->value);

    /* which command switch was activated? Process only one of these */
    action_switch = NULL;
    if (!panel_lock) {
        if (!action_switch && _this->switch_LOAD_ADRS->value == 1
                && _this->switch_LOAD_ADRS->value_previous == 0)
            action_switch = _this->switch_LOAD_ADRS;
        if (!action_switch && _this->switch_EXAM->value == 1
                && _this->switch_EXAM->value_previous == 0)
            action_switch = _this->switch_EXAM;
        if (!action_switch && _this->switch_DEPOSIT->value == 1
                && _this->switch_DEPOSIT->value_previous == 0)
            action_switch = _this->switch_DEPOSIT;
        if (!action_switch && _this->switch_CONT->value == 1
                && _this->switch_CONT->value_previous == 0)
            action_switch = _this->switch_CONT;
        if (!action_switch && // START actions on rising and falling edge!
                _this->switch_START->value ^ _this->switch_START->value_previous)
            action_switch = _this->switch_START;
    }
    // first: reset "switch changed" condition
    if (action_switch)
        action_switch->value_previous = action_switch->value;

    /*
     * ATTENTION:
     * Switch actions may NOT manipulate the console state directly!
     * They ONLY may generated SimH commands,
     * which in turn lets SimH signal a changed machine state
     * which is evaluated for display of panel indicators
     */

    // accept input only in CONSOLE mode. Exception: HALT is always enabled, see above
    if (!console_mode)
        action_switch = NULL;

    /* About the "Console address Register"
     * there is no physical "console adr register" in 11/70,
     * CONS PHY is connect to virtual address, as LOAD ADR clocks data in to DR reg on ALU
     * which apparently located on the "virtual" side of the MMU.
     * so when the CPU is running, the CONS PHY display shows in fact PROG PHY,
     * instead of "SR", code uses "console_loaded_address"
     */

    /* raise of ADRS ERROR: may be overwritten by "auto increment" error */
    if (SIGNAL_GET(cpusignal_memory_status) != SCPE_NXM) {
        _this->last_memory_status = SIGNAL_GET(cpusignal_memory_status); // prohibit re-raise
    } else {
        if (console_mode && _this->last_memory_status != SCPE_NXM) {
            // LCM2018: bits 15..0 of CONS PHY cleared -> internal virt addess cleared?
            // Now CONS PHY shows only SWR 21..16
            _this->loaded_address_15_00_datapathSR = 0;
            // what else does ADR err clear? EXAM/DEP STEP?
            _this->last_memory_status = SCPE_NXM; // prohibit re-raise
        }
        _this->autoinc_addr_action_switch = NULL; // break EXAM/DEP STEP
    }

    if (action_switch) {
        /* auto addr inc logic */
        addr_inc_error = 0;
        if (action_switch != _this->autoinc_addr_action_switch)
            // change of switch: DEP or EXAM sequence broken
            _this->autoinc_addr_action_switch = NULL;
        else {
            // inc console address register, may give ADRS ERR
            // I think they only increment the 16 bit SR, not bits 21:16 ?
            _this->loaded_address_15_00_datapathSR =
                increment_address(loaded_address_22bit(_this), &addr_inc_error) & 0xffff;
            if (addr_inc_error == 1) {
                // error stepping into register bank. Effect like an LOAD ADRS ??!! LCM2018 script 9
                _this->loaded_address_15_00_datapathSR = _this->switch_SR->value & 0xffff;
                // error in auto inc: no further memory access, would clear the error flag
                SIGNAL_SET(cpusignal_memory_status, SCPE_NXM);
                _this->last_memory_status = SCPE_NXM; // prohibit re-raise
                _this->autoinc_addr_action_switch = NULL; // break EXAM/DEP STEP
            }
        }

        if (action_switch == _this->switch_LOAD_ADRS) {
            _this->loaded_address_15_00_datapathSR =
                    (realcons_machine_word_t ) (_this->switch_SR->value & 0xffff); // 16 bit
            _this->loaded_address_datapathSWR = (realcons_machine_word_t) (_this->switch_SR->value & 0x3fffff); // 22 bit

            // duplicate LOAD ADRs on PROG PHY?
            //SIGNAL_SET(cpusignal_memory_address_phys_register,_this->console_loaded_address) ;
            SIGNAL_SET(cpusignal_DATAPATH_shifter, 0); // LCM2018 video script10_TB8A3123 show: DATA is cleared

            // LOAD ADRS clears ADRS ERR (LCM2018)
            SIGNAL_SET(cpusignal_memory_status, SCPE_OK);


            if (_this->realcons->debug)
                printf("LOADADR %o\n", _this->loaded_address_15_00_datapathSR);
        }


        if (action_switch == _this->switch_EXAM && !addr_inc_error) {
//            t_addr pa; // physical address
            _this->autoinc_addr_action_switch = _this->switch_EXAM; // inc addr on next EXAM
            // generate simh "exam cmd"
            // fix octal, should use SimH-radix

//            pa = console_addr_register_physical(_this);
            sprintf(_this->realcons->simh_cmd_buffer, "examine %s\n",
                realcons_console_pdp11_70_addr_panel2simh(
                    loaded_address_22bit(_this),
                            _this->switch_ADDR_SELECT->value));
        }


        if (action_switch == _this->switch_DEPOSIT && !addr_inc_error) {
//            t_addr pa; // physical address
            unsigned dataval;
            dataval = (realcons_machine_word_t) _this->switch_SR->value & 0xffff; // trunc to switches 15..0
            _this->autoinc_addr_action_switch = _this->switch_DEPOSIT; // inc addr on next DEP

            // produce SimH cmd. fix octal, should use SimH-radix
//            pa = console_addr_register_physical(_this);
            sprintf(_this->realcons->simh_cmd_buffer, "deposit %s %o\n",
                    realcons_console_pdp11_70_addr_panel2simh(
                            loaded_address_22bit(_this),
                            _this->switch_ADDR_SELECT->value), dataval);
        }

        /* function of CONT, START mixed with HALT:
         *  Switch  HALT    SimH          Function
         *  ------  ----    ----          ---------
         *  START   ON	    reset         INITIALIZE
         *  START   OFF	    run <cons phy> INITIALIZE, and start processor operation at cons phy addr
         *  CONT    ON	    step 1        execute next single step
         *  CONT    OFF	    cont          continue execution at PC
         *
         *  The 11/70 has no BOOT switch.
         */
        if (action_switch == _this->switch_START) {
            // START has actions on rising AND falling edge!
            if (_this->switch_START->value && _this->switch_HALT->value) { // rising edge and HALT: INITIALIZE = SimH "RESET"
                SIGNAL_SET(cpusignal_memory_status, SCPE_OK); // clr ADRS ERR
                sprintf(_this->realcons->simh_cmd_buffer, "reset all\n") ;
                // set PC to LOAD ADRS, is don in event_cput_reset (msut also work on "sim>reset"
            }
            if (_this->switch_START->value && !_this->switch_HALT->value) {
                // rising edge and not HALT: will start on falling edge
                // but CONSOLE LED goes of now! Too difficult ....
                // console_led_prestart_off = 1;
            }
            if (!_this->switch_START->value && !_this->switch_HALT->value) {
                // falling edge and not HALT: start
                // INITIALIZE, and start processor operation at CONS PHY
                // not documented, but start at virtual addr must be also be converted to physical
                // t_addr pa = console_addr_register_physical(_this);
                sprintf(_this->realcons->simh_cmd_buffer, "run %o\n",
                        _this->loaded_address_15_00_datapathSR // always 22 bit physical ?
                                );
            }
        } else if (action_switch == _this->switch_CONT && _this->switch_HALT->value) {
            // single step = SimH "STEP 1"
            sprintf(_this->realcons->simh_cmd_buffer, "step 1\n");
        } else if (action_switch == _this->switch_CONT && !_this->switch_HALT->value) {
            // continue =  SimH "CONT"
            sprintf(_this->realcons->simh_cmd_buffer, "cont\n");
        }

    } // action_switch

    /***************************************************
     * update lights
     */
    // while test not expired: do lamp test
    if (!_this->realcons->lamp_test && _this->realcons->timer_running_msec[TIMER_TEST])
        realcons_lamp_test(_this->realcons, 1); // begin lamptest
    if (_this->realcons->lamp_test && !_this->realcons->timer_running_msec[TIMER_TEST])
        realcons_lamp_test(_this->realcons, 0); // end lamptest
    if (_this->realcons->lamp_test)
        return SCPE_OK; // no lights need to be set


    if (console_mode) {
        // micro code translates SR to virtual address, but not to PHYS
        // see FLOWS 14, COM.00: "DISPLAY ADDRESS & DATA. uADDRS 170,167
        SIGNAL_SET(cpusignal_memory_address_virt_register, _this->loaded_address_15_00_datapathSR);
        // In single step, PC must remain in adress_virt.
        // all HALTs go through FLOWS 12, BRK.00, which sets SR := PC
    }

    // ADR ERROR
    {
        t_stat tmp = SIGNAL_GET(cpusignal_memory_status);
        if (tmp == SCPE_NXM) {
            _this->led_ADRS_ERR->value = 1 ;
        }
        else _this->led_ADRS_ERR->value = 0 ;
    }


    {
        /*	ADDRESS SELECT: "VIRTUAL - Six positions: KERNEL, SUPER and USER I space and KERNEL, SUPER and
         * USER D space. The address displayed is a 16-bit virtual address; bits
         * 21 -16 are always off. During Console DEP or EXAM operations, bits
         * 15:00 of the Switch Register are considered to be a Virtual Address.
         * If Memory Management is enabled, this Virtual Address is relocated.
         * The set of PAR/PDRs indicated by the switch position is used."
         */
        switch (_this->switch_ADDR_SELECT->value) {
        case ADDR_SELECT_VALUE_CONS_PHY: {

            if (!console_mode) { // running:
                // show last virtual address 0:15, bits 21:16 from Switch Register
                unsigned val = SIGNAL_GET(cpusignal_memory_address_virt_register) & 0xffff;
                val |= (_this->loaded_address_datapathSWR & 0x3f0000); // bits 21:16 from SWR
                _this->leds_ADDRESS->value = val ;
                // RUN destroy the result of last LOAD ADRS. No chance to guess
                // the content of the SR register in data path, use something.
                // HALT however initializes SR with PC (FPMS FLOWS 12 BRK.00)
                _this->loaded_address_15_00_datapathSR = 0x123456; // "invalid" indicator, never visible!
            }
            else {
                // when HALT: micro code show value of SR (LOAD ADR) in virtual addresses, without running through MMU?
                _this->leds_ADDRESS->value = loaded_address_22bit(_this);
            }
            }
            break;
        case ADDR_SELECT_VALUE_PROG_PHY:
            // show last physical UNIBUS address
            _this->leds_ADDRESS->value = SIGNAL_GET(cpusignal_memory_address_phys_register);
            break;
        default: // One of the VIRTUAL positions:
                 // interpret console address as 16 bit virtual and display
            // console_address_register_physical(_this)
            // PATCH: show physical truncated
            _this->leds_ADDRESS->value = SIGNAL_GET(cpusignal_memory_address_virt_register) & 0xffff;
        }
    }
    //	if (_this->realcons->debug)
    //		printf("led_ADDRESS=%o, ledDATA=%o\n", (unsigned) _this->led_ADDRESS->value,
    //				(unsigned) _this->led_DATA->value);

    // DATA - CPU intern mux output ... a lot of different things.
    // see realcons_console_pdp11_70_set_machine_state()
    // state transition is signaled by SimH over machine_set_state()
    // DATA shows intern 11/70 processor ALU output (SHIFTER)
    switch (_this->switch_DATA_SELECT->value) {
    case DATA_SELECT_VALUE_uADDR_FPU_CPU:
        if (_this->run_state) {
        // no simulator devilvers the correct stream of micro program addresses.
        // display const pattern all around, gives steady dimmed display
        unsigned uadridx = _this->realcons->service_cycle_count % MICROADDRPATTERN_RUN_LEN;
        _this->leds_DATA->value = MicroAddrPatternRUN[uadridx];
        }
        else {
            unsigned uadridx = _this->realcons->service_cycle_count % MICROADDRPATTERN_CON_LEN;
            _this->leds_DATA->value = MicroAddrPatternCON[uadridx];
        }
        break;
    case DATA_SELECT_VALUE_DISPLAY_REGISTER:
        _this->leds_DATA->value = SIGNAL_GET(cpusignal_display_register);
        break;
    case DATA_SELECT_VALUE_DATA_PATH:
        // "SHIFTER" (EXAM/DEPOSIT)
        _this->leds_DATA->value = SIGNAL_GET(cpusignal_DATAPATH_shifter);
        break;
    case DATA_SELECT_VALUE_BUS_REG:
        if (console_mode)
            // when halted, BUS_REG shows switches
            _this->leds_DATA->value = _this->switch_SR->value;
        else
            _this->leds_DATA->value = SIGNAL_GET(cpusignal_memory_data_register);
        break;
    }

    // parity LEDs: show on READ from cache = physical memory
    // clear on write ... on all write or only on write in memory?
    if (SIGNAL_GET(cpusignal_memory_write_access)) {
        _this->led_PARITY_HIGH->value = 0 ;
        _this->led_PARITY_LOW->value = 0 ;
    } else if (SIGNAL_GET(cpusignal_memory_address_phys_register) <=  017760000) {
        // READ memory from cache: parity ON if EVEN number of bits
        unsigned val = SIGNAL_GET(cpusignal_memory_data_register) ;
        _this->led_PARITY_LOW->value = ! ( bitcount(val & 0xff) & 1) ;
        _this->led_PARITY_HIGH->value = ! ( bitcount((val >> 8) & 0xff) & 1) ;
    }

    // Encode User, Super, Kernel mode.
    // Panel electronic does not bit-encode the USER/SUPER/KERNEL LEDs,
    // but uses an LED addressing scheme: 0=Kernel, 1=off, 2=Super, 3=User
    switch (SIGNAL_GET(cpusignal_cpu_mode)) {
    case REALCONS_CPU_PDP11_CPUMODE_USER:
        _this->leds_MMR0_MODE->value = 3;
        break;
    case REALCONS_CPU_PDP11_CPUMODE_SUPERVISOR:
        _this->leds_MMR0_MODE->value = 2;
        break;
    case REALCONS_CPU_PDP11_CPUMODE_KERNEL:
        _this->leds_MMR0_MODE->value = 0;
        break;
    }
    // ADDRESSING 16,18,22 determined by MMR0 and MMR3 bits, see
    // KB11C PROCESSOR MANUAL, page IV-I-2
    // Under RSX11  always "22" is ON, while on this video its always "16":
    // https://www.youtube.com/watch?v=tWOfN9p5E8k

    _this->led_ADDRESSING_16->value = 0;
    _this->led_ADDRESSING_18->value = 0;
    _this->led_ADDRESSING_22->value = 0;
    // "16 bit mapping if MMR0[0] cleared"
    if (!(SIGNAL_GET(cpusignal_MMR0) & 0x01)) // bit0=0 means MMU is off
        _this->led_ADDRESSING_16->value = 1;
    // "18 bit mapping when bit 4 of MMR3 is cleared and bit 0 of MMR0 set"
    if (!(SIGNAL_GET(cpusignal_MMR3) & 0x10) && ( SIGNAL_GET(cpusignal_MMR0) & 0x01))
        _this->led_ADDRESSING_18->value = 1;
    // "22 bit mapping, when both bit 4 of MMR3 and bit 0 of MMR0 are set"
    if ((SIGNAL_GET(cpusignal_MMR3) & 0x10) && ( SIGNAL_GET(cpusignal_MMR0) & 0x01))
        _this->led_ADDRESSING_22->value = 1;

    // D mode, I mode?
    _this->led_DATA_SPACE->value = SIGNAL_GET(cpusignal_bus_ID_mode);

    // MASTER: according to LCM2018, ON if HALT, OFF else
    if (console_mode)
        _this->led_MASTER->value = 1;
    else
        _this->led_MASTER->value = 0;

    // RUN: OFF on HALT, during PAUSE. ON during WAIT and RESET
    // PAUSE not implemented
    if (SIGNAL_GET(cpusignal_run))
        // Running: ON
        _this->led_RUN->value = 1;
    else
        // not running.
        _this->led_RUN->value = 0;

    return SCPE_OK;
}

/*
 * start 1 sec test sequence.
 * - lamps ON for 1 sec
 * - print state of all switches
 */
int realcons_console_pdp11_70_test(realcons_console_logic_pdp11_70_t *_this, int arg)
{
    // send end time for test: 1 second = curtime + 1000
    // lamp test is set in service()
    _this->realcons->timer_running_msec[TIMER_TEST] = _this->realcons->service_cur_time_msec
            + TIME_TEST_MS;

    realcons_printf(_this->realcons, stdout, "Verify lamp test!\n");
    realcons_printf(_this->realcons, stdout, "Switch SR          = %llo\n",
            _this->switch_SR->value);
    realcons_printf(_this->realcons, stdout, "Switch LOAD ADRS   = %llo\n",
            _this->switch_LOAD_ADRS->value);
    realcons_printf(_this->realcons, stdout, "Switch EXAM        = %llo\n",
            _this->switch_EXAM->value);
    realcons_printf(_this->realcons, stdout, "Switch DEPOSIT     = %llo\n",
            _this->switch_DEPOSIT->value);
    realcons_printf(_this->realcons, stdout, "Switch CONT        = %llo\n",
            _this->switch_CONT->value);
    realcons_printf(_this->realcons, stdout, "Switch HALT        = %llo\n",
            _this->switch_HALT->value);
    realcons_printf(_this->realcons, stdout, "Switch S INST/S BUS= %llo\n",
            _this->switch_S_BUS_CYCLE->value);
    realcons_printf(_this->realcons, stdout, "Switch START       = %llo\n",
            _this->switch_START->value);
    realcons_printf(_this->realcons, stdout, "Switch ADDR SELECT = %llo\n",
            _this->switch_ADDR_SELECT->value);
    realcons_printf(_this->realcons, stdout, "Switch DATA SELECT = %llo\n",
            _this->switch_DATA_SELECT->value);
    return 0; // OK
}


#endif // USE_REALCONS
