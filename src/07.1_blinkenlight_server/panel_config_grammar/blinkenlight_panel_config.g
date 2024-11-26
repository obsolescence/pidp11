
// blinkenlight_panel_config.g: ANTLR grammar for BlinkenBus server config
//
// Copyright (c) 2012-2016, Joerg Hoppe
// j_hoppe@t-online.de, www.retrocmp.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// JOERG HOPPE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//
// 23-Feb-2016  JH      control.bitlen added for dummy input controls without register wiring
// 09-Feb-2012  JH      created


grammar blinkenlight_panel_config;


options
{
    language=C;
}

// parser actions fill "blinkenlight_panels" structs of
// the config database tree
@header {
#include <string.h>

#include "config.h"
#include "namevaluelist.h"

// global panel config
extern blinkenlight_panel_list_t *blinkenlight_panel_list ;

}
/**/



/*------------------------------------------------------------------
 * LEXER RULES
 *------------------------------------------------------------------*/

// some reserved words. UPPERCASE, since case insensitive lexing is enabled
// in the program
 PANEL :
	'PANEL'    ;
 CONTROL :
	'CONTROL'    ;
NAME	:	 'NAME' ;
INFO	:	 'INFO' ;
RADIX	:	 'RADIX' ;

TYPE	:	 'TYPE' ;
TYPE_SWITCH
	:	 'SWITCH' {USER1=1;} ;	// USER1 = blinkenlight_control_type_enum code!
TYPE_LAMP
	:	 'LAMP' {USER1=2;} ;
TYPE_KNOB
	:	 'KNOB' {USER1=3;} ;
TYPE_POINTER
	:	 'POINTER' {USER1=4;} ;
TYPE_OTHERINPUT
	:	 'INPUT' {USER1=5;} ;
TYPE_OTHEROUTPUT
	:	 'OUTPUT' {USER1=6;} ;

ENCODING:	  'ENCODING' ;
ENCODING_BINARY
	:	  'BINARY' {USER1=1;} ;	// USER1 = blinkenlight_control_value_encoding_enum code!
ENCODING_BITPOSITION
	:	  'BITPOSITION' {USER1=2;};
DEFAULT :	'DEFAULT';
BITS_MIRRORED:	'BITS_MIRRORED';
FMAX	:	'FMAX';
BITLEN	:	'BITLEN' ;
BLINKENBUS_WIRING	:	 'BLINKENBUS_WIRING' ;
REGISTER_WIRING	:	 'REGISTER_WIRING' ;
VALUE_BIT_OFFSET
	:	 'VALUE_BIT_OFFSET' ;
LEVELS  :	 'LEVELS' ;
ACTIVE_HIGH
	:	 'ACTIVE_HIGH' {USER1=0;} ; // USER1: value for blinkenbus_levels_active_low
ACTIVE_LOW
	:	 'ACTIVE_LOW'  {USER1=1;};
BOARD	:	 'BOARD' ;
REGISTER
	:		'REGISTER' ;
// register names are fixed tokens.
// User1 = register space (0=output, 1 = input)
// User2= register address on board
// needs another implementation, if more board types are build
IN0 	:	'IN0' {USER1=1;USER2=0;} ;
IN1 	:	'IN1' {USER1=1;USER2=1;} ;
IN2 	:	'IN2' {USER1=1;USER2=2;} ;
IN3 	:	'IN3' {USER1=1;USER2=3;} ;
IN4 	:	'IN4' {USER1=1;USER2=4;} ;
IN5 	:	'IN5' {USER1=1;USER2=5;} ;
IN6 	:	'IN6' {USER1=1;USER2=6;} ;
IN7 	:	'IN7' {USER1=1;USER2=7;} ;
IN8 	:	'IN8' {USER1=1;USER2=8;} ;
IN9 	:	'IN9' {USER1=1;USER2=9;} ;
IN10 	:	'IN10' {USER1=1;USER2=10;} ;
IN11 	:	'IN11' {USER1=1;USER2=11;} ;
IN12 	:	'IN12' {USER1=1;USER2=12;} ;
IN13 	:	'IN13' {USER1=1;USER2=13;} ;
IN14 	:	'IN14' {USER1=1;USER2=14;} ; // 15 is board control register
OUT0 	:	'OUT0' {USER1=0;USER2=0;} ;
OUT1 	:	'OUT1' {USER1=0;USER2=1;} ;
OUT2 	:	'OUT2' {USER1=0;USER2=2;} ;
OUT3 	:	'OUT3' {USER1=0;USER2=3;} ;
OUT4 	:	'OUT4' {USER1=0;USER2=4;} ;
OUT5 	:	'OUT5' {USER1=0;USER2=5;} ;
OUT6 	:	'OUT6' {USER1=0;USER2=6;} ;
OUT7 	:	'OUT7' {USER1=0;USER2=7;} ;
OUT8 	:	'OUT8' {USER1=0;USER2=8;} ;
OUT9 	:	'OUT9' {USER1=0;USER2=9;} ;
OUT10 	:	'OUT10' {USER1=0;USER2=10;} ;
OUT11 	:	'OUT11' {USER1=0;USER2=11;} ;
OUT12 	:	'OUT12' {USER1=0;USER2=12;} ;
OUT13 	:	'OUT13' {USER1=0;USER2=13;} ;
OUT14 	:	'OUT14' {USER1=0;USER2=14;} ;

