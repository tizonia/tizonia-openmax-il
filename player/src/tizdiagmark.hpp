#ifndef TIZDIAGMARK_HPP
#define TIZDIAGMARK_HPP

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace tizdiag
{
  inline void emit (const char *tag, long a, long b, long c, long d,
                    bool critical, bool has_d)
  {
    static unsigned count = 0;
    static unsigned long seq = 0;
    int saved_errno = errno;
    const char *guard = getenv ("TIZ_SMOKE_FLAC_MARK");
    if (guard && 0 == strcmp (guard, "1"))
      {
        flockfile (stderr);
        if (count < 128 && (critical || count < 96))
          {
            if (has_d)
              {
                fprintf (stderr, "SMK %lu %s %ld %ld %ld %ld\n", seq++, tag,
                         a, b, c, d);
              }
            else
              {
                fprintf (stderr, "SMK %lu %s %ld %ld %ld\n", seq++, tag, a, b,
                         c);
              }
            ++count;
          }
        else if (count == 128)
          {
            fprintf (stderr, "SMK %lu truncated budget-exhausted\n", seq);
            ++count;
          }
        fflush (stderr);
        funlockfile (stderr);
      }
    errno = saved_errno;
  }

  inline void mark (const char *tag, long a, long b, long c,
                    bool critical = false)
  {
    emit (tag, a, b, c, 0, critical, false);
  }

  inline void mark4 (const char *tag, long a, long b, long c, long d,
                     bool critical = false)
  {
    emit (tag, a, b, c, d, critical, true);
  }
}

#endif
