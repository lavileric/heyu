/*----------------------------------------------------------------------------+
 |                                                                            |
 |            HEYU Support for External Power Line Sync                       |
 |             Copyright 2005,2006 Charles W. Sullivan                        |
 |                                                                            |
 |                                                                            |
 | As used herein, HEYU is a trademark of Daniel B. Suthers.                  | 
 | X10, CM11A, and ActiveHome are trademarks of X-10 (USA) Inc.               |
 | The author is not affiliated with either entity.                           |
 |                                                                            |
 | Charles W. Sullivan                                                        |
 | Co-author and Maintainer                                                   |
 | Greensboro, North Carolina                                                 |
 | Email ID: cwsulliv01                                                       |
 | Email domain: -at- heyu -dot- org                                          |
 |                                                                            |
 +----------------------------------------------------------------------------*/

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
#include <stdlib.h>
#include <ctype.h>
#if 0
#include "x10.h"
#include "process.h"
#endif

#ifdef        SCO
#define _SVID3 /* required for correct termio handling */
#undef  _IBCS2 /* conflicts with SVID3  */
#endif

#include <time.h>
#include <unistd.h>
#include <signal.h>

#ifdef LINUX
#include <asm/ioctls.h>
#   ifdef OLDLINUX
#include <linux/serial_reg.h>
#   endif
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <syslog.h>
#else
#    if (defined(POSIX) || defined(FREEBSD) || defined(OPENBSD))
#include <sys/termios.h>
#    else
#         ifdef SCO
#include <sys/termio.h>
#         else
#              ifdef DARWIN
#include <termios.h>
#              else
#include <termio.h>
#              endif
#         endif
#    endif
#endif

#if (defined(OSF) || defined(DARWIN) || defined(NETBSD))
#include <sys/ioctl.h>
#endif

#ifdef HASSELECT
#include <sys/time.h>
#include <sys/types.h>
#endif

#include "x10.h"
#include "process.h"

#ifdef pid_t
#define PID_T pid_t
#else
#define PID_T long
#endif

extern int verbose;
extern int sptty;
extern int tty;
extern int ttylock(), munlock();
extern PID_T lockpid( char * );
extern int i_am_relay;

extern CONFIG config;
extern CONFIG *configp;

static volatile unsigned long trigger_loopcount;
static volatile unsigned long countdown;
static volatile int dummy = 1;

/*----------------------------------------------------------------------------+
 | Millisecond timer using timing loop.                                       |
 +----------------------------------------------------------------------------*/
static void msec_timer ( unsigned long loopcount )
{
   countdown = loopcount;	
   while (dummy && countdown--);
   return;
}


/*---------------------------------------------------------------+
 | Optional wait for external sync trigger for dims/brights.     |
 | A one millisec timing loop must be configured and additional  |
 | hardware is required as follows:                              |
 |                                                               |
 | The output of a 4 to 8 VAC (RMS) transformer connected from   |
 | the Carrier Detect to Signal Ground pins of the serial port   |
 | (DB9 pins 1 and 5 respectively) allows Heyu to distinguish    |
 | between the positive and negative half-cycles of the AC power |
 | line and trigger the command to begin exclusively on either   |
 | the rising or falling zero crossing, depending on the         |
 | relative phase of the voltages on the CD pin and the AC line. |
 +---------------------------------------------------------------*/
int wait_external_trigger ( int mode )
{
   int j, status, retcode, jmax = 20;

   if ( mode == NO_SYNC )
      return 0;

   if ( configp->timer_loopcount == 0 ) {
      fprintf(stderr, "No timing loop has been configured - trigger ignored.\n");
      return 1;
   }

   /* Timing loopcount for 1 millisecond */
   trigger_loopcount = configp->timer_loopcount / 1000L;

   if ( mode == RISE_SYNC ) {
      /* Rising zero-crossing triggering */
      /* Wait until the Carrier Detect line becomes active */
      for ( j = 0; j < jmax; j++ ) {
         retcode = ioctl(tty, TIOCMGET, &status);
         if ( status & TIOCM_CD )
            break;
         msec_timer(trigger_loopcount);
      }
      if ( j >= jmax ) {
         fprintf(stderr, "No trigger detected.\n");
         return 1;
      }

      /* Now wait until it just becomes inactive */
      for ( j = 0; j < jmax; j++ ) {
         retcode = ioctl(tty, TIOCMGET, &status);
         if ( !(status & TIOCM_CD) )
            break;
         msec_timer(trigger_loopcount);
      }
   }
   else {
      /* Falling zero-crossing triggering */
      /* Wait until the Carrier Detect line becomes inactive */
      for ( j = 0; j < jmax; j++ ) {
         retcode = ioctl(tty, TIOCMGET, &status);
         if ( !(status & TIOCM_CD) )
            break;
         msec_timer(trigger_loopcount);
      }
      /* Now wait until it just becomes active */
      for ( j = 0; j < jmax; j++ ) {
         retcode = ioctl(tty, TIOCMGET, &status);
         if ( status & TIOCM_CD )
            break;
         msec_timer(trigger_loopcount);
      }
      if ( j >= jmax ) {
         fprintf(stderr, "No trigger detected.\n");
         return 1;
      }
   }

   /* Wait an additional few milliconds to make   */
   /* sure we're out of the zero crossing region  */
   msec_timer(2 * trigger_loopcount);

   return 0;
}