BITS	:	 'BITS' ;



 DIGITS: '1'..'9' '0'..'9'*;
OCTAL_DIGITS: '0' '0'..'7'+;
HEX_DIGITS: '0X' ('0'..'9' | 'a'..'f' | 'A'..'F')+;


IDENTIFIER  :	('a'..'z'|'A'..'Z'|'_') ('a'..'z'|'A'..'Z'|'0'..'9'|'_')*
    ;


COMMENT
    :   '//' ~('\n'|'\r')* '\r'? '\n' {$channel=HIDDEN;}
    |   '/*' ( options {greedy=false;} : . )* '*/' {$channel=HIDDEN;}
    ;

WS  :   ( ' '
        | '\t'
        | '\r'
        | '\n'
        ) {$channel=HIDDEN;}
    ;

STRING
    :  '"' ( ESC_SEQ | ~('\\'|'"') )* '"'
    ;

CHAR:  '\'' ( ESC_SEQ | ~('\''|'\\') ) '\''
    ;

fragment
HEX_DIGIT : ('0'..'9'|'a'..'f'|'A'..'F') ;

fragment
ESC_SEQ
    :   '\\' ('b'|'t'|'n'|'f'|'r'|'\"'|'\''|'\\')
    |   UNICODE_ESC
    |   OCTAL_ESC
    ;

fragment
OCTAL_ESC
    :   '\\' ('0'..'3') ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7') ('0'..'7')
    |   '\\' ('0'..'7')
    ;

fragment
UNICODE_ESC
    :   '\\' 'u' HEX_DIGIT HEX_DIGIT HEX_DIGIT HEX_DIGIT
    ;





/*------------------------------------------------------------------
 * PARSER RULES
 *------------------------------------------------------------------*/

definition_list
	:	definition (';' definition)* (';')?  ;

definition
	:	 variable_definition | panel_definition ;

// set a string or int named value
variable_definition
	:	 id = IDENTIFIER '=' (string=string_expr
	{namevaluelist_set_string_value($id.text->chars, $string.text->chars) ;}
	| intval=unsigned_int_expr
		{namevaluelist_set_int_value($id.text->chars, $intval.value) ;}

	) 		;


panel_definition
	:	PANEL '='
	{ parser_cur_panel = blinkenlight_add_panel(blinkenlight_panel_list) ;}
	'{' panel_parameter (',' panel_parameter)* '}'
	;

panel_parameter
	:
	NAME '=' panel_name = string_expr
	{ char *s = $panel_name.value ;
  	  strncpy(parser_cur_panel->name, s, sizeof(parser_cur_panel->name));}
	| INFO '=' panel_info = string_expr
	{ char *s = $panel_info.value ;
  	  strncpy(parser_cur_panel->info, s, sizeof(parser_cur_panel->info));}
  	| RADIX '=' radix = unsigned_int_expr {parser_cur_panel->default_radix = $radix.value;}
	|	control_definition ;

control_definition
	: 	CONTROL '='
	{ parser_cur_control = blinkenlight_add_control(blinkenlight_panel_list, parser_cur_panel) ;}
	'{' control_parameter (',' control_parameter)* '}'
	;

