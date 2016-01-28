/*
 * LaTeX.c
 * pidgin-latex plugin
 *
 * This a plugin for Pidgin to display LaTeX formula in conversation
 *
 * PLEASE, send any comment, bug report, etc. to the trackers at sourceforge.net
 *
 * Copyright (C) 2006-2011 Benjamin Moll (qjuh@users.sourceforge.net)
 * some portions : Copyright (C) 2004-2006 Nicolas Schoonbroodt (nicolas@ffsa.be)
 *                 Copyright (C) 2004-2006 GRIm@ (thegrima@altern.org).
 *                 Copyright (C) 2004-2006 Eric Betts (bettse@onid.orst.edu).
 *                 Copyright (C) 2008-2009 Martin Keßler (martin@moegger.de).
 * Other portions heavily inspired and copied from gaim sources
 * Copyright (C) 1998-2011 Pidgin developers pidgin.im
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; This document is under the scope of
 * the version 2 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see COPYING); if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA, 02110-1301, USA
 *
 */
#include "latex.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <regex.h>
#include <sys/types.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static PurplePlugin *me;

static gboolean contains_work(const char *message){
    if (strstr(message, "\\"))
        return TRUE;
    return FALSE;
}

static void open_log(PurpleConversation *conv)
{
	conv->logs = g_list_append(NULL, 
            purple_log_new(conv->type == PURPLE_CONV_TYPE_CHAT ? PURPLE_LOG_CHAT :
                PURPLE_LOG_IM, conv->name, conv->account,
                conv, time(NULL), NULL));
}

static gboolean is_blacklisted(const char *message){
	char *not_secure[NB_BLACKLIST] = BLACKLIST;
	int reti;
	int i;

	for (i = 0; i < NB_BLACKLIST; i++) {
		regex_t regex;
		char *begin_not_secure = 
            malloc((strlen(not_secure[i]) + 18) * sizeof(char));
		strcpy(begin_not_secure, "\\\\begin\\W*{\\W*");
		strcat(begin_not_secure, not_secure[i] + 0x01);
		strcat(begin_not_secure, "\\W*}");
		reti = regcomp(&regex, begin_not_secure, 0);
		reti = regexec(&regex, message, 0, NULL, 0);
		regfree(&regex);
		if (strstr(message, not_secure[i]) != NULL || 
                reti != REG_NOMATCH) return TRUE;

        free(begin_not_secure);
	}

	return FALSE;
}

static GString *fgcolor_as_string();

static GString *bgcolor_as_string();

static GString *fgcolor_as_string(){
    GString *result = g_string_new(NULL);
	int rgb;
	char const *pidgin_fgcolor;
    /* Gather some information about the current pidgin settings
     * so that we can populate the latex template file appropriately */
	if (!strcmp(purple_prefs_get_string("/pidgin/conversations/fgcolor"), "")) {
        g_string_append("0,0,0");
	} else {
		pidgin_fgcolor = purple_prefs_get_string("/pidgin/conversations/fgcolor");
		rgb = strtol(pidgin_fgcolor + 1, NULL, 16);
		purple_debug_info("LaTeX", "Numerical: %d\n", rgb);
		purple_debug_info("LaTeX", "Found foregroundcolor '%s'\n", pidgin_fgcolor);
		g_string_append_printf(result, "%d,%d,%d", rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
	}
	purple_debug_info("LaTeX", "Using '%s' for foreground\n", fgcolor);

    return result;
}

static GString *fgcolor_as_string(){
    GString *result = g_string_new(NULL);
	int rgb;
	char const *pidgin_bgcolor;

	if (!strcmp(purple_prefs_get_string("/pidgin/conversations/bgcolor"), "")) {
        g_string_append(result, "255,255,255");
	} else {
		pidgin_bgcolor = purple_prefs_get_string("/pidgin/conversations/bgcolor");
		rgb = strtol(pidgin_bgcolor + 1, NULL, 16);
		purple_debug_info("LaTeX", "Numerical: %d\n", rgb);
		purple_debug_info("LaTeX", "Found backgroundcolor '%s'\n", pidgin_bgcolor);
		g_string_append_printf(result, "%d,%d,%d", rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff);
	}
	purple_debug_info("LaTeX", "Using '%s' for background\n", bgcolor);

    return result;
}

static GPtrArray *get_commands(GString *buffer){
    GPtrArray *commands = g_ptr_array_new();
    GString *command = NULL
    char current;
    int i;

    for (i=0; i<buffer->len; i++){

        if (buffer->str[i] == '\\') {
            command = g_string_new(NULL);
            i++;
            while (g_ascii_isalnum(buffer->str[i])){
                g_string_append_c(command, buffer->str[i++]);
            }

            g_ptr_array_add(commands, command);
        }
    }

    return commands;
}

static GPtrArray *get_snippets(GString *buffer){
    GPtrArray *snippets = g_ptr_array_new();
    GString *snippet = NULL;
    int i;
    int stackcnt = 0;
    char current;


    for (i=0; i<buffer->len; i++){
        current = buffer->str[i];

        if (current == '{' && stackcnt == 0){
            snippet = g_string_new(NULL);
            stackcnt++;
            continue;
        }

        if (stackcnt > 0) {
            if (current == '{'){
                stackcnt++;
            }

            if (current == '}'){
                stackcnt--;
                if (stackcnt == 0)
                    g_ptr_array_add(snippets, snippet);
            }

            g_string_append_c(snippet, current);
        }
    }

    return snippets;
}

static gboolean replace(char **message, GString *command, GString *snippet, int id){
    GString *replacer = g_string_new(IMG_BEG);
    GString *to_replace = g_string_new("\\");
    char idbuffer[10];
    char *new_msg;

    g_string_append(to_replace, command->str);
    g_string_append(to_replace, snippet->str);

	sprintf(idbuffer, "%d\0", id);
    g_string_append(replacer, idbuffer);
    g_string_append(replacer, IMG_END);

    new_msg = str_replace(*message, to_replace->str, replacer->str);
    free(*message);
    *message = new_msg;

    g_free(replacer);
    g_free(to_replace);

    return TRUE;
}

static char *str_replace(char *orig, char *rep, char *with);

/* Credit to http://stackoverflow.com/questions/779875/
 * what-is-the-function-to-replace-string-in-c*/
static char *str_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig)
        return NULL;
    if (!rep)
        rep = "";
    len_rep = strlen(rep);
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

