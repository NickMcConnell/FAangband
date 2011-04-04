/** \file gen-themed.c
    \brief Initialization of themed levels

 * Old initialization of themed levels - needs update
 * 
 * Copyright (c) 1997-2011 Leon Marrick, Bahman Rabii, Nick McConnell 
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
 *
 */

#include "angband.h"


struct header
{
        byte v_major;           /* Version -- major */
        byte v_minor;           /* Version -- minor */
        byte v_patch;           /* Version -- patch */
        byte v_extra;           /* Version -- extra */


        u16b info_num;          /* Number of "info" records */

        u16b info_len;          /* Size of each "info" record */


        u32b head_size;         /* Size of the "header" in bytes */

        u32b info_size;         /* Size of the "info" array in bytes */

        u32b name_size;         /* Size of the "name" array in bytes */

        u32b text_size;         /* Size of the "text" array in bytes */

};

header *t_head;

/**
 * Initialize the "t_info" array, by parsing an ascii "template" file.
 * Load only the selected themed level into memory (this allows an arbi-
 * trarily large t_info.txt file). -LM-
 * Otherwise, code is essentially that of "init_v_info_txt".
 */
errr init_t_info_txt(ang_file *fp, char *buf, byte chosen_level)
{
  int i;
  
  /* Assume we cannot start collecting data yet. */
  bool can_use = FALSE;
  
  char *s;
  
  /* Current entry */
  vault_type *t_ptr = NULL;
  
#ifndef NO_THEMED_LEVELS /* Themed levels and old machines don't mix. */
  
  /* Just before the first record */
  error_idx = -1;
  
  /* Just before the first line */
  error_line = -1;
  
  
  /* Prepare the "fake" stuff */
  t_head->name_size = 0;
  t_head->text_size = 0;
  
  /* Parse */
  while (file_getl(fp, buf, 1024))
    {
      /* Advance the line number */
      error_line++;
      
      /* Skip comments and blank lines */
      if (!buf[0] || (buf[0] == '#')) continue;
      
      /* Verify correct "colon" format */
      if (buf[1] != ':') return (1);
      
      
      /* Process 'N' for "New/Number/Name" */
      if (buf[0] == 'N')
        {
          /* Find the colon before the name */
          s = strchr(buf+2, ':');
          
          /* Verify that colon */
          if (!s) return (1);
          
          /* Nuke the colon, advance to the name */
          *s++ = '\0';
          
          /* Paranoia -- require a name */
          if (!*s) return (1);
          
          /* Get the index */
          i = atoi(buf+2);
          
          /* If correct themed level found, start processing information. */
          if (i == chosen_level) can_use = TRUE;
          else 
            {
              can_use = FALSE;
              continue;
            }
          
          /* Verify information */
          if (i <= error_idx) return (4);
          
          /* Verify information */
          if (i >= t_head->info_num) return (2);
          
          /* Save the index */
          error_idx = i;
          
          /* Point at the "info" */
          t_ptr = &t_info[i];
          
          /* Hack -- Verify space */
          if (t_head->name_size + strlen(s) + 8 > FAKE_NAME_SIZE) return (7);
          
          /* Advance and Save the name index */
          if (!t_ptr->name) t_ptr->name = ++t_head->name_size;
          
          /* Append chars to the name */
          strcpy(t_name + t_head->name_size, s);
          
          /* Advance the index */
          t_head->name_size += strlen(s);
          
          /* Next... */
          continue;
        }
      
      /* There better be a current t_ptr */
      if ((can_use) && (!t_ptr)) return (3);
      
      
      /* Process 'M' for special themed level feeling. */
      if (buf[0] == 'M')
        {
          /* Accept only correct themed level information. */
          if (!can_use) continue;
          
          /* Acquire the text */
          s = buf+2;
          
          /* Copy the message */
          strcpy(themed_feeling, s);
          
          /* Next... */
          continue;
        }
      
      
      /* Process 'D' for "Description" */
      if (buf[0] == 'D')
        {
          /* Accept only correct themed level information. */
          if (!can_use) continue;
          
          /* Acquire the text */
          s = buf+2;
          
          /* Hack -- Verify space */
          if (t_head->text_size + strlen(s) + 8 > FAKE_TEXT_SIZE) return (7);
          
          /* Advance and Save the text index */
          if (!t_ptr->text) t_ptr->text = ++t_head->text_size;
          
          /* Append chars to the name */
          strcpy(t_text + t_head->text_size, s);
          
          /* Advance the index */
          t_head->text_size += strlen(s);
          
          /* Next... */
          continue;
        }
      
      /* Oops */
      return (6);
    }
  
  /* Eventually, the correct themed level must be found. */
  if (!t_ptr) return (3);
  
  
  /* Complete the "name" and "text" sizes */
  ++t_head->name_size;
  ++t_head->text_size;
  
#endif

  /* Success */
  return (0);
}


