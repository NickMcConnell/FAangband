/** \file util.c 
    \brief Utility function file

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
#include "randname.h"

#ifdef _WIN32_WCE
#include "angbandcw.h"
#endif /* _WIN32_WCE */



/**
 * Convert a decimal to a single digit hex number
 */
static char hexify(int i)
{
	return (hexsym[i % 16]);
}

/**
 * Convert a hexidecimal-digit into a decimal
 */
static int dehex(char c)
{
	if (isdigit((unsigned char)c)) return (D2I(c));
	if (isalpha((unsigned char)c)) return (A2I(tolower((unsigned char)c)) + 10);
	return (0);
}


/**
 * Transform macro trigger name ('\[alt-D]' etc..)
 * into macro trigger key code ('^_O_64\r' or etc..)
 */
static size_t trigger_text_to_ascii(char *buf, size_t max, cptr *strptr)
{
	cptr str = *strptr;
	bool mod_status[MAX_MACRO_MOD];

	int i, len = 0;
	int shiftstatus = 0;
	cptr key_code;
	
	size_t current_len = strlen(buf);

	/* No definition of trigger names */
	if (macro_template == NULL) return 0;

	/* Initialize modifier key status */	
	for (i = 0; macro_modifier_chr[i]; i++)
		mod_status[i] = FALSE;

	str++;

	/* Examine modifier keys */
	while (1)
	{
		/* Look for modifier key name */
		for (i = 0; macro_modifier_chr[i]; i++)
		{
			len = strlen(macro_modifier_name[i]);

			if (!my_strnicmp(str, macro_modifier_name[i], len))
				break;
		}

		/* None found? */
		if (!macro_modifier_chr[i]) break;

		/* Proceed */
		str += len;

		/* This modifier key is pressed */
		mod_status[i] = TRUE;

		/* Shift key might be going to change keycode */
		if (macro_modifier_chr[i] == 'S')
			shiftstatus = 1;
	}

	/* Look for trigger name */
	for (i = 0; i < max_macrotrigger; i++)
	{
		len = strlen(macro_trigger_name[i]);

		/* Found it and it is ending with ']' */
		if (!my_strnicmp(str, macro_trigger_name[i], len) && (']' == str[len]))
			break;
	}

	/* Invalid trigger name? */
	if (i == max_macrotrigger)
	{
		/*
		 * If this invalid trigger name is ending with ']',
		 * skip whole of it to avoid defining strange macro trigger
		 */
		str = strchr(str, ']');

		if (str)
		{
			strnfcat(buf, max, &current_len, "\x1F\r");

			*strptr = str; /* where **strptr == ']' */
		}

		return current_len;
	}

	/* Get keycode for this trigger name */
	key_code = macro_trigger_keycode[shiftstatus][i];

	/* Proceed */
	str += len;

	/* Begin with '^_' */
	strnfcat(buf, max, &current_len, "\x1F");

	/* Write key code style trigger using template */
	for (i = 0; macro_template[i]; i++)
	{
		char ch = macro_template[i];

		switch (ch)
		{
			/* Modifier key character */
			case '&':
			{
				size_t j;
				for (j = 0; macro_modifier_chr[j]; j++)
				{
					if (mod_status[j])
						strnfcat(buf, max, &current_len, "%c", macro_modifier_chr[j]);
				}
				break;
			}

			/* Key code */
			case '#':
			{
				strnfcat(buf, max, &current_len, "%s", key_code);
				break;
			}

			/* Fixed string */
			default:
			{
				strnfcat(buf, max, &current_len, "%c", ch);
				break;
			}
		}
	}

	/* End with '\r' */
	strnfcat(buf, max, &current_len, "\r");

	/* Succeed */
	*strptr = str; /* where **strptr == ']' */
	
	return current_len;
}


/**
 * Hack -- convert a printable string into real ascii
 *
 * This function will not work on non-ascii systems.
 *
 * To be safe, "buf" should be at least as large as "str".
 */
void text_to_ascii(char *buf, size_t len, cptr str)
{
	char *s = buf;

	/* Analyze the "ascii" string */
	while (*str)
	{
		/* Check if the buffer is long enough */
		if (s >= buf + len - 1) break;

		/* Backslash codes */
		if (*str == '\\')
		{
			str++;
			if (*str == '\0') break;

			switch (*str)
			{
				/* Macro trigger */
				case '[':
				{
					/* Terminate before appending the trigger */
					*s = '\0';
					s += trigger_text_to_ascii(buf, len, &str);
					break;
				}

				/* Hex-mode */
				case 'x':
				{
					if (isxdigit((unsigned char)(*(str + 1))) &&
					    isxdigit((unsigned char)(*(str + 2))))
					{
						*s = 16 * dehex(*++str);
						*s++ += dehex(*++str);
					}
					else
					{
						/* HACK - Invalid hex number */
						*s++ = '?';
					}
					break;
				}

				case 'e':
					*s++ = ESCAPE;
					break;
				case 's':
					*s++ = ' ';
					break;
				case 'b':
					*s++ = '\b';
					break;
				case 'n':
					*s++ = '\n';
					break;
				case 'r':
					*s++ = '\r';
					break;
				case 't':
					*s++ = '\t';
					break;
				case 'a':
					*s++ = '\a';
					break;
				case '\\':
					*s++ = '\\';
					break;
				case '^':
					*s++ = '^';
					break;

				default:
					*s = *str;
					break;
			}

			/* Skip the final char */
			str++;
		}

		/* Normal Control codes */
		else if (*str == '^')
		{
			str++;
			if (*str == '\0') break;

			*s++ = KTRL(*str);
			str++;
		}

		/* Normal chars */
		else
		{
			*s++ = *str++;
		}
	}

	/* Terminate */
	*s = '\0';
}


/**
 * Transform macro trigger key code ('^_O_64\r' or etc..) 
 * into macro trigger name ('\[alt-D]' etc..)
 */
static size_t trigger_ascii_to_text(char *buf, size_t max, cptr *strptr)
{
	cptr str = *strptr;
	char key_code[100];
	int i;
	cptr tmp;
	size_t current_len = strlen(buf);
	

	/* No definition of trigger names */
	if (macro_template == NULL) return 0;

	/* Trigger name will be written as '\[name]' */
	strnfcat(buf, max, &current_len, "\\[");

	/* Use template to read key-code style trigger */
	for (i = 0; macro_template[i]; i++)
	{
		char ch = macro_template[i];

		switch (ch)
		{
			/* Read modifier */
			case '&':
			{
				size_t j;
				while ((tmp = strchr(macro_modifier_chr, *str)) != 0)
				{
					j = tmp - macro_modifier_chr;
					strnfcat(buf, max, &current_len, "%s", macro_modifier_name[j]);
					str++;
				}
				break;
			}

			/* Read key code */
			case '#':
			{
				size_t j;
				for (j = 0; *str && (*str != '\r') && (j < sizeof(key_code) - 1); j++)
					key_code[j] = *str++;
				key_code[j] = '\0';
				break;
			}

			/* Skip fixed strings */
			default:
			{
				if (ch != *str) return 0;
				str++;
			}
		}
	}

	/* Key code style triggers always end with '\r' */
	if (*str++ != '\r') return 0;

	/* Look for trigger name with given keycode (normal or shifted keycode) */
	for (i = 0; i < max_macrotrigger; i++)
	{
		if (!my_stricmp(key_code, macro_trigger_keycode[0][i]) ||
		    !my_stricmp(key_code, macro_trigger_keycode[1][i]))
			break;
	}

	/* Not found? */
	if (i == max_macrotrigger) return 0;

	/* Write trigger name + "]" */
	strnfcat(buf, max, &current_len, "%s]", macro_trigger_name[i]);

	/* Succeed */
	*strptr = str;
	return current_len;
}


/**
 * Hack -- convert a string into a printable form
 *
 * This function will not work on non-ascii systems.
 */
void ascii_to_text(char *buf, size_t len, cptr str)
{
	char *s = buf;

	/* Analyze the "ascii" string */
	while (*str)
	{
		byte i = (byte)(*str++);

		/* Check if the buffer is long enough */
		/* HACK - always assume worst case (hex-value + '\0') */
		if (s >= buf + len - 5) break;

		if (i == ESCAPE)
		{
			*s++ = '\\';
			*s++ = 'e';
		}
		else if (i == ' ')
		{
			*s++ = '\\';
			*s++ = 's';
		}
		else if (i == '\b')
		{
			*s++ = '\\';
			*s++ = 'b';
		}
		else if (i == '\t')
		{
			*s++ = '\\';
			*s++ = 't';
		}
		else if (i == '\a')
		{
			*s++ = '\\';
			*s++ = 'a';
		}
		else if (i == '\n')
		{
			*s++ = '\\';
			*s++ = 'n';
		}
		else if (i == '\r')
		{
			*s++ = '\\';
			*s++ = 'r';
		}
		else if (i == '\\')
		{
			*s++ = '\\';
			*s++ = '\\';
		}
		else if (i == '^')
		{
			*s++ = '\\';
			*s++ = '^';
		}
		/* Macro Trigger */
		else if (i == 31)
		{
			size_t offset;

			/* Terminate before appending the trigger */
			*s = '\0';

			offset = trigger_ascii_to_text(buf, len, &str);
			
			if (offset == 0)
			{
				/* No trigger found */
				*s++ = '^';
				*s++ = '_';
			}
			else
				s += offset;
		}
		else if (i < 32)
		{
			*s++ = '^';
			*s++ = UN_KTRL(i);
		}
		else if (i < 127)
		{
			*s++ = i;
		}
		else
		{
			*s++ = '\\';
			*s++ = 'x';
			*s++ = hexify((int)i / 16);
			*s++ = hexify((int)i % 16);
		}
	}

	/* Terminate */
	*s = '\0';
}



/**
 * The "macro" package
 *
 * Functions are provided to manipulate a collection of macros, each
 * of which has a trigger pattern string and a resulting action string
 * and a small set of flags.
 */



/**
 * Determine if any macros have ever started with a given character.
 */
static bool macro__use[256];


/**
 * Find the macro (if any) which exactly matches the given pattern
 */
int macro_find_exact(cptr pat)
{
	int i;

        /* Nothing possible */
        if (!macro__use[(byte)(pat[0])])
                return -1;

        /* Scan the macros */
        for (i = 0; i < macro__num; ++i)
        {
                if (streq(macro__pat[i], pat))
                        return i;
        }

        /* No matches */
        return -1;
}


/**
 * Find the first macro (if any) which contains the given pattern
 */
static int macro_find_check(cptr pat)
{
	int i;

        /* Nothing possible */
        if (!macro__use[(byte)(pat[0])])
                return -1;

        /* Scan the macros */
        for (i = 0; i < macro__num; ++i)
        {
                if (prefix(macro__pat[i], pat))
                        return i;
        }

        /* Nothing */
        return -1;
}


/**
 * Find the first macro (if any) which contains the given pattern and more
 */
static int macro_find_maybe(cptr pat)
{
	int i;

        /* Nothing possible */
        if (!macro__use[(byte)(pat[0])])
                return -1;

        /* Scan the macros */
        for (i = 0; i < macro__num; ++i)
        {
                if (prefix(macro__pat[i], pat) && !streq(macro__pat[i], pat))
                        return i;
        }

        /* Nothing */
        return -1;
}



/**
 * Find the longest macro (if any) which starts with the given pattern
 */
static int macro_find_ready(cptr pat)
{
	int i, t, n = -1, s = -1;

        /* Nothing possible */
        if (!macro__use[(byte)(pat[0])])
                return -1;

        /* Scan the macros */
        for (i = 0; i < macro__num; ++i)
	{
		/* Skip macros which are not contained by the pattern */
		if (!prefix(pat, macro__pat[i])) continue;

		/* Obtain the length of this macro */
		t = strlen(macro__pat[i]);

		/* Only track the "longest" pattern */
		if ((n >= 0) && (s > t)) continue;

		/* Track the entry */
		n = i;
		s = t;
        }

        /* Result */
        return n;
}


/**
 * Add a macro definition (or redefinition).
 *
 * We should use "act == NULL" to "remove" a macro, but this might make it
 * impossible to save the "removal" of a macro definition.  XXX XXX XXX
 *
 * We should consider refusing to allow macros which contain existing macros,
 * or which are contained in existing macros, because this would simplify the
 * macro analysis code.  XXX XXX XXX
 *
 * We should consider removing the "command macro" crap, and replacing it
 * with some kind of "powerful keymap" ability, but this might make it hard
 * to change the "roguelike" option from inside the game.  XXX XXX XXX
 */
errr macro_add(cptr pat, cptr act)
{
        int n;

        if (!pat || !act) return (-1);


	/* Look for any existing macro */
	n = macro_find_exact(pat);

        /* Replace existing macro */
        if (n >= 0)
        {
                string_free(macro__act[n]);
        }

	/* Create a new macro */
	else
        {
                /* Get a new index */
                n = macro__num++;
                if (macro__num >= MACRO_MAX) quit("Too many macros!");

                /* Save the pattern */
		macro__pat[n] = string_make(pat);
	}

	/* Save the action */
	macro__act[n] = string_make(act);

	/* Efficiency */
	macro__use[(byte)(pat[0])] = TRUE;

	/* Success */
	return (0);
}



/**
 * Initialize the "macro" package
 */
errr macro_init(void)
{
        /* Macro patterns */
        macro__pat = C_ZNEW(MACRO_MAX, cptr);

        /* Macro actions */
        macro__act = C_ZNEW(MACRO_MAX, cptr);

        /* Success */
        return (0);
}


/**
 * Free the macro package
 */
errr macro_free(void)
{
        int i;
        size_t j;

        /* Free the macros */
        for (i = 0; i < macro__num; ++i)
	{
		string_free(macro__pat[i]);
                string_free(macro__act[i]);
        }

        FREE(macro__pat);
        FREE(macro__act);

        /* Free the keymaps */
        for (i = 0; i < KEYMAP_MODES; ++i)
        {
                for (j = 0; j < N_ELEMENTS(keymap_act[i]); ++j)
                {
                        string_free(keymap_act[i][j]);
                        keymap_act[i][j] = NULL;
		}
	}

	/* Success */
	return (0);
}


/**
 * Free the macro trigger package
 */
errr macro_trigger_free(void)
{
	int i;
	int num;

	if (macro_template != NULL)
	{
		/* Free the template */
		string_free(macro_template);
		macro_template = NULL;

		/* Free the trigger names and keycodes */
		for (i = 0; i < max_macrotrigger; i++)
		{
			string_free(macro_trigger_name[i]);

			string_free(macro_trigger_keycode[0][i]);
			string_free(macro_trigger_keycode[1][i]);
		}

		/* No more macro triggers */
		max_macrotrigger = 0;

		/* Count modifier-characters */
		num = strlen(macro_modifier_chr);

                /* Free modifier names */
                for (i = 0; i < num; i++)
                        string_free(macro_modifier_name[i]);

                /* Free modifier chars */
                string_free(macro_modifier_chr);
        }

        /* Success */
	return (0);
}


/**
 * Flush all pending input.
 *
 * Actually, remember the flush, using the "inkey_xtra" flag, and in the
 * next call to "inkey()", perform the actual flushing, for efficiency,
 * and correctness of the "inkey()" function.
 */
void flush(void)
{
	/* Do it later */
	inkey_xtra = TRUE;
}


/**
 * Flush all pending input if the flush_failure option is set.
 */
void flush_fail(void)
{
	if (flush_failure) flush();
}


/**
 * Local variable -- we are inside a "macro action"
 *
 * Do not match any macros until "ascii 30" is found.
 */
static bool parse_macro = FALSE;


/**
 * Local variable -- we are inside a "macro trigger"
 *
 * Strip all keypresses until a low ascii value is found.
 */
static bool parse_under = FALSE;

/**
 * The mousebutton code. Buttons should be created when neccessary and 
 * destroyed when no longer necessary.  By default, buttons occupy the section
 * of the bottom line between the status display and the location display
 * in normal screen mode, and the bottom line after any prompt in alternate
 * screen mode.
 *
 * Individual ports may (and preferably will) handle this differently using
 * add_button_gui and kill_button_gui.
 */

/**
 * Add a button 
 */
