#! /usr/bin/env python3
#
PROG_NAME = 'Name: PiDP1170Test.py'
PROG_DESC = "Description: Stand-alone program to test the LEDs and switches on Oscar Vermulen's PiDP-11/70"
PROG_AUTHOR = 'Author: Neil Higgins'
PROG_DOC = 'Documentation: See PDF document, "PiDP-11/70 Test Program"'
PROG_LICENCE = 'Licence: Free as (in beer) and free (as in thought): Use at your own risk'
PROG_VER = 'Version: 0.5 - Selective inversion of switches added, to enable display of logical state'
PROG_OPTIONS1 = 'Options: '
PROG_OPTIONS2 = '  -h or --help  ... This help'
PROG_OPTIONS3 = '  -d or --debug ... Run program with I/O turned off and debug data displayed'
PROG_OPTIONS4 = '  No option     ... Run program'
PROG_TIPS1 = 'Please maximise your command window before using this program'
PROG_TIPS2 = 'Please ensure that the simh client and the blinkenlight server are shut down before using this program'
PROG_HELP = [PROG_NAME, PROG_DESC, PROG_AUTHOR, PROG_DOC, PROG_LICENCE, PROG_VER,
            PROG_OPTIONS1, PROG_OPTIONS2, PROG_OPTIONS3, PROG_OPTIONS4,
            PROG_TIPS1, PROG_TIPS2]
