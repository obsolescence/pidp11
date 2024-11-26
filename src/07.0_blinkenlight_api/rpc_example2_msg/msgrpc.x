/*
* msg.x: Remote message printing protocol
*/



/* assigned program numbers
 *	http://www.iana.org/assignments/rpc-program-numbers/rpc-program-numbers.xml
 */
program MESSAGEPROG {
version MESSAGEVERS {
/*  new style (option "-N"): multiple arguemtns allowed */
  int PRINTMESSAGE(string<255> /*message*/, int /*counter*/) = 1;
} = 1;
} = 99;