/*
 * LaTeX.h
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
 * Windows port  : Copyright (C) 2005-2006 Nicolai Stange (nic-stange@t-online.de)
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
#ifndef _PIDGIN_LATEX_H_
#define _PIDGIN_LATEX_H_
#ifndef G_GNUC_NULL_TERMINATED
#  if __GNUC__ >= 4
#    define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
#  else
#    define G_GNUC_NULL_TERMINATED
#  endif /* __GNUC__ >= 4 */
#endif /* G_GNUC_NULL_TERMINATED */

#define PURPLE_PLUGINS

//include
#include <pidgin/gtkplugin.h>
#include <libpurple/conversation.h>
//#include <libpurple/core.h>
#include <libpurple/debug.h>
#include <libpurple/signals.h>
#include <libpurple/imgstore.h>
#include <libpurple/util.h>
#include <libpurple/notify.h>
#include <libpurple/server.h>
#include <libpurple/log.h>
#include <libpurple/version.h>
#include <pidgin/gtksmiley.h>

// Constant
#define IMG_BEGIN "<img id=\""
#define IMG_END "\">"

#define BEG "[tex]"
#define END "[/tex]"
#define KOPETE_TEX "$$"

#define LATEX_PLUGIN_ID "qjuh-LaTeX"
#define WEBSITE "http://sourceforge.net/projects/pidgin-latex/"

#define HEADER "\\documentclass[12pt]{article}\\usepackage{color}\\usepackage[dvips]{graphicx}\\usepackage{amsmath}\\usepackage{amssymb}\\pagestyle{empty}\\definecolor{fgcolor}{RGB}{"
#define HEADER_COLOR "}\\definecolor{bgcolor}{RGB}{"
#define HEADER_MATH "}\\begin{document}\\pagecolor{bgcolor}\\begin{gather*}\\color{fgcolor}"

#define FOOTER "\\end{document}"
#define FOOTER_MATH "\\end{gather*}"

#define FILTER_AND "&amp;"
#define FILTER_LT "&lt;"
#define FILTER_GT "&gt;"
#define FILTER_BR "<br>"

/* Yes, this is simply a copy/paste of KopeteTex blacklist. */
/* But too bad in LaTeX and system security to verify all   */
/* of this */
#define NB_BLACKLIST (42)
#define BLACKLIST { "\\def", "\\let", "\\futurelet", "\\newcommand", "\\renewcommand", "\\else", "\\fi", "\\write", "\\input", "\\include", "\\chardef", "\\catcode", "\\makeatletter", "\\noexpand", "\\toksdef", "\\every", "\\errhelp", "\\errorstopmode", "\\scrollmode", "\\nonstopmode", "\\batchmode", "\\read", "\\csname", "\\newhelp", "\\relax", "\\afterground", "\\afterassignment", "\\expandafter", "\\noexpand", "\\special", "\\command", "\\loop", "\\repeat", "\\toks", "\\output", "\\line", "\\mathcode", "\\name", "\\item", "\\section", "\\mbox", "\\DeclareRobustCommand" }


// prototypes

/* Verify Blacklist */
/* return true if one word of the message is blacklisted */
static gboolean is_blacklisted(char *message);

/*
 * latex_to_image creates PNG-image  with the LaTeX code pointed by *latex
 * *latex points to latex-input
 * **file_png receives filename of temporary generated png-file; file must be deleted and string must be freed by caller
 *
 * returns TRUE on success, false otherwise
 */
static gboolean latex_to_image(char *latex, char **file_png);

/*
 * Transform *tmp2 extracting some *startdelim here *enddelim thing, make png-image from latex-input
 *  and tmp2 becomes 'some<img="number">thing'
 *  smileys whether or not to add the formula as a custom smiley
 * returns TRUE on success, FALSE otherwise
 */
static gboolean analyse(char **tmp2, char *startdelim, char *enddelim, gboolean smileys);

/*
 * pidgin_latex_write perform the effective write of the latex code in the IM or Chat windows
 *      *conv is a pointer onto the conversation context
 *	*nom is the name of the correspondent
 *	*message is the modified message with the image
 *	*messFlags is Flags related to the messages
 *	*original is the original message unmodified
 * return TRUE.
 */
static gboolean pidgin_latex_write(PurpleConversation *conv, const char *nom, char *message, PurpleMessageFlags messFlag, char *original);

/* to intercept outgoing messages */
static void message_send(PurpleConversation *conv, char **buffer);

/* to intercept ingoing messages */
static gboolean message_receive(PurpleAccount *account, const char *who, char **buffer, PurpleConversation *conv, PurpleMessageFlags flags);
/*
 * getdirname returns the directory's part of a filename.
 * Parsing is done OS-dependently (with path-separator as defined in glib/gutils.h)
 *      *path is the filename you want to extract a directory-part from
 * return directory's part on success, NULL otherwise; must be freed with
 * free()!
 */
static char* getdirname(const char const *file);

/*
 * get_latex_cmd returns a system()-executable-command for latex
 *
 * return the executable command if successfull, NULL otherwise; must be
 * freed with free()
 */
static char* get_latex_cmd();

/*
 * get_dvipng_cmd returns a system()-executable-command for dvipng
 *
 * return the executable command if successfull, NULL otherwise; must be
 * freed with free()
 */
static char* get_dvipng_cmd();

/*
 * win32_purple_notify extracts error information of last occured WIN32-Error due to an API-call and asserts it to pidgin via purple_notify_error
 * Error message will be prepended by *prep:
 */
void win32_purple_notify_error(char *prep);

/*
 * searchPATH searches the PATH-environment for the specified file.
 * *file is the name of the executable to search for
 * returns the right full path, e.g. with the executable's name appended, NULL on failure, must be freed with free()
 */
char* searchPATH(const char const *file);

/*
 * execute executes the *cmd with opts appended to the commandline.
 * Advantage to system()-call under win32 ist that no console window pops up.
 * On systems other than windows this function is forks and makes an execvp()-call.
 * *cmd is the command to execute
 * *opts[] is an array of elements to be appended to the commandline
 * copts is the count of elements within *opts[]
 *
 * returns -1 if execution failed, otherwise the return code of the executed program.
 */
static int execute(char *cmd, char *opts[], int copts);
#endif