#
# ---------- Start of imports ----------
#
import sys
import random
import os
import time
import RPi.GPIO as GPIO
#
# ---------- Start of declarations ----------
#
DEBUG = False
#
LED_PULSE_DURATION_MS = 250 # milliseconds
SWITCH_SETTLE_TIME_MS = 10 # milliseconds
SWITCH_SAMPLE_INTERVAL_MS = 1000 # milliseconds
ENCODER_SETTLE_TIME_MS = 0 # milliseconds
ENCODER_SAMPLE_INTERVAL_MS = 10 # milliseconds
#
# Tuple for an LED
# (number, name, function, electronic row GPIO, electronic column GPIO, physical row, physical column)
# eg. (37, 'LED37', 'Function', 1, 2, 3, 4)
# LED attribute indices
LEDNUM = 0
LEDNAME = 1
LEDFUNC = 2
LEDROWGPIO = 3
LEDCOLGPIO = 4
LEDPHYSROW = 5
LEDPHYSCOL = 6
# LEDs
LED1 = (1, 'LED1', 'A15', 21, 5, 5, 7)
LED2 = (2, 'LED2', 'A14', 21, 4, 5, 8)
LED3 = (3, 'LED3', 'A13', 21, 27, 5, 9)
LED4 = (4, 'LED4', 'A12', 21, 26, 5, 10)
LED5 = (5, 'LED5', 'A11', 20, 13, 5, 11)
LED6 = (6, 'LED6', 'A10', 20, 12, 5, 12)
LED7 = (7, 'LED7', 'A9', 20, 11, 5, 13)
LED8 = (8, 'LED8', 'A8', 20, 10, 5, 14)
LED9 = (9, 'LED9', 'A7', 20, 9, 5, 15)
LED10 = (10, 'LED10', 'A6', 20, 8, 5, 16)
LED11 = (11, 'LED11', 'A5', 20, 7, 5, 17)
LED12 = (12, 'LED12', 'A4', 20, 6, 5, 18)
LED13 = (13, 'LED13', 'A3', 20, 5, 5, 19)
LED14 = (14, 'LED14', 'A2', 20, 4, 5, 20)
LED15 = (15, 'LED15', 'A1', 20, 27, 5, 21)
LED16 = (16, 'LED16', 'A0', 20, 26, 5, 22)
LED17 = (17, 'LED17', 'A21', 21, 11, 5, 1)
LED18 = (18, 'LED18', 'A20', 21, 10, 5, 2)
LED19 = (19, 'LED19', 'A19', 21, 9, 5, 3)
LED20 = (20, 'LED20', 'A18', 21, 8, 5, 4)
LED21 = (21, 'LED21', 'A17', 21, 7, 5, 5)
LED22 = (22, 'LED22', 'A16', 21, 6, 5, 6)
LED23 = (23, 'LED23', 'RUN', 22, 11, 1, 3)
LED24 = (24, 'LED24', 'PAUSE', 22, 10, 1, 4)
LED25 = (25, 'LED25', 'MASTER', 22, 9, 1, 5)
LED26 = (26, 'LED26', 'USER', 22, 8, 1, 6)
LED27 = (27, 'LED27', 'SUPER', 22, 7, 1, 7)
LED28 = (28, 'LED28', 'KERNEL', 22, 6, 1, 8)
LED29 = (29, 'LED29', 'DATA', 22, 5, 1, 9)
LED30 = (30, 'LED30', 'ADD16', 22, 4, 1, 10)
LED31 = (31, 'LED31', 'ADD18', 22, 27, 1, 11)
LED32 = (32, 'LED32', 'ADD22', 22, 26, 1, 12)
LED33 = (33, 'LED33', 'D15', 24, 5, 8, 3)
LED34 = (34, 'LED34', 'D14', 24, 4, 8, 4)
LED35 = (35, 'LED35', 'D13', 24, 27, 8, 5)
LED36 = (36, 'LED36', 'D12', 24, 26, 8, 6)
LED37 = (37, 'LED37', 'D11', 23, 13, 8, 7)
LED38 = (38, 'LED38', 'D10', 23, 12, 8, 8)
LED39 = (39, 'LED39', 'D9', 23, 11, 8, 9)
LED40 = (40, 'LED40', 'D8', 23, 10, 8, 10)
LED41 = (41, 'LED41', 'D7', 23, 9, 8, 11)
LED42 = (42, 'LED42', 'D6', 23, 8, 8, 12)
LED43 = (43, 'LED43', 'D5', 23, 7, 8, 13)
LED44 = (44, 'LED44', 'D4', 23, 6, 8, 14)
LED45 = (45, 'LED45', 'D3', 23, 5, 8, 15)
LED46 = (46, 'LED46', 'D2', 23, 4, 8, 16)
LED47 = (47, 'LED47', 'D1', 23, 27, 8, 17)
LED48 = (48, 'LED48', 'D0', 23, 26, 8, 18)
LED49 = (49, 'LED49', 'PAR_HIGH', 24, 7, 8, 1)
LED50 = (50, 'LED50', 'PAR_LOW', 24, 6, 8, 2)
LED51 = (51, 'LED51', 'PAR_ERR', 22, 13, 1, 1)
LED52 = (52, 'LED52', 'ADDR_ERR', 22, 12, 1, 2)
LED53 = (53, 'LED53', 'MU_A_FPP/CPU', 25, 12, 6, 2)
LED54 = (54, 'LED54', 'R2_DISP_REG', 25, 13, 7, 2)
LED55 = (55, 'LED55', 'R2_BUS_REG', 24, 13, 7, 1)
LED56 = (56, 'LED56', 'R2_DATA_PATHS', 24, 12, 6, 1)
LED57 = (57, 'LED57', 'R1_KERNEL_D', 24, 10, 3, 1)
LED58 = (58, 'LED58', 'R1_SUPER_D', 24, 9, 2, 1)
LED59 = (59, 'LED59', 'R1_USER_D', 24, 8, 1, 13)
LED60 = (60, 'LED60', 'R1_USER_I', 25, 8, 1, 14)
LED61 = (61, 'LED61', 'R1_SUPER_I', 25, 9, 2, 2)
LED62 = (62, 'LED62', 'R1_KERNEL_I', 25, 10, 3, 2)
LED63 = (63, 'LED63', 'R1_PROG_PHY', 25, 11, 4, 2)
LED64 = (64, 'LED64', 'R1_CONS_PHY', 24, 11, 4, 1)
# Associated constants
LED_ROW_GPIOs = [20, 21, 22, 23, 24, 25]
LED_PHYS_ROWS = 8
LED_PHYS_COLUMNS = 22
# All the LEDs
LEDs =  (
LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8, LED9, LED10,
LED11, LED12, LED13, LED14, LED15, LED16, LED17, LED18, LED19, LED20,
LED21, LED22, LED23, LED24, LED25, LED26, LED27, LED28, LED29, LED30,
LED31, LED32, LED33, LED34, LED35, LED36, LED37, LED38, LED39, LED40,
LED41, LED42, LED43, LED44, LED45, LED46, LED47, LED48, LED49, LED50,
LED51, LED52, LED53, LED54, LED55, LED56, LED57, LED58, LED59, LED60,
LED61, LED62, LED63, LED64
)
#
# Tuple for a switch
# (number, name, function, electronic row GPIO, electronic column GPIO, physical row, physical column, octal digit, weight)
# eg. (37, 'SW37', 'Function', 1, 2, 3, 4, 11, 4)
# Switch attribute indices
SWNUM = 0
SWNAME = 1
SWFUNC = 2
SWROWGPIO = 3
SWCOLGPIO = 4
SWINVERT = 5
SWPHYSROW = 6
SWPHYSCOL = 7
SWOCTDIG = 8
SWOCTWGHT = 9
# Switches
SW1 = (1, 'SW1', 'SR16', 17, 6, True, 3, 6, 3, 2)
SW2 = (2, 'SW2', 'SR17', 17, 7, True, 3, 5, 3, 4)
SW3 = (3, 'SW3', 'SR18', 17, 8, True, 3, 4, 2, 1)
SW4 = (4, 'SW4', 'SR19', 17, 9, True, 3, 3, 2, 2)
SW5 = (5, 'SW5', 'SR20', 17, 10, True, 3, 2, 2, 4)
SW6 = (6, 'SW6', 'SR21', 17, 11, True, 3, 1, 1, 1)
SW7 = (7, 'SW7', 'SR0', 16, 26, True, 3, 22, 8, 1)
SW8 = (8, 'SW8', 'SR1', 16, 27, True, 3, 21, 8, 2)
SW9 = (9, 'SW9', 'SR2', 16, 4, True, 3, 20, 8, 4)
SW10 = (10, 'SW10', 'SR3', 16, 5, True, 3, 19, 7, 1)
SW11 = (11, 'SW11', 'SR4', 16, 6, True, 3, 18, 7, 2)
SW12 = (12, 'SW12', 'SR5', 16, 7, True, 3, 17, 7, 4)
SW13 = (13, 'SW13', 'SR6', 16, 8, True, 3, 16, 6, 1)
SW14 = (14, 'SW14', 'SR7', 16, 9, True, 3, 15, 6, 2)
SW15 = (15, 'SW15', 'SR8', 16, 10, True, 3, 14, 6, 4)
SW16 = (16, 'SW16', 'SR9', 16, 11, True, 3, 13, 5, 1)
SW17 = (17, 'SW17', 'SR10', 16, 12, True, 3, 12, 5, 2)
SW18 = (18, 'SW18', 'SR11', 16, 13, True, 3, 11, 5, 4)
SW19 = (19, 'SW19', 'START', 18, 9, True, 3, 30, 16, 1)
# SW20 = ()
SW21 = (21, 'SW21', 'LOAD-ADDR', 18, 27, True, 3, 24, 10, 1)
SW22 = (22, 'SW22', 'EXAM', 18, 4, True, 3, 25, 11, 1)
SW23 = (23, 'SW23', 'DEP', 18, 5, True, 3, 26, 12, 1)
SW24 = (24, 'SW24', 'CONT', 18, 6, True, 3, 27, 13, 1)
SW25 = (25, 'SW25', 'ENA_HALT', 18, 7, False, 3, 28, 14, 1)
SW26 = (26, 'SW26', 'SING_INST', 18, 8, True, 3, 29, 15, 1)
# SW27 = ()
SW28 = (28, 'SW28', 'TEST', 18, 26, False, 3, 23, 9, 1)
SW29 = (29, 'SW29', 'SR12', 17, 26, True, 3, 10, 4, 1)
SW30 = (30, 'SW30', 'SR13', 17, 27, True, 3, 9, 4, 2)
SW31 = (31, 'SW31', 'SR14', 17, 4, True, 3, 8, 4, 4)
SW32 = (32, 'SW32', 'SR15', 17, 5, True, 3, 7, 3, 1)
SW33 = (33, 'SW33', 'E1-ADDR', 17, 12, True, 1, 1, 17, 1)
SW34 = (34, 'SW34', 'E1-A (I)', 18, 10, False, 1, 2, 17, 0)
SW35 = (35, 'SW35', 'E1-B (Q)', 18, 11, False, 1, 3, 17, 0)
SW36 = (36, 'SW36', 'E2-DATA', 17, 13, True, 2, 1, 18, 1)
SW37 = (37, 'SW37', 'E2-A (I)', 18, 12, False, 2, 2, 18, 0)
SW38 = (38, 'SW38', 'E2-B (Q)', 18, 13, False, 2, 3, 18, 0)
#Associated constants
NUM_SWITCHES = 38
NUM_ENCODERS = 2
SWITCH_ROW_GPIOs = [16, 17, 18]
SWITCH_PHYS_ROWS = 3
SWITCH_PHYS_COLUMNS = 30
# All the switches
Switches = (
SW1, SW2, SW3, SW4, SW5, SW6, SW7, SW8, SW9, SW10,
SW11, SW12, SW13, SW14, SW15, SW16, SW17, SW18, SW19,
SW21, SW22, SW23, SW24, SW25, SW26, SW28, SW29, SW30,
SW31, SW32, SW33, SW34, SW35, SW36, SW37, SW38
)
# The encoder switches
EncoderSwitches = (SW34, SW35, SW37, SW38)
# Directions
ENCODER_NONE = 0
ENCODER_UP = 1
ENCODER_DOWN = -1
#
# Tuple for an octal digit
# (number, name, function, display row, display column)
# eg. (7, 'DIG7', 'Function', 1, 2)
# Octal digit attribute indices
DIGNUM = 0
DIGNAME = 1
DIGFUNC = 2
DIGY = 3
DIGX = 4
# Octal digits for switches
DIGSR7 = (1, '0oSR7', 'SR21', 1, 1) 
DIGSR6 = (2, '0oSR6', 'SR20-18', 1, 2) 
DIGSR5 = (3, '0oSR5', 'SR17-15', 1, 3) 
DIGSR4 = (4, '0oSR4', 'SR14-12', 1, 4) 
DIGSR3 = (5, '0oSR3', 'SR11-9', 1, 5) 
DIGSR2 = (6, '0oSR2', 'SR8-6', 1, 6) 
DIGSR1 = (7, '0oSR1', 'SR5-3', 1, 7) 
DIGSR0 = (8, '0oSR0', 'SR2-0', 1, 8)
DIGTEST = (9, 'TEST', 'TEST', 2, 1)
DIGLOAD_ADDR = (10, 'LOAD_ADDR', 'LOAD_ADDR', 2, 2)
DIGEXAM = (11, 'EXAM', 'EXAM', 2, 3)
DIGDEP = (12, 'DEP', 'DEP', 2, 4)
DIGCONT = (13, 'CONT', 'CONT', 2, 5)
DIGENA_HALT = (14, 'ENA_HALT', 'ENA_HALT', 2, 6)
DIGSING_INST = (15, 'SING_INST', 'SING_INST', 2, 7)
DIGSTART = (16, 'START', 'START', 2, 8)
DIGADDRESS = (17, 'ADDRESS', 'ADDR - PRESS', 3, 1)
DIGDATA = (18, 'DATA', 'DATA - PRESS', 3, 2)
# Associated constants
NUM_OCTDIGITS = 18
NUM_OCT_ROWS = 3
NUM_OCT_COLS = 8
CHARS_PER_OCT_COL = 9
# All the octal digits
OctalDigits = (
DIGSR7, DIGSR6, DIGSR5, DIGSR4, DIGSR3, DIGSR2, DIGSR1, DIGSR0,
DIGTEST, DIGLOAD_ADDR, DIGEXAM, DIGDEP, DIGCONT, DIGENA_HALT, DIGSING_INST, DIGSTART,
DIGADDRESS, DIGDATA
)
#
# Column GPIOs
COLUMN_GPIOs = [26, 27, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13]
#
# Row GPIO states
ROW_GPIO_TRIS = 0
ROW_GPIO_LOW = 1
ROW_GPIO_HIGH = 2
ROW_GPIO_STATES = (ROW_GPIO_TRIS, ROW_GPIO_LOW, ROW_GPIO_HIGH)
#
# Column GPIO states
COLUMN_GPIO_TRIS = 100
COLUMN_GPIO_LOW = 101
COLUMN_GPIO_HIGH = 102
COLUMN_GPIO_INPUT = 103 # Note: With weak pullup
COLUMN_GPIO_STATES = (COLUMN_GPIO_TRIS, COLUMN_GPIO_LOW, COLUMN_GPIO_HIGH, COLUMN_GPIO_INPUT)
#
# ---------- Start of functions ----------
#
# Python doesn't have this
def logical_xor(a, b):
   return((a and not(b)) or (not(a) and b))
