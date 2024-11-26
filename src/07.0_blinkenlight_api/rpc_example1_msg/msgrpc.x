/*
* msg.x: Remote message printing protocol
*/


/* only single aparmaeter and single return value allowed.
 * so use struct for mutliple arguments
 */
struct my2params {
	string	message<255> ;
	int		counter ;
} ;

/* assigned program numbers
 *	http://www.iana.org/assignments/rpc-program-numbers/rpc-program-numbers.xml
 */
program MESSAGEPROG {
version MESSAGEVERS {
int PRINTMESSAGE(my2params) = 1;
} = 1;
} = 99;