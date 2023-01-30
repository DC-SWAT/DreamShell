#include <string.h>
#include <ctype.h>

#include "private/lftpd_string.h"

char* lftpd_string_trim(char* s) {
	char* p = s;
	for (int i = 0, len = strlen(s); i < len && isspace((int) s[i]); i++) {
		p++;
	}
	for (int i = strlen(p); i >= 0 && isspace((int) p[i]); i--) {
		p[i] = '\0';
	}
	return p;
}