#
# Echo the configuration data for comparison with the specification document
def echo_config_data():
    # List the connections table - LEDs
    for column_gpio in COLUMN_GPIOs:
        for row_gpio in LED_ROW_GPIOs:
            # find the LED and output its name and function
                a_LED = led_by_GPIOs(row_gpio, column_gpio)
                if (a_LED):
                    print(column_gpio, row_gpio, a_LED[LEDNAME], a_LED[LEDFUNC]) 
                else:
                    print(column_gpio, row_gpio, '-', '-')
        print()
#
    # List the connections table - switches
    for column_gpio in COLUMN_GPIOs:
        for row_gpio in SWITCH_ROW_GPIOs:
            # find the switch and output its name and function
                a_SW = switch_by_GPIOs(row_gpio, column_gpio)
                if (a_SW):
                    print(column_gpio, row_gpio, a_SW[SWNAME], a_SW[SWFUNC]) 
                else:
                    print(column_gpio, row_gpio, '-', '-')
        print()
#
    # List the physical rows - LEDs
    for phys_row in range(1, (LED_PHYS_ROWS+1)):
        for phys_column in range(1, (LED_PHYS_COLUMNS+1)):
            # find the LED and output its name and function
                a_LED = led_by_physicals(phys_row, phys_column)
                if (a_LED):
                    print(phys_row, phys_column, a_LED[LEDNAME], a_LED[LEDFUNC]) 
        print()
