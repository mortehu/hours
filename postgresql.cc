#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <err.h>
#include <syslog.h>

#include "postgresql.h"

postgresql::postgresql(const char *connect_string)
{
  pg = PQconnectdb(connect_string);

  if (PQstatus(pg) != CONNECTION_OK)
    errx (EXIT_FAILURE, "PostgreSQL connection failed: %s", PQerrorMessage(pg));

#if PG_VERSION_NUM < 90100
  if (!query ("SET STANDARD_CONFORMING_STRINGS TO ON"))
    errx (EXIT_FAILURE, "Failed to enable STANDARD_CONFORMING_STRINGS");
#endif

  if (!query ("SET TRANSACTION ISOLATION LEVEL SERIALIZABLE"))
    errx (EXIT_FAILURE, "Failed to set transaction isolation level");
}

postgresql::~postgresql()
{
  if (pg)
    PQfinish (pg);
}

postgresql::result postgresql::query(const char *format, ...)
{
  char query[4096];
  static char numbufs[10][128];
  const char *args[10];
  int lengths[10];
  int formats[10];
  const char *c;
  int argcount = 0;
  va_list ap;

  char *o, *end;

  o = query;
  end = o + sizeof(query);

  va_start(ap, format);

  for (c = format; *c; )
  {
    bool is_size_t = false;

    switch (*c)
    {
      case '%':

        ++c;

        if (*c == 'z')
        {
          is_size_t = true;
          ++c;
        }

        switch (*c)
        {
          case 's':

            args[argcount] = va_arg(ap, const char*);
            lengths[argcount] = strlen(args[argcount]);
            formats[argcount] = 0;

            break;

          case 'd':

            snprintf(numbufs[argcount], 127, "%d", va_arg(ap, int));
            args[argcount] = numbufs[argcount];
            lengths[argcount] = strlen(args[argcount]);
            formats[argcount] = 0;

            break;

          case 'u':

            if (is_size_t)
              snprintf(numbufs[argcount], 127, "%llu", (unsigned long long) va_arg(ap, size_t));
            else
              snprintf(numbufs[argcount], 127, "%u", va_arg(ap, unsigned int));
            args[argcount] = numbufs[argcount];
            lengths[argcount] = strlen(args[argcount]);
            formats[argcount] = 0;

            break;

          case 'l':

            snprintf(numbufs[argcount], 127, "%lld", va_arg(ap, long long));
            args[argcount] = numbufs[argcount];
            lengths[argcount] = strlen(args[argcount]);
            formats[argcount] = 0;

            break;

          case 'f':

            snprintf(numbufs[argcount], 127, "%f", va_arg(ap, double));
            args[argcount] = numbufs[argcount];
            lengths[argcount] = strlen(args[argcount]);
            formats[argcount] = 0;

            break;

          case 'B':

            args[argcount] = va_arg(ap, const char *);
            lengths[argcount] = va_arg(ap, size_t);
            formats[argcount] = 1;

            break;

          default:

            assert(!"unknown format character");

            return result(NULL);
        }

        ++c;
        ++argcount;

        assert(o + 3 < end);

        *o++ = '$';
        if (argcount >= 10)
          *o++ = '0' + (argcount / 10);
        *o++ = '0' + (argcount % 10);

        break;

      default:

        assert(o + 1 < end);

        *o++ = *c++;
    }
  }

  va_end(ap);

  *o = 0;

  return result(PQexecParams(pg, query, argcount, 0, args, lengths, formats, 0));
}

const char *postgresql::last_error()
{
  return PQerrorMessage(pg);
}

postgresql::result::result(PGresult *pgresult)
  : pgresult(pgresult)
{
}

postgresql::result::~result()
{
  if (pgresult)
    PQclear (pgresult);
}

size_t postgresql::result::affected_row_count() const
{
  return strtol(PQcmdTuples(pgresult), 0, 0);
}

size_t postgresql::result::row_count() const
{
  return PQntuples(pgresult);
}

const char *postgresql::result::operator()(size_t row, size_t column) const
{
  return PQgetvalue(pgresult, row, column);
}

bool postgresql::result::operator!() const
{
  if (!pgresult)
    return true; /* Failed */

  switch (PQresultStatus(pgresult))
  {
    case PGRES_COMMAND_OK:
    case PGRES_COPY_IN:
    case PGRES_COPY_OUT:
    case PGRES_NONFATAL_ERROR:
    case PGRES_TUPLES_OK:

      return false; /* OK */

    default:

      return true; /* Failed */
  }
}
