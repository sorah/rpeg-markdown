/**********************************************************************

  markdown.c - markdown in C using a PEG grammar.
  (c) 2008 John MacFarlane (jgm at berkeley dot edu).

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

 ***********************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <glib.h>
#include "markdown_peg.h"

static int extensions;

/**********************************************************************

  The main program is just a wrapper around the library functions in
  markdown_lib.c.  It parses command-line options, reads the text to
  be converted from input files or stdin, converts the text, and sends
  the output to stdout or a file.  Character encodings are ignored.

 ***********************************************************************/

#define VERSION "0.3"
#define COPYRIGHT "Copyright (c) 2008 John MacFarlane.\n" \
                  "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n" \
                  "This is free software: you are free to change and redistribute it.\n" \
                  "There is NO WARRANTY, to the extent permitted by law."

/* print version and copyright information */
void version(const char *progname)
{
  printf("%s version %s\n"
         "%s\n",
         progname,
         VERSION,
         COPYRIGHT);
}

int main(int argc, char * argv[]) {

    int numargs;            /* number of filename arguments */
    int i;

    GString *inputbuf;
    char *out;              /* string containing processed output */

    FILE *input;
    FILE *output;
    char curchar;
    char *progname = argv[0];

    int output_format = HTML_FORMAT;

    /* Code for command-line option parsing. */

    static gboolean opt_version = FALSE;
    static gchar *opt_output = 0;
    static gchar *opt_to = 0;
    static gboolean opt_smart = FALSE;
    static gboolean opt_notes = FALSE;
    static gboolean opt_allext = FALSE;

    static GOptionEntry entries[] =
    {
      { "version", 'v', 0, G_OPTION_ARG_NONE, &opt_version, "print version and exit", NULL },
      { "output", 'o', 0, G_OPTION_ARG_STRING, &opt_output, "send output to FILE (default is stdout)", "FILE" },
      { "to", 't', 0, G_OPTION_ARG_STRING, &opt_to, "convert to FORMAT (default is html)", "FORMAT" },
      { "extensions", 'x', 0, G_OPTION_ARG_NONE, &opt_allext, "use all syntax extensions", NULL },
      { NULL }
    };

    /* Options to active syntax extensions.  These appear separately in --help. */
    static GOptionEntry ext_entries[] =
    {
      { "smart", 0, 0, G_OPTION_ARG_NONE, &opt_smart, "use smart typography extension", NULL },
      { "notes", 0, 0, G_OPTION_ARG_NONE, &opt_notes, "use notes extension", NULL },
      { NULL }
    };

    GError *error = NULL;
    GOptionContext *context;
    GOptionGroup *ext_group;

    context = g_option_context_new ("[FILE...]");
    g_option_context_add_main_entries (context, entries, NULL);
    ext_group = g_option_group_new ("extensions", "Syntax extensions", "show available syntax extensions", NULL, NULL);
    g_option_group_add_entries (ext_group, ext_entries);
    g_option_context_add_group (context, ext_group);
    g_option_context_set_description (context, "Converts text in specified files (or stdin) from markdown to FORMAT.\n"
                                               "Available FORMATs:  html, latex, groff-mm");
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("option parsing failed: %s\n", error->message);
        exit (1);
    }
    g_option_context_free(context);

    /* Process command-line options and arguments. */

    if (opt_version) {
        version(progname);
        return EXIT_SUCCESS;
    }

    extensions = 0;
    if (opt_allext)
        extensions = 0xFFFFFF;  /* turn on all extensions */
    if (opt_smart)
        extensions = extensions | EXT_SMART;
    if (opt_notes)
        extensions = extensions | EXT_NOTES;

    if (opt_to == NULL)
        output_format = HTML_FORMAT;
    else if (strcmp(opt_to, "html") == 0)
        output_format = HTML_FORMAT;
    else if (strcmp(opt_to, "latex") == 0)
        output_format = LATEX_FORMAT;
    else if (strcmp(opt_to, "groff-mm") == 0)
        output_format = GROFF_MM_FORMAT;
    else {
        fprintf(stderr, "%s: Unknown output format '%s'\n", progname, opt_to);
        exit(EXIT_FAILURE);
    }

    /* we allow "-" as a synonym for stdout here */
    if (opt_output == NULL || strcmp(opt_output, "-") == 0)
        output = stdout;
    else if (!(output = fopen(opt_output, "w"))) {
        perror(opt_output);
        return 1;
    }

    inputbuf = g_string_new("");   /* string for concatenated input */

    /* Read input from stdin or input files into inputbuf */

    numargs = argc - 1;
    if (numargs == 0) {        /* use stdin if no files specified */
        while ((curchar = fgetc(stdin)) != EOF)
            g_string_append_c(inputbuf, curchar);
        fclose(stdin);
    }
    else {                  /* open all the files on command line */
       for (i = 0; i < numargs; i++) {
            if ((input = fopen(argv[i+1], "r")) == NULL) {
                perror(argv[i+1]);
                exit(EXIT_FAILURE);
            }
            while ((curchar = fgetc(input)) != EOF)
                g_string_append_c(inputbuf, curchar);
            fclose(input);
       }
    }

    out = markdown_to_string(inputbuf->str, extensions, output_format);
    fprintf(output, "%s\n", out);
    free(out);

    g_string_free(inputbuf, true);

    return(EXIT_SUCCESS);
}