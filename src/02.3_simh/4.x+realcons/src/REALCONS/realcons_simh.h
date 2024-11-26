/* realcons_simh.h: Interface to SimH, "realcons" device.

   Copyright (c) 2012-2016, Joerg Hoppe
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

   18-Jun-2016  JH      added param "bootimage" (for PDP-15)
   25-Mar-2012  JH      created
*/

#ifndef REALCONS_SIMH_H_
#define REALCONS_SIMH_H_

t_stat sim_set_realcons(int32 flag, CONST char *cptr);
t_stat realcons_simh_set_hostname(int32 flg, CONST char *cptr);
t_stat realcons_simh_set_panelname(int32 flg, CONST char *cptr);
t_stat realcons_simh_set_service_interval(int32 flg, CONST char *cptr);
t_stat realcons_simh_set_connect(int32 flg, CONST char *cptr);
t_stat realcons_simh_set_boot_image(int32 flg, CONST char *cptr);
t_stat realcons_simh_test(int32 flg, CONST char *cptr);
t_stat realcons_simh_set_debug(int32 flg, CONST char *cptr);

t_stat sim_show_realcons(FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr);
t_stat realcons_simh_show_hostname(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag,
    CONST char *cptr);
t_stat realcons_simh_show_panelname(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag,
    CONST char *cptr);
t_stat realcons_simh_show_service_interval(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag,
    CONST char *cptr);
t_stat realcons_simh_show_connected(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag,
    CONST char *cptr);
t_stat realcons_simh_show_boot_image(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag,
    CONST char *cptr);
t_stat realcons_simh_show_debug(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr);
t_stat realcons_simh_show_cycles(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr) ;

t_stat realcons_simh_show_server(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr);

#endif /* REALCONS_SIMH_H_ */
