#include <err.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h>

#include "postgresql.h"

static Display *display;

int main (int argc, char **argv)
{
  static XScreenSaverInfo *mit_info;
  time_t idle_time;

  if (!(display = XOpenDisplay (NULL)))
    return -1;

  mit_info = XScreenSaverAllocInfo ();

  postgresql db ("dbname=mortehu options='-c search_path=hours'");

  daemon (0, 0);

  for (;;)
    {
      float idle_time;

      XScreenSaverQueryInfo (display, RootWindow (display, DefaultScreen (display)), mit_info);

      idle_time = mit_info->idle / 1000.0;

      if (idle_time < 60)
        {
          auto span = db.query ("SELECT id FROM spans WHERE stop > NOW () - INTERVAL '20 minute' + %f * INTERVAL '1 second' ORDER BY stop DESC LIMIT 1", idle_time);

          if (!span)
            errx (EXIT_FAILURE, "PostgreSQL query failed");

          if (!span.row_count ())
            {
              if (!db.query ("INSERT INTO spans (id) VALUES ((SELECT COALESCE ((SELECT id FROM spans ORDER BY id DESC LIMIT 1 FOR UPDATE), 0) + 1))"))
                errx (EXIT_FAILURE, "Failed to create new span: %s", db.last_error ());
            }
          else
            {
              if (!(db.query ("UPDATE spans SET stop = NOW () - %f * INTERVAL '1 second' WHERE id = %s::INTEGER",
                              idle_time, span (0, 0))))
                errx (EXIT_FAILURE, "Failed to update span: %s", db.last_error ());
            }
        }

      sleep (60);
    }

  return EXIT_SUCCESS;
}
