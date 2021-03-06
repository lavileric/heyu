
/*
 * Copyright 1986 by Larry Campbell, 73 Concord Street, Maynard MA 01754 USA
 * (maynard!campbell).
 *
 * John Chmielewski (tesla!jlc until 9/1/86, then rogue!jlc) assisted
 * by doing the System V port and adding some nice features.  Thanks!
 */

 /* Changes for the CM11A made by Daniel Suthers, dbs@tanj.com */

/*
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <time.h>
#include "x10.h"
#include <unistd.h>
#ifdef SYSV
#include <sys/types.h>
#endif

#include <sys/param.h>
#ifdef BEFORE
#include <sys/filsys.h>
#endif
#include "x10.h"


extern void error();
extern int invalidate_for_cm10a();

extern int
 Idays, Ijday, Ihours, Iminutes, Iseconds;

/* ARGSUSED */

int c_date( int argc, char *argv[] )
{
    int  today;
    int  expire;
    struct tm *tp;
    char RCSID[]= "@(#) $Id: date.c,v 1.8 2000/01/02 23:00:32 dbs Exp dbs $\n";
    extern int usage(), get_status(), dowX2U();
    extern struct tm *cm11a_to_legal();
    time_t now;

    if ( invalidate_for_cm10a() != 0 )
       return 1;

    display(RCSID);
    if (argc != 2)
	usage(E_2MANY);
    time(&now);
    tp = localtime(&now);

    if( get_status() < 1 )
        error(" No reponse from CM11A.  Program exiting");

    /* Translate CM11a clock data to Legal Time via */
    /* info in X10 Record File (if it exists).      */
    tp = cm11a_to_legal( &Idays, &Ijday, &Ihours,
	      &Iminutes, &Iseconds, &expire );
    expire = expire;  /* Keep compiler from complaining */
    
    today = dowX2U(Idays);
#ifndef POSIX
    while (tp->tm_wday % 7 != today)
	tp->tm_wday++, tp->tm_mday++;
#endif

#ifdef VENIX
    (void) printf("%2d%02d%02d%02d%02d\n",
	     tp->tm_year % 100, tp->tm_mon + 1, tp->tm_mday, Ihours, Iminutes);
#else
    (void) printf("%02d%02d%02d%02d%02d\n",
	     tp->tm_mon + 1, tp->tm_mday, Ihours, Iminutes, tp->tm_year % 100);
#endif
    return(0);
}