int add_button_text(char *label, unsigned char keypress)
{
  int i;
  int length = strlen(label) + 2;
  int button_start = (normal_screen ? status_end : prompt_end);
  int button_end = (normal_screen ? depth_start : Term->wid - 2);

  /* Check the label length */
  if (length > MAX_MOUSE_LABEL) 
    {
      return 0;
    }

  /* Check we haven't already got a button for this keypress */
  for (i = 0; i < num_buttons; i++)
    if (mse_button[i].key == keypress)
      return 0;

  /* Check we haven't run out of room */
  button_length += length;
  if (button_length + button_start > button_end) 
    {
      button_length -= length;
      return 0;
    }

  /* Make the button */
  strncpy(mse_button[num_buttons].label, label, MAX_MOUSE_LABEL);
  mse_button[num_buttons].left  = button_length;
  mse_button[num_buttons].right = button_length - length + 1;
  mse_button[num_buttons++].key = keypress;

  /* Redraw */
  p_ptr->redraw |= (PR_BUTTONS | PR_DEPTH);
  redraw_stuff();

  /* Return the size of the button */
  return (length);
}

/**
 * Add a button 
 */
int add_button(char *label, unsigned char keypress)
{
  if (!add_button_hook) return 0;
  else return (*add_button_hook)(label, keypress);
}

/**
 * Make a backup of thr current buttons
 */
void backup_buttons_text(void)
{
  /* Check we haven't already done this */
  if (backup_button[0].key) return;

  /* Straight memory copy */
  (void)C_COPY(backup_button, mse_button, MAX_MOUSE_BUTTONS, mouse_button);
}

void backup_buttons(void)
{
  if (backup_buttons_hook) 
  (*backup_buttons_hook)();
}

/**
 * Restore the buttons from backup
 */
void restore_buttons_text(void)
{
  int i = 0;

  /* Remove the current lot */
  kill_all_buttons();

  /* Get all the previous buttons, copy them back */
  while (backup_button[i].key)
    {
      /* Add them all back, forget the backups */
      add_button(backup_button[i].label, backup_button[i].key);
      backup_button[i].key = '\0';
      i++;
    }
}

void restore_buttons(void)
{
  if (restore_buttons_hook) 
  (*restore_buttons_hook)();
}
   

/**
 * Remove a button
 */
int kill_button_text(unsigned char keypress)
{
  int i, j, length;

  /* Find the button */
  for (i = 0; i < num_buttons; i++)
    if (mse_button[i].key == keypress) break;

  /* No such button */
  if (i == num_buttons)
    {
      return 0;
    }

  /* Find the length */
  length = mse_button[i].left - mse_button[i].right + 1;
  button_length -= length;

  /* Move each button up one */
  for (j = i; j < num_buttons - 1; j++)
    {
      mse_button[j] = mse_button[j+1];

      /* Adjust length */
      mse_button[j].left -= length;
      mse_button[j].right -= length;
    }

  /* Wipe the data */
  mse_button[num_buttons - 1].label[0] = '\0';
  mse_button[num_buttons - 1].left  = 0;
  mse_button[num_buttons - 1].right = 0;
  mse_button[num_buttons - 1].key = 0;
  num_buttons--;

  /* Redraw */
  p_ptr->redraw |= (PR_BUTTONS | PR_DEPTH);
  redraw_stuff();

  /* Return the size of the button */
  return (length);
}

/**
 * Kill a button 
 */
int kill_button(unsigned char keypress)
{
  if (!kill_button_hook) return 0;
  else return (*kill_button_hook)(keypress);
}

/**
 * Kill all buttons (use sparingly!) 
 */
void kill_all_buttons_text(void)
{
  int i;

  /* Paranoia */
  if (!kill_button_hook) return;

  /* One by one */
  for (i = num_buttons - 1; i >= 0; i--)
    (void)(*kill_button_hook)(mse_button[i].key);
}

void kill_all_buttons(void)
{
  if (kill_all_buttons_hook)
  (*kill_all_buttons_hook)();
}

/**
 * Helper function called only from "inkey()"
 *
 * This function does almost all of the "macro" processing.
 *
 * We use the "Term_key_push()" function to handle "failed" macros, as well
 * as "extra" keys read in while choosing the proper macro, and also to hold
 * the action for the macro, plus a special "ascii 30" character indicating
 * that any macro action in progress is complete.  Embedded macros are thus
 * illegal, unless a macro action includes an explicit "ascii 30" character,
 * which would probably be a massive hack, and might break things.
 *
 * Only 500 (0+1+2+...+29+30) milliseconds may elapse between each key in
 * the macro trigger sequence.  If a key sequence forms the "prefix" of a
 * macro trigger, 500 milliseconds must pass before the key sequence is
 * known not to be that macro trigger.  XXX XXX XXX
 */
static event_type inkey_aux(int scan_cutoff)
{
  int k = 0, n, p = 0, w = 0;
  
  event_type ke = EVENT_EMPTY;
  event_type ke0 = EVENT_EMPTY;
  char ch;
  
  cptr pat, act;
	
  char buf[1024];
  
  /* Wait for a keypress */
  if (scan_cutoff == SCAN_OFF)
    {
      (void)(Term_inkey(&ke, TRUE, TRUE));
      ch = ke.key;
    }
  else
    {
      w = 0;
      
      /* Wait only as long as macro activation would wait*/
      while (Term_inkey(&ke, FALSE, TRUE) != 0)
	{
	  /* Increase "wait" */
	  w++;
	  
	  /* Excessive delay */
	  if (w >= scan_cutoff)
	    {
	      ke0.type = EVT_KBRD;
	      return ke0;
	    }
	  
	  /* Delay */
	  Term_xtra(TERM_XTRA_DELAY, 10);
	}
      ch = ke.key;
    }
  
  
  /* End "macro action" */
  if ((ch == 30) || (ch == '\xff'))
	{
		parse_macro = FALSE;
		return (ke);
	}
	
	/* Inside "macro action" */
	if (parse_macro) return (ke);
	
	/* Inside "macro trigger" */
	if (parse_under) return (ke);
	

	/* Save the first key, advance */
	buf[p++] = ch;
	buf[p] = '\0';
	
	
	/* Check for possible macro */
	k = macro_find_check(buf);
	
	/* No macro pending */
	if (k < 0) return (ke);
	
	
	/* Wait for a macro, or a timeout */
	while (TRUE)
	{
		/* Check for pending macro */
		k = macro_find_maybe(buf);
		
		/* No macro pending */
		if (k < 0) break;
		
		/* Check for (and remove) a pending key */
		if (0 == Term_inkey(&ke, FALSE, TRUE))
		{
			/* Append the key */
			buf[p++] = ke.key;
			buf[p] = '\0';
		
			/* Restart wait */
			w = 0;
		}
		
		/* No key ready */
      else
        {
          /* Increase "wait" */
                        w ++;
          
          /* Excessive delay */
                        if (w >= SCAN_MACRO) break;
          
          /* Delay */
                        Term_xtra(TERM_XTRA_DELAY, 10);
        }
    }
  
	
	/* Check for available macro */
	k = macro_find_ready(buf);

	/* No macro available */
	if (k < 0)
	{
		/* Push all the "keys" back on the queue */
		/* The most recent event may not be a keypress. */
		if(p)
		{
			if(Term_event_push(&ke)) return (ke0);
			p--;
		}
		while (p > 0)
		{
			/* Push the key, notice over-flow */
			if (Term_key_push(buf[--p])) return (ke0);
		}
		
		/* Wait for (and remove) a pending key */
		(void)Term_inkey(&ke, TRUE, TRUE);
		
		/* Return the key */
		return (ke);
	}
	
	
	/* Get the pattern */
	pat = macro__pat[k];
	
	/* Get the length of the pattern */
	n = strlen(pat);
	
	/* Push the "extra" keys back on the queue */
	while (p > n)
	{
		/* Push the key, notice over-flow */
		if (Term_key_push(buf[--p])) return (ke0);
	}
	
	
	/* Begin "macro action" */
	parse_macro = TRUE;
	
	/* Push the "end of macro action" key */
	if (Term_key_push(30)) return (ke0);
	
	
	/* Access the macro action */
	act = macro__act[k];
	
	/* Get the length of the action */
	n = strlen(act);
	
	/* Push the macro "action" onto the key queue */
	while (n > 0)
	{
		/* Push the key, notice over-flow */
		if (Term_key_push(act[--n])) return (ke0);
	}
	
	
	/* Hack -- Force "inkey()" to call us again */
	return (ke0);
}



/**
 * Mega-Hack -- special "inkey_next" pointer.  XXX XXX XXX
 *
 * This special pointer allows a sequence of keys to be "inserted" into
 * the stream of keys returned by "inkey()".  This key sequence will not
 * trigger any macros, and cannot be bypassed by the Borg.  It is used
 * in Angband to handle "keymaps".
 */
static cptr inkey_next = NULL;


#ifdef ALLOW_BORG

/**
 * Mega-Hack -- special "inkey_hack" hook.  XXX XXX XXX
 *
 * This special function hook allows the "Borg" (see elsewhere) to take
 * control of the "inkey()" function, and substitute in fake keypresses.
 */
char (*inkey_hack)(int flush_first) = NULL;

#endif /* ALLOW_BORG */


/**
 * Get a keypress from the user.
 *
 * This function recognizes a few "global parameters".  These are variables
 * which, if set to TRUE before calling this function, will have an effect
 * on this function, and which are always reset to FALSE by this function
 * before this function returns.  Thus they function just like normal
 * parameters, except that most calls to this function can ignore them.
 *
 * If "inkey_xtra" is TRUE, then all pending keypresses will be flushed,
 * and any macro processing in progress will be aborted.  This flag is
 * set by the "flush()" function, which does not actually flush anything
 * itself, but rather, triggers delayed input flushing via "inkey_xtra".
 *
 * If "inkey_scan" is TRUE, then we will immediately return "zero" if no
 * keypress is available, instead of waiting for a keypress.
 *
 * If "inkey_base" is TRUE, then all macro processing will be bypassed.
 * If "inkey_base" and "inkey_scan" are both TRUE, then this function will
 * not return immediately, but will wait for a keypress for as long as the
 * normal macro matching code would, allowing the direct entry of macro
 * triggers.  The "inkey_base" flag is extremely dangerous!
 *
 * If "inkey_flag" is TRUE, then we will assume that we are waiting for a
 * normal command, and we will only show the cursor if "hilite_player" is
 * TRUE (or if the player is in a store), instead of always showing the
 * cursor.  The various "main-xxx.c" files should avoid saving the game
 * in response to a "menu item" request unless "inkey_flag" is TRUE, to
 * prevent savefile corruption.
 *
 * If we are waiting for a keypress, and no keypress is ready, then we will
 * refresh (once) the window which was active when this function was called.
 *
 * Note that "back-quote" is automatically converted into "escape" for
 * convenience on machines with no "escape" key.  This is done after the
 * macro matching, so the user can still make a macro for "backquote".
 *
 * Note the special handling of "ascii 30" (ctrl-caret, aka ctrl-shift-six)
 * and "ascii 31" (ctrl-underscore, aka ctrl-shift-minus), which are used to
 * provide support for simple keyboard "macros".  These keys are so strange
 * that their loss as normal keys will probably be noticed by nobody.  The
 * "ascii 30" key is used to indicate the "end" of a macro action, which
 * allows recursive macros to be avoided.  The "ascii 31" key is used by
 * some of the "main-xxx.c" files to introduce macro trigger sequences.
 *
 * Hack -- we use "ascii 29" (ctrl-right-bracket) as a special "magic" key,
 * which can be used to give a variety of "sub-commands" which can be used
 * any time.  These sub-commands could include commands to take a picture of
 * the current screen, to start/stop recording a macro action, etc.
 *
 * If "angband_term[0]" is not active, we will make it active during this
 * function, so that the various "main-xxx.c" files can assume that input
 * is only requested (via "Term_inkey()") when "angband_term[0]" is active.
 *
 * Mega-Hack -- This function is used as the entry point for clearing the
 * "signal_count" variable, and of the "character_saved" variable.
 *
 * Hack -- Note the use of "inkey_next" to allow "keymaps" to be processed.
 *
 * Mega-Hack -- Note the use of "inkey_hack" to allow the "Borg" to steal
 * control of the keyboard from the user.
 */
event_type inkey_ex(void)
{
  bool cursor_state;
  event_type kk = EVENT_EMPTY;
  event_type ke = EVENT_EMPTY;
  
  bool done = FALSE;
  
  term *old = Term;
  
  /* Hack -- Use the "inkey_next" pointer */
  if (inkey_next && *inkey_next && !inkey_xtra)
    {
      /* Get next character, and advance */
      ke.key = *inkey_next++;
      ke.type = EVT_KBRD;
      
      /* Cancel the various "global parameters" */
      inkey_base = inkey_xtra = inkey_flag = FALSE;
      inkey_scan = 0;
      
      /* Accept result */
      return (ke);
    }
  
  /* Forget pointer */
  inkey_next = NULL;
  
#ifdef ALLOW_BORG
  
  /* Mega-Hack -- Use the special hook */
  if (inkey_hack && ((ch = (*inkey_hack)(inkey_xtra)) != 0))
    {
      /* Cancel the various "global parameters" */
      inkey_base = inkey_xtra = inkey_flag = FALSE;
      inkey_scan = 0;
      ke.type = EVT_KBRD;
      
      /* Accept result */
      return (ke);
    }
  
#endif /* ALLOW_BORG */
  
  /* Hack -- handle delayed "flush()" */
  if (inkey_xtra)
    {
      /* End "macro action" */
      parse_macro = FALSE;
      
      /* End "macro trigger" */
      parse_under = FALSE;
      
		/* Forget old keypresses */
      Term_flush();
    }
  
  
  /* Get the cursor state */
  (void)Term_get_cursor(&cursor_state);
  
  /* Show the cursor if waiting, except sometimes in "command" mode */
  if (!inkey_scan && (!inkey_flag || hilite_player || character_icky))
    {
      /* Show the cursor */
      (void)Term_set_cursor(TRUE);
    }
  

  /* Hack -- Activate main screen */
  Term_activate(term_screen);
  
  
  /* Get a key */
  while (ke.type == EVT_NONE)
    {
      /* Hack -- Handle "inkey_scan == SCAN_INSTANT */
      if (!inkey_base && inkey_scan == SCAN_INSTANT &&
          (0 != Term_inkey(&kk, FALSE, FALSE)))
        {
	  ke.type = EVT_KBRD;
	  break;
        }
      
      
      /* Hack -- Flush output once when no key ready */
      if (!done && (0 != Term_inkey(&kk, FALSE, FALSE)))
	{
	  
	  /* Hack -- activate proper term */
	  Term_activate(old);
	  
	  /* Flush output */
	  Term_fresh();
	  
	  /* Hack -- activate main screen */
	  Term_activate(term_screen);
	  
	  /* Mega-Hack -- reset saved flag */
	  character_saved = FALSE;
	  
	  /* Mega-Hack -- reset signal counter */
	  signal_count = 0;
	  
	  /* Only once */
	  done = TRUE;
	}
      
      
      /* Hack -- Handle "inkey_base" */
      if (inkey_base)
	{
	  int w = 0;
	  
	  /* Wait forever */
	  if (!inkey_scan)
	    {
	      /* Wait for (and remove) a pending key */
	      if (0 == Term_inkey(&ke, TRUE, TRUE))
		{
		  /* Done */
		  ke.type = EVT_KBRD;
		  break;
		}
	      
	      /* Oops */
	      break;
	    }
	  
	  /* Wait only as long as macro activation would wait*/
	  while (TRUE)
	    {
	      /* Check for (and remove) a pending key */
	      if (0 == Term_inkey(&ke, FALSE, TRUE))
		{
		  /* Done */
		  ke.type = EVT_KBRD;
		  break;
		}
	      
	      /* No key ready */
              else
                {
                  /* Increase "wait" */
		  w ++;
                  
                  /* Excessive delay */
		  if (w >= SCAN_MACRO) break;
                  
                  /* Delay */
		  Term_xtra(TERM_XTRA_DELAY, 10);
                  
                }
            }
	  
	  /* Done */
	  ke.type = EVT_KBRD;
          break;
        }
      
      
      /* Get a key (see above) */
      ke = inkey_aux(inkey_scan);
      
      /* Handle mouse buttons */
      if ((ke.type == EVT_MOUSE) && (mouse_buttons))
        {
          int i;
          int button_end = (normal_screen ? depth_start : Term->wid - 2);
	  
          /* Check to see if we've hit a button */
          /* Assuming text buttons here for now - this would have to
           * change for GUI buttons */
          for (i = 0; i < num_buttons; i++)
            if ((ke.mousey == Term->hgt - 1) &&
                (ke.mousex >= button_end - mse_button[i].left) &&
                (ke.mousex <= button_end - mse_button[i].right))
              {
                /* Rewrite the event */
                ke.type = EVT_KBRD;
                ke.key = mse_button[i].key;
                ke.index = 0;
                ke.mousey = 0;
                ke.mousex = 0;
		
		/* Done */
		break;
              }
        }
      
      /* Handle "control-right-bracket" */
      if (ke.key == 29)
        {
          /* Strip this key */
	  ke.type = EVT_NONE;
          
          /* Continue */
          continue;
	}

      
      /* Treat back-quote as escape */
      if (ke.key == '`') ke.key = ESCAPE;
      
      
      /* End "macro trigger" */
      if (parse_under && (ke.key >=0 && ke.key <= 32))
        {
          /* Strip this key */
	  ke.type = EVT_NONE;
          ke.key = 0;
          
          /* End "macro trigger" */
	  parse_under = FALSE;
	}
      
      /* Handle "control-caret" */
      if (ke.key == 30)
	{
	  /* Strip this key */
	  ke.type = EVT_NONE;
	  ke.key = 0;
	}
      
      /* Handle "control-underscore" */
      else if (ke.key == 31)
	{
	  /* Strip this key */
	  ke.type = EVT_NONE;
	  ke.key = 0;
	  
	  /* Begin "macro trigger" */
	  parse_under = TRUE;
	}
      
      /* Inside "macro trigger" */
      else if (parse_under)
	{
	  /* Strip this key */
	  ke.type = EVT_NONE;
	  ke.key = 0;
	}
    }
  
  
  /* Hack -- restore the term */
  Term_activate(old);
  
  
  /* Restore the cursor */
  Term_set_cursor(cursor_state);
  
  
  /* Cancel the various "global parameters" */
  inkey_base = inkey_xtra = inkey_flag = FALSE;
  inkey_scan = 0;
  
  /* Return the keypress */
  return (ke);
}


/**
 * Get a keypress or mouse click from the user.
 */
char anykey(void)
{
  event_type ke = EVENT_EMPTY;
  
  /* Only accept a keypress or mouse click*/
  do
    {
      ke = inkey_ex();
    } while (!(ke.type & (EVT_MOUSE|EVT_KBRD)));
  
  return ke.key;
}

/**
 * Get a "keypress" from the user.
 */
char inkey(void)
{
  event_type ke = EVENT_EMPTY;
  
  /* Only accept a keypress */
  do
    {
      ke = inkey_ex();
    } while ((ke.type != EVT_KBRD) && (ke.type != EVT_ESCAPE));
  /* Paranoia */
  if(ke.type == EVT_ESCAPE) ke.key = ESCAPE;
  
  return ke.key;
}



/**
 * We are delaying message display
 */
static bool must_more = FALSE;


/**
 * Flush the screen, make a noise
 */
void bell(cptr reason)
{
  /* Mega-Hack -- Flush the output */
  Term_fresh();
  
  /* Hack -- memorize the reason if possible */
  if (character_generated && reason && !must_more)
    {
      message_add(reason, MSG_BELL);
      
      /* Window stuff */
      p_ptr->window |= (PW_MESSAGE);
      
      /* Force window redraw */
      window_stuff();
    }
  
  /* Make a bell noise (if allowed) */
  if (ring_bell) Term_xtra(TERM_XTRA_NOISE, 0);
  
  /* Flush the input (later!) */
  flush();
}



/**
 * Hack -- Make a (relevant?) sound
 */
void sound(int val)
{
  /* No sound */
  if (!use_sound) return;
  
  /* Make a noise */
  if (sound_hook)
    sound_hook(val);
}




/*
 * The "quark" package
 *
 * This package is used to reduce the memory usage of object inscriptions.
 *
 * We use dynamic string allocation because otherwise it is necessary to
 * pre-guess the amount of quark activity.  We limit the total number of
 * quarks, but this is much easier to "expand" as needed.  XXX XXX XXX
 *
 * Two objects with the same inscription will have the same "quark" index.
 *
 * Some code uses "zero" to indicate the non-existance of a quark.
 *
 * Note that "quark zero" is NULL and should never be "dereferenced".
 *
 * ToDo: Add reference counting for quarks, so that unused quarks can
 * be overwritten.
 *
 * ToDo: Automatically resize the array if necessary.
 */


/**
 * The number of quarks (first quark is NULL)
 */
static s16b quark__num = 1;


/**
 * The array[QUARK_MAX] of pointers to the quarks
 */
static char **quark__str;


/**
 * Add a new "quark" to the set of quarks.
 */
s16b quark_add(cptr str)
{
        int i;

        /* Look for an existing quark */
        for (i = 1; i < quark__num; i++)
        {
                /* Check for equality */
                if (streq(quark__str[i], str)) return (i);
        }

        /* Hack -- Require room XXX XXX XXX */
        if (quark__num == QUARK_MAX) return (0);

        /* New quark */
        i = quark__num++;

        /* Add a new quark */
        quark__str[i] = string_make(str);

        /* Return the index */
        return (i);
}


/**
 * This function looks up a quark
 */
char *quark_str(s16b i)
{
        char *q;

        /* Verify */
        if ((i < 0) || (i >= quark__num)) i = 0;

        /* Get the quark */
        q = quark__str[i];

        /* Return the quark */
        return (q);
}


/**
 * Initialize the "quark" package
 */
errr quarks_init(void)
{
        /* Quark variables */
        C_MAKE(quark__str, QUARK_MAX, cptr);

        /* Success */
        return (0);
}


/**
 * Free the "quark" package
 */
errr quarks_free(void)
{
        int i;

        /* Free the "quarks" */
        for (i = 1; i < quark__num; i++)
        {
                string_free(quark__str[i]);
        }

        /* Free the list of "quarks" */
        FREE(quark__str);

        /* Success */
        return (0);
}


/*
 * The "message memorization" package.
 *
 * Each call to "message_add(s)" will add a new "most recent" message
 * to the "message recall list", using the contents of the string "s".
 *
 * The number of memorized messages is available as "message_num()".
 *
 * Old messages can be retrieved by "message_str(age)", where the "age"
 * of the most recently memorized message is zero, and the oldest "age"
 * which is available is "message_num() - 1".  Messages outside this
 * range are returned as the empty string.
 *
 * The messages are stored in a special manner that maximizes "efficiency",
 * that is, we attempt to maximize the number of semi-sequential messages
 * that can be retrieved, given a limited amount of storage space, without
 * causing the memorization of new messages or the recall of old messages
 * to be too expensive.
 *
 * We keep a buffer of chars to hold the "text" of the messages, more or
 * less in the order they were memorized, and an array of offsets into that
 * buffer, representing the actual messages, but we allow the "text" to be
 * "shared" by two messages with "similar" ages, as long as we never cause
 * sharing to reach too far back in the the buffer.
 *
 * The implementation is complicated by the fact that both the array of
 * offsets, and the buffer itself, are both treated as "circular arrays"
 * for efficiency purposes, but the strings may not be "broken" across
 * the ends of the array.
 *
 * When we want to memorize a new message, we attempt to "reuse" the buffer
 * space by checking for message duplication within the recent messages.
 *
 * Otherwise, if we need more buffer space, we grab a full quarter of the
 * total buffer space at a time, to keep the reclamation code efficient.
 *
 * The "message_add()" function is rather "complex", because it must be
 * extremely efficient, both in space and time, for use with the Borg.
 */


/**
 * The next "free" index to use
 */
static u16b message__next;

/**
 * The index of the oldest message (none yet)
 */
static u16b message__last;

/**
 * The next "free" offset
 */
static u16b message__head;

/**
 * The offset to the oldest used char (none yet)
 */
static u16b message__tail;

/**
 * The next message to display for the easy_more code (none yet)
 */
static u16b message__easy;

/**
 * The array[MESSAGE_MAX] of offsets, by index
 */
static u16b *message__ptr;

/**
 * The array[MESSAGE_BUF] of chars, by offset
 */
static char *message__buf;

/**
 * The array[MESSAGE_MAX] of u16b for the types of messages
 */
static u16b *message__type;

/**
 * The array[MESSAGE_MAX] of u16b for the count of messages
 */
static u16b *message__count;


/**
 * Table of colors associated to message-types
 */
static byte message__color[MSG_MAX];


/**
 * Calculate the index of a message
 */
static s16b message_age2idx(int age)
{
        return ((message__next + MESSAGE_MAX - (age + 1)) % MESSAGE_MAX);
}


/**
 * How many messages are "available"?
 */
s16b message_num(void)
{
        /* Determine how many messages are "available" */
        return (message_age2idx(message__last - 1));
}



/**
 * Recall the "text" of a saved message
 */
cptr message_str(s16b age)
{
        static char buf[1024];
        s16b x;
        u16b o;
        cptr s;

        /* Forgotten messages have no text */
        if ((age < 0) || (age >= message_num())) return ("");

        /* Get the "logical" index */
        x = message_age2idx(age);

        /* Get the "offset" for the message */
        o = message__ptr[x];

        /* Get the message text */
        s = &message__buf[o];

        /* HACK - Handle repeated messages */
        if (message__count[x] > 1)
        {
                strnfmt(buf, sizeof(buf), "%s <%dx>", s, message__count[x]);
                s = buf;
        }

        /* Return the message text */
        return (s);
}


/**
 * Recall the "type" of a saved message
 */
u16b message_type(s16b age)
{
        s16b x;

        /* Paranoia */
        if (!message__type) return (MSG_GENERIC);

        /* Forgotten messages are generic */
        if ((age < 0) || (age >= message_num())) return (MSG_GENERIC);

        /* Get the "logical" index */
        x = message_age2idx(age);

        /* Return the message type */
        return (message__type[x]);
}


/**
 * Recall the "color" of a message type
 */
static byte message_type_color(u16b type)
{
        byte color = message__color[type];

        if (color == TERM_DARK) color = TERM_WHITE;

        return (color);
}


/**
 * Recall the "color" of a saved message
 */
byte message_color(s16b age)
{
        return message_type_color(message_type(age));
}


errr message_color_define(u16b type, byte color)
{
        /* Ignore illegal types */
        if (type >= MSG_MAX) return (1);

        /* Store the color */
        message__color[type] = color;

        /* Success */
        return (0);
}


/**
 * Add a new message, with great efficiency
 *
 * We must ignore long messages to prevent internal overflow, since we
 * assume that we can always get enough space by advancing "message__tail"
 * by one quarter the total buffer space.
 *
 * We must not attempt to optimize using a message index or buffer space
 * which is "far away" from the most recent entries, or we will lose a lot
 * of messages when we "expire" the old message index and/or buffer space.
 */
void message_add(cptr str, u16b type)
{
        int k, i, x, o;
        size_t n;

        cptr s;

        cptr u;
        char *v;


        /*** Step 1 -- Analyze the message ***/

        /* Hack -- Ignore "non-messages" */
        if (!str) return;

        /* Message length */
        n = strlen(str);

        /* Hack -- Ignore "long" messages */
        if (n >= MESSAGE_BUF / 4) return;


        /*** Step 2 -- Attempt to optimize ***/

        /* Get the "logical" last index */
        x = message_age2idx(0);

        /* Get the "offset" for the last message */
        o = message__ptr[x];

        /* Get the message text */
        s = &message__buf[o];

        /* Last message repeated? */
        if (streq(str, s))
        {
                /* Increase the message count */
                message__count[x]++;

                /* Success */
                return;
        }

        /*** Step 3 -- Attempt to optimize ***/

        /* Limit number of messages to check */
        k = message_num() / 4;

        /* Limit number of messages to check */
        if (k > 32) k = 32;

        /* Start just after the most recent message */
        i = message__next;

        /* Check the last few messages for duplication */
        for ( ; k; k--)
        {
                u16b q;

                cptr old;

                /* Back up, wrap if needed */
                if (i-- == 0) i = MESSAGE_MAX - 1;

                /* Stop before oldest message */
                if (i == message__last) break;

                /* Index */
                o = message__ptr[i];

                /* Extract "distance" from "head" */
                q = (message__head + MESSAGE_BUF - o) % MESSAGE_BUF;

                /* Do not optimize over large distances */
                if (q >= MESSAGE_BUF / 4) continue;

                /* Get the old string */
                old = &message__buf[o];

                /* Continue if not equal */
                if (!streq(str, old)) continue;

                /* Get the next available message index */
                x = message__next;

                /* Advance 'message__next', wrap if needed */
                if (++message__next == MESSAGE_MAX) message__next = 0;

                /* Kill last message if needed */
                if (message__next == message__last)
                {
                        /* Update the 'message__easy', wrap if needed */
                        if ((message__last == message__easy) && 
                            (++message__last == MESSAGE_MAX)) 
                          message__easy = 0;
                        
                        /* Advance 'message__last', wrap if needed */
                        if (++message__last == MESSAGE_MAX) message__last = 0;
                }

                /* Assign the starting address */
                message__ptr[x] = message__ptr[i];

                /* Store the message type */
                message__type[x] = type;

                /* Store the message count */
                message__count[x] = 1;

                /* Success */
                return;
        }

        /*** Step 4 -- Ensure space before end of buffer ***/

        /* Kill messages, and wrap, if needed */
        if (message__head + (n + 1) >= MESSAGE_BUF)
        {
                bool update_easy = FALSE;
                
                /* Kill all "dead" messages */
                for (i = message__last; TRUE; i++)
                {
                        /* Wrap if needed */
                        if (i == MESSAGE_MAX) i = 0;

                        /* Stop before the new message */
                        if (i == message__next) break;

                        /* Update message__easy if required */
                        if (i == message__easy) update_easy = TRUE;

                        /* Get offset */
                        o = message__ptr[i];

                        /* Kill "dead" messages */
                        if (o >= message__head)
                        {
                                /* Track oldest message */
                                message__last = i + 1;
                                
                                /* Update easy if required */
                                if (update_easy) message__easy = i + 1;
                        }
                }

                /* Wrap "tail" if needed */
                if (message__tail >= message__head) message__tail = 0;

                /* Start over */
                message__head = 0;
        }


        /*** Step 5 -- Ensure space for actual characters ***/

        /* Kill messages, if needed */
        if (message__head + (n + 1) > message__tail)
        {
                bool update_easy = FALSE;
                
                /* Advance to new "tail" location */
                message__tail += (MESSAGE_BUF / 4);

                /* Kill all "dead" messages */
                for (i = message__last; TRUE; i++)
                {
                        /* Wrap if needed */
                        if (i == MESSAGE_MAX) i = 0;

                        /* Stop before the new message */
                        if (i == message__next) break;

                        /* Update message__easy if required */
                        if (i == message__easy) update_easy = TRUE;

                        /* Get offset */
                        o = message__ptr[i];

                        /* Kill "dead" messages */
                        if ((o >= message__head) && (o < message__tail))
                        {
                                /* Track oldest message */
                                message__last = i + 1;
                                
                                /* Update easy if required */
                                if (update_easy) message__easy = i + 1;
                        }
                }
        }


        /*** Step 6 -- Grab a new message index ***/

        /* Get the next available message index */
        x = message__next;

        /* Advance 'message__next', wrap if needed */
        if (++message__next == MESSAGE_MAX) message__next = 0;

        /* Kill last message if needed */
        if (message__next == message__last)
        {
                /* Update the 'message__easy', wrap if needed */
                if ((message__last == message__easy) && 
                    (++message__last == MESSAGE_MAX)) 
                  message__easy = 0;
                
                /* Advance 'message__last', wrap if needed */
                if (++message__last == MESSAGE_MAX) message__last = 0;
        }


        /*** Step 7 -- Insert the message text ***/

        /* Assign the starting address */
        message__ptr[x] = message__head;

        /* Inline 'strcpy(message__buf + message__head, str)' */
        v = message__buf + message__head;
        for (u = str; *u; ) *v++ = *u++;
        *v = '\0';

        /* Advance the "head" pointer */
        message__head += (n + 1);

        /* Store the message type */
        message__type[x] = type;

        /* Store the message count */
        message__count[x] = 1;
}