static int load_imgage(const GString *resulting_png){
    int img_id = 0;
    gchar *filedata;
    gsize size;
    GError *error;

	if (!g_file_get_contents(resulting_png->str, &filedata, &size, &error)) {
		purple_notify_error(me, "LaTeX", 
                "Error while reading the generated image!", error->message);
		g_error_free(error);
		free(shortcut);
		return FALSE;
	}

	img_id = purple_imgstore_add_with_id(filedata, 
            MAX(1024, size), getfilename(resulting_png->str));

	if (img_id == 0) {
		purple_notify_error(me, "LaTeX", 
                "Error while reading the generated image!", 
                "Failed to store image.");
		return -1;
	}

    return img_id;
}

//TODO check for mem leaks
static gboolean modify_message(char **message){
    enum format format = get_format(*tmp2);
    const char *startdelim = get_format_string(format);
    const char *enddelim = KOPETE_END;
    GString *snippet;
    GString *command;
    GString *picpath;

    int i;
    int image_id;

    GPtrArray *snippets = get_snippets(*message);
    GPtrArray *commands = get_commands(*message);

    if (snippets->len != commands->len){
		purple_notify_error(me, "LaTeX", 
                "Error while analysing the message!", 
                "Different amounts of snippets and commands!");
    }

    for (i=0; i<commands->len; i++){
        command = g_ptr_array_index(commands, i);
        snippet = g_ptr_array_index(snippets, i);

        picpath = dispatch_command(command, snippet);
        image_id = load_image(picpath);

        replace(message, command, snippet, image_id);
    }

    return TRUE;
}

//TODO: Check sanity of this function!
static gboolean pidgin_latex_write(PurpleConversation *conv, 
        const char *nom, const char *message, 
        PurpleMessageFlags messFlag, const char *original){

	gboolean logflag = purple_conversation_is_logging(conv);

	if (logflag) {
		GList *log;

		if (conv->logs == NULL)
			open_log(conv);

		log = conv->logs;
		while (log != NULL) {
			purple_log_write((PurpleLog*)log->data, 
                    messFlag, nom, time(NULL), original);
			log = log->next;
		}

		purple_conversation_set_logging(conv, FALSE);
	}

    /* Write trimmed message. */
	if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_CHAT){
		purple_conv_chat_write(PURPLE_CONV_CHAT(conv), 
                nom, message, messFlag, time(NULL));
    } else if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM) {
		purple_conv_im_write(PURPLE_CONV_IM(conv), 
                nom, message, messFlag, time(NULL));
    }

	if (logflag){
		purple_conversation_set_logging(conv, TRUE);
    }

	return TRUE;
}

static void message_send(PurpleConversation *conv, const char **buffer){
	char *temp_buffer;
	gboolean smileys;

	purple_debug_info("LaTeX", "[message_send()] Sending Message: %s\n", *buffer);

	if (!contains_work(*buffer)){
		return;
	}

	if (is_blacklisted(*buffer)) {
		purple_debug_info("LaTeX", 
                "Message not analysed, because it contained blacklisted code.\n");
		return;
	}

	temp_buffer = g_strdup(*buffer);
	if (temp_buffer == NULL) {
		purple_notify_error(me, "LaTeX", 
                "Error while analysing the message!", 
                "Out of memory!");
		return;
	}

	if (analyse(&temp_buffer)) {
		*buffer = temp_buffer;
	} else {
        g_free(temp_buffer);
    }
}