control_parameter
	: NAME '=' control_name = string_expr
	{ strcpy(parser_cur_control->name, $control_name.value);}
//	{ printf("NAME=\%s\n", $control_name.text->chars) ;}
	| TYPE '=' control_type = (TYPE_SWITCH | TYPE_LAMP | TYPE_KNOB | TYPE_POINTER | TYPE_OTHERINPUT | TYPE_OTHEROUTPUT)
	{ parser_cur_control->type = $control_type->user1 ; }
//	{ printf("TYPE=\%d\n", $control_type->user1) ;}
	| ENCODING '=' control_encoding = (ENCODING_BINARY | ENCODING_BITPOSITION)
	{ parser_cur_control->encoding = $control_encoding->user1 ; }
	| DEFAULT '=' default_value=unsigned_int_expr{parser_cur_control->value_default = $default_value.value;}
	| BITLEN '=' bitlen = unsigned_int_expr	{parser_cur_control->value_bitlen = $bitlen.value;}
	| (BITS_MIRRORED {parser_cur_control->mirrored_bit_order = 1;})
	| FMAX '=' fmax = unsigned_int_expr	{parser_cur_control->fmax = $fmax.value;}
//	{ printf("ENCODING=\%d\n", $control_encoding->user1) ;}
  	| RADIX '=' radix = unsigned_int_expr {parser_cur_control->radix = $radix.value;}
	| BLINKENBUS_WIRING '=' register_wiring_list
	;


register_wiring_list
	:	 '{' register_wiring (',' register_wiring)*  '}' ;

register_wiring
	: 	REGISTER_WIRING '='
	{ parser_cur_register_wiring = blinkenlight_add_register_wiring(parser_cur_control) ;}
	'{' register_wiring_parameter (',' register_wiring_parameter)*  '}' ;

register_wiring_parameter
	:
	VALUE_BIT_OFFSET '=' unsigned_int_expr
	{parser_cur_register_wiring->control_value_bit_offset = $unsigned_int_expr.value;}
	| BOARD '=' unsigned_int_expr
	{parser_cur_register_wiring->blinkenbus_board_address = $unsigned_int_expr.value;}
	| REGISTER '=' in_out=(IN0 | IN1 | IN2 | IN3 | IN4 | IN5 | IN6 | IN7
      | IN8 | IN9 | IN10 | IN11 | IN12 | IN13 | IN14
      | OUT0 | OUT1 | OUT2 | OUT3 | OUT4 | OUT5 | OUT6 | OUT7
      | OUT8 | OUT9 | OUT10 | OUT11 | OUT12 | OUT13 | OUT14 )
	{parser_cur_register_wiring->board_register_space = in_out->user1;
	parser_cur_register_wiring->board_register_address = in_out->user2;}
	| BITS '=' lsb = unsigned_int_expr
	{parser_cur_register_wiring->blinkenbus_lsb = parser_cur_register_wiring->blinkenbus_msb = $lsb.value;}
	('..' msb=unsigned_int_expr
	{parser_cur_register_wiring->blinkenbus_msb = $msb.value;}
	)?
	| LEVELS '=' polarity=(ACTIVE_HIGH | ACTIVE_LOW)
	{parser_cur_register_wiring->blinkenbus_levels_active_low = polarity->user1; }
	 ;

// a string literal, or a variable
string_expr returns [char  *value] 	 :
	string = STRING {$value = parser_strip_quotes($string.text->chars);}
	| id = IDENTIFIER {$value = namevaluelist_get_string_value($id.text->chars);}
	;

// a numeric literal, or a variable
unsigned_int_expr returns [long value]
	: '0'{$value = 0;}
	| DIGITS
	{$value = strtol($DIGITS.text->chars,NULL,10) ;}
	| OCTAL_DIGITS
	{$value = strtol(($OCTAL_DIGITS.text->chars)+1,NULL,8) ;} // "+1": strip prefix "0"
	| HEX_DIGITS
	{$value = strtol(($HEX_DIGITS.text->chars)+2,NULL,16) ;}  // "+2": strip prefix "0x"
	| id = IDENTIFIER {$value = namevaluelist_get_int_value($id.text->chars);}
	;