/**
 * This displays all the messages on the screen, trying to
 * minimise the amount of times the -more- key has to be
 * pressed, by using all the available screen space.
 * 
 * If command is set to true, we re-display the command
 * prompt once this is done, and pass back the last key
 * press as a command.
 */
void messages_easy(bool command)
{
  int y, x, w, h;
  byte a = TERM_L_BLUE;
  event_type ke = EVENT_EMPTY;
  
  char *t;
  char buf[1024];
  
  /* Initialize */
  ke.type = EVT_KBRD;
  ke.key = 0;
  ke.index = 0;
  ke.mousey = 0;
  ke.mousex = 0;
  
  /* Easy more option not selected. */
  if (!easy_more)
    {
      message__easy = message__next;
      
      return;
    }
  
  /* Nothing to display. */
  else if (!must_more)
    {
      return;
    }
  
  /* Nothing to display. */
  else if (message__easy == message__next)
    {
      return;
    }
  
  /* Don't display if character is dead or not yet generated */
  else if (!character_generated || p_ptr->is_dead)
    {
      return;
    }
  
  /* Save the screen */
  screen_save();
  
  /* Obtain the size */
  (void)Term_get_size(&w, &h);
  
  prt("", 1, 0);

  /* Display remaining messages on line 2 of the display onwards */
  for (y = (msg_flag ? 0 : 1), x = 0 ; (message__easy != message__next); )
    {
      /* Get the "offset" for the message */
      int o = message__ptr[message__easy];
      
      /* Get the message text */
      cptr msg = &message__buf[o];
      
      /* Get the color */
      byte color = message_type_color(message__type[message__easy]);
      
      int n = strlen(msg);
      
      bool long_line = FALSE;
      
      if ((x) && (x + n) > (w))
        {
          /* Go to next row if required */
          x = 0;
          y++;
          prt("", y, 0);
        }
      
      /* Improve legibility of long entries */          
      if (n > (w - 8)) long_line = TRUE;
      
      /* Copy it */
      strncpy(buf, msg, sizeof(buf));
      buf[sizeof(buf) - 1] = '\0';
      
      /* Analyze the buffer */
      t = buf;
      
      /* Split message */
      while (n > (w - 1))
        {
          char oops;
          
          int check, split;
          
          /* Default split */
          split = (w - 1);
          
          /* Find the "best" split point */
          for (check = (w / 2); check < (w - 1); check++)
            {
              /* Found a valid split point */
              if (t[check] == ' ') split = check;
            }
          
          /* Save the split character */
          oops = t[split];
          
          /* Split the message */
          t[split] = '\0';
          
          /* Display part of the message */
          Term_putstr(x, y, split, color, t);
          
          /* Erase to end of line to improve legibility */
          if (long_line)
            {
              /* Clear top line */
              Term_erase(x + split, y, 255);                    
            }
          else
            {
              /* Add a space for legibility */
              Term_putstr(x + split, y, -1, TERM_WHITE, " ");
            }
          
          /* Restore the split character */
          t[split] = oops;
          
          /* Prepare to recurse on the rest of "buf" */
          t += split; n -= split;
          
          /* Reset column and line */
          x = 0;
          y++;
        }
      
      /* Display the tail of the message */
      Term_putstr(x, y, n, color, t);
      
      /* Add a space for legibility */
      Term_putstr(x + n, y, -1, TERM_WHITE, " ");
      
      /* Get next message */
      message__easy = (message__easy + 1) % MESSAGE_MAX;
      
      /* Get next position */
      x += n + 1;
      
      /* Display more prompt if reached near end of page */
      if ((y >= (h < 12 ? h - (bottom_status ? 2 : 3) : 
                 (h > 23 ? (h / 2) - (bottom_status ? 1 : 2) : 11 - 
                  (bottom_status ? 2 : 3))))
          /* Display more prompt if out of messages */
          || (message__easy == message__next))
        {
          /* Pause for response */
          prt("", y + 1, 0);
          Term_putstr(0, y + 1, -1, a, message__easy == message__next ? 
                      "-end-" : "-more-");
          
          /* Get an acceptable keypress. */
          while (1)
            {
              ke = inkey_ex();
#if 0   
              if ((ke.key == '\xff') && !(ke.mousebutton))
                {
                  int y = KEY_GRID_Y(p_ptr->command_cmd_ex);
                  int x = KEY_GRID_X(p_ptr->command_cmd_ex);
                  int room = dun_room[p_ptr->py/BLOCK_HGT][p_ptr->px/BLOCK_WID];
                  
                  if (in_bounds_fully(y, x)) target_set_interactive_aux(y, x, &room, TARGET_PEEK, (use_mouse ? "*,left-click to target, right-click to go to" : "*"));
                  
                  continue;
                }
              
              if ((p_ptr->chp < warning) && (ke.key != 'c')) { bell("Press c to continue."); continue; }
#endif
              if (quick_messages) break;
              if ((ke.key == ESCAPE) || (ke.key == ' ')) break;
              if ((ke.key == '\n') || (ke.key == '\r')) break;
              if ((ke.key == '\xff') && (ke.type == EVT_MOUSE)) break;
              bell("Illegal response to a 'more' prompt!");
            }
          
          /* Refresh screen */
          screen_load();
          
          /* Tried a command - avoid rest of messages */
          if (ke.key != ' ') break;
          
          if (message__easy != message__next) screen_save();
          
          /* Start at top left hand side */
          y = 0;
          x = 0;
        }
    }
  
  /* Allow 1 line messages again */
  must_more = FALSE;
  
  /* Clear the message flag */
  msg_flag = FALSE;
  
  /* Clear top line */
  Term_erase(0, 0, 255);
  
  /* Display command prompt */
  if (command)
    {
      Term_putstr(0, 0, -1, TERM_WHITE, "Command:");
      
      /* Requeue command just pressed */
      p_ptr->command_new = ke.key;
      
      /* Hack -- Process "Escape"/"Spacebar"/"Return" */
      if ((p_ptr->command_new == ESCAPE) ||
          (p_ptr->command_new == ' ') ||
          (p_ptr->command_new == '\r') ||
          (p_ptr->command_new == '\n') ||
          (p_ptr->command_new == '\xff'))
        {
          /* Reset stuff */
          p_ptr->command_new = 0;
        }
    }
}


/**
 * Initialize the "message" package
 */
errr messages_init(void)
{
        /* Message variables */
        C_MAKE(message__ptr, MESSAGE_MAX, u16b);
        C_MAKE(message__buf, MESSAGE_BUF, char);
        C_MAKE(message__type, MESSAGE_MAX, u16b);
        C_MAKE(message__count, MESSAGE_MAX, u16b);

        /* Init the message colors to white */
        (void)C_BSET(message__color, TERM_WHITE, MSG_MAX, byte);

        /* Hack -- No messages yet */
        message__tail = MESSAGE_BUF;

        /* Hack -- No messages for easy_more */
        message__easy = MESSAGE_BUF;
        
        /* Success */
        return (0);
}


/**
 * Free the "message" package
 */
void messages_free(void)
{
        /* Free the messages */
        FREE(message__ptr);
        FREE(message__buf);
        FREE(message__type);
        FREE(message__count);
}


/*
 * XXX XXX XXX Important note about "colors" XXX XXX XXX
 *
 * The "TERM_*" color definitions list the "composition" of each
 * "Angband color" in terms of "quarters" of each of the three color
 * components (Red, Green, Blue), for example, TERM_UMBER is defined
 * as 2/4 Red, 1/4 Green, 0/4 Blue.
 *
 * The following info is from "Torbjorn Lindgren" (see "main-xaw.c").
 *
 * These values are NOT gamma-corrected.  On most machines (with the
 * Macintosh being an important exception), you must "gamma-correct"
 * the given values, that is, "correct for the intrinsic non-linearity
 * of the phosphor", by converting the given intensity levels based
 * on the "gamma" of the target screen, which is usually 1.7 (or 1.5).
 *
 * The actual formula for conversion is unknown to me at this time,
 * but you can use the table below for the most common gamma values.
 *
 * So, on most machines, simply convert the values based on the "gamma"
 * of the target screen, which is usually in the range 1.5 to 1.7, and
 * usually is closest to 1.7.  The converted value for each of the five
 * different "quarter" values is given below:
 *
 *  Given     Gamma 1.0       Gamma 1.5       Gamma 1.7     Hex 1.7
 *  -----       ----            ----            ----          ---
 *   0/4        0.00            0.00            0.00          #00
 *   1/4        0.25            0.27            0.28          #47
 *   2/4        0.50            0.55            0.56          #8f
 *   3/4        0.75            0.82            0.84          #d7
 *   4/4        1.00            1.00            1.00          #ff
 *
 * Note that some machines (i.e. most IBM machines) are limited to a
 * hard-coded set of colors, and so the information above is useless.
 *
 * Also, some machines are limited to a pre-determined set of colors,
 * for example, the IBM can only display 16 colors, and only 14 of
 * those colors resemble colors used by Angband, and then only when
 * you ignore the fact that "Slate" and "cyan" are not really matches,
 * so on the IBM, we use "orange" for both "Umber", and "Light Umber"
 * in addition to the obvious "Orange", since by combining all of the
 * "indeterminate" colors into a single color, the rest of the colors
 * are left with "meaningful" values.
 */


/**
 * Looks if "inscrip" is present on the given object.
 */
bool check_for_inscrip(const object_type *o_ptr, const char *inscrip)
{
        if (o_ptr->note)
        {
                const char *s = strstr(quark_str(o_ptr->note), inscrip);
                if (s) return TRUE;
        }
        
        return FALSE;
}



/**
 * Hack -- flush
 */
static void msg_flush(int x)
{
        byte a = TERM_L_BLUE;

        /* Handle easy_more */
        if (easy_more) return;  
        
        /* Pause for response */
        Term_putstr(x, 0, -1, a, "-more-");

        if (!auto_more)
        {
                /* Get an acceptable keypress */
                while (1)
                {
                        event_type ke = EVENT_EMPTY;
                        ke = inkey_ex();
                        if (quick_messages) break;
                        if ((ke.key == ESCAPE) || (ke.key == ' ')) break;
                        if ((ke.key == '\n') || (ke.key == '\r')) break;
                        if (ke.key == '\xff') break;
                        bell("Illegal response to a 'more' prompt!");
                }
        }

        /* Clear the line */
        Term_erase(0, 0, 255);
}


static int message_column = 0;


/**
 * Output a message to the top line of the screen.
 *
 * Break long messages into multiple pieces (40-72 chars).
 *
 * Allow multiple short messages to "share" the top line.
 *
 * Prompt the user to make sure he has a chance to read them.
 *
 * These messages are memorized for later reference (see above).
 *
 * We could do a "Term_fresh()" to provide "flicker" if needed.
 *
 * The global "msg_flag" variable can be cleared to tell us to "erase" any
 * "pending" messages still on the screen, instead of using "msg_flush()".
 * This should only be done when the user is known to have read the message.
 *
 * We must be very careful about using the "msg_print()" functions without
 * explicitly calling the special "msg_print(NULL)" function, since this may
 * result in the loss of information if the screen is cleared, or if anything
 * is displayed on the top line.
 *
 * Hack -- Note that "msg_print(NULL)" will clear the top line even if no
 * messages are pending.
 */
static void msg_print_aux(u16b type, cptr msg)
{
        int n;
        char *t;
        char buf[1024];
        byte color;
        int w, h;


        /* Obtain the size */
        (void)Term_get_size(&w, &h);

        /* Hack -- Reset */
        if (!msg_flag) message_column = 0;

        /* Message Length */
        n = (msg ? strlen(msg) : 0);

        /* Hack -- flush when requested or needed */
        if ((message_column || easy_more) && 
            (!msg || ((message_column + n) > (w - 8))))
        {
                bool hack_use_first_line = (easy_more && !must_more && 
                                            !message_column && msg);
                bool hack_flush = (easy_more && message_column && 
                                   ((message_column + n) <= (w)) && 
                                   !must_more && !msg);
                
                /* Handle easy_more */
                if (easy_more && msg && !must_more)
                {
                        /* Display messages from this point onwards */
                        message__easy = message__next;

                        /* Delay displaying remaining messages */
                        must_more = TRUE;
                }
                
                /* Hack -- allow single line '-more-' */
                if (hack_flush) easy_more = FALSE;
                
                /* Flush */
                msg_flush(message_column);

                /* Hack -- allow single line '-more-' */
                if (hack_flush) easy_more = TRUE;

                /* Forget it */
                msg_flag = hack_use_first_line;

                /* Reset */
                message_column = 0;
        }


        /* No message */
        if (!msg) return;

        /* Paranoia */
        if (n > 1000) return;


        /* Memorize the message (if legal) */
        if (character_generated && !(p_ptr->is_dead))
                message_add(msg, type);

        /* Window stuff */
        p_ptr->window |= (PW_MESSAGE);

        /* Handle "auto_more"/"must_more" */
        if (auto_more || must_more)
        {
                /* Force window update */
                window_stuff();

                /* Done */
                return;
        }


        /* Copy it */
        my_strcpy(buf, msg, sizeof(buf));

	/* Analyze the buffer */
	t = buf;

	/* Get the color of the message */
	color = message_type_color(type);

	/* Split message */
	while (n > (w - 8))
	{
		char oops;

		int check, split;

		/* Default split */
		split = (w - 8);

		/* Find the "best" split point */
		for (check = (w / 2); check < (w - 8); check++)
		{
			/* Found a valid split point */
			if (t[check] == ' ') split = check;
		}

		/* Save the split character */
		oops = t[split];

		/* Split the message */
		t[split] = '\0';

		/* Display part of the message */
		Term_putstr(0, 0, split, color, t);

		/* Flush it */
		msg_flush(split + 1);

		/* Restore the split character */
		t[split] = oops;

		/* Insert a space */
		t[--split] = ' ';

		/* Prepare to recurse on the rest of "buf" */
		t += split; n -= split;
	}

	/* Display the tail of the message */
	Term_putstr(message_column, 0, n, color, t);

	/* Remember the message */
	msg_flag = TRUE;

	/* Remember the position */
	message_column += n + 1;
}


/**
 * Print a message in the default color (white)
 */
void msg_print(cptr msg)
{
	msg_print_aux(MSG_GENERIC, msg);
}


/**
 * Display a formatted message, using "vstrnfmt()" and "msg_print()".
 */