#
    # List the physical rows - switches and octal mappings
    for phys_row in range(1, (SWITCH_PHYS_ROWS+1)):
        for phys_column in range(1, (SWITCH_PHYS_COLUMNS+1)):
            # find the switch and output its name, function, octal digit, mask and octal digit function
                a_SW = switch_by_physicals(phys_row, phys_column)
                if (a_SW):
                    a_DIGNUM = a_SW[SWOCTDIG]
                    a_DIG = digit_by_number(a_DIGNUM)
                    if(a_DIG):
                        print(phys_row, phys_column, a_SW[SWNAME], a_SW[SWFUNC], a_SW[SWOCTDIG], a_SW[SWOCTWGHT], a_DIG[DIGFUNC])
        print()
#
# Find an LED by electronic row and column and return its tuple
def led_by_GPIOs(a_row, a_col):
    for a_led in LEDs:              
        if ((a_led[LEDROWGPIO] == a_row) and (a_led[LEDCOLGPIO] == a_col)):
            return a_led
#
# Find an LED by physical row and column and return its tuple
def led_by_physicals(a_row, a_col):
    for a_led in LEDs:
        if ((a_led[LEDPHYSROW] == a_row) and (a_led[LEDPHYSCOL] == a_col)):
            return a_led
#
# Find a switch by electronic row and column and return its tuple
def switch_by_GPIOs(a_row, a_col):
    for a_switch in Switches:
        if ((a_switch[SWROWGPIO] == a_row) and (a_switch[SWCOLGPIO] == a_col)):
            return a_switch
