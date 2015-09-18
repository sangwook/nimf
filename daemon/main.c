#include "config.h"
#include <libintl.h>
#include "dasom-server.h"
#include <glib-unix.h>
#include <syslog.h>
#include "dasom-private.h"

gboolean syslog_initialized = FALSE;

int
main (int argc, char **argv)
{
  g_debug (G_STRLOC ": %s", G_STRFUNC);

  DasomServer *server;
  GMainLoop   *loop;
  GError      *error = NULL;

  gboolean        no_daemon = FALSE;
  GOptionContext *context;
  GOptionEntry    entries[] = {
    {"no-daemon", 0, 0, G_OPTION_ARG_NONE, &no_daemon, "Do not daemonize", NULL},
    {NULL}
  };

  context = g_option_context_new ("- dasom daemon");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);

  if (error != NULL)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    return EXIT_FAILURE;
  }

#if ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, DASOM_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  if (no_daemon == FALSE)
  {
    openlog (g_get_prgname (), LOG_PID | LOG_PERROR, LOG_DAEMON);
    syslog_initialized = TRUE;
    g_log_set_default_handler (dasom_log_default_handler, NULL);

    if (daemon (0, 0) != 0)
    {
      g_critical ("Couldn't daemonize.");
      return EXIT_FAILURE;
    }
  }

  server = dasom_server_new ("unix:abstract=dasom", &error);

  if (server == NULL)
  {
    g_critical ("%s", error->message);
    g_clear_error (&error);

    return EXIT_FAILURE;
  }

  dasom_server_start (server);

  loop = g_main_loop_new (NULL, FALSE);

  g_unix_signal_add (SIGINT,  (GSourceFunc) g_main_loop_quit, loop);
  g_unix_signal_add (SIGTERM, (GSourceFunc) g_main_loop_quit, loop);

  g_main_loop_run (loop);

  g_main_loop_unref (loop);
  g_object_unref (server);

  if (syslog_initialized)
    closelog ();

  return EXIT_SUCCESS;
}