void msg_format(cptr fmt, ...)
{
	va_list vp;

	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args, save the length */
	(void)vstrnfmt(buf, sizeof(buf), fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Display */
	msg_print_aux(MSG_GENERIC, buf);
}


/**
 * Display a message and play the associated sound.
 *
 * The "extra" parameter is currently unused.
 */
void message(u16b message_type, s16b extra, cptr message)
{
	/* Unused parameter */
	(void)extra;

	sound(message_type);

	msg_print_aux(message_type, message);
}



/**
 * Display a formatted message and play the associated sound.
 *
 * The "extra" parameter is currently unused.
 */
void message_format(u16b message_type, s16b extra, cptr fmt, ...)
{
	va_list vp;

	char buf[1024];

	/* Begin the Varargs Stuff */
	va_start(vp, fmt);

	/* Format the args, save the length */
	(void)vstrnfmt(buf, sizeof(buf), fmt, vp);

	/* End the Varargs Stuff */
	va_end(vp);

	/* Display */
	message(message_type, extra, buf);
}


/**
 * Print the queued messages.
 */
void message_flush(void)
{
	/* Hack -- Reset */
	if (!msg_flag) message_column = 0;

	/* Flush when needed */
	if (message_column)
	{
		/* Print pending messages */
		msg_flush(message_column);

		/* Forget it */
		msg_flag = FALSE;

		/* Reset */
		message_column = 0;
	}
}



/**
 * Save the screen, and increase the "icky" depth.
 *
 * This function must match exactly one call to "screen_load()".
 */
void screen_save(void)
{
	/* Hack -- Flush messages */
	message_flush();

	/* Save the screen (if legal) */
	Term_save();

	/* Increase "icky" depth */
	character_icky++;
}


/**
 * Load the screen, and decrease the "icky" depth.
 *
 * This function must match exactly one call to "screen_save()".
 */
void screen_load(void)
{
	/* Hack -- Flush messages */
	message_flush();

	/* Load the screen (if legal) */
	Term_load();

	/* Decrease "icky" depth */
	character_icky--;
}


/**
 * Display a string on the screen using an attribute.
 *
 * At the given location, using the given attribute, if allowed,
 * add the given string.  Do not clear the line.
 */
void c_put_str(byte attr, cptr str, int row, int col)
{
  /* Copy to get extended chars */
  char buf[1024];

  my_strcpy(buf, str, sizeof(buf));

  /* Translate it to 7-bit ASCII or system-specific format */
  xstr_trans(buf, LATIN1);
  
  /* Position cursor, Dump the attr/text */
  Term_putstr(col, row, -1, attr, buf);
}


/**
 * As above, but in "white"
 */
void put_str(cptr str, int row, int col)
{
  /* Spawn */
  c_put_str(TERM_WHITE, str, row, col);
}



/**
 * As above, but centered horizontally
 */
void put_str_center(cptr str, int row)
{
  int len = strlen(str);
  int col = (Term->wid - len) / 2;
  
  c_put_str(TERM_WHITE, str, row, col);
}


/**
 * Display a string on the screen using an attribute, and clear
 * to the end of the line.
 */
void c_prt(byte attr, cptr str, int row, int col)
{
  /* Copy to get extended chars */
  char buf[1024];

  my_strcpy(buf, str, sizeof(buf));

        /* Translate it to 7-bit ASCII or system-specific format */
        xstr_trans(buf, LATIN1);

        /* Clear line, position cursor */
        Term_erase(col, row, 255);

        /* Dump the attr/text */
        Term_addstr(-1, attr, buf);
}


/**
 * As above, but in "white"
 */
void prt(cptr str, int row, int col)
{
	/* Spawn */
	c_prt(TERM_WHITE, str, row, col);
}


/**
 * As above, but in "white"
 */
void prt_center(cptr str, int row)
{
  int len = strlen(str);
  int col = (Term->wid - len) / 2;
  
  /* Spawn */
  c_prt(TERM_WHITE, str, row, col);
}


/**
 * Print some (colored) text to the screen at the current cursor position,
 * automatically "wrapping" existing text (at spaces) when necessary to
 * avoid placing any text into the last column, and clearing every line
 * before placing any text in that line.  Also, allow "newline" to force
 * a "wrap" to the next line.  Advance the cursor as needed so sequential
 * calls to this function will work correctly.
 *
 * Once this function has been called, the cursor should not be moved
 * until all the related "text_out()" calls to the window are complete.
 *
 * This function will correctly handle any width up to the maximum legal
 * value of 256, though it works best for a standard 80 character width.
 */
void text_out_to_screen(byte a, char *str)
{
	int x, y;

	int wid, h;

	int wrap;

	cptr s;


	/* Obtain the size */
	(void)Term_get_size(&wid, &h);

	/* Obtain the cursor */
	(void)Term_locate(&x, &y);

	/* Use special wrapping boundary? */
	if ((text_out_wrap > 0) && (text_out_wrap < wid))
		wrap = text_out_wrap;
	else
		wrap = wid;

	/* Process the string */
	for (s = str; *s; s++)
	{
		char ch;

		/* Force wrap */
		if (*s == '\n')
		{
			/* Wrap */
			x = text_out_indent;
			y++;

			/* Clear line, move cursor */
			Term_erase(x, y, 255);

			continue;
                }

                /* Clean up the char */
                ch = (my_isprint((unsigned char)*s) ? *s : ' ');

                /* Wrap words as needed */
                if ((x >= wrap - 1) && (ch != ' '))
		{
			int i, n = 0;

			byte av[256];
			char cv[256];

			/* Wrap word */
			if (x < wrap)
			{
				/* Scan existing text */
				for (i = wrap - 2; i >= 0; i--)
				{
					/* Grab existing attr/char */
					Term_what(i, y, &av[i], &cv[i]);

					/* Break on space */
					if (cv[i] == ' ') break;

					/* Track current word */
					n = i;
				}
			}

			/* Special case */
			if (n == 0) n = wrap;

			/* Clear line */
			Term_erase(n, y, 255);

			/* Wrap */
			x = text_out_indent;
			y++;

			/* Clear line, move cursor */
			Term_erase(x, y, 255);

			/* Wrap the word (if any) */
			for (i = n; i < wrap - 1; i++)
			{
				/* Dump */
				Term_addch(av[i], cv[i]);

				/* Advance (no wrap) */
				if (++x > wrap) x = wrap;
			}
                }

                /* Dump */
                if (Term->xchar_hook)
                  Term_addch(a, Term->xchar_hook(ch));
                else
                  Term_addch(a, ch);

                /* Advance */
		if (++x > wrap) x = wrap;
	}
}


/**
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
void text_out_to_file(byte a, char *str)
{
        char *s;
        char buf[1024];
	char buf1[2];
		  

        /* Current position on the line */
        static int pos = 0;

        /* Wrap width */
        int wrap = (text_out_wrap ? text_out_wrap : 75);

        /* We use either ascii or system-specific encoding */
        int encoding = (xchars_to_file) ? SYSTEM_SPECIFIC : ASCII;

        /* Unused parameter */
        (void)a;

        /* Copy to a rewriteable string */
        my_strcpy(buf, str, 1024);

        /* Translate it to 7-bit ASCII or system-specific format */
        xstr_trans(buf, encoding);

	/* Set up the character buffer */
	buf1[0] = ' ';
	buf1[1] = '\0';

        /* Current location within "buf" */
        s = buf;

        /* Process the string */
        while (*s)
        {
		char ch;
		int n = 0;
		int len = wrap - pos;
		int l_space = -1;

		/* If we are at the start of the line... */
		if (pos == 0)
		{
			int i;

                        /* Output the indent */
                        for (i = 0; i < text_out_indent; i++)
                        {
			  buf1[0] = ' ';
			  file_put(text_out_file, buf1);
			  pos++;
                        }
                }

		/* Find length of line up to next newline or end-of-string */
		while ((n < len) && !((s[n] == '\n') || (s[n] == '\0')))
		{
			/* Mark the most recent space in the string */
			if (s[n] == ' ') l_space = n;

			/* Increment */
			n++;
		}

		/* If we have encountered no spaces */
		if ((l_space == -1) && (n == len))
		{
			/* If we are at the start of a new line */
			if (pos == text_out_indent)
			{
				len = n;
			}
			/* HACK - Output punctuation at the end of the line */
			else if ((s[0] == ' ') || (s[0] == ',') || (s[0] == '.'))
			{
				len = 1;
                        }
                        else
                        {
                                /* Skip newlines -DG- */
                                if (s[0] == '\n') s++;

                                /* Begin a new line */
				buf1[0] = '\n';
                                file_put(text_out_file, buf1);

                                /* Reset */
                                pos = 0;

				continue;
			}
		}
		else
		{
			/* Wrap at the newline */
			if ((s[n] == '\n') || (s[n] == '\0')) len = n;

			/* Wrap at the last space */
			else len = l_space;
		}

		/* Write that line to file */
                for (n = 0; n < len; n++)
                {
		  /* Ensure the character is printable */
                  ch = (my_isprint((unsigned char)s[n]) ? s[n] : ' ');
		  buf1[0] = ch;
		  
		  /* Write out the character */
		  file_put(text_out_file, buf);

		  /* Increment */
		  pos++;
		}

                /* Move 's' past the stuff we've written */
                s += len;

                /* Skip whitespace -DG- */
                while (*s == ' ') s++;

                /* If we are at the end of the string, end */
                if (*s == '\0') return;

		/* Skip newlines */
                if (*s == '\n') s++;

                /* Begin a new line */
		buf1[0] = '\n';
		file_put(text_out_file, buf1);

                /* Reset */
                pos = 0;
        }

        /* We are done */
	return;
}


/**
 * Output text to the screen or to a file depending on the selected
 * text_out hook.
 */
void text_out(char *str)
{
        text_out_c(TERM_WHITE, str);
}


/**
 * Output text to the screen (in color) or to a file depending on the
 * selected hook.
 */
void text_out_c(byte a, char *str)
{
        text_out_hook(a, str);
}


/**
 * Clear part of the screen
 */
