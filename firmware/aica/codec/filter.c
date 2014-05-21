/*
  Copyright (C) 2006 Andreas Schwarz <andreas@andreas-s.net>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "filter.h"

short filter(short x)
{
	static short s[6] = {0, 0, 0, 0, 0, 0};
	const short h[6] = { 126, 719, -5838, 10032};
	short y;
	
	s[5] = s[4];
	s[4] = s[3];
	s[3] = s[2];
	s[2] = s[1];
	s[1] = s[0];
	s[0] = x;
	
	y = ((s[0] + s[5])*h[0]) >> 16 + ((s[1] + s[4])*h[1]) >> 16 + ((s[2] + s[3])*h[2]) >> 16 + (s[4]*h[3]) >> 16;
	
	return y;
}