/*
 * kbhit.c
 *
 * Implementation of kbhit() function ...
 * signals, wether a char has been typed on the TTY
 *  Created on: 04.01.2012
 *      Author:
 *      http://cboard.cprogramming.com/c-programming/63166-kbhit-linux.html
 *
 *      This here does NOT work:
 *      http://stackoverflow.com/questions/448944/c-non-blocking-keyboard-input
 */
#ifdef WIN32
#include <conio.h>

/*
 * Test for character being hit.
 * Return: 0: nothing hit
 * else: char
 */
int os_kbhit(void) {
	if (!_kbhit())
		return 0 ;
	else
		return _getch() ; // return the char
}
#else

#include <stdio.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef OLD
static struct termios orig_termios;

void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}
#endif
/*
 * Test for character being hit.
 * Return: 0: nothing hit
 * else: char
 */
int os_kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch == EOF)
	  return 0 ;
  else return ch ;
/*
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
   return 0;
*/
}

#endif