#
# Find a switch by physical row and column and return its tuple
def switch_by_physicals(a_row, a_col):
    for a_switch in Switches:
        if ((a_switch[SWPHYSROW] == a_row) and (a_switch[SWPHYSCOL] == a_col)):
            return a_switch
#
# Find an octal digit by number
def digit_by_number(a_num):
    for a_dig in OctalDigits:
        if a_dig[DIGNUM] == a_num:
            return a_dig
#
# Set the GPIO state for an LED row - tristate, low (inactive), high (active)
def set_LED_row_state(a_row, a_state):
    assert a_row in LED_ROW_GPIOs
    assert a_state in ROW_GPIO_STATES
    if DEBUG:
        pass
    else:
        if a_state == ROW_GPIO_TRIS:
            GPIO.setup(a_row, GPIO.IN, GPIO.PUD_OFF)
        elif a_state == ROW_GPIO_LOW:
            GPIO.setup(a_row, GPIO.OUT, initial = GPIO.LOW)
        elif a_state == ROW_GPIO_HIGH:
            GPIO.setup(a_row, GPIO.OUT, initial = GPIO.HIGH)   
#
# Set the GPIO state for a switch row - tristate, low (inactive), high (active)
def set_switch_row_state(a_row, a_state):
    assert a_row in SWITCH_ROW_GPIOs
    assert a_state in ROW_GPIO_STATES
    if DEBUG:
        pass
    else:
        if a_state == ROW_GPIO_TRIS:
            GPIO.setup(a_row, GPIO.IN, GPIO.PUD_OFF)
        elif a_state == ROW_GPIO_LOW:
            GPIO.setup(a_row, GPIO.OUT, initial = GPIO.LOW)
        elif a_state == ROW_GPIO_HIGH:
            GPIO.setup(a_row, GPIO.OUT, initial = GPIO.HIGH)   
#
# Set the GPIO state for a column - tristate, high (inactive), low (active), input w/weak pullup
def set_column_state(a_column, a_state):
    assert a_column in COLUMN_GPIOs
    assert a_state in COLUMN_GPIO_STATES
    if DEBUG:
        pass
    else:
        if a_state == COLUMN_GPIO_TRIS:
            GPIO.setup(a_column, GPIO.IN, GPIO.PUD_OFF)
        elif a_state == COLUMN_GPIO_LOW:
            GPIO.setup(a_column, GPIO.OUT, initial = GPIO.LOW)
        elif a_state == COLUMN_GPIO_HIGH:
            GPIO.setup(a_column, GPIO.OUT, initial = GPIO.HIGH)
        elif a_state == COLUMN_GPIO_INPUT:
            GPIO.setup(a_column, GPIO.IN, GPIO.PUD_UP)
#
# Read a column
def read_column(a_column): 
    if DEBUG:
        return bool(round(random.random(), 0)) 
    else:
        return GPIO.input(a_column)
#
# Put all rows and columns into tristate
def tristate_all():
    for a_row in LED_ROW_GPIOs:
        set_LED_row_state(a_row, ROW_GPIO_TRIS)
    for a_row in SWITCH_ROW_GPIOs:
        set_switch_row_state(a_row, ROW_GPIO_TRIS)
    for a_column in COLUMN_GPIOs:
        set_column_state(a_column, COLUMN_GPIO_TRIS)
#
# Wait for a duration in milliseconds
def local_wait_ms(duration_ms):
    if DEBUG:
        print('Waiting ' + str(duration_ms) + 'ms')
    time.sleep(duration_ms/1000.0)
