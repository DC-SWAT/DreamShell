#!/usr/bin/env python

fd = open("font.bin", "r").read()
for i in range(0, len(fd), 12):
	sub = map(lambda x:ord(x), fd[i:i+12])
	print "\t.byte\t0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x" % \
		(sub[0], sub[1], sub[2], sub[3], sub[4], sub[5], sub[6], sub[7], sub[8], \
		 sub[9], sub[10], sub[11])

