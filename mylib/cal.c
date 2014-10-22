/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kim Letkeman.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* 1999-02-01	Jean-Francois Bignolles: added option '-m' to display
 * 		monday as the first day of the week.
 */

/* This defines _LINUX_C_LIB_VERSION_MAJOR, dunno about gnulibc.  We
   don't want it to read /usr/i586-unknown-linux/include/_G_config.h
   so we specify fill path.  Were we got /usr/i586-unknown-linux from?
   Dunno. */

/*

  moved all the main stuff out to functions
  and created necessary functions for Perl to get hold of the date 
  info

*/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "/usr/include/_G_config.h"

#include <sys/types.h>

#include <ctype.h>
//#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>

/* Test changes to deal with gnulibc, Linux libc 5. */
/* #if defined(__linux__) && _LINUX_C_LIB_VERSION_MAJOR > 4 */
#if _LINUX_C_LIB_VERSION_MAJOR - 0 > 4 || __GNU_LIBRARY__ - 0 >= 5
# define LANGINFO 1
#else
# define LANGINFO 0
#endif

#if LANGINFO
# include <langinfo.h>
#else
# include <localeinfo.h>
#endif

#define	THURSDAY		4		/* for reformation */
#define	SATURDAY 		6		/* 1 Jan 1 was a Saturday */

#define	FIRST_MISSING_DAY 	639787		/* 3 Sep 1752 */
#define	NUMBER_MISSING_DAYS 	11		/* 11 day correction */

#define	MAXDAYS			43		/* max slots in a month array */
#define	SPACE			-1		/* used in day array */

