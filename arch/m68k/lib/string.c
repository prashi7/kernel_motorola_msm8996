/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#define __IN_STRING_C

#include <linux/module.h>
#include <linux/string.h>

char *strcpy(char *dest, const char *src)
{
	return __kernel_strcpy(dest, src);
}
EXPORT_SYMBOL(strcpy);

char *strcat(char *dest, const char *src)
{
	return __kernel_strcpy(dest + __kernel_strlen(dest), src);
}
EXPORT_SYMBOL(strcat);

void *memcpy(void *to, const void *from, size_t n)
{
	void *xto = to;
	size_t temp, temp1;

	if (!n)
		return xto;
	if ((long)to & 1) {
		char *cto = to;
		const char *cfrom = from;
		*cto++ = *cfrom++;
		to = cto;
		from = cfrom;
		n--;
	}
	if (n > 2 && (long)to & 2) {
		short *sto = to;
		const short *sfrom = from;
		*sto++ = *sfrom++;
		to = sto;
		from = sfrom;
		n -= 2;
	}
	temp = n >> 2;
	if (temp) {
		long *lto = to;
		const long *lfrom = from;

		asm volatile (
			"	movel %2,%3\n"
			"	andw  #7,%3\n"
			"	lsrl  #3,%2\n"
			"	negw  %3\n"
			"	jmp   %%pc@(1f,%3:w:2)\n"
			"4:	movel %0@+,%1@+\n"
			"	movel %0@+,%1@+\n"
			"	movel %0@+,%1@+\n"
			"	movel %0@+,%1@+\n"
			"	movel %0@+,%1@+\n"
			"	movel %0@+,%1@+\n"
			"	movel %0@+,%1@+\n"
			"	movel %0@+,%1@+\n"
			"1:	dbra  %2,4b\n"
			"	clrw  %2\n"
			"	subql #1,%2\n"
			"	jpl   4b"
			: "=a" (lfrom), "=a" (lto), "=d" (temp), "=&d" (temp1)
			: "0" (lfrom), "1" (lto), "2" (temp));
		to = lto;
		from = lfrom;
	}
	if (n & 2) {
		short *sto = to;
		const short *sfrom = from;
		*sto++ = *sfrom++;
		to = sto;
		from = sfrom;
	}
	if (n & 1) {
		char *cto = to;
		const char *cfrom = from;
		*cto = *cfrom;
	}
	return xto;
}
EXPORT_SYMBOL(memcpy);
