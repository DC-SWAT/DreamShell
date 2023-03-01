//
// strftime.c
//
// Date to string conversion
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 
// 1. Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.  
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.  
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission. 
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
// SUCH DAMAGE.
// 

#include <stdio.h>
#include <string.h>
#include <time.h>

//extern long _timezone;
static const char *_days[7] = {"Monday", "Tuesday", "Wednsday", "Thurday", "Friday", "Saturday", "Sunday"};
static const char *_days_abbrev[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static const char *_months[12] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
static const char *_months_abbrev[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

#define TM_YEAR_BASE   1900

#define DAYSPERLYEAR   366
#define DAYSPERNYEAR   365
#define DAYSPERWEEK    7

#define LEAPYEAR(year) (!((year) % 4) && (((year) % 100) || !((year) % 400)))

static char *_fmt(const char *format, const struct tm *t, char *pt, const char *ptlim);
static char *_conv(const int n, const char *format, char *pt, const char *ptlim);
static char *_add(const char *str, char *pt, const char *ptlim);

size_t strftime(char *s, size_t maxsize, const char *format, const struct tm *t)
{
  char *p;

  p = _fmt(((format == NULL) ? "%c" : format), t, s, s + maxsize);
  if (p == s + maxsize) return 0;
  *p = '\0';
  return p - s;
}

static char *_fmt(const char *format, const struct tm *t, char *pt, const char *ptlim)
{
  for ( ; *format; ++format) 
  {
    if (*format == '%') 
    {
      if (*format == 'E') 
        format++; // Alternate Era
      else if (*format == 'O')
        format++; // Alternate numeric symbols

      switch (*++format) 
      {
        case '\0':
          --format;
          break;

        case 'A':
          pt = _add((t->tm_wday < 0 || t->tm_wday > 6) ? "?" : _days[t->tm_wday], pt, ptlim);
          continue;

        case 'a':
          pt = _add((t->tm_wday < 0 || t->tm_wday > 6) ? "?" : _days_abbrev[t->tm_wday], pt, ptlim);
          continue;

        case 'B':
          pt = _add((t->tm_mon < 0 || t->tm_mon > 11) ? "?" : _months[t->tm_mon], pt, ptlim);
          continue;

        case 'b':
        case 'h':
          pt = _add((t->tm_mon < 0 || t->tm_mon > 11) ? "?" : _months_abbrev[t->tm_mon], pt, ptlim);
          continue;

        case 'C':
          pt = _conv((t->tm_year + TM_YEAR_BASE) / 100, "%02d", pt, ptlim);
          continue;

        case 'c':
          pt = _fmt("%a %b %e %H:%M:%S %Y", t, pt, ptlim);
          continue;

        case 'D':
          pt = _fmt("%m/%d/%y", t, pt, ptlim);
          continue;

        case 'd':
          pt = _conv(t->tm_mday, "%02d", pt, ptlim);
          continue;

        case 'e':
          pt = _conv(t->tm_mday, "%2d", pt, ptlim);
          continue;

        case 'F':
          pt = _fmt("%Y-%m-%d", t, pt, ptlim);
          continue;

        case 'H':
          pt = _conv(t->tm_hour, "%02d", pt, ptlim);
          continue;

        case 'I':
          pt = _conv((t->tm_hour % 12) ? (t->tm_hour % 12) : 12, "%02d", pt, ptlim);
          continue;

        case 'j':
          pt = _conv(t->tm_yday + 1, "%03d", pt, ptlim);
          continue;

        case 'k':
          pt = _conv(t->tm_hour, "%2d", pt, ptlim);
          continue;

        case 'l':
          pt = _conv((t->tm_hour % 12) ? (t->tm_hour % 12) : 12, "%2d", pt, ptlim);
          continue;

        case 'M':
          pt = _conv(t->tm_min, "%02d", pt, ptlim);
          continue;

        case 'm':
          pt = _conv(t->tm_mon + 1, "%02d", pt, ptlim);
          continue;

        case 'n':
          pt = _add("\n", pt, ptlim);
          continue;

        case 'p':
          pt = _add((t->tm_hour >= 12) ? "pm" : "am", pt, ptlim);
          continue;

        case 'R':
          pt = _fmt("%H:%M", t, pt, ptlim);
          continue;

        case 'r':
          pt = _fmt("%I:%M:%S %p", t, pt, ptlim);
          continue;

        case 'S':
          pt = _conv(t->tm_sec, "%02d", pt, ptlim);
          continue;

        case 's':
        {
          struct tm  tm;
          char buf[32];
          time_t mkt;

          tm = *t;
          mkt = mktime(&tm);
          sprintf(buf, "%llu", mkt);
          pt = _add(buf, pt, ptlim);
          continue;
        }

        case 'T':
          pt = _fmt("%H:%M:%S", t, pt, ptlim);
          continue;

        case 't':
          pt = _add("\t", pt, ptlim);
          continue;

        case 'U':
          pt = _conv((t->tm_yday + 7 - t->tm_wday) / 7, "%02d", pt, ptlim);
          continue;

        case 'u':
          pt = _conv((t->tm_wday == 0) ? 7 : t->tm_wday, "%d", pt, ptlim);
          continue;

        case 'V':  // ISO 8601 week number
        case 'G':  // ISO 8601 year (four digits)
        case 'g':  // ISO 8601 year (two digits)
        {
          int  year;
          int  yday;
          int  wday;
          int  w;

          year = t->tm_year + TM_YEAR_BASE;
          yday = t->tm_yday;
          wday = t->tm_wday;
          while (1) 
          {
            int  len;
            int  bot;
            int  top;

            len = LEAPYEAR(year) ? DAYSPERLYEAR : DAYSPERNYEAR;
            bot = ((yday + 11 - wday) % DAYSPERWEEK) - 3;
            top = bot - (len % DAYSPERWEEK);
            if (top < -3) top += DAYSPERWEEK;
            top += len;
            if (yday >= top) 
            {
              ++year;
              w = 1;
              break;
            }
            if (yday >= bot) 
            {
              w = 1 + ((yday - bot) / DAYSPERWEEK);
              break;
            }
            --year;
            yday += LEAPYEAR(year) ? DAYSPERLYEAR : DAYSPERNYEAR;
          }
          if (*format == 'V')
            pt = _conv(w, "%02d", pt, ptlim);
          else if (*format == 'g') 
            pt = _conv(year % 100, "%02d", pt, ptlim);
          else  
            pt = _conv(year, "%04d", pt, ptlim);

          continue;
        }

        case 'v':
          pt = _fmt("%e-%b-%Y", t, pt, ptlim);
          continue;

        case 'W':
          pt = _conv((t->tm_yday + 7 - (t->tm_wday ? (t->tm_wday - 1) : 6)) / 7, "%02d", pt, ptlim);
          continue;

        case 'w':
          pt = _conv(t->tm_wday, "%d", pt, ptlim);
          continue;

        case 'X':
          pt = _fmt("%H:%M:%S", t, pt, ptlim);
          continue;

        case 'x':
          pt = _fmt("%m/%d/%y", t, pt, ptlim);
          continue;

        case 'y':
          pt = _conv((t->tm_year + TM_YEAR_BASE) % 100, "%02d", pt, ptlim);
          continue;

        case 'Y':
          pt = _conv(t->tm_year + TM_YEAR_BASE, "%04d", pt, ptlim);
          continue;

        case 'Z':
          pt = _add("?", pt, ptlim);
          continue;

        case 'z':
        {
          long absoff;
          if (_timezone >= 0) 
          {
            absoff = _timezone;
            pt = _add("+", pt, ptlim);
          } 
          else 
          {
            absoff = _timezone;
            pt = _add("-", pt, ptlim);
          }
          pt = _conv(absoff / 3600, "%02d", pt, ptlim);
          pt = _conv((absoff % 3600) / 60, "%02d", pt, ptlim);

          continue;
        }

        case '+':
          pt = _fmt("%a, %d %b %Y %H:%M:%S %z", t, pt, ptlim);
          continue;

        case '%':
        default:
          break;
      }
    }

    if (pt == ptlim) break;
    *pt++ = *format;
  }

  return pt;
}

static char *_conv(const int n, const char *format, char *pt, const char *ptlim)
{
  char  buf[32];

  sprintf(buf, format, n);
  return _add(buf, pt, ptlim);
}

static char *_add(const char *str, char *pt, const char *ptlim)
{
  while (pt < ptlim && (*pt = *str++) != '\0') ++pt;
  return pt;
}

