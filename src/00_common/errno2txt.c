 /* errno2txt.c:  error texts, for use in Linux kernel modules

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


   07-Jan-2012  JH      created
*/

#ifdef WIN32
#include <errno.h>
#else
#include <asm-generic/errno-base.h>
#endif

char *errno2txt(int errornumber)
{
	switch (errornumber)
	{
	case 0: return "SUCCESS";
	case EPERM:
		return "EPERM";
	case ENOENT:
		return "ENOENT";
	case ESRCH:
		return "ESRCH";
	case EINTR:
		return "EINTR";
	case EIO:
		return "EIO";
	case ENXIO:
		return "ENXIO";
	case E2BIG:
		return "E2BIG";
	case ENOEXEC:
		return "ENOEXEC";
	case EBADF:
		return "EBADF";
	case ECHILD:
		return "ECHILD";
	case EAGAIN:
		return "EAGAIN";
	case ENOMEM:
		return "ENOMEM";
	case EACCES:
		return "EACCES";
	case EFAULT:
		return "EFAULT";
//	case ENOTBLK:
//		return "ENOTBLK";
	case EBUSY:
		return "EBUSY";
	case EEXIST:
		return "EEXIST";
	case EXDEV:
		return "EXDEV";
	case ENODEV:
		return "ENODEV";
	case ENOTDIR:
		return "ENOTDIR";
	case EISDIR:
		return "EISDIR";
	case EINVAL:
		return "EINVAL";
	case ENFILE:
		return "ENFILE";
	case EMFILE:
		return "EMFILE";
	case ENOTTY:
		return "ENOTTY";
//	case ETXTBSY:
//		return "ETXTBSY";
	case EFBIG:
		return "EFBIG";
	case ENOSPC:
		return "ENOSPC";
	case ESPIPE:
		return "ESPIPE";
	case EROFS:
		return "EROFS";
	case EMLINK:
		return "EMLINK";
	case EPIPE:
		return "EPIPE";
	case EDOM:
		return "EDOM";
	case ERANGE:
		return "ERANGE";

// /usr/include/asm-generic/errno.h

// #include <asm-generic/errno-base.h>
/*
	case EDEADLK:
		return "EDEADLK";
	case ENAMETOOLONG:
		return "ENAMETOOLONG";
	case ENOLCK:
		return "ENOLCK";
	case ENOSYS:
		return "ENOSYS";
	case ENOTEMPTY:
		return "ENOTEMPTY";
	case ELOOP:
		return "ELOOP,EWOULDBLOCK";
		//case EWOULDBLOCK : return"";
	case ENOMSG:
		return "ENOMSG";
	case EIDRM:
		return "EIDRM";
	case ECHRNG:
		return "ECHRNG";
	case EL2NSYNC:
		return "EL2NSYNC";
	case EL3HLT:
		return "EL3HLT";
	case EL3RST:
		return "EL3RST";
	case ELNRNG:
		return "ELNRNG";
	case EUNATCH:
		return "EUNATCH";
	case ENOCSI:
		return "ENOCSI";
	case EL2HLT:
		return "EL2HLT";
	case EBADE:
		return "EBADE";
	case EBADR:
		return "EBADR";
	case EXFULL:
		return "EXFULL";
	case ENOANO:
		return "ENOANO";
	case EBADRQC:
		return "EBADRQC";
	case EBADSLT:
		return "EBADSLT";
	case ENOSTR:
		return "ENOSTR";
	case ENODATA:
		return "ENODATA";
	case ETIME:
		return "ETIME";
	case ENOSR:
		return "ENOSR";
	case ENONET:
		return "ENONET";
	case ENOPKG:
		return "ENOPKG";
	case EREMOTE:
		return "EREMOTE";
	case ENOLINK:
		return "ENOLINK";
	case EADV:
		return "EADV";
	case ESRMNT:
		return "ESRMNT";
	case ECOMM:
		return "ECOMM";
	case EPROTO:
		return "EPROTO";
	case EMULTIHOP:
		return "EMULTIHOP";
	case EDOTDOT:
		return "EDOTDOT";
	case EBADMSG:
		return "EBADMSG";
	case EOVERFLOW:
		return "EOVERFLOW";
	case ENOTUNIQ:
		return "ENOTUNIQ";
	case EBADFD:
		return "EBADFD";
	case EREMCHG:
		return "EREMCHG";
	case ELIBACC:
		return "ELIBACC";
	case ELIBBAD:
		return "ELIBBAD";
	case ELIBSCN:
		return "ELIBSCN";
	case ELIBMAX:
		return "ELIBMAX";
	case ELIBEXEC:
		return "ELIBEXEC";
	case EILSEQ:
		return "EILSEQ";
	case ERESTART:
		return "ERESTART";
	case ESTRPIPE:
		return "ESTRPIPE";
	case EUSERS:
		return "EUSERS";
	case ENOTSOCK:
		return "ENOTSOCK";
	case EDESTADDRREQ:
		return "EDESTADDRREQ";
	case EMSGSIZE:
		return "EMSGSIZE";
	case EPROTOTYPE:
		return "EPROTOTYPE";
	case ENOPROTOOPT:
		return "ENOPROTOOPT";
	case EPROTONOSUPPORT:
		return "EPROTONOSUPPORT";
	case ESOCKTNOSUPPORT:
		return "ESOCKTNOSUPPORT";
	case EOPNOTSUPP:
		return "EOPNOTSUPP";
	case EPFNOSUPPORT:
		return "EPFNOSUPPORT";
	case EAFNOSUPPORT:
		return "EAFNOSUPPORT";
	case EADDRINUSE:
		return "EADDRINUSE";
	case EADDRNOTAVAIL:
		return "EADDRNOTAVAIL";
	case ENETDOWN:
		return "ENETDOWN";
	case ENETUNREACH:
		return "ENETUNREACH";
	case ENETRESET:
		return "ENETRESET";
	case ECONNABORTED:
		return "ECONNABORTED";
	case ECONNRESET:
		return "ECONNRESET";
	case ENOBUFS:
		return "ENOBUFS";
	case EISCONN:
		return "EISCONN";
	case ENOTCONN:
		return "ENOTCONN";
	case ESHUTDOWN:
		return "ESHUTDOWN";
	case ETOOMANYREFS:
		return "ETOOMANYREFS";
	case ETIMEDOUT:
		return "ETIMEDOUT";
	case ECONNREFUSED:
		return "ECONNREFUSED";
	case EHOSTDOWN:
		return "EHOSTDOWN";
	case EHOSTUNREACH:
		return "EHOSTUNREACH";
	case EALREADY:
		return "EALREADY";
	case EINPROGRESS:
		return "EINPROGRESS";
	case ESTALE:
		return "ESTALE";
	case EUCLEAN:
		return "EUCLEAN";
	case ENOTNAM:
		return "ENOTNAM";
	case ENAVAIL:
		return "ENAVAIL";
	case EISNAM:
		return "EISNAM";
	case EREMOTEIO:
		return "EREMOTEIO";
	case EDQUOT:
		return "EDQUOT";
	case EMEDIUMTYPE:
		return "EMEDIUMTYPE";
	case ECANCELED:
		return "ECANCELED";
	case ENOKEY:
		return "ENOKEY";
	case EKEYEXPIRED:
		return "EKEYEXPIRED";
	case EKEYREVOKED:
		return "EKEYREVOKED";
	case EKEYREJECTED:
		return "EKEYREJECTED";
*/
	default:
		return "UNKNOWN";
	}
}