#
# Pulse an LED given the electronic row and column and a pulse duration in milliseconds
def pulseLED(a_row, a_column, pulse_ms):
    # Look up the LED
    if led_by_GPIOs(a_row, a_column):
        # Set up the row and column
        set_LED_row_state(a_row, ROW_GPIO_HIGH)
        set_column_state(a_column, COLUMN_GPIO_LOW)
        # Wait
        local_wait_ms(pulse_ms)
        # Tear down the row and column
        tristate_all()
#
# Cycle through the LEDs by electronic row and column
def cycle_LEDs_electronic():
    print()
    print('          ' + 'row gpio'.center(10) + 'col gpio'.center(10) +
          'function'.center(10))
    for row_gpio in LED_ROW_GPIOs:
        for column_gpio in COLUMN_GPIOs:
            # find the LED
            a_LED = led_by_GPIOs(row_gpio, column_gpio)
            if(a_LED):
                function = a_LED[LEDFUNC]
                # pulse the LED
                print('Pulsing   ' +
                       str(row_gpio).center(10) + str(column_gpio).center(10) +
                       function.center(10))
                pulseLED(row_gpio, column_gpio, LED_PULSE_DURATION_MS)
        print()
#
# Cycle through the LEDs by physical row and column
def cycle_LEDs_physical():
    print()
    print('          ' + 'phys row'.center(10) + 'phys col'.center(10) +
          'row gpio'.center(10) + 'col gpio'.center(10) +
          'function'.center(10))
    for phys_row in range(1, (LED_PHYS_ROWS+1)):
        for phys_column in range(1, (LED_PHYS_COLUMNS+1)):
            # find the LED
            a_LED = led_by_physicals(phys_row, phys_column)
            if (a_LED):
                row_gpio = a_LED[LEDROWGPIO]
                column_gpio = a_LED[LEDCOLGPIO]
                function = a_LED[LEDFUNC]
                # pulse the LED
                print('Pulsing   ' + str(phys_row).center(10) + str(phys_column).center(10) +
                      str(row_gpio).center(10) + str(column_gpio).center(10) +
                      function.center(10))
                pulseLED(row_gpio, column_gpio, LED_PULSE_DURATION_MS)
        print()
#
# Read a switch given the electronic row and column and a settling time in milliseconds
def readSwitch(a_row, a_column, settle_ms):
    # Look up the switch
    a_switch = switch_by_GPIOs(a_row, a_column)
    if a_switch:
        # Set up the row and column
        tristate_all() # just to be sure
        set_column_state(a_column, COLUMN_GPIO_INPUT)
        set_switch_row_state(a_row, ROW_GPIO_LOW)
        # Wait
        local_wait_ms(settle_ms)
        # Read the column
        value = logical_xor(read_column(a_column), a_switch[SWINVERT])
        # Tear down the row and column
        tristate_all()
        return value
#
# Read all of the switches into a list
def readAllSwitches(samples):
    for a_switch in Switches:
        a_switch_sample = readSwitch(a_switch[SWROWGPIO], a_switch[SWCOLGPIO], SWITCH_SETTLE_TIME_MS)
        samples[a_switch[SWNUM]-1] = a_switch_sample
    if DEBUG:
        print() # visual separation
#
# Read the encoder switches into a list
def readEncoderSwitches(samples):
    for index in range(NUM_ENCODERS * 2):
        a_switch = EncoderSwitches[index]
        a_switch_sample = readSwitch(a_switch[SWROWGPIO], a_switch[SWCOLGPIO], ENCODER_SETTLE_TIME_MS)
        samples[index] = a_switch_sample
    if DEBUG:
        print() # visual separation
#
# Display mapping of switches to octal digits with switch values
def displaySwitchValues(samples):
    for a_switch in Switches:
        print(str(a_switch[SWOCTDIG]) + ', ' + str(a_switch[SWOCTWGHT]) + ' ' + str(samples[a_switch[SWNUM]-1]))

# Populate octals from switch values
def populateOctals(samples, octals):
    for an_octal in octals:
        an_octal = 0
    for a_switch in Switches:
        octal_digit_num = a_switch[SWOCTDIG]
        octal_digit_mask = a_switch[SWOCTWGHT]
        if samples[a_switch[SWNUM]-1]:
            octals[octal_digit_num-1] = octals[octal_digit_num-1] + octal_digit_mask
#
# Calculate the xor of two lists of octals 
def calculateXor(octals1, octals2, octalsxor):
    for index in range(0, NUM_OCTDIGITS):
        octalsxor[index] = octals1[index] ^ octals2[index]
