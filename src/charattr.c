/** \file charattr.c 
    \brief New character dump display

    * Copyright (c) 2009 Nick McConnell, Andi Sidwell, 
    * Ben Harrison, James E. Wilson, Robert A. Koeneke
    *
    * This work is free software; you can redistribute it and/or modify it
    * under the terms of either:
    *
    * a) the GNU General Public License as published by the Free Software
    *    Foundation, version 2, or
    *
    * b) the "Angband licence":
    *    This software may be copied and distributed for educational, research,
    *    and not for profit purposes provided that this copyright and statement
    *    are included in all such copies.  Other copyrights may also apply.
    */

#include "angband.h"
#include "cmds.h"
#include "game-cmd.h"
#include "tvalsval.h"
#include "option.h"
#include "ui-menu.h"


/**
 * Format and translate a string, then print it out to file.
 */
void x_fprintf(ang_file * f, int encoding, const char *fmt, ...)
{
	va_list vp;

	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args, save the length */
	(void) vstrnfmt(buf, sizeof(buf), fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	file_put(f, buf);
}

/**
 * New (as of FA0.3.0) info printing functions, designed for use in character 
 * dumps and screens.  Basic data structure is an array of the new char_attr 
 * type.  
 */

/**
 * Put a coloured string at a location in the char_attr line dump_ptr
 */
void dump_put_str(byte attr, const char *str, int col)
{
	int i = 0;
	wchar_t *s;
	wchar_t buf[1024];
	bool finished = FALSE;

	/* Find the start point */
	while ((i != col) && (i < MAX_C_A_LEN)) {
		if (dump_ptr[i].pchar == L'\0')
			finished = TRUE;
		if (finished) {
			dump_ptr[i].pchar = L' ';
			dump_ptr[i].pattr = TERM_WHITE;
		}
		i++;
	}

	/* Copy to a rewriteable string */
	Term_mbstowcs(buf, str, 1024);

	/* Current location within "buf" */
	s = buf;

	/* Write the characters */
	while ((*s != L'\0') && (i < MAX_C_A_LEN)) {
		dump_ptr[i].pattr = attr;
		dump_ptr[i++].pchar = *s++;
	}

	/* Paranoia */
	if (i >= MAX_C_A_LEN)
		i--;

	/* Terminate */
	dump_ptr[i].pchar = L'\0';
}

/**
 * Print long number with header at given column
 * Use the color for the number, not the header
 */
void dump_lnum(char *header, s32b num, int col, byte color)
{
	int len = strlen(header);
	char out_val[32];

	dump_put_str(TERM_WHITE, header, col);
	sprintf(out_val, "%9ld", (long) num);
	dump_put_str(color, out_val, col + len);
}

/**
 * Print number with header at given row, column
 */
void dump_num(char *header, int num, int col, byte color)
{
	int len = strlen(header);
	char out_val[32];
	dump_put_str(TERM_WHITE, header, col);
	sprintf(out_val, "%6ld", (long) num);
	dump_put_str(color, out_val, col + len);
}

/**
 * Print decimal number with header at given row, column
 */
void dump_deci(char *header, int num, int deci, int col, byte color)
{
	int len = strlen(header);
	char out_val[32];
	dump_put_str(TERM_WHITE, header, col);
	sprintf(out_val, "%6ld", (long) num);
	dump_put_str(color, out_val, col + len);
	sprintf(out_val, ".");
	dump_put_str(color, out_val, col + len + 6);
	sprintf(out_val, "%8ld", (long) deci);
	dump_put_str(color, &out_val[7], col + len + 7);
}


/**
 * Hook function - dump a char_attr line to a file 
 */
void dump_line_file(char_attr * this_line)
{
	int x = 0;
	char_attr xa = this_line[0];

	char buf[1024];
	char *p = buf;

	/* Dump the line */
	while (xa.pchar != L'\0') {
		/* Add the char/attr */
		p += wctomb(p, xa.pchar);

		/* Advance */
		xa = this_line[++x];
	}

	/* Terminate the line */
	*p = '\0';
	file_putf(dump_out_file, "%s\n", buf);
}

/**
 * Hook function - dump a char_attr line to the screen 
 */
void dump_line_screen(char_attr * this_line)
{
	int x = 0;
	char_attr xa = this_line[0];

	/* Erase the row */
	Term_erase(0, dump_row, 255);

	/* Dump the line */
	while (xa.pchar != L'\0') {
		/* Add the char/attr */
		Term_addch(xa.pattr, xa.pchar);

		/* Advance */
		xa = this_line[++x];
	}

	/* Next row */
	dump_row++;
}

/**
 * Hook function - dump a char_attr line to a memory location
 */
void dump_line_mem(char_attr * this_line)
{
	dump_ptr = this_line;
}

/**
 * Dump a char_attr line 
 */
void dump_line(char_attr * this_line)
{
	dump_line_hook(this_line);
}



/**
 * Convert an input from tenths of a pound to tenths of a kilogram. -LM-
 */
int make_metric(int wgt)
{
	int metric_wgt;

	/* Convert to metric values, using normal rounding. */
	metric_wgt = wgt * 10 / 22;
	if ((wgt * 10) % 22 > 10)
		metric_wgt++;

	return metric_wgt;
}

/*
 * Write text to the given file and apply line-wrapping.
 *
 * Hook function for text_out(). Make sure that text_out_file points
 * to an open text-file.
 *
 * Long lines will be wrapped at text_out_wrap, or at column 75 if that
 * is not set; or at a newline character.  Note that punctuation can
 * sometimes be placed one column beyond the wrap limit.
 *
 * You must be careful to end all file output with a newline character
 * to "flush" the stored line position.
 */
void text_out_dump(byte a, char *str, char_attr_line ** line,
				   int *current_line, int indent, int wrap)
{
	const char *s;
	char buf[1024];

	/* Current position on the line */
	int pos = 0;

	char_attr_line *lline = *line;

	/* Copy to a rewriteable string */
	my_strcpy(buf, str, 1024);

	/* Current location within "buf" */
	s = buf;

	/* Process the string */
	while (*s) {
		int n = 0;
		int len = wrap - pos;
		int l_space = -1;

		/* If we are at the start of the line... */
		if (pos == 0) {
			int i;

			/* Output the indent */
			for (i = 0; i < indent; i++) {
				dump_put_str(TERM_WHITE, " ", pos);
				pos++;
			}
		}

		/* Find length of line up to next newline or end-of-string */
		while ((n < len) && !((s[n] == '\n') || (s[n] == '\0'))) {
			/* Mark the most recent space in the string */
			if (s[n] == ' ')
				l_space = n;

			/* Increment */
			n++;
		}

		/* If we have encountered no spaces */
		if ((l_space == -1) && (n == len)) {
			/* If we are at the start of a new line */
			if (pos == indent) {
				len = n;
			}
			/* HACK - Output punctuation at the end of the line */
			else if ((s[0] == ' ') || (s[0] == ',') || (s[0] == '.')) {
				len = 1;
			} else {
				/* Begin a new line */
				(*current_line)++;
				dump_ptr = (char_attr *) & lline[*current_line];

				/* Reset */
				pos = 0;

				continue;
			}
		} else {
			/* Wrap at the newline */
			if ((s[n] == '\n') || (s[n] == '\0'))
				len = n;

			/* Wrap at the last space */
			else
				len = l_space;
		}

		/* Write that line to dump */
		for (n = 0; n < len; n++) {
			/* Write out the character */
			dump_put_str(a, format("%c", s[n]), pos);

			/* Increment */
			pos++;
		}

		/* Move 's' past the stuff we've written */
		s += len;

		/* Begin a new line */
		(*current_line)++;
		dump_ptr = (char_attr *) & lline[*current_line];


		/* If we are at the end of the string, end */
		if (*s == '\0')
			return;

		/* Begin a new line */
		if (*s == '\n') {
			(*current_line)++;
			dump_ptr = (char_attr *) & lline[*current_line];
		}

		/* Reset */
		pos = 0;

		/* Skip whitespace */
		while (*s == ' ')
			s++;
	}

	/* We are done */
	return;
}

void textblock_dump(textblock * tb, char_attr_line ** line,
					int *current_line, int indent, int wrap)
{
	const wchar_t *text = textblock_text(tb);
	const byte *attrs = textblock_attrs(tb);

	size_t *line_starts = NULL, *line_lengths = NULL;
	size_t i, j, n_lines;

	char_attr_line *lline = *line;

	n_lines = textblock_calculate_lines(tb, &line_starts, &line_lengths,
										wrap - indent);

	for (i = 0; i < n_lines; i++) {
		for (j = 0; j < line_lengths[i]; j++) {

			/* Write the character */
			dump_ptr[j].pattr = attrs[line_starts[i] + j];
			dump_ptr[j].pchar = text[line_starts[i] + j];

			/* Paranoia */
			if (j >= MAX_C_A_LEN){
				j--;
				break;
			}
		}

		/* Terminate */
		dump_ptr[j].pchar = L'\0';

		(*current_line)++;
		dump_ptr = (char_attr *) &lline[*current_line];
	}

	if (!n_lines)
		(*current_line)--;
}
