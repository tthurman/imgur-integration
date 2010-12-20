#include "eog-imgur-plugin.h"

/* FIXME: These should be in config.h */
#define GETTEXT_PACKAGE "eog-imgur-plugin"
#define EOG_LOCALEDIR "/usr/share/locale"

#include <glib/gi18n-lib.h>
#include <gmodule.h>
#include <eog/eog-debug.h>

#include "eog-imgur-lsw.h"
#include "eog-imgur-ui.h"
#include "eog-imgur-post.h"

#define MENU_PATH "/MainMenu/"

EOG_PLUGIN_REGISTER_TYPE (EogImgurPlugin, eog_imgur_plugin);

GtkUIManager *manager = NULL;
GtkActionGroup *menu = NULL;
EogWindow *the_window = NULL;
int ui_id;

typedef struct {
	GtkWindow *window;
	gchar *message;
	gchar *service;
} ImgurMessage;

/**
 * Callback both after posting to imgur only
 * (so we open a browser) and after posting to
 * imgur in order to post to libsocialweb
 * (so we forward stuff to libsocialweb).
 */
static void
post_callback (gboolean success,
	gchar *result,
	gpointer user_data)
{
	/* STUB */

	ImgurMessage *im = user_data;
	gchar *temp = g_strdup_printf("%d // %s // %s // %s",
		success,
		result,
		im->message? im->message : "NONE",
		im->service? im->service: "NONE");

	eog_imgur_ui_display (GTK_WINDOW (the_window),
		temp,
		FALSE);

	g_free (im->message);
	g_free (im->service);
	g_free (im);
	g_free (temp);
}

/**
 * Called by the menu option to post to imgur.
 */
static void
post_to_imgur (GtkAction *action,
	EogWindow *window)
{
	ImgurMessage *im = g_malloc(sizeof(ImgurMessage));

	im->window = GTK_WINDOW (window);
	im->message = NULL;
	im->service = NULL;

	eog_imgur_post ("filename", /* FIXME */
        	post_callback,
	        im);
}

/**
 * Callback for the dialogue which requests an
 * extra message to be addded to libsocialweb
 * messages.
 */
static void
handle_libsocialweb_message (GtkWindow *parent,
	gchar *status_message,
	const gchar *service)
{
	ImgurMessage *im = g_malloc(sizeof(ImgurMessage));

	im->window = GTK_WINDOW (parent);
	im->message = g_strdup (status_message);
	im->service = g_strdup (service);

	eog_imgur_post ("filename", /* FIXME */
		post_callback,
		im);
}

/**
 * Called by the menu options which post to libsocialweb,
 */
static void
post_to_libsocialweb (GtkAction *action,
	EogWindow *window)
{
	gint character_limit = 0; /* no limit */

	/*
	 * the 12 is to bypass the "ImgurService"
	 * prefix
	 */
	const gchar *service = gtk_action_get_name (action) + 12;

	if (strcmp (service, "twitter")==0)
	  {
	    /* special case, bit of a hack */
	    character_limit = 140 - (28+3);
	    /* 28 is the typical length of an imgur URL */
	  }

	eog_imgur_ui_get_message (GTK_WINDOW (window),
			service,
			character_limit,
			handle_libsocialweb_message);
}

static const GtkActionEntry our_menu =
{
	"ImgurMenu",
	"imgurmenu",
	N_("_Post"),
	NULL,
	NULL,
	NULL
};

static const GtkActionEntry our_post =
{
	"RunImgur",
	"imgur",
	N_("Post to imgur.com"),
	NULL,
	N_("Host this picture on the web"),
	G_CALLBACK(post_to_imgur)
};

static void
eog_imgur_plugin_finalize (GObject *object)
{
        eog_debug_message (DEBUG_PLUGINS, "EogImgurPlugin finalizing");

        G_OBJECT_CLASS (eog_imgur_plugin_parent_class)->finalize (object);
}

static void
add_lsw_service (GtkUIManager *manager,
	EogWindow *window,
	gchar *name)
{
	gchar *message = g_strdup_printf (N_("Post to _%s"),
				name);

	gchar *id = g_strdup_printf ("ImgurService%s",
				name);

	const GtkActionEntry lsw_menu =
	{
		id,
		id,
		message,
		NULL,
		NULL,
		G_CALLBACK(post_to_libsocialweb)
	};

	gtk_action_group_add_actions (menu,
		&lsw_menu,
		1,
		window);

	gtk_ui_manager_add_ui (manager,
			ui_id,
			MENU_PATH "ImgurMenu/",
			id,
			id,
			GTK_UI_MANAGER_MENUITEM,
			FALSE);

	g_free (id);
	g_free (message);
}

static void
add_lsw_services (const GList *services)
{
	const GList *cursor = services;

	while (cursor)
	  {
	    add_lsw_service (manager, the_window,
		cursor->data);
	    cursor = cursor->next;
	  }
}

static void
impl_activate (EogPlugin *plugin,
               EogWindow *window)
{
	manager = eog_window_get_ui_manager (window);
	the_window = window;

        eog_debug (DEBUG_PLUGINS);

	menu = gtk_action_group_new ("EogImgurPluginMenu");

	gtk_action_group_add_actions (menu,
		&our_menu,
		1,
		window);

	gtk_action_group_add_actions (menu,
		&our_post,
		1,
		window);

	ui_id = gtk_ui_manager_new_merge_id (manager);

	gtk_ui_manager_insert_action_group (manager,
					    menu,
					    -1);

	gtk_ui_manager_add_ui (manager,
			ui_id,
			MENU_PATH,
			"ImgurMenu",
			"ImgurMenu",
			GTK_UI_MANAGER_MENU,
			FALSE);

	gtk_ui_manager_add_ui (manager,
			ui_id,
			MENU_PATH "ImgurMenu/",
			"RunImgur",
			"RunImgur",
			GTK_UI_MANAGER_MENUITEM,
			FALSE);

	gtk_ui_manager_add_ui (manager,
			ui_id,
			MENU_PATH "ImgurMenu/",
			"ImgurSep",
			"ImgurSep",
			GTK_UI_MANAGER_SEPARATOR,
			FALSE);

	lsw_list_services (add_lsw_services);	
}

static void
impl_deactivate (EogPlugin *plugin,
                 EogWindow *window)
{
	/* something */
}

static void
impl_update_ui (EogPlugin *plugin,
                EogWindow *window)
{
}

static void
eog_imgur_plugin_init (EogImgurPlugin *plugin)
{
	eog_debug_message (DEBUG_PLUGINS, "EogImgurPlugin initializing");
}

static void
eog_imgur_plugin_class_init (EogImgurPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        EogPluginClass *plugin_class = EOG_PLUGIN_CLASS (klass);

        object_class->finalize = eog_imgur_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;
        plugin_class->update_ui = impl_update_ui;
}