#
# Display a set of octals in simple fashion - current, previous, xor
def displayOctalsSimple(current_vals, previous_vals, xor_vals):
    for index in range(0, NUM_OCTDIGITS):
        print(str(index+1), ', ' + str(current_vals[index]) + ', ' + str(previous_vals[index]) + ', ' + str(xor_vals[index]))
    print() # spacer
#
# Find an octal digit by row and column
def octdig_by_rowcol(a_row, a_col):
    for an_octdig in OctalDigits:
        if a_row == an_octdig[DIGY] and a_col == an_octdig[DIGX]:
            return an_octdig[DIGNUM]
#
# The yukky bit - formatting output
# Display a set of octals in fancy fashion - current, previous, xor
def displayOctalsFancy(current_vals, previous_vals, xor_vals):
    for an_octalRow in range(1, NUM_OCT_ROWS+1):
        # display the functions of the octal digits in a row heading - over multiple lines if necessary
        # start by finding the maximum function descriptor length - that will determine how many heading lines are required
        max_len = 0
        for an_octalColumn in range(1, NUM_OCT_COLS+1):
            # find the digit in that row and column
            an_octalDignum = octdig_by_rowcol(an_octalRow, an_octalColumn)
            if an_octalDignum:
                if len(OctalDigits[an_octalDignum-1][DIGFUNC]) > max_len:
                    max_len = len(OctalDigits[an_octalDignum-1][DIGFUNC])
        # Note: CHARS_PER_OCT_COLUMN includes separating space
        num_heading_lines = (max_len + CHARS_PER_OCT_COL-2) // (CHARS_PER_OCT_COL-1)
        for line_index in range(0, num_heading_lines):
            heading_line = (' ' * CHARS_PER_OCT_COL)
            for an_octalColumn in range(1, NUM_OCT_COLS+1):
                # find the digit in that row and column
                an_octalDignum = octdig_by_rowcol(an_octalRow, an_octalColumn)
                if an_octalDignum:
                    # the worst line in this program
                    new_content = \
                        OctalDigits[an_octalDignum-1][DIGFUNC][(line_index)*(CHARS_PER_OCT_COL-1):\
                        ((line_index+1)*(CHARS_PER_OCT_COL-1))]
                    heading_line = heading_line + new_content.center(CHARS_PER_OCT_COL)
                else:
                    heading_line = heading_line + (' ' * CHARS_PER_OCT_COL)
            print(heading_line)
        for a_lineLabel in ('Current', 'Previous', 'XOR'):
            value_line = (a_lineLabel + (' ' * CHARS_PER_OCT_COL))[0:(CHARS_PER_OCT_COL-1)]
            for an_octalColumn in range(1, NUM_OCT_COLS+1):
                # find the digit in that row and column
                an_octalDignum = octdig_by_rowcol(an_octalRow, an_octalColumn)
                if an_octalDignum: 
                    if a_lineLabel == 'Current':
                        value_line = value_line + str(current_vals[an_octalDignum-1]).center(CHARS_PER_OCT_COL)
                    elif a_lineLabel == 'Previous':
                        value_line = value_line + str(previous_vals[an_octalDignum-1]).center(CHARS_PER_OCT_COL)
                    else: # it's XOR
                        value_line = value_line + str(xor_vals[an_octalDignum-1]).center(CHARS_PER_OCT_COL)
                else:
                    value_line = value_line + (' ' * CHARS_PER_OCT_COL)
            print(value_line)
        print() # row spacer
#
def copy_clear_load_xor_display(switch_vals, currentOctals, previousOctals, xorOctals):
    # Copy current octal values to previous octal values
    previousOctals[:] = currentOctals
    # Initialise current octal values
    currentOctals[:] = ([0] * NUM_OCTDIGITS)
    # Populate current octal values
    readAllSwitches(switch_vals)
    populateOctals(switch_vals, currentOctals)
    # Populate delta
    calculateXor(currentOctals, previousOctals, xorOctals)
    # Display results
    if DEBUG:
        displayOctalsSimple(currentOctals, previousOctals, xorOctals)
    displayOctalsFancy(currentOctals, previousOctals, xorOctals)
#
def test_switches():
    # Allocate switch values, octals and delta
    switch_vals = [False] * NUM_SWITCHES
    previousOctals = [0] * NUM_OCTDIGITS
    currentOctals = [0] * NUM_OCTDIGITS
    xorOctals = [0] * NUM_OCTDIGITS
    #
    # Initially populate switch values
    readAllSwitches(switch_vals)
    if DEBUG:
        displaySwitchValues(switch_vals)
        print() # visual separation
    # Initially populate current octal values
    populateOctals(switch_vals, currentOctals)
    # Rinse and repeat
    while True:
        copy_clear_load_xor_display(switch_vals, currentOctals, previousOctals, xorOctals)
        _ = time.sleep(SWITCH_SAMPLE_INTERVAL_MS / 1000.0)
        _ = os.system('clear')
