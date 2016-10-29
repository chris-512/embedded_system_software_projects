#include "debug.h"

void DEBUG(const char *debug, ...)
{
	va_list ap;

	va_start(ap, debug);
	
	vprintf(debug, ap);

	va_end(ap);
}
