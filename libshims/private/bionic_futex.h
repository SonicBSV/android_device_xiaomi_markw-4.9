#ifndef _BIONIC_FUTEX_H
#define _BIONIC_FUTEX_H

#include <errno.h>
#include <linux/futex.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/cdefs.h>
#include <sys/syscall.h>
#include <unistd.h>

__BEGIN_DECLS

struct timespec;

static inline __always_inline int __futex(volatile void* ftx, int op, int value, const struct timespec* timeout) {
  // Our generated syscall assembler sets errno, but our callers (pthread functions) don't want to.
  int saved_errno = errno;
  int result = syscall(__NR_futex, ftx, op, value, timeout);
  if (__predict_false(result == -1)) {
    result = -errno;
    errno = saved_errno;
  }
  return result;
}

static inline int __futex_wake(volatile void* ftx, int count) {
  return __futex(ftx, FUTEX_WAKE, count, NULL);
}

static inline int __futex_wake_ex(volatile void* ftx, bool shared, int count) {
  return __futex(ftx, shared ? FUTEX_WAKE : FUTEX_WAKE_PRIVATE, count, NULL);
}

static inline int __futex_wait(volatile void* ftx, int value, const struct timespec* timeout) {
  return __futex(ftx, FUTEX_WAIT, value, timeout);
}

static inline int __futex_wait_ex(volatile void* ftx, bool shared, int value, const struct timespec* timeout) {
  return __futex(ftx, shared ? FUTEX_WAIT : FUTEX_WAIT_PRIVATE, value, timeout);
}

__END_DECLS

#endif /* _BIONIC_FUTEX_H */
