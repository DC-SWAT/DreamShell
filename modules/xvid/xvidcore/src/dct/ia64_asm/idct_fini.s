// ****************************************************************************
// *
// *  XVID MPEG-4 VIDEO CODEC
// *  - IA64 inverse discrete cosine transform -
// *
// *  Copyright(C) 2002 Christian Schwarz, Haiko Gaisser, Sebastian Hack
// *
// *  This program is free software; you can redistribute it and/or modify it
// *  under the terms of the GNU General Public License as published by
// *  the Free Software Foundation; either version 2 of the License, or
// *  (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// *  GNU General Public License for more details.
// *
// *  You should have received a copy of the GNU General Public License
// *  along with this program; if not, write to the Free Software
// *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
// *
// * $Id: idct_fini.s,v 1.2 2009-02-19 17:07:29 Isibaar Exp $
// *
// ***************************************************************************/
//
// ****************************************************************************
// *                                                                            
// *  idct_fini.s, IA-64 optimized inverse DCT                                  
// *                                                                            
// *  This version was implemented during an IA-64 practical training at 	    
// *  the University of Karlsruhe (http://i44w3.info.uni-karlsruhe.de/)		    
// *                                                                            
// ****************************************************************************
//

	mov	ar.pfs = r16
	br.ret.sptk.few b0
