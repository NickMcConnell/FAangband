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
void x_fprintf(ang_file *f, int encoding, cptr fmt, ...)
{
        va_list vp;

        char buf[1024];

        /* Begin the Varargs Stuff */
        va_start(vp, fmt);

        /* Format the args, save the length */
        (void)vstrnfmt(buf, sizeof(buf), fmt, vp);

        /* End the Varargs Stuff */
        va_end(vp);

        /* Translate */
        xstr_trans(buf, encoding);

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
  char *s;
  char buf[1024];
  bool finished = FALSE;

  /* Find the start point */
  while ((i != col) && (i < MAX_C_A_LEN))
    {
      if (dump_ptr[i].pchar == '\0') finished = TRUE;
      if (finished) 
        {
          dump_ptr[i].pchar = ' ';
          dump_ptr[i].pattr = TERM_WHITE;
        }
      i++;
    }
  
  /* Copy to a rewriteable string */
  my_strcpy(buf, str, 1024);

  /* Hack - translate if we do that */
  if (Term->xchar_hook)
    xstr_trans(buf, (Term->xchar_hook(128) == 128));

  /* Current location within "buf" */
  s = buf;

  /* Write the characters */
  while ((*s != '\0') && (i < MAX_C_A_LEN))
    {
      dump_ptr[i].pattr = attr;
      dump_ptr[i++].pchar = *s++;
    }

  /* Paranoia */
  if (i >= MAX_C_A_LEN)
    i--;

  /* Terminate */
  dump_ptr[i].pchar = '\0';
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
  sprintf(out_val, "%9ld", (long)num);
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
  sprintf(out_val, "%6ld", (long)num);
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
  sprintf(out_val, "%6ld", (long)num);
  dump_put_str(color, out_val, col + len);
  sprintf(out_val, ".");
  dump_put_str(color, out_val, col + len + 6);
  sprintf(out_val, "%8ld", (long)deci);
  dump_put_str(color, &out_val[7], col + len + 7);
}


/**
 * Hook function - dump a char_attr line to a file 
 */
void dump_line_file(char_attr *this_line)
{
  int x = 0;
  char_attr xa = this_line[0];
  byte (*old_xchar_hook)(byte c) = Term->xchar_hook;
  char buf[2];

  /* We use either ascii or system-specific encoding */
  int encoding = OPT(xchars_to_file) ? SYSTEM_SPECIFIC : ASCII;

  /* Display the requested encoding -- ASCII or system-specific */
  if (!OPT(xchars_to_file)) Term->xchar_hook = NULL;

  /* Dump the line */
  while (xa.pchar != '\0')
    {
      /* Add the char/attr */
      x_fprintf(dump_out_file, encoding, "%c", xa.pchar);

      /* Advance */
      xa = this_line[++x];
    }

  /* Return to standard display */
  Term->xchar_hook = old_xchar_hook;

  /* Terminate the line */
  buf[0] = '\n';
  buf[1] = '\0';
  file_put(dump_out_file, buf);
}

/**
 * Hook function - dump a char_attr line to the screen 
 */
void dump_line_screen(char_attr *this_line)
{
  int x = 0;
  char_attr xa = this_line[0];

  /* Erase the row */
  Term_erase(0, dump_row, 255);

  /* Dump the line */
  while (xa.pchar != '\0')
    {
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
void dump_line_mem(char_attr *this_line)
{
  dump_ptr = this_line;
}

/**
 * Dump a char_attr line 
 */
void dump_line(char_attr *this_line)
{
  dump_line_hook(this_line);
}



/**
 * Deal with pre-selected items from the item menu
 */
int handle_item(void)
{
  int item;

  /* Set the item */
  item = p_ptr->command_item;
  if (item == 100) item = 0;

  /* Handle repeat */  
  repeat_push(item);

  /* Reset */
  p_ptr->command_item = 0;
  p_ptr->command_new = 0;

  /* Done */
  return(item);
}



/**
 * Convert an input from tenths of a pound to tenths of a kilogram. -LM-
 */
int make_metric(int wgt)
{
  int metric_wgt;
  
  /* Convert to metric values, using normal rounding. */
  metric_wgt = wgt * 10 / 22;
  if ((wgt * 10) % 22 > 10) metric_wgt++;
  
  return metric_wgt;
}