void clear_from(int row)
{
        int y;

        /* Erase requested rows */
        for (y = row; y < Term->hgt; y++)
        {
                /* Erase part of the screen */
                Term_erase(0, y, 255);
        }
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
  int encoding = (xchars_to_file) ? SYSTEM_SPECIFIC : ASCII;

  /* Display the requested encoding -- ASCII or system-specific */
  if (!xchars_to_file) Term->xchar_hook = NULL;

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
 * The default "keypress handling function" for askfor_aux, this takes the
 * given keypress, input buffer, length, etc, and does the appropriate action
 * for each keypress, such as moving the cursor left or inserting a character.
 *
 * It should return TRUE when editing of the buffer is "complete" (e.g. on
 * the press of RETURN).
 */
bool askfor_aux_keypress(char *buf, size_t buflen, size_t *curs, size_t *len, 
                         char keypress, bool firsttime)
{
  switch (keypress)
    {
		case ESCAPE:
		{
			*curs = 0;
			return TRUE;
			break;
		}
		
		case '\n':
		case '\r':
		{
			*curs = *len;
			return TRUE;
			break;
		}
		
		case ARROW_LEFT:
		{
			if (firsttime) *curs = 0;
			if (*curs > 0) (*curs)--;
			break;
		}
		
		case ARROW_RIGHT:
		{
			if (firsttime) *curs = *len - 1;
			if (*curs < *len) (*curs)++;
			break;
		}
		
		case 0x7F:
		case '\010':
		{
			/* If this is the first time round, backspace means "delete all" */
			if (firsttime)
			{
				buf[0] = '\0';
				*curs = 0;
				*len = 0;

				break;
			}

			/* Refuse to backspace into oblivion */
			if (*curs == 0) break;

			/* Move the string from k to nul along to the left by 1 */
			memmove(&buf[*curs - 1], &buf[*curs], *len - *curs);

			/* Decrement */
			(*curs)--;
			(*len)--;

			/* Terminate */
			buf[*len] = '\0';

			break;
		}
		
		default:
		{
			bool atnull = (buf[*curs] == 0);


			if (!isprint((unsigned char)keypress))
			{
				bell("Illegal edit key!");
				break;
			}

			/* Clear the buffer if this is the first time round */
			if (firsttime)
			{
				buf[0] = '\0';
				*curs = 0;
				*len = 0;
				atnull = 1;
			}

			if (atnull)
			{
				/* Make sure we have enough room for a new character */
				if ((*curs + 1) >= buflen) break;
			}
			else
			{
				/* Make sure we have enough room to add a new character */
				if ((*len + 1) >= buflen) break;

				/* Move the rest of the buffer along to make room */
				memmove(&buf[*curs+1], &buf[*curs], *len - *curs);
			}

			/* Insert the character */
			buf[(*curs)++] = keypress;
			(*len)++;

			/* Terminate */
			buf[*len] = '\0';

			break;
		}
	}

	/* By default, we aren't done. */
	return FALSE;
}


/**
 * Get some input at the cursor location.
 *
 * The buffer is assumed to have been initialized to a default string.
 * Note that this string is often "empty" (see below).
 *
 * The default buffer is displayed in yellow until cleared, which happens
 * on the first keypress, unless that keypress is Return.
 *
 * Normal chars clear the default and append the char.
 * Backspace clears the default or deletes the final char.
 * Return accepts the current buffer contents and returns TRUE.
 * Escape clears the buffer and the window and returns FALSE.
 *
 * Note that 'len' refers to the size of the buffer.  The maximum length
 * of the input is 'len-1'.
 *
 * 'keypress_h' is a pointer to a function to handle keypresses, altering
 * the input buffer, cursor position and suchlike as required.  See
 * 'askfor_aux_keypress' (the default handler if you supply NULL for
 * 'keypress_h') for an example.
 */
bool askfor_aux(char *buf, size_t len, 
                bool keypress_h(char *, size_t, size_t *, 
                                size_t *, char, bool))
{

  int y, x;
  
  size_t k = 0;         /* Cursor position */
  size_t nul = 0;               /* Position of the null byte in the string */

  event_type ke = EVENT_EMPTY;  
  
  bool done = FALSE;
  bool firsttime = TRUE;
	if (keypress_h == NULL)
	{
		keypress_h = askfor_aux_keypress;
	}

	/* Locate the cursor */
	Term_locate(&x, &y);


	/* Paranoia */
	if ((x < 0) || (x >= 80)) x = 0;


	/* Restrict the length */
	if (x + len > 80) len = 80 - x;

	/* Truncate the default entry */
	buf[len-1] = '\0';

	/* Get the position of the null byte */
	nul = strlen(buf);

	/* Display the default answer */
	Term_erase(x, y, (int)len);
	Term_putstr(x, y, -1, TERM_YELLOW, buf);

	/* Process input */
	while (!done)
	{
		/* Place cursor */
      Term_gotoxy(x + k, y);
      
      /* Get a key */
      ke = inkey_ex();
      if (ke.type != EVT_KBRD) continue;
      
      /* Let the keypress handler deal with the keypress */
      done = keypress_h(buf, len, &k, &nul, ke.key, firsttime);
      
      /* Update the entry */
      Term_erase(x, y, (int)len);
		Term_putstr(x, y, -1, TERM_WHITE, buf);

		/* Not the first time round anymore */
		firsttime = FALSE;
    }
  
  /* Done */
  return (ke.key != ESCAPE);
}

/**
 * A "keypress" handling function for askfor_aux, that handles the special
 * case of '*' for a new random "name" and passes any other "keypress"
 * through to the default "editing" handler.
 */
bool get_name_keypress(char *buf, size_t buflen, size_t *curs, size_t *len, 
                       char keypress, bool firsttime)
{
  bool result;
  
  switch (keypress)
    {
    case '*':
      {
	*len = randname_make(RANDNAME_TOLKIEN, 4, 8, buf, buflen);
	buf[0] = toupper((unsigned char) buf[0]);
	*curs = 0;
	result = FALSE;
	break;
      }
      
      
    default:
      {
        result = askfor_aux_keypress(buf, buflen, curs, len, keypress, 
                                     firsttime);
        break;
      }
    }

  return result;
}


/**
 * Gets a name for the character, reacting to name changes.
 *
 * If sf is TRUE, we change the savefile name depending on the character name.
 *
 * What a horrible name for a global function.  XXX XXX XXX
 */
bool get_name(bool sf)
{
  bool res;
  char tmp[32];
  
  /* Paranoia XXX XXX XXX */
  message_flush();
  
  /* Buttons */
  /* This can lead to problems when changing name from C screen as buttons 
     already backed up to get to there
  backup_buttons();
  kill_all_buttons(); */
  add_button("ESC", ESCAPE);
  add_button("Enter", '\r');
  add_button("*", '*');
  prt("", Term->hgt - 1, prompt_end);
  update_statusline();
  
  /* Display prompt */
  if (small_screen)
    prt("Enter a character name (* for random): ", 0, 0);
  else
    prt("Enter a name for your character (* for a random name): ", 0, 0);
  
  /* Save the player name */
  my_strcpy(tmp, op_ptr->full_name, sizeof(tmp));
  
  /* Ask the user for a string */
  res = askfor_aux(tmp, sizeof(tmp), get_name_keypress);
  
  /* Clear prompt */
  prt("", 0, 0);
  
  if (res)
    {
      /* Use the name */
      my_strcpy(op_ptr->full_name, tmp, sizeof(op_ptr->full_name));
      
      /* Process the player name */
      process_player_name(sf);
    }
  
  /* Done */
/*  restore_buttons(); */
  update_statusline();
  return res;
}

/**
 * Prompt for a number from a user.
 * + will increase the current amount, - will decrease (with wrapping).
 * * will select the maximum amount.
 */
bool get_num(char *prompt, int max, int amt)
{
  char tmp[80];
  
  char buf[6];
  
  int y, x;
  
  int d, j, k = 0;
  
  event_type ke = EVENT_EMPTY;
  
  bool done = FALSE;
  
  ke.key = '\0';
  
  /* Build a prompt if needed */
  if (!prompt)
    {
      /* Build a prompt */
      strnfmt(tmp, sizeof(tmp), 
              "Quantity (0-%d, +=incr, -=decr, *=all): ", max);
      
      /* Use that prompt */
      prompt = tmp;
    }
  
  /* Paranoia XXX XXX XXX */
  msg_print(NULL);

  /* Buttons */
  backup_buttons();
  kill_all_buttons();
  add_button("ESC", ESCAPE);
  add_button("Ent", '\r');
  add_button("+", '+');
  add_button("-", '-');
  add_button("*", '*');
  update_statusline();
  
  /* Display prompt */
  prt(prompt, 0, 0);
  
  /* Build the default */
  sprintf(buf, "%d", amt);
  
  /* Locate the cursor */
  Term_locate(&x, &y);
  
  /* Paranoia -- check column */
  if ((x < 0) || (x >= 80)) x = 0;
  
  /* Paranoia -- Clip the default entry */
  buf[5] = '\0';
  
  
  /* Display the default answer */
  Term_erase(x, y, 6);
  Term_putstr(x, y, -1, TERM_YELLOW, buf);
  
  
  /* Process input */
  while (!done)
    {
      /* Place cursor */
      Term_gotoxy(x + k, y);
      
      /* Get a key */
      ke = inkey_ex();
      
      /* Analyze the key */
      switch (ke.key)
        {
        case ESCAPE:
          {
            k = 0;
            done = TRUE;
            break;
          }
          
        case '\n':
        case '\r':
          {
            k = strlen(buf);
            done = TRUE;
            break;
          }
          
        case 0x7F:
        case '\010':
          {
            if (k > 0) k--;
            break;
          }
        case '*':
          {
            sprintf(buf, "%d", max);
            k = strlen(buf);
            done = TRUE;
            break;
          }
        case '+':
          {
            bool writing = FALSE;
            
            k = 0;
            amt++;
            if (amt > max) amt = 0;
            for(j = 100000; j >= 1; j /= 10)
              {
                if ((amt >= j) || (writing))
                  {
                    writing = TRUE;
                    d = amt / j;
                    amt -= d * j;
                    buf[k++] = I2D(d);
                  }
              }
            buf[k] = '\0';
            break;
          }
        case '-':
          {
            bool writing = FALSE;
            
            k = 0;
            amt--;
            if (amt < 0) amt = max;
            for(j = 100000; j >= 1; j /= 10)
              {
                if ((amt >= j) || (writing))
                  {
                    writing = TRUE;
                    d = amt / j;
                    amt -= d * j;
                    buf[k++] = I2D(d);
                  }
              }
            buf[k] = '\0';
            break;
          }
              
        default:
          {
            if ((k < 5) && (isdigit(ke.key)))
              {
                buf[k++] = ke.key;
              }
            else
              {
                bell("Illegal edit key!");
              }
            break;
          }
      
        }
      
      /* Terminate */
      buf[k] = '\0';
      
      /* Extract a number */
      amt = atoi(buf);
      
      /* Update the entry */
      Term_erase(x, y, 6);
      Term_putstr(x, y, -1, TERM_WHITE, buf);
    }
  
  /* Buttons */
  kill_all_buttons();
  restore_buttons();
  update_statusline();
   
  /* Wipe the prompt */
  prt("", 0, 0);
  
  /* Extract a number */
  amt = atoi(buf);
  
  return (amt);
}
  

/**
 * Prompt for a string from the user.
 *
 * The "prompt" should take the form "Prompt: ".
 *
 * See "askfor_aux" for some notes about "buf" and "len", and about
 * the return value of this function.
 */
bool get_string(cptr prompt, char *buf, size_t len)
{
	bool res;

	/* Paranoia XXX XXX XXX */
	message_flush();

	/* Display prompt */
	prt(prompt, 0, 0);

  /* Ask the user for a string */
  res = askfor_aux(buf, len, NULL);
  
  /* Translate it to 8-bit (Latin-1) */
  xstr_trans(buf, LATIN1);

  /* Clear prompt */
  prt("", 0, 0);
  
	/* Result */
	return (res);
}



/**
 * Request a "quantity" from the user
 *
 * Allow "p_ptr->command_arg" to specify a quantity
 */
s16b get_quantity(cptr prompt, int max)
{
	int amt = 1;


	/* Use "command_arg" */
	if (p_ptr->command_arg)
	{
		/* Extract a number */
		amt = p_ptr->command_arg;

		/* Clear "command_arg" */
		p_ptr->command_arg = 0;
	}

	/* Get the item index */
	else if ((max != 1) && repeat_pull(&amt))
	{
		/* nothing */
	}

  /* Prompt if needed */
  else if ((max != 1))
    {
      /* Ask for a quantity */
      amt = get_num((char *)prompt, max, amt);
    }
  
  /* Enforce the maximum */
	if (amt > max) amt = max;

	/* Enforce the minimum */
	if (amt < 0) amt = 0;

	if (amt) repeat_push(amt);

	/* Return the result */
  return (amt);
}

/**
 * Hack - duplication of get_check prompt to give option of setting destroyed
 * option to squelch.
 *
 * 0 - No
 * 1 = Yes
 * 2 = third option
 *
 * The "prompt" should take the form "Query? "
 *
 * Note that "[y/n/{char}]" is appended to the prompt.
 */
int get_check_other(cptr prompt, char other)
{
  event_type ke = EVENT_EMPTY;
  char buf[80];
  char label[4];
  bool extra = (KTRL(other) == other);
  
  int result;

  bool repeat = FALSE;
  
  /* Flush easy_more messages */
  if (easy_more) messages_easy(FALSE);
        
  /* Paranoia XXX XXX XXX */
  else message_flush();
  
  /* Hack -- Build a "useful" prompt */
  if (extra)
    strnfmt(buf, 78, "%.70s [y/n/^%c]", prompt, UN_KTRL(other)); 
  else
    strnfmt(buf, 78, "%.70s [y/n/%c] ", prompt, other);

  /* Hack - kill the repeat button */
  if (kill_button('n')) repeat = TRUE;
  
  /* Make some buttons */
  add_button("y", 'y');
  add_button("n", 'n');
  if (extra)
    strnfmt(label, 4, "^%c", UN_KTRL(other));
  else
    strnfmt(label, 4, "%c", other);
  add_button(label, other);

  /* Prompt for it */
  prt(buf, 0, 0);
  
  /* Get an acceptable answer */
  while (TRUE)
    {
      ke = inkey_ex();
      if (quick_messages) break;
      if (ke.key == ESCAPE) break;
      if (strchr("YyNn", ke.key)) break;
      if (ke.key == toupper(other)) break;
      if (ke.key == tolower(other)) break;
      bell("Illegal response to question!");
    }

  /* Erase the prompt */
  prt("", 0, 0);
  
  /* Kill the buttons */
  kill_button('y');
  kill_button('n');
  kill_button(other);

  /* Hack - restore the repeat button */
  if (repeat) add_button("Rpt", 'n');
  update_statusline();

  /* Yes */
  if ((ke.key == 'Y') || (ke.key == 'y'))
    result = 1;
  
  /* Third option */
  else if ((ke.key == toupper(other)) || (ke.key == tolower(other)))
    result = 2;
  
  /* Default to no */
  else
    result = 0;
  
  
  /* Success */
  return (result);
}


/**
 * Verify something with the user
 *
 * The "prompt" should take the form "Query? "
 *
 * Note that "[y/n]" is appended to the prompt.
 */
bool get_check(cptr prompt)
{
  event_type ke = EVENT_EMPTY;
  
  char buf[80];

  bool repeat = FALSE;

  feature_type *f_ptr = &f_info[cave_feat[p_ptr->py][p_ptr->px]];
  
  /* Flush easy_more messages */
  if (easy_more) messages_easy(FALSE);
        
  /* Paranoia XXX XXX XXX */
  else message_flush();
  
  /* Hack -- Build a "useful" prompt */
  strnfmt(buf, 78, "%.70s [y/n] ", prompt);
  
  /* Hack - kill the repeat button */
  if (kill_button('n')) repeat = TRUE;
  
  /* Make some buttons */
  add_button("y", 'y');
  add_button("n", 'n');
  update_statusline();
  
  /* Prompt for it */
  prt(buf, 0, 0);

	/* Get an acceptable answer */
  while (TRUE)
    {
      ke = inkey_ex();
      if (ke.key == ESCAPE) break;
      if (strchr("YyNn", ke.key)) break;
      /* Hack of the century */
      if ((ke.key == '\r') && (f_ptr->flags & TF_SHOP))
        {
          ke.key = 'y';
          break;
        }

      if (quick_messages) break;
      bell("Illegal response to a 'yes/no' question!");
    }
  
  /* Kill the buttons */
  kill_button('y');
  kill_button('n');

  /* Hack - restore the repeat button */
  if (repeat) add_button("Rpt", 'n');
  clear_from(Term->hgt - 1);
  update_statusline();
  
  /* Erase the prompt */
  prt("", 0, 0);

	/* Normal negation */
	if ((ke.key != 'Y') && (ke.key != 'y')) return (FALSE);

	/* Success */
	return (TRUE);
}


/**
 * Prompts for a keypress
 *
 * The "prompt" should take the form "Command: "
 *
 * Returns TRUE unless the character is "Escape"
 */
bool get_com(cptr prompt, char *command)
{
        event_type ke = EVENT_EMPTY;
        bool result;

        result = get_com_ex(prompt, &ke);
	*command = ke.key;

        return result;
}

bool get_com_ex(cptr prompt, event_type *command)
{
        event_type ke = EVENT_EMPTY;

        /* Paranoia XXX XXX XXX */
        message_flush();

	/* Display a prompt */
	prt(prompt, 0, 0);

	/* Get a key */
	ke = inkey_ex();

	/* Clear the prompt */
	prt("", 0, 0);

	/* Save the command */
	*command = ke;

	/* Done */
	return (ke.key != ESCAPE);
}


/**
 * Pause for user response
 *
 * This function is stupid.  XXX XXX XXX
 */
void pause_line(int row)
{
  int col = (small_screen ? 10 : 23);

  prt("", row, 0);
  put_str("[Press any key to continue]", row, col);
  (void)inkey_ex();
  prt("", row, 0);
}




/**
 * Hack -- special buffer to hold the action of the current keymap
 */
static char request_command_buffer[256];


/**
 * Request a command from the user.
 *
 * Sets p_ptr->command_cmd, p_ptr->command_dir, p_ptr->command_rep,
 * p_ptr->command_arg.  May modify p_ptr->command_new.
 *
 * Note that "caret" ("^") is treated specially, and is used to
 * allow manual input of control characters.  This can be used
 * on many machines to request repeated tunneling (Ctrl-H) and
 * on the Macintosh to request "Control-Caret".
 *
 * Note that "backslash" is treated specially, and is used to bypass any
 * keymap entry for the following character.  This is useful for macros.
 *
 * Note that this command is used both in the dungeon and in
 * stores, and must be careful to work in both situations.
 *
 * Note that "p_ptr->command_new" may not work any more.  XXX XXX XXX
 */
void request_command(void)
{
  int i;
  
  event_type ke = EVENT_EMPTY;
  
  int mode;
  
  cptr act;
  
  
  /* Roguelike */
  if (rogue_like_commands)
    {
      mode = KEYMAP_MODE_ROGUE;
    }
  
  /* Original */
  else
    {
      mode = KEYMAP_MODE_ORIG;
    }
  
  
  /* No command yet */
  p_ptr->command_cmd = 0;
  
  /* No "argument" yet */
  p_ptr->command_arg = 0;

  /* No "direction" yet */
  p_ptr->command_dir = 0;
  
  /* Flush messages */
  if (easy_more && must_more) messages_easy(TRUE);
  
  /* Get command */
  while (1)
	{
		/* Hack -- auto-commands */
		if (p_ptr->command_new)
		{
			/* Flush messages */
			message_flush();

                        /* Use auto-command */
                        ke.key = (char)p_ptr->command_new;

                        /* Forget it */
                        p_ptr->command_new = 0;
		}

		/* Get a keypress in "command" mode */
		else
		{
			/* Hack -- no flush needed */
			msg_flag = FALSE;

			/* Activate "command mode" */
			inkey_flag = TRUE;

			/* Get a command */
			ke = inkey_ex();
		}

		/* Clear top line */
		prt("", 0, 0);


		/* Resize events XXX XXX */
		if (ke.type == EVT_RESIZE)
		{
			p_ptr->command_cmd_ex = ke;
			p_ptr->command_new = ' ';
		}


		/* Command Count */
		if (ke.key == '0')
		{
			int old_arg = p_ptr->command_arg;

			/* Reset */
			p_ptr->command_arg = 0;

			/* Begin the input */
			prt("Count: ", 0, 0);

			/* Get a command count */
			while (1)
			{
				/* Get a new keypress */
				ke.key = inkey();

				/* Simple editing (delete or backspace) */
				if ((ke.key == 0x7F) || (ke.key == KTRL('H')))
				{
					/* Delete a digit */
					p_ptr->command_arg = p_ptr->command_arg / 10;

					/* Show current count */
					prt(format("Count: %d", p_ptr->command_arg), 0, 0);
				}

				/* Actual numeric data */
				else if (isdigit((unsigned char)ke.key))
				{
					/* Stop count at 9999 */
					if (p_ptr->command_arg >= 1000)
					{
						/* Warn */
						bell("Invalid repeat count!");

						/* Limit */
						p_ptr->command_arg = 9999;
					}

					/* Increase count */
					else
					{
						/* Incorporate that digit */
						p_ptr->command_arg = p_ptr->command_arg * 10 + D2I(ke.key);
					}

					/* Show current count */
					prt(format("Count: %d", p_ptr->command_arg), 0, 0);
				}

				/* Exit on "unusable" input */
				else
				{
					break;
				}
			}

			/* Hack -- Handle "zero" */
			if (p_ptr->command_arg == 0)
			{
				/* Default to 99 */
				p_ptr->command_arg = 99;

				/* Show current count */
				prt(format("Count: %d", p_ptr->command_arg), 0, 0);
			}

			/* Hack -- Handle "old_arg" */
			if (old_arg != 0)
			{
				/* Restore old_arg */
				p_ptr->command_arg = old_arg;

				/* Show current count */
				prt(format("Count: %d", p_ptr->command_arg), 0, 0);
			}

			/* Hack -- white-space means "enter command now" */
			if ((ke.key == ' ') || (ke.key == '\n') || (ke.key == '\r'))
			{
				/* Get a real command */
				if (!get_com("Command: ", &ke.key))
				{
					/* Clear count */
					p_ptr->command_arg = 0;

					/* Continue */
					continue;
				}
            }
        }
      
      
      /* Special case for the arrow keys */
      if (isarrow(ke.key))
        {
			switch (ke.key)
			{
				case ARROW_DOWN:    ke.key = '2'; break;
				case ARROW_LEFT:    ke.key = '4'; break;
				case ARROW_RIGHT:   ke.key = '6'; break;
				case ARROW_UP:      ke.key = '8'; break;
            }
        }
      
      
      /* Allow "keymaps" to be bypassed */
      if (ke.key == '\\')
        {
			/* Get a real command */
			(void)get_com("Command: ", &ke.key);

			/* Hack -- bypass keymaps */
			if (!inkey_next) inkey_next = "";
		}


		/* Allow "control chars" to be entered */
		if (ke.key == '^')
		{
			/* Get a new command and controlify it */
			if (get_com("Control: ", &ke.key)) ke.key = KTRL(ke.key);
                }


                /* Look up applicable keymap */
                        act = keymap_act[mode][(byte)(ke.key)];

                /* Apply keymap if not inside a keymap already */
		if (act && !inkey_next)
		{
			/* Install the keymap */
			my_strcpy(request_command_buffer, act,
			          sizeof(request_command_buffer));

			/* Start using the buffer */
			inkey_next = request_command_buffer;

			/* Continue */
			continue;
		}


		/* Paranoia */
		if (ke.key == '\0') continue;


		/* Use command */
		p_ptr->command_cmd = ke.key;
		p_ptr->command_cmd_ex = ke;

		/* Done */
		break;
	}

	/* Hack -- Auto-repeat certain commands */
	if (p_ptr->command_arg <= 0)
	{
		/* Hack -- auto repeat certain commands */
		if (strchr(AUTO_REPEAT_COMMANDS, p_ptr->command_cmd))
		{
			/* Repeat 99 times */
			p_ptr->command_arg = 99;
		}
	}


  /* Hack -- Scan equipment */
  for (i = INVEN_WIELD; i < INVEN_TOTAL; i++)
    {
      cptr s;
      
      object_type *o_ptr = &inventory[i];
      
      /* Skip non-objects */
      if (!o_ptr->k_idx) continue;
      
      /* No inscription */
      if (!o_ptr->note) continue;
      
      /* Find a '^' */
      s = strchr(quark_str(o_ptr->note), '^');
      
      /* Process preventions */
      while (s)
        {
          /* Check the "restriction" character */
          if ((s[1] == p_ptr->command_cmd) || (s[1] == '*'))
            {
              /* Hack -- Verify command */
              if (!get_check("Are you sure? "))
                {
                  /* Hack -- Use "newline" */
                  p_ptr->command_cmd = '\n';
                }
            }
          
          /* Find another '^' */
          s = strchr(s + 1, '^');
        }
    }
  
  
  /* Hack -- erase the message line. */
  prt("", 0, 0);
  
  /* Hack again -- apply the modified key command */
  p_ptr->command_cmd_ex.key = p_ptr->command_cmd;
}




/**
 * Generates damage for "2d6" style dice rolls
 */
int damroll(int num, int sides)
{
        int i;
        int sum = 0;


        /* HACK - prevent undefined behaviour */
        if (sides <= 0) return (0);

        for (i = 0; i < num; i++)
        {
                sum += rand_die(sides);
        }

        return (sum);
}


/**
 * Same as above, but always maximal
 */
int maxroll(int num, int sides)
{
        return (num * sides);
}



/**
 * Check a char for "vowel-hood"
 */
bool is_a_vowel(int ch)
{
        switch (ch)
        {
                case 'a':
                case 'e':
                case 'i':
                case 'o':
                case 'u':
                case 'A':
                case 'E':
                case 'I':
                case 'O':
                case 'U':
                return (TRUE);
        }

        return (FALSE);
}


/**
 * Convert a "color letter" into an "actual" color
 * The colors are: dwsorgbuDWvyRGBU, as shown below
 */
extern int color_char_to_attr(char c)
{
        switch (c)
        {
		case 'd': return (TERM_DARK);
		case 'w': return (TERM_WHITE);
		case 's': return (TERM_SLATE);
		case 'o': return (TERM_ORANGE);
		case 'r': return (TERM_RED);
		case 'g': return (TERM_GREEN);
		case 'b': return (TERM_BLUE);
		case 'u': return (TERM_UMBER);

		case 'D': return (TERM_L_DARK);
		case 'W': return (TERM_L_WHITE);
		case 'v': return (TERM_VIOLET);
		case 'y': return (TERM_YELLOW);
		case 'R': return (TERM_L_RED);
		case 'G': return (TERM_L_GREEN);
		case 'B': return (TERM_L_BLUE);
		case 'U': return (TERM_L_UMBER);
	}

	return (-1);
}





#if 0

/**
 * Replace the first instance of "target" in "buf" with "insert"
 * If "insert" is NULL, just remove the first instance of "target"
 * In either case, return TRUE if "target" is found.
 *
 * Could be made more efficient, especially in the case where "insert"
 * is smaller than "target".
 */
static bool insert_str(char *buf, cptr target, cptr insert)
{
        int i, len;
        int b_len, t_len, i_len;

        /* Attempt to find the target (modify "buf") */
        buf = strstr(buf, target);

        /* No target found */
        if (!buf) return (FALSE);

        /* Be sure we have an insertion string */
        if (!insert) insert = "";

        /* Extract some lengths */
        t_len = strlen(target);
        i_len = strlen(insert);
        b_len = strlen(buf);

        /* How much "movement" do we need? */
        len = i_len - t_len;

        /* We need less space (for insert) */
        if (len < 0)
        {
                for (i = t_len; i < b_len; ++i) buf[i+len] = buf[i];
        }

        /* We need more space (for insert) */
        else if (len > 0)
        {
                for (i = b_len-1; i >= t_len; --i) buf[i+len] = buf[i];
        }

        /* If movement occured, we need a new terminator */
        if (len) buf[b_len+len] = '\0';

        /* Now copy the insertion string */
        for (i = 0; i < i_len; ++i) buf[i] = insert[i];

        /* Successful operation */
        return (TRUE);
}


#endif


#define REPEAT_MAX 20

/**
 * Number of chars saved 
 */
static int repeat__cnt = 0;

/**
 * Current index 
 */
static int repeat__idx = 0;

/**
 * Saved "stuff" 
 */
static int repeat__key[REPEAT_MAX];


/**
 * Push data.
 */
void repeat_push(int what)
{
	/* Too many keys */
	if (repeat__cnt == REPEAT_MAX) return;

	/* Push the "stuff" */
	repeat__key[repeat__cnt++] = what;

	/* Prevents us from pulling keys */
	++repeat__idx;
}


/**
 * Pull data.
 */
bool repeat_pull(int *what)
{
	/* All out of keys */
	if (repeat__idx == repeat__cnt) return (FALSE);

	/* Grab the next key, advance */
	*what = repeat__key[repeat__idx++];

	/* Success */
	return (TRUE);
}


void repeat_clear(void)
{
	/* Start over from the failed pull */
	if (repeat__idx)
		repeat__cnt = --repeat__idx;

	/* Paranoia */
	else
		repeat__cnt = repeat__idx;

	return;
}


/**
 * Repeat previous command, or begin memorizing new command.
 */
void repeat_check(void)
{
	int what;

	/* Ignore some commands */
	if (p_ptr->command_cmd == ESCAPE) return;
	if (p_ptr->command_cmd == ' ') return;
	if (p_ptr->command_cmd == '\n') return;
	if (p_ptr->command_cmd == '\r') return;

	/* Repeat Last Command */
	if (p_ptr->command_cmd == KTRL('V'))
	{
		/* Reset */
		repeat__idx = 0;

		/* Get the command */
		if (repeat_pull(&what))
		{
			/* Save the command */
			p_ptr->command_cmd = what;
		}
	}

	/* Start saving new command */
	else
	{
		/* Reset */
		repeat__cnt = 0;
		repeat__idx = 0;

		/* Get the current command */
		what = p_ptr->command_cmd;

		/* Save this command */
		repeat_push(what);
	}
}


#ifdef SUPPORT_GAMMA

/**
 * Table of gamma values 
 */
byte gamma_table[256];

/**
 * Table of ln(x / 256) * 256 for x going from 0 -> 255 
 */
static const s16b gamma_helper[256] =
{
	0, -1420, -1242, -1138, -1065, -1007, -961, -921, -887, -857, -830,
	-806, -783, -762, -744, -726, -710, -694, -679, -666, -652, -640,
	-628, -617, -606, -596, -586, -576, -567, -577, -549, -541, -532,
	-525, -517, -509, -502, -495, -488, -482, -475, -469, -463, -457,
	-451, -455, -439, -434, -429, -423, -418, -413, -408, -403, -398,
	-394, -389, -385, -380, -376, -371, -367, -363, -359, -355, -351,
	-347, -343, -339, -336, -332, -328, -325, -321, -318, -314, -311,
	-308, -304, -301, -298, -295, -291, -288, -285, -282, -279, -276,
	-273, -271, -268, -265, -262, -259, -257, -254, -251, -248, -246,
	-243, -241, -238, -236, -233, -231, -228, -226, -223, -221, -219,
	-216, -214, -212, -209, -207, -205, -203, -200, -198, -196, -194,
	-192, -190, -188, -186, -184, -182, -180, -178, -176, -174, -172,
	-170, -168, -166, -164, -162, -160, -158, -156, -155, -153, -151,
	-149, -147, -146, -144, -142, -140, -139, -137, -135, -134, -132,
	-130, -128, -127, -125, -124, -122, -120, -119, -117, -116, -114,
	-112, -111, -109, -108, -106, -105, -103, -102, -100, -99, -97, -96,
	-95, -93, -92, -90, -89, -87, -86, -85, -83, -82, -80, -79, -78,
	-76, -75, -74, -72, -71, -70, -68, -67, -66, -65, -63, -62, -61,
	-59, -58, -57, -56, -54, -53, -52, -51, -50, -48, -47, -46, -45,
	-44, -42, -41, -40, -39, -38, -37, -35, -34, -33, -32, -31, -30,
	-29, -27, -26, -25, -24, -23, -22, -21, -20, -19, -18, -17, -16,
	-14, -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1
};


/**
 * Build the gamma table so that floating point isn't needed.
 *
 * Note gamma goes from 0->256.  The old value of 100 is now 128.
 */
void build_gamma_table(int gamma)
{
	int i, n;

	/*
	 * value is the current sum.
	 * diff is the new term to add to the series.
	 */
	long value, diff;

	/* Hack - convergence is bad in these cases. */
	gamma_table[0] = 0;
	gamma_table[255] = 255;

	for (i = 1; i < 255; i++)
	{
		/*
		 * Initialise the Taylor series
		 *
		 * value and diff have been scaled by 256
		 */
		n = 1;
		value = 256L * 256L;
		diff = ((long)gamma_helper[i]) * (gamma - 256);

		while (diff)
		{
			value += diff;
			n++;

			/*
			 * Use the following identiy to calculate the gamma table.
			 * exp(x) = 1 + x + x^2/2 + x^3/(2*3) + x^4/(2*3*4) +...
			 *
			 * n is the current term number.
			 *
			 * The gamma_helper array contains a table of
			 * ln(x/256) * 256
			 * This is used because a^b = exp(b*ln(a))
			 *
			 * In this case:
			 * a is i / 256
			 * b is gamma.
			 *
			 * Note that everything is scaled by 256 for accuracy,
			 * plus another factor of 256 for the final result to
			 * be from 0-255.  Thus gamma_helper[] * gamma must be
			 * divided by 256*256 each itteration, to get back to
			 * the original power series.
			 */
			diff = (((diff / 256) * gamma_helper[i]) * (gamma - 256)) / (256 * n);
		}

		/*
		 * Store the value in the table so that the
		 * floating point pow function isn't needed.
		 */
		gamma_table[i] = ((long)(value / 256) * i) / 256;
	}
}

#endif /* SUPPORT_GAMMA */

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


/**
 * Accept values for y and x (considered as the endpoints of lines) between 
 * 0 and 40, and return an angle in degrees (divided by two). -LM-
 *
 * This table's input and output needs some processing:
 *
 * Because this table gives degrees for a whole circle, up to radius 20, its 
 * origin is at (x,y) = (20, 20).  Therefore, the input code needs to find 
 * the origin grid (where the lines being compared come from), and then map 
 * it to table grid 20,20.  Do not, however, actually try to compare the 
 * angle of a line that begins and ends at the origin with any other line - 
 * it is impossible mathematically, and the table will return the value "255".
 *
 * The output of this table also needs to be massaged, in order to avoid the 
 * discontinuity at 0/180 degrees.  This can be done by:
 *   rotate = 90 - first value 
 *   this rotates the first input to the 90 degree line)
 *   tmp = ABS(second value + rotate) % 180
 *   diff = ABS(90 - tmp) = the angular difference (divided by two) between 
 *   the first and second values.
 */
byte get_angle_to_grid[41][41] = 
  {
    {  67,  66,  66,  66,  66,  66,  66,  65,  63,  62,  61,  60,  58,  57,  55,  54,  52,  50,  48,  46,  45,  44,  41,  40,  38,  36,  35,  33,  32,  30,  29,  28,  27,  25,  24,  24,  23,  23,  23,  23,  22 },
    {  68,  67,  66,  66,  66,  66,  66,  65,  63,  62,  61,  60,  58,  57,  55,  54,  52,  50,  49,  47,  45,  43,  41,  40,  38,  36,  35,  33,  32,  30,  29,  28,  27,  25,  24,  24,  23,  23,  23,  22,  21 },
    {  68,  68,  67,  66,  66,  66,  66,  65,  63,  62,  61,  60,  58,  57,  55,  54,  52,  50,  49,  47,  45,  43,  41,  40,  38,  36,  35,  33,  32,  30,  29,  28,  27,  25,  24,  24,  23,  23,  22,  21,  21 },
    {  68,  68,  68,  67,  66,  66,  66,  65,  63,  62,  61,  60,  58,  57,  55,  54,  52,  50,  49,  47,  45,  43,  41,  40,  38,  36,  35,  33,  32,  30,  29,  28,  27,  25,  24,  24,  23,  22,  21,  21,  21 },
    {  68,  68,  68,  68,  67,  66,  66,  65,  63,  62,  61,  60,  58,  57,  55,  54,  52,  50,  49,  47,  45,  43,  41,  40,  38,  36,  35,  33,  32,  30,  29,  28,  27,  25,  24,  24,  22,  21,  21,  21,  21 },
    {  68,  68,  68,  68,  68,  67,  66,  65,  64,  63,  62,  60,  59,  58,  56,  54,  52,  51,  49,  47,  45,  43,  41,  39,  38,  36,  34,  32,  31,  30,  28,  27,  26,  25,  24,  22,  21,  21,  21,  21,  21 },
    {  69,  69,  69,  69,  69,  68,  67,  66,  65,  64,  63,  61,  60,  58,  57,  55,  53,  51,  49,  47,  45,  43,  41,  39,  37,  35,  33,  32,  30,  29,  27,  26,  25,  24,  22,  21,  21,  21,  21,  21,  21 },
    {  70,  70,  70,  70,  70,  70,  69,  67,  66,  65,  64,  62,  61,  59,  57,  56,  54,  51,  49,  47,  45,  43,  41,  39,  36,  34,  33,  31,  29,  28,  26,  25,  24,  22,  21,  20,  20,  20,  20,  20,  20 },
    {  72,  72,  72,  72,   72,  71,  70,  69,  67,  66,  65,  63,  62,  60,  58,  56,  54,  52,  50,  47,  45,  43,  40,  38,  36,  34,  32,  30,  28,  27,  25,  24,  22,  21,  20,  19,  18,  18,  18,  18,  18 },
    {  73,  73,  73,  73,  73,  72,  71,  70,  69,  67,  66,  65,  63,  61,  59,  57,  55,  53,  50,  48,  45,  42,  40,  37,  35,  33,  31,  29,  27,  25,  24,  22,  21,  20,  19,  18,  17,  17,  17,  17,  17 },
    {  74,  74,  74,  74,  74,  73,  72,  71,  70,  69,  67,  66,  64,  62,  60,  58,  56,  53,  51,  48,  45,  42,  39,  37,  34,  32,  30,  28,  26,  24,  22,  21,  20,  19,  18,  17,  16,  16,  16,  16,  16 },
    {  75,  75,  75,  75,  75,  75,  74,  73,  72,  70,  69,  67,  66,  64,  62,  60,  57,  54,  51,  48,  45,  42,  39,  36,  33,  30,  28,  26,  24,  22,  21,  20,  18,  17,  16,  15,  15,  15,  15,  15,  15 },
    {  77,  77,  77,  77,  77,  76,  75,  74,  73,  72,  71,  69,  67,  66,  63,  61,  58,  55,  52,  49,  45,  41,  38,  35,  32,  29,  27,  24,  22,  21,  19,  18,  17,  16,  15,  14,  13,  13,  13,  13,  13 },
    {  78,  78,  78,  78,  78,  77,  77,  76,  75,  74,  73,  71,  69,  67,  65,  63,  60,  57,  53,  49,  45,  41,  37,  33,  30,  27,  25,  22,  21,  19,  17,  16,  15,  14,  13,  13,  12,  12,  12,  12,  12 },
    {  80,  80,  80,  80,  80,  79,  78,  78,  77,  76,  75,  73,  72,  70,  67,  65,  62,  58,  54,  50,  45,  40,  36,  32,  28,  25,  22,  20,  18,  17,  15,  14,  13,  12,  12,  11,  10,  10,  10,  10,  10 },
    {  81,  81,  81,  81,  81,  81,  80,  79,  79,  78,  77,  75,  74,  72,  70,  67,  64,  60,  56,  51,  45,  39,  34,  30,  26,  22,  20,  18,  16,  15,  13,  12,  11,  11,  10,   9,   9,   9,   9,   9,   9 },
    {  83,  83,  83,  83,  83,  83,  82,  81,  81,  80,  79,  78,  77,  75,  73,  71,  67,  63,  58,  52,  45,  38,  32,  27,  22,  19,  17,  15,  13,  12,  11,  10,   9,   9,   8,   7,   7,   7,   7,   7,   7 },
    {  85,  85,  85,  85,  85,  84,  84,  84,  83,  82,  82,  81,  80,  78,  77,  75,  72,  67,  62,  54,  45,  36,  28,  22,  18,  15,  13,  12,  10,   9,   8,   8,   7,   6,   6,   6,   5,   5,   5,   5,   5 },
    {  86,  86,  86,  86,  86,  86,  86,  86,  85,  85,  84,  84,  83,  82,  81,  79,  77,  73,  67,  58,  45,  32,  22,  17,  13,  11,   9,   8,   7,   6,   6,   5,   5,   4,   4,   4,   4,   4,   4,   4,   3 },
    {  87,  88,  88,  88,  88,  88,  88,  88,  88,  87,  87,  87,  86,  86,  85,  84,  83,  81,  77,  67,  45,  22,  13,   9,   7,   6,   5,   4,   4,   3,   3,   3,   2,   2,   2,   2,   2,   2,   2,   2,   1 },
    {  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90,  90, 255,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
    {  91,  92,  92,  92,  92,  92,  92,  92,  92,  93,  93,  93,  94,  94,  95,  96,  97,  99, 103, 113, 135, 158, 167, 171, 173, 174, 175, 176, 176, 177, 177, 177, 178, 178, 178, 178, 178, 178, 178, 178, 179 },
    {  94,  94,  94,  94,  94,  94,  94,  94,  95,  95,  96,  96,  97,  98,  99, 101, 103, 107, 113, 122, 135, 148, 158, 163, 167, 169, 171, 172, 173, 174, 174, 175, 175, 176, 176, 176, 176, 176, 176, 176, 177 },
    {  95,  95,  95,  95,  95,  96,  96,  96,  97,  98,  98,  99, 100, 102, 103, 105, 108, 113, 118, 126, 135, 144, 152, 158, 162, 165, 167, 168, 170, 171, 172, 172, 173, 174, 174, 174, 175, 175, 175, 175, 175 },
    {  97,  97,  97,  97,  97,  97,  98,  99,  99, 100, 101, 102, 103, 105, 107, 109, 113, 117, 122, 128, 135, 142, 148, 153, 158, 161, 163, 165, 167, 168, 169, 170, 171, 171, 172, 173, 173, 173, 173, 173, 173 },
    {  99,  99,  99,  99,  99,  99, 100, 101, 101, 102, 103, 105, 106, 108, 110, 113, 116, 120, 124, 129, 135, 141, 146, 150, 154, 158, 160, 162, 164, 165, 167, 168, 169, 169, 170, 171, 171, 171, 171, 171, 171 },
    { 100, 100, 100, 100, 100, 101, 102, 102, 103, 104, 105, 107, 108, 110, 113, 115, 118, 122, 126, 130, 135, 140, 144, 148, 152, 155, 158, 160, 162, 163, 165, 166, 167, 168, 168, 169, 170, 170, 170, 170, 170 },
    { 102, 102, 102, 102, 102, 103, 103, 104, 105, 106, 107, 109, 111, 113, 115, 117, 120, 123, 127, 131, 135, 139, 143, 147, 150, 153, 155, 158, 159, 161, 163, 164, 165, 166, 167, 167, 168, 168, 168, 168, 168 },
    { 103, 103, 103, 103, 103, 104, 105, 106, 107, 108, 109, 111, 113, 114, 117, 119, 122, 125, 128, 131, 135, 139, 142, 145, 148, 151, 153, 156, 158, 159, 161, 162, 163, 164, 165, 166, 167, 167, 167, 167, 167 },
    { 105, 105, 105, 105, 105, 105, 106, 107, 108, 110, 111, 113, 114, 116, 118, 120, 123, 126, 129, 132, 135, 138, 141, 144, 147, 150, 152, 154, 156, 158, 159, 160, 162, 163, 164, 165, 165, 165, 165, 165, 165 },
    { 106, 106, 106, 106, 106, 107, 108, 109, 110, 111, 113, 114, 116, 118, 120, 122, 124, 127, 129, 132, 135, 138, 141, 143, 146, 148, 150, 152, 154, 156, 158, 159, 160, 161, 162, 163, 164, 164, 164, 164, 164 },
    { 107, 107, 107, 107, 107, 108, 109, 110, 111, 113, 114, 115, 117, 119, 121, 123, 125, 127, 130, 132, 135, 138, 140, 143, 145, 147, 149, 151, 153, 155, 156, 158, 159, 160, 161, 162, 163, 163, 163, 163, 163 },
    { 108, 108, 108, 108, 108, 109, 110, 111, 113, 114, 115, 117, 118, 120, 122, 124, 126, 128, 130, 133, 135, 137, 140, 142, 144, 146, 148, 150, 152, 153, 155, 156, 158, 159, 160, 161, 162, 162, 162, 162, 162 },
    { 110, 110, 110, 110, 110, 110, 111, 113, 114, 115, 116, 118, 119, 121, 123, 124, 126, 129, 131, 133, 135, 137, 139, 141, 144, 146, 147, 149, 151, 152, 154, 155, 156, 158, 159, 160, 160, 160, 160, 160, 160 },
    { 111, 111, 111, 111, 111, 112, 113, 114, 115, 116, 117, 119, 120, 122, 123, 125, 127, 129, 131, 133, 135, 137, 139, 141, 143, 145, 147, 148, 150, 151, 153, 154, 155, 156, 158, 159, 159, 159, 159, 159, 159 },
    { 112, 112, 112, 112, 112, 113, 114, 115, 116, 117, 118, 120, 121, 122, 124, 126, 128, 129, 131, 133, 135, 137, 139, 141, 142, 144, 146, 148, 149, 150, 152, 153, 154, 155, 157, 158, 159, 159, 159, 159, 159 },
    { 112, 112, 112, 112, 113, 114, 114, 115, 117, 118, 119, 120, 122, 123, 125, 126, 128, 130, 131, 133, 135, 137, 139, 140, 142, 144, 145, 147, 148, 150, 151, 152, 153, 155, 156, 157, 158, 159, 159, 159, 159 },
    { 112, 112, 112, 113, 113, 114, 114, 115, 117, 118, 119, 120, 122, 123, 125, 126, 128, 130, 131, 133, 135, 137, 139, 140, 142, 144, 145, 147, 148, 150, 151, 152, 153, 155, 156, 157, 158, 158, 158, 158, 158 },
    { 112, 112, 113, 114, 114, 114, 114, 115, 117, 118, 119, 120, 122, 123, 125, 126, 128, 130, 131, 133, 135, 137, 139, 140, 142, 144, 145, 147, 148, 150, 151, 152, 153, 155, 156, 157, 158, 158, 158, 158, 158 },
    { 112, 113, 114, 114, 114, 114, 114, 115, 117, 118, 119, 120, 122, 123, 125, 126, 128, 130, 131, 133, 135, 137, 139, 140, 142, 144, 145, 147, 148, 150, 151, 152, 153, 155, 156, 157, 158, 158, 158, 158, 158 },
    { 113, 114, 114, 114, 114, 114, 114, 115, 117, 118, 119, 120, 122, 123, 125, 126, 128, 130, 130, 132, 135, 136, 138, 140, 142, 144, 145, 147, 148, 150, 151, 152, 153, 155, 156, 157, 158, 158, 158, 158, 158 }
    
  };



/**
 * Returns a string which contains the name of a extended color.
 * Examples: "Dark", "Red1", "Yellow5", etc.
 * IMPORTANT: the returned string is statically allocated so it must *not* be
 * freed and its value changes between calls to this function.
 */
cptr get_ext_color_name(byte ext_color)
{
  static char buf[25];
  
  if (GET_SHADE(ext_color) > 0)
    {
      strnfmt(buf, sizeof(buf), "%s%d", color_names[GET_BASE_COLOR(ext_color)],
              GET_SHADE(ext_color));
    }
  else
    {
      strnfmt(buf, sizeof(buf), "%s", color_names[GET_BASE_COLOR(ext_color)]);
    }
  
  return buf;
}


/**
 * Converts a string to a terminal color byte.
 */
int color_text_to_attr(cptr name)
{
  int i, len, base, shade;
  
  /* Optimize name searching. See below */
  static byte len_names[MAX_BASE_COLORS];
  
  /* Separate the color name and the shade number */
  /* Only letters can be part of the name */
  for (i = 0; isalpha(name[i]); i++) ;
  
  /* Store the start of the shade number */
  len = i;
  
  /* Check for invalid characters in the shade part */
  while (name[i])
    {
      /* No digit, exit */
      if (!isdigit(name[i])) return (-1);
      ++i;
    }
  
  /* Initialize the shade */
  shade = 0;
  
  /* Only analyze the shade if there is one */
  if (name[len])
    {
      /* Convert to number */
      shade = atoi(name + len);
      
      /* Check bounds */
      if ((shade < 0) || (shade > MAX_SHADES - 1)) return (-1);
    }
  
  /* Extra, allow the use of strings like "r1", "U5", etc. */
  if (len == 1)
    {
      /* Convert one character, check sanity */
      if ((base = color_char_to_attr(name[0])) == -1) return (-1);
      
      /* Build the extended color */
      return (MAKE_EXTENDED_COLOR(base, shade));
    }
  
  /* Hack - Initialize the length array once */
  if (!len_names[0])
    {
      for (base = 0; base < MAX_BASE_COLORS; base++)
        {
          /* Store the length of each color name */
          len_names[base] = (byte)strlen(color_names[base & 0x0F]);
        }
    }
  
  /* Find the name */
  for (base = 0; base < MAX_BASE_COLORS; base++)
    {
      /* Somewhat optimize the search */
      if (len != len_names[base]) continue;
      
      /* Compare only the found name */
      if (my_strnicmp(name, color_names[base & 0x0F], len) == 0)
        {
          /* Build the extended color */
          return (MAKE_EXTENDED_COLOR(base, shade));
        }
    }
  
  /* We can not find it */
  return (-1);
}


static char *short_color_names[MAX_BASE_COLORS] =
{
  "Dark",
  "White",
  "Slate",
  "Orange",
  "Red",
  "Green",
  "Blue",
  "Umber",
  "L.Dark",
  "L.Slate",
  "Violet",
  "Yellow",
  "L.Red",
  "L.Green",
  "L.Blue",
  "L.Umber"
};

/**
 * Extract a textual representation of an attribute
 */
cptr attr_to_text(byte a)
{
  char *base;

  base = short_color_names[GET_BASE_COLOR(a)];

#if DO_YOU_WANT_THIS_IN_MONSTER_SPOILERS_Q

  if (GET_SHADE(a) > 0)
  {
    static char buf[25];

    strnfmt(buf, sizeof(buf), "%s%d", base, GET_SHADE(a));

    return (buf);
  }

#endif

  return (base);
}

/**
 * Path finding algorithm variables 
 */
static int terrain[MAX_PF_RADIUS][MAX_PF_RADIUS];
static int ox, oy, ex, ey;

bool is_valid_pf(int y, int x)
{
  feature_type *f_ptr = NULL;
  int feat = cave_feat[y][x];
  
  /* Hack -- assume unvisited is permitted */
  if (!(cave_info[y][x] & (CAVE_MARK))) return (TRUE);
  
  /* Get mimiced feat */
  f_ptr = &f_info[f_info[feat].mimic];
  
  /* Optionally alter known traps/doors on movement */
  if (((easy_open) && (f_ptr->flags & TF_DOOR_CLOSED)) ||
      ((easy_disarm) && (f_ptr->flags & TF_TRAP)))
    {
      return (TRUE);
    }
  
  /* Require moveable space */
  if (f_ptr->flags & TF_WALL) return (FALSE);

  /* Don't move over lava, web or void */
  if ((feat == FEAT_LAVA) || (feat == FEAT_WEB) || (feat == FEAT_VOID)) 
    return (FALSE);

  /* Otherwise good */
  return (TRUE);
}

static void fill_terrain_info(void)
{
  int i,j;
  
  ox = MAX(p_ptr->px - MAX_PF_RADIUS / 2, 0);
  oy = MAX(p_ptr->py - MAX_PF_RADIUS / 2, 0);

  ex = MIN(p_ptr->px + MAX_PF_RADIUS / 2 - 1, DUNGEON_WID);
  ey = MIN(p_ptr->py + MAX_PF_RADIUS / 2 - 1, DUNGEON_HGT);
        
  for (i = 0; i < MAX_PF_RADIUS * MAX_PF_RADIUS; i++)
    terrain[0][i] = -1;

  for (j = oy; j < ey; j++)
    for (i = ox; i < ex; i++)
      if (is_valid_pf(j, i))
        terrain[j - oy][i - ox] = MAX_PF_LENGTH;
  
  terrain[p_ptr->py - oy][p_ptr->px - ox] = 1;
}

#define MARK_DISTANCE(c,d) if ((c <= MAX_PF_LENGTH) && (c > d)) \
                              { c = d; try_again = (TRUE); }

bool findpath(int y, int x)
{
  int i, j, k, dir, starty = 0, startx = 0, startdir, start_index;
  bool try_again;
  int cur_distance;
  int findir[] = {1, 4, 7, 8, 9, 6, 3, 2};

  fill_terrain_info();

  terrain[p_ptr->py - oy][p_ptr->px - ox] = 1;

  if ((x >= ox) && (x < ex) && (y >= oy) && (y < ey))
    {
      if ((cave_m_idx[y][x] > 0) && (m_list[cave_m_idx[y][x]].ml))
        {
          terrain[y - oy][x - ox] = MAX_PF_LENGTH;
        }
      /* else if (terrain[y - oy][x - ox] != MAX_PF_LENGTH)
        {
        bell("Target blocked");
        return (FALSE);
        }*/
      terrain[y - oy][x - ox] = MAX_PF_LENGTH;
    }
  else
    {
      bell("Target out of range.");
      return (FALSE);
    }
  
  if (terrain[y - oy][x - ox] == -1)
    {
      bell("Target space forbidden");
      return (FALSE);
    }
  
  
  /* 
   * And now starts the very naive and very 
   * inefficient pathfinding algorithm
   */
  do
    {
      try_again = (FALSE);
      for (j = oy + 1; j < ey - 1; j++)
        for (i = ox + 1; i < ex - 1; i++)
          {
            cur_distance = terrain[j - oy][i - ox] + 1;
            if ((cur_distance > 0) && (cur_distance < MAX_PF_LENGTH))
              {
                for (dir = 1; dir < 10; dir++)
                  {
                    if (dir == 5) continue;
                    MARK_DISTANCE(terrain[j - oy + ddy[dir]][i - ox +ddx[dir]],
                                  cur_distance);
                  } 
              } 
          } 
      if (terrain[y - oy][x - ox] < MAX_PF_LENGTH)
        try_again = (FALSE);
    } while (try_again);
  
  /* Failure */
  if (terrain[y - oy][x - ox] == MAX_PF_LENGTH)
    {
      bell("Target space unreachable.");
      return (FALSE);
    }

  /* Success */
  i = x;
  j = y;
  
  pf_result_index = 0;
  
  while ((i != p_ptr->px) || (j != p_ptr->py))
    {
      int xdiff = i - p_ptr->px, ydiff = j - p_ptr->py;
      
      cur_distance = terrain[j - oy][i - ox] - 1;

      /* Starting direction */
      if (xdiff < 0) startx = 1;
      else if (xdiff > 0) startx = -1;
      else startx = 0;

      if (ydiff < 0) starty = 1;
      else if (ydiff > 0) starty = -1;
      else starty = 0;

      for (dir = 1; dir < 10; dir++)
        if ((ddx[dir] == startx) && (ddy[dir] == starty)) break;

      /* Should never happend */
      if ((dir % 5) == 0)
        {
          bell("Wtf ?");
          return (FALSE);
        }
      
      for (start_index = 0; findir[start_index % 8] != dir; start_index++) 
        ;

      for (k = 0; k < 5; k++)
        {
          dir = findir[(start_index + k) % 8];
          if (terrain[j - oy + ddy[dir]][i - ox + ddx[dir]] 
              == cur_distance)
            break; 
          dir = findir[(8 + start_index - k) % 8];
          if (terrain[j - oy + ddy[dir]][i - ox + ddx[dir]] 
              == cur_distance)
            break; 
        }

      /* Should never happend */
      if (k == 5)
        {
          bell("Heyyy !");
          return (FALSE);
        }
      
      pf_result[pf_result_index++] = '0' + (char)(10 - dir);
      i += ddx[dir];
      j += ddy[dir];
      starty = 0;
      startx = 0;
      startdir = 0;
    }
  pf_result_index--;
  return (TRUE);
}

/**
 * Move the cursor
 */
void move_cursor(int row, int col)
{
  Term_gotoxy(col, row);
}