static int days_in_month[2][13] = {
	{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

int sep1752[MAXDAYS] = {
	SPACE,	SPACE,	1,	2,	14,	15,	16,
	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE
}, j_sep1752[MAXDAYS] = {
	SPACE,	SPACE,	245,	246,	258,	259,	260,
	261,	262,	263,	264,	265,	266,	267,
	268,	269,	270,	271,	272,	273,	274,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE
}, empty[MAXDAYS] = {
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,	SPACE,
	SPACE
};

char day_headings[]   = " S  M Tu  W Th  F  S ";
/* week1stday = 1  =>   " M Tu  W Th  F  S  S " */
char j_day_headings[] = "  S   M  Tu   W  Th   F   S ";
/* week1stday = 1  =>   "  M  Tu   W  Th   F   S   S " */
const char *full_month[12];

/* leap year -- account for gregorian reformation in 1752 */
#define	leap_year(yr) \
	((yr) <= 1752 ? !((yr) % 4) : \
	(!((yr) % 4) && ((yr) % 100)) || !((yr) % 400))

/* number of centuries since 1700, not inclusive */
#define	centuries_since_1700(yr) \
	((yr) > 1700 ? (yr) / 100 - 17 : 0)

/* number of centuries since 1700 whose modulo of 400 is 0 */
#define	quad_centuries_since_1700(yr) \
	((yr) > 1600 ? ((yr) - 1600) / 400 : 0)

/* number of leap years between year 1 and this year, not inclusive */
#define	leap_years_since_year_1(yr) \
	((yr) / 4 - centuries_since_1700(yr) + quad_centuries_since_1700(yr))

/* 0 => sunday (default), 1 => monday */
int week1stday;
int julian;
int yflag;

SV*	ascii_day __P((char *, int));
void	day_array __P((int, int, int *));
int	day_in_week __P((int, int, int));
int	day_in_year __P((int, int, int));
//void	j_yearly __P((int));
AV*	j_yearly __P((int));
//void	monthly __P((int, int));
AV*	monthly __P((int, int));
void	usage __P((void));
//void	yearly __P((int));
AV*	yearly __P((int));
//void    headers_init(void);
AV*    headers_init(void);


#define	DAY_LEN		3		/* 3 spaces per day */
#define	J_DAY_LEN	4		/* 4 spaces per day */
#define	WEEK_LEN	21		/* 7 days * 3 characters */
#define	J_WEEK_LEN	28		/* 7 days * 4 characters */
#define	HEAD_SEP	2		/* spaces between day headings */
#define	J_HEAD_SEP	2

int
onLoad(int p_monday, int p_julian, int p_year)
{
	struct tm *local_time;
	time_t now;
	int ch, month, year;
  	int i;

	setlocale(LC_ALL,"");
	week1stday = p_monday;
	julian = p_julian;
	yflag = p_year;

  
	for (i = 0; i < 12; i++) {
#if defined(__linux__) && (_LINUX_C_LIB_VERSION_MAJOR > 4 || __GNU_LIBRARY__ > 1)
	     full_month[i] = nl_langinfo(MON_1+i);
#else
	     full_month[i] = _time_info->full_month[i];
#endif
	}

        return 1;

}


SV*
doCal(int p_month, int p_year)
{
	struct tm *local_time;
	time_t now;
	int ch, month, year;

	month = year = 0;

        if ( p_month != 0 )
	  month = p_month;
	if ( p_year != 0 )
	  year = p_year;

        if ( p_month == 0 && p_year == 0 )
	{
		(void)time(&now);
		local_time = localtime(&now);
		year = local_time->tm_year + 1900;
		if (!yflag)
			month = local_time->tm_mon + 1;
	}

	if (month)
		return newRV_noinc((SV*) monthly(month, year));
	else if (julian)
		return newRV_noinc((SV*) j_yearly(year));
	else
		return newRV_noinc((SV*) yearly(year));

}


AV* headers_init(void)
{
  int i, wd;
  AV* array = newAV();


#if defined(__linux__) && (_LINUX_C_LIB_VERSION_MAJOR > 4 || __GNU_LIBRARY__ > 1)
# define weekday(wd)	nl_langinfo(ABDAY_1+wd)
#else
# define weekday(wd)	_time_info->abbrev_wkday[wd]
#endif
  
  for(i = 0 ; i < 7 ; i++ ) {
     wd = (i + week1stday) % 7;
     av_push(array, newSVpv(weekday(wd),julian == 1 ? 3 : 2 ));
  }

#undef weekday

  return array;
}

AV*
monthly(month, year)
	int month, year;
{
	int col, row, len, days[MAXDAYS];
	char *p, lineout[30];
	AV* warray;
	AV* marray = newAV();
	AV* mhdr = newAV();

	day_array(month, year, days);

        av_push(mhdr, newSVpvf(full_month[month - 1]));
        av_push(mhdr, newSViv(year));
	av_push(marray, newRV_noinc((SV*) mhdr));
	av_push(marray, newRV_noinc((SV*) headers_init()));

	for (row = 0; row < 6; row++) {
		if (days[row * 7 + 0] == SPACE && row > 1 )
		   break;

	        warray = newAV();
		for (col = 0, p = lineout; col < 7; col++) {
			av_push(warray, ascii_day(p, days[row * 7 + col]));
		}
	        av_push(marray, newRV_noinc((SV*) warray));
	}

  return marray;
}

AV*
j_yearly(year)
	int year;
{
	int col, *dp, i, month, row, which_cal;
	int days[12][MAXDAYS];
	char *p, lineout[80];
	AV* yarray = newAV();
	AV* yhdr = newAV();

	av_push(yhdr, newSViv(year));
	av_push(yarray, newRV_noinc((SV*) yhdr));

	for (i = 0; i < 12; i++)
		av_push(yarray, newRV_noinc((SV*) monthly(i + 1, year)));

	return yarray;
}

AV*
yearly(year)
	int year;
{
	int col, *dp, i, month, row, which_cal;
	int days[12][MAXDAYS];
	char *p, lineout[80];
	AV* yarray = newAV();
	AV* yhdr = newAV();

	av_push(yhdr, newSViv(year));
	av_push(yarray, newRV_noinc((SV*) yhdr));
	for (i = 0; i < 12; i++)
		av_push(yarray, newRV_noinc((SV*) monthly(i + 1, year)));
	return yarray;
}

/*
 * day_array --
 *	Fill in an array of 42 integers with a calendar.  Assume for a moment
 *	that you took the (maximum) 6 rows in a calendar and stretched them
 *	out end to end.  You would have 42 numbers or spaces.  This routine
 *	builds that array for any month from Jan. 1 through Dec. 9999.
 */
void
day_array(month, year, days)
	int month, year;
	int *days;
{
	int day, dw, dm;
	int *d_sep1752;

	if (month == 9 && year == 1752) {
		d_sep1752 = julian ? j_sep1752 : sep1752;
		memcpy(days, d_sep1752 + week1stday, MAXDAYS * sizeof(int));
		return;
	}
	memcpy(days, empty, MAXDAYS * sizeof(int));
	dm = days_in_month[leap_year(year)][month];
	dw = (day_in_week(1, month, year) - week1stday + 7) % 7;
	day = julian ? day_in_year(1, month, year) : 1;
	while (dm--)
		days[dw++] = day++;
}

/*
 * day_in_year --
 *	return the 1 based day number within the year
 */
int
day_in_year(day, month, year)
	int day, month, year;
{
	int i, leap;

	leap = leap_year(year);
	for (i = 1; i < month; i++)
		day += days_in_month[leap][i];
	return (day);
}

/*
 * day_in_week
 *	return the 0 based day number for any date from 1 Jan. 1 to
 *	31 Dec. 9999.  Assumes the Gregorian reformation eliminates
 *	3 Sep. 1752 through 13 Sep. 1752.  Returns Thursday for all
 *	missing days.
 */
int
day_in_week(day, month, year)
	int day, month, year;
{
	long temp;

	temp = (long)(year - 1) * 365 + leap_years_since_year_1(year - 1)
	    + day_in_year(day, month, year);
	if (temp < FIRST_MISSING_DAY)
		return ((temp - 1 + SATURDAY) % 7);
	if (temp >= (FIRST_MISSING_DAY + NUMBER_MISSING_DAYS))
		return (((temp - 1 + SATURDAY) - NUMBER_MISSING_DAYS) % 7);
	return (THURSDAY);
}

SV*
ascii_day(p, day)
	char *p;
	int day;
{
	int display, val;
	static char *aday[] = {
		"",
		" 1", " 2", " 3", " 4", " 5", " 6", " 7",
		" 8", " 9", "10", "11", "12", "13", "14",
		"15", "16", "17", "18", "19", "20", "21",
		"22", "23", "24", "25", "26", "27", "28",
		"29", "30", "31",
	};

	if (day == SPACE) {
		return newSVpvf("");
	}
	if (julian) {
	        return newSViv(day);
	} else {
	        return newSViv(atoi(aday[day]));
	}
}