static void message_send_chat(PurpleAccount *account, const char **buffer, int id){
	PurpleConnection *conn = purple_account_get_connection(account);
	message_send(purple_find_chat(conn, id), buffer);
}

static void message_send_im(PurpleAccount *account, const char *who, const char **buffer){
	message_send(purple_find_conversation_with_account(
                PURPLE_CONV_TYPE_IM, who, account), buffer);
}

static gboolean message_receive(PurpleAccount *account, 
        const char *who, const char **buffer, 
        PurpleConversation *conv, PurpleMessageFlags flags){

	char *temp_buffer;
	purple_debug_info("LaTeX", "[message_receive()] Writing Message: %s\n", *buffer);

	if (!contains_work(*buffer)){
		return FALSE;
	}

	if (is_blacklisted(*buffer)) {
		purple_debug_info("LaTeX", 
                "Message not analysed, because it contained blacklisted code.\n");
		return FALSE;
	}

    temp_buffer = g_strdup(*buffer);
	if (temp_buffer == NULL) {
		purple_notify_error(me, "LaTeX", 
                "Error while analysing the message!", 
                "Out of memory!");
		return FALSE;
	}

	purple_debug_info("LaTeX", "[message_receive()] Analyse: %s\n", temp_buffer);
	if (analyse(&temp_buffer)) {
		pidgin_latex_write(conv, who, temp_buffer, flags, *buffer);
		g_free(temp_buffer);
		return TRUE;
	}

	g_free(temp_buffer);
	return FALSE;
}

static gboolean plugin_load(PurplePlugin *plugin){
	void *conv_handle = purple_conversations_get_handle();

	me = plugin;
	purple_signal_connect(conv_handle, "sending-im-msg",
			      plugin, PURPLE_CALLBACK(message_send_im), NULL);

	purple_signal_connect(conv_handle, "sending-chat-msg",
			      plugin, PURPLE_CALLBACK(message_send_chat), NULL);

	purple_signal_connect(conv_handle, "writing-im-msg",
			      plugin, PURPLE_CALLBACK(message_receive), NULL);

	purple_signal_connect(conv_handle, "writing-chat-msg",
			      plugin, PURPLE_CALLBACK(message_receive), NULL);

	purple_debug_info("LaTeX", "LaTeX loaded\n");

	return TRUE;
}

static gboolean plugin_unload(PurplePlugin * plugin)
{
	void *conv_handle = purple_conversations_get_handle(); 
	purple_signal_disconnect(conv_handle, 
            "sending-im-msg", plugin, 
            PURPLE_CALLBACK(message_send_im));
	purple_signal_disconnect(conv_handle, 
            "sending-chat-msg", plugin, 
            PURPLE_CALLBACK(message_send_chat));
	purple_signal_disconnect(conv_handle, 
            "writing-im-msg", plugin, 
            PURPLE_CALLBACK(message_receive));
	purple_signal_disconnect(conv_handle, 
            "writing-chat-msg", plugin, 
            PURPLE_CALLBACK(message_receive));

	me = NULL;
	purple_debug_info("LaTeX", "LaTeX unloaded\n");

	return TRUE;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,                 /**< type           */
	PIDGIN_PLUGIN_TYPE,                     /**< ui_requirement */
	0,                                      /**< flags          */
	NULL,                                   /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                /**< priority       */

	LATEX_PLUGIN_ID,                        /**< id             */
	"LaTeX",                                /**< name           */
	"1.5",                                  /**< version        */
	/**  summary        */
	"To display LaTeX formula into Pidgin conversation.",
	/**  description    */
	"Put LaTeX-code between $$ ... $$ markup to have it displayed as " 
    "Picture in your conversation.\nRemember that your contact needs "
    "an similar plugin or else he will just see the pure LaTeX-code\nYou "
    "must have LaTeX and dvipng installed (in your PATH)",
	"Benjamin Moll <qjuh@users.sourceforge.net>\nNicolas Schoonbroodt "
    "<nicolas@ffsa.be>\nNicolai Stange <nic-stange@t-online.de>", /**< author       */
	WEBSITE,                                /**< homepage       */
	plugin_load,                            /**< load           */
	plugin_unload,                          /**< unload         */
	NULL,                                   /**< destroy        */
	NULL,                                   /**< ui_info        */
	NULL,                                   /**< extra_info     */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin) { }

PURPLE_INIT_PLUGIN(LaTeX, init_plugin, info)
