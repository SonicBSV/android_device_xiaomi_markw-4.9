#ifndef _BIONIC_TIME_CONVERSIONS_H
#define _BIONIC_TIME_CONVERSIONS_H

#include <time.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

__LIBC_HIDDEN__ void timespec_from_ms(timespec& ts, const int ms);
__LIBC_HIDDEN__ bool timespec_from_absolute_timespec(timespec& ts, const timespec& abs_ts, clockid_t clock);

__END_DECLS

#endif