/**
 * Initialize the "t_info" array.  Code from "init_v_info". -LM-
 * Unlike other, similar arrays, that for themed levels is not written to a 
 * binary file, since themed levels are neither important enough to warrant 
 * the use of extra space, nor used often enough to worry excessively about 
 * speed.
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
errr init_t_info(byte chosen_level)
{
  errr err;
  
  ang_file *fp;
  
  /* General buffer */
  char buf[1024];
  
#ifndef NO_THEMED_LEVELS        /* Themed levels and old machines don't mix. */
  
  /*** Make the header ***/
  
  /* Allocate the "header" */
  MAKE(t_head, header);
  
  /* Save the "version" */
  t_head->v_major = VERSION_MAJOR;
  t_head->v_minor = VERSION_MINOR;
  t_head->v_patch = VERSION_PATCH;
  t_head->v_extra = 0;
  
  /* Save the "record" information */
  t_head->info_num = z_info->v_max;
  t_head->info_len = sizeof(vault_type);
  
  /* Save the size of "v_head" and "v_info" */
  t_head->head_size = sizeof(header);
  t_head->info_size = t_head->info_num * t_head->info_len;
  
  
  /*** Make the fake arrays ***/
  
  /* Allocate the "t_info" array */
  C_MAKE(t_info, t_head->info_num, vault_type);
  
  /* Hack -- make "fake" arrays */
  C_MAKE(t_name, FAKE_NAME_SIZE, char);
  C_MAKE(t_text, FAKE_TEXT_SIZE, char);
  
  
  /*** Load the ascii template file ***/
  
  /* Build the filename */
  path_build(buf, 1024, ANGBAND_DIR_EDIT, "themed.txt");

  /* Open the file */
  fp = file_open(buf, MODE_READ, FTYPE_TEXT);
  
  /* Parse it, canceling on failure. */
  if (!fp) return(TRUE);
  
  /* Parse the file */
  err = init_t_info_txt(fp, buf, chosen_level);
  
  /* Close it */
  file_close(fp);
  
  /* Errors */
  if (err)
    {
      cptr oops;
      
      /* Error string */
      oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");
      
      /* Oops */
      msg_format("Non-critical error: themed.txt is unusable.");
      msg_format("Error %d at line %d of 'themed.txt'.", err, error_line);
      msg_format("Record %d contains a '%s' error.", error_idx, oops);
      msg_format("Parsing '%s'.", buf);
      msg_print(NULL);
      
      /* Failure */
      return(TRUE);
    }
  
  
  
  
  /* Success */
  return (0);
  
#else
  
  /* No themed levels allowed if compiler option forbids them. */
  return(1);
  
#endif
}

/**
 * Release memory used to store information about a themed level. -LM-
 */
void kill_t_info(void)
{
  
  /*** Kill the fake arrays ***/
  
  /* Free the "t_info" array */
  FREE(t_info);
  
  /* Hack -- Free the "fake" arrays */
  FREE(t_name);
  FREE(t_text);
}