#
def test_rotary_encoders():
# We only want one count (up or down) per full Gray-code cycle
# We want to avoid multiple-counts caused by the bouncing of a single bit
# We will assume that only one bit changes (or bounces) at a time (due to Gray code encoding)
# Up count, detent to detent: AB = TT, AB = TF (B was T), AB = FF (A was T), AB = FT (B was F), AB = TT (A was F)
# Down count, detent to detent: AB = TT, AB = FT (A was T), AB = FF (B was T), AB = TF (A was F), AB = TT (B was F)
# i.e. must leave detent and arrive at detent in the same direction
    counters = [0] * NUM_ENCODERS
    previous_switchVals = [False] * (NUM_ENCODERS * 2)
    current_switchVals = [False] * (NUM_ENCODERS * 2)
    encoder_direction = ENCODER_NONE
    # print heading
    print()
    print('Encoder 1'.center(10) + 'Encoder 2'.center(10))
    readEncoderSwitches(current_switchVals)
    while True:
        previous_switchVals[:] = current_switchVals
        readEncoderSwitches(current_switchVals)
        # update counters
        for index in range(NUM_ENCODERS):
            if (
                current_switchVals[index * 2] == previous_switchVals[index * 2] and
                current_switchVals[(index * 2) + 1] == previous_switchVals[(index * 2) + 1]
            ):
                # no change
                pass
            elif (
                current_switchVals[index * 2] == True and
                current_switchVals[(index * 2) + 1] == True
            ):
                # at detent A=T & B=T
                if (
                    previous_switchVals[index * 2] == False and
                    encoder_direction == ENCODER_UP
                ):
                    # arriving in UP direction - A was F, direction is UP
                    counters[index] = (counters[index] + 1) % 10
                    encoder_direction = ENCODER_NONE
                    # show counters
                    print(str(counters[0]).center(10) + str(counters[1]).center(10))
                elif (
                    previous_switchVals[(index * 2) + 1] == False and
                    encoder_direction == ENCODER_DOWN
                ):
                    # arriving in DOWN direction - B was F, direction is DOWN
                    counters[index] = (counters[index] - 1) % 10 
                    encoder_direction = ENCODER_NONE
                    # show counters
                    print(str(counters[0]).center(10) + str(counters[1]).center(10))
                else:
                    # bouncing around
                    encoder_direction = ENCODER_NONE
            elif (
                current_switchVals[index * 2] == True and
                current_switchVals[(index * 2) + 1] == False and
                previous_switchVals[(index * 2) + 1] == True
            ):
                # 4/4 to 1/4 - leaving detent in UP direction
                encoder_direction = ENCODER_UP
                # 1/4 to 2/4 (ignore microstep)
                # 2/4 to 3/4 (ignore microstep)
            elif (
                current_switchVals[index * 2] == False and
                current_switchVals[(index * 2) + 1] == True and
                previous_switchVals[index * 2] == True
            ):
                # 4/4 to 3/4 - leaving detent in DOWN direction
                encoder_direction = ENCODER_DOWN
                # 3/4 to 2/4 (ignore microstep
                # 2/4 to 1/4 (ignore microstep)
    _ = time.sleep(ENCODER_SAMPLE_INTERVAL_MS)
#
# ---------- Start of main program ----------
#
if sys.argv[1:]:
    if ('-h' in sys.argv[1:]) or ('--help' in sys.argv[1:]):
        for a_string in PROG_HELP:
            print(a_string)
        print()
        sys.exit()
    elif ('-d' in sys.argv[1:]) or ('--debug' in sys.argv[1:]):
        DEBUG = True # override
    else:
        print('Unknown command line option')
        sys.exit()
try:
    # Initiallise hardware
    if DEBUG:
        pass
    else:
        GPIO.setmode(GPIO.BCM)
        tristate_all()
    #
    # Verify configuration
    if DEBUG:
        echo_config_data()
    #
    # Cycle the LEDs two different ways
    cycle_LEDs_electronic()
    cycle_LEDs_physical()
    #
    _ = os.system('clear')
    _ = input("Press enter to test switches (^C stops test)")
    _ = os.system('clear')
    # Test the switches
    test_switches()
    #
except KeyboardInterrupt:
    pass
try:
    _ = os.system('clear')
    _ = input("Press enter to test rotary encoders (^C stops test)")
    _ = os.system('clear')
    # Test the rotary encoders
    test_rotary_encoders()
    #
except KeyboardInterrupt:
    # Finalise hardware
    if DEBUG:
        pass
    else:
        tristate_all()
        GPIO.cleanup()
#
    print()
#
# ----------End of program ----------

