#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <dbus/dbus-glib-bindings.h>
#include "imgur-client-glue.h"

#define UPLOAD_METHOD "Upload"
#define TIMEOUT_MS 3000

enum {
	EXIT_OK = 0,
	EXIT_ARGUMENT_PARSE_ERROR,
	EXIT_BUS_ERROR,
	EXIT_NO_ARGUMENT,
	EXIT_IMGUR_ERROR = 255
};

gboolean show_browser = FALSE;
gchar *filename = NULL;

DBusGConnection *connection = NULL;
DBusGProxy *uploader = NULL;

static GOptionEntry entries[] =
{
	{ "browser", 'b', 0, G_OPTION_ARG_NONE, &show_browser,
		"Launch browser if successful", NULL },
	{ NULL }
};

static void
parse_commandline (int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context =
		g_option_context_new ("[PATH] - post images to the web");

	g_option_context_add_main_entries (context,
		entries, "imgur");

	if (!g_option_context_parse (context,
		&argc, &argv, &error))
	  {
	    g_print ("%s\n", error->message);
	    g_error_free (error);
	    exit (EXIT_ARGUMENT_PARSE_ERROR);
	  }

	if (argc!=0)
	  {
	    filename = argv[1];
	  }

	if (filename && strncmp (filename, "file:", 5)==0)
	  {
	    filename += 5;
	  }

	if (!filename)
	  {
	    gchar *help = g_option_context_get_help (context,
		FALSE, NULL);

	    g_print ("%s", help);
	    g_free (help);

	    exit (EXIT_NO_ARGUMENT);
	  }
}

static void
get_proxy (void)
{
	GError *error = NULL;

	connection = dbus_g_bus_get (DBUS_BUS_SESSION,
		&error);

	if (!connection)
	  {
	    g_print ("Could not connect to bus: %s\n",
		error->message);
	    g_error_free (error);
	    exit (EXIT_BUS_ERROR);
	  }

	uploader = dbus_g_proxy_new_for_name (connection,
		"com.imgur",
		"/com/imgur",
		"com.imgur");
}

static void
print_one_line (gpointer key, gpointer value, gpointer dummy)
{
	GValue *v = (GValue*) value;

	g_print ("%s\t%s\n",
	  (gchar*) key,
	  g_value_get_string (v));
}

static void
make_call (void)
{
	GError *error = NULL;
	GHashTable *result = NULL;

	if (com_imgur_upload (uploader, filename, &result, &error))
	  {
	    if (show_browser)
	      {
	        GValue *url = g_hash_table_lookup (result, "imgur_page");

		if (url)
	          {
	            gchar *command_line = g_strdup_printf ("xdg-open %s",
	              g_value_get_string (url));

		    if (!g_spawn_command_line_async (command_line, &error))
	              {
	                g_warning ("Failed to spawn browser: %s", error->message);
			g_error_free (error);
	                error = NULL;
	                show_browser = FALSE;
	              }
	          }
	        else
	          {
	            /* it failed; just print details */
	            show_browser = FALSE;
	          }
	      }

	    /*
	     * note: this is not the "else" case of the
	     * block above.
	     */

	    if (!show_browser)
	      {
	        g_hash_table_foreach (result,
	          print_one_line,
	          NULL);
	      }

	    exit (EXIT_OK);
	  }
	else
	  {
	    g_print ("error\t%s\n",
		error->message);
	    g_error_free (error);

	    exit (EXIT_IMGUR_ERROR);
	  }
}

int
main (int argc, char **argv)
{
	g_type_init ();

	parse_commandline (argc, argv);

	get_proxy ();

	make_call ();
}

/* EOF imgur.c */
