/* Textual dumping of CTF data.
   Copyright (C) 2019-2021 Free Software Foundation, Inc.

   This file is part of libctf.

   libctf is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not see
   <http://www.gnu.org/licenses/>.  */

#include <ctf-impl.h>
#include <string.h>

#define str_append(s, a) ctf_str_append_noerr (s, a)

/* One item to be dumped, in string form.  */

typedef struct ctf_dump_item
{
  ctf_list_t cdi_list;
  char *cdi_item;
} ctf_dump_item_t;

/* Cross-call state for dumping.  Basically just enough to track the section in
   use and a list of return strings.  */

struct ctf_dump_state
{
  ctf_sect_names_t cds_sect;
  ctf_dict_t *cds_fp;
  ctf_dump_item_t *cds_current;
  ctf_list_t cds_items;
};

/* Cross-call state for ctf_dump_member. */

typedef struct ctf_dump_membstate
{
  char **cdm_str;
  ctf_dict_t *cdm_fp;
  char *cdm_toplevel_indent;
} ctf_dump_membstate_t;

static int
ctf_dump_append (ctf_dump_state_t *state, char *str)
{
  ctf_dump_item_t *cdi;

  if ((cdi = malloc (sizeof (struct ctf_dump_item))) == NULL)
    return (ctf_set_errno (state->cds_fp, ENOMEM));

  cdi->cdi_item = str;
  ctf_list_append (&state->cds_items, cdi);
  return 0;
}

static void
ctf_dump_free (ctf_dump_state_t *state)
{
  ctf_dump_item_t *cdi, *next_cdi;

  if (state == NULL)
    return;

  for (cdi = ctf_list_next (&state->cds_items); cdi != NULL;
       cdi = next_cdi)
    {
      free (cdi->cdi_item);
      next_cdi = ctf_list_next (cdi);
      free (cdi);
    }
}

/* Return a dump for a single type, without member info: but do show the
   type's references.  */

static char *
ctf_dump_format_type (ctf_dict_t *fp, ctf_id_t id, int flag)
{
  ctf_id_t new_id;
  char *str = NULL, *bit = NULL, *buf = NULL;

  new_id = id;
  do
    {
      ctf_encoding_t enc;
      const char *nonroot_leader = "";
      const char *nonroot_trailer = "";

      id = new_id;
      if (flag == CTF_ADD_NONROOT)
	{
	  nonroot_leader = "{";
	  nonroot_trailer = "}";
	}

      buf = ctf_type_aname (fp, id);
      if (!buf)
	{
	  if (id == 0 || ctf_errno (fp) == ECTF_NONREPRESENTABLE)
	    {
	      str = str_append (str, " (type not represented in CTF)");
	      ctf_set_errno (fp, ECTF_NOTREF);
	      break;
	    }

	  goto err;
	}

      if (asprintf (&bit, " %s0x%lx: ", nonroot_leader, id) < 0)
	goto oom;
      str = str_append (str, bit);
      free (bit);
      bit = NULL;

      if (buf[0] != '\0')
	{
	  str = str_append (str, buf);
	  str = str_append (str, " ");
	}

      free (buf);
      buf = NULL;

      /* Slices get a different print representation.  */
      if (ctf_type_kind_unsliced (fp, id) == CTF_K_SLICE)
	{
	  ctf_type_encoding (fp, id, &enc);
	  if (asprintf (&bit, "[slice 0x%x:0x%x] ",
			enc.cte_offset, enc.cte_bits) < 0)
	    goto oom;
	}
      else if (ctf_type_kind (fp, id) == CTF_K_INTEGER)
	{
	  ctf_type_encoding (fp, id, &enc);
	  if (asprintf (&bit, "[0x%x:0x%x] ",
			enc.cte_offset, enc.cte_bits) < 0)
	    goto oom;
	}
      str = str_append (str, bit);
      free (bit);
      bit = NULL;

      if (asprintf (&bit, "(size 0x%lx)%s",
		    (unsigned long) ctf_type_size (fp, id),
		    nonroot_trailer) < 0)
	goto oom;

      str = str_append (str, bit);
      free (bit);
      bit = NULL;

      new_id = ctf_type_reference (fp, id);
      if (new_id != CTF_ERR)
	str = str_append (str, " ->");
    } while (new_id != CTF_ERR);

  if (ctf_errno (fp) != ECTF_NOTREF)
    {
      free (str);
      return NULL;
    }

  return str;

 oom:
  ctf_set_errno (fp, errno);
 err:
  ctf_err_warn (fp, 1, 0, _("cannot format name dumping type 0x%lx"), id);
  free (buf);
  free (str);
  free (bit);
  return NULL;
}

/* Dump one string field from the file header into the cds_items.  */
static int
ctf_dump_header_strfield (ctf_dict_t *fp, ctf_dump_state_t *state,
			  const char *name, uint32_t value)
{
  char *str;
  if (value)
    {
      if (asprintf (&str, "%s: %s\n", name, ctf_strptr (fp, value)) < 0)
	goto err;
      ctf_dump_append (state, str);
    }
  return 0;

 err:
  return (ctf_set_errno (fp, errno));
}

/* Dump one section-offset field from the file header into the cds_items.  */
static int
ctf_dump_header_sectfield (ctf_dict_t *fp, ctf_dump_state_t *state,
			   const char *sect, uint32_t off, uint32_t nextoff)
{
  char *str;
  if (nextoff - off)
    {
      if (asprintf (&str, "%s:\t0x%lx -- 0x%lx (0x%lx bytes)\n", sect,
		    (unsigned long) off, (unsigned long) (nextoff - 1),
		    (unsigned long) (nextoff - off)) < 0)
	goto err;
      ctf_dump_append (state, str);
    }
  return 0;

 err:
  return (ctf_set_errno (fp, errno));
}

/* Dump the file header into the cds_items.  */
static int
ctf_dump_header (ctf_dict_t *fp, ctf_dump_state_t *state)
{
  char *str;
  char *flagstr = NULL;
  const ctf_header_t *hp = fp->ctf_header;
  const char *vertab[] =
    {
     NULL, "CTF_VERSION_1",
     "CTF_VERSION_1_UPGRADED_3 (latest format, version 1 type "
     "boundaries)",
     "CTF_VERSION_2",
     "CTF_VERSION_3", NULL
    };
  const char *verstr = NULL;

  if (asprintf (&str, "Magic number: 0x%x\n", hp->cth_magic) < 0)
      goto err;
  ctf_dump_append (state, str);

  if (hp->cth_version <= CTF_VERSION)
    verstr = vertab[hp->cth_version];

  if (verstr == NULL)
    verstr = "(not a valid version)";

  if (asprintf (&str, "Version: %i (%s)\n", hp->cth_version,
		verstr) < 0)
    goto err;
  ctf_dump_append (state, str);

  /* Everything else is only printed if present.  */

  /* The flags are unusual in that they represent the ctf_dict_t *in memory*:
     flags representing compression, etc, are turned off as the file is
     decompressed.  So we store a copy of the flags before they are changed, for
     the dumper.  */

  if (fp->ctf_openflags > 0)
    {
      if (asprintf (&flagstr, "%s%s%s%s%s%s%s",
		    fp->ctf_openflags & CTF_F_COMPRESS
		    ? "CTF_F_COMPRESS": "",
		    (fp->ctf_openflags & CTF_F_COMPRESS)
		    && (fp->ctf_openflags & ~CTF_F_COMPRESS)
		    ? ", " : "",
		    fp->ctf_openflags & CTF_F_NEWFUNCINFO
		    ? "CTF_F_NEWFUNCINFO" : "",
		    (fp->ctf_openflags & (CTF_F_COMPRESS | CTF_F_NEWFUNCINFO))
		    && (fp->ctf_openflags & ~(CTF_F_COMPRESS | CTF_F_NEWFUNCINFO))
		    ? ", " : "",
		    fp->ctf_openflags & CTF_F_IDXSORTED
		    ? "CTF_F_IDXSORTED" : "",
		    fp->ctf_openflags & (CTF_F_COMPRESS | CTF_F_NEWFUNCINFO
					 | CTF_F_IDXSORTED)
		    && (fp->ctf_openflags & ~(CTF_F_COMPRESS | CTF_F_NEWFUNCINFO
					      | CTF_F_IDXSORTED))
		    ? ", " : "",
		    fp->ctf_openflags & CTF_F_DYNSTR
		    ? "CTF_F_DYNSTR" : "") < 0)
	goto err;

      if (asprintf (&str, "Flags: 0x%x (%s)", fp->ctf_openflags, flagstr) < 0)
	goto err;
      ctf_dump_append (state, str);
    }

  if (ctf_dump_header_strfield (fp, state, "Parent label",
				hp->cth_parlabel) < 0)
    goto err;

  if (ctf_dump_header_strfield (fp, state, "Parent name", hp->cth_parname) < 0)
    goto err;

  if (ctf_dump_header_strfield (fp, state, "Compilation unit name",
				hp->cth_cuname) < 0)
    goto err;

  if (ctf_dump_header_sectfield (fp, state, "Label section", hp->cth_lbloff,
				 hp->cth_objtoff) < 0)
    goto err;

  if (ctf_dump_header_sectfield (fp, state, "Data object section",
				 hp->cth_objtoff, hp->cth_funcoff) < 0)
    goto err;

  if (ctf_dump_header_sectfield (fp, state, "Function info section",
				 hp->cth_funcoff, hp->cth_objtidxoff) < 0)
    goto err;

  if (ctf_dump_header_sectfield (fp, state, "Object index section",
				 hp->cth_objtidxoff, hp->cth_funcidxoff) < 0)
    goto err;

  if (ctf_dump_header_sectfield (fp, state, "Function index section",
				 hp->cth_funcidxoff, hp->cth_varoff) < 0)
    goto err;

  if (ctf_dump_header_sectfield (fp, state, "Variable section",
				 hp->cth_varoff, hp->cth_typeoff) < 0)
    goto err;

  if (ctf_dump_header_sectfield (fp, state, "Type section",
				 hp->cth_typeoff, hp->cth_stroff) < 0)
    goto err;

  if (ctf_dump_header_sectfield (fp, state, "String section", hp->cth_stroff,
				 hp->cth_stroff + hp->cth_strlen + 1) < 0)
    goto err;

  return 0;
 err:
  free (flagstr);
  return (ctf_set_errno (fp, errno));
}

/* Dump a single label into the cds_items.  */

static int
ctf_dump_label (const char *name, const ctf_lblinfo_t *info,
		void *arg)
{
  char *str;
  char *typestr;
  ctf_dump_state_t *state = arg;

  if (asprintf (&str, "%s -> ", name) < 0)
    return (ctf_set_errno (state->cds_fp, errno));

  if ((typestr = ctf_dump_format_type (state->cds_fp, info->ctb_type,
				       CTF_ADD_ROOT)) == NULL)
    {
      free (str);
      return 0;				/* Swallow the error.  */
    }

  str = str_append (str, typestr);
  free (typestr);

  ctf_dump_append (state, str);
  return 0;
}

/* Dump all the object or function entries into the cds_items.  */

static int
ctf_dump_objts (ctf_dict_t *fp, ctf_dump_state_t *state, int functions)
{
  const char *name;
  ctf_id_t id;
  ctf_next_t *i = NULL;
  char *str = NULL;

  if ((functions && fp->ctf_funcidx_names)
      || (!functions && fp->ctf_objtidx_names))
    str = str_append (str, _("Section is indexed.\n"));
  else if (fp->ctf_symtab.cts_data == NULL)
    str = str_append (str, _("No symbol table.\n"));

  while ((id = ctf_symbol_next (fp, &i, &name, functions)) != CTF_ERR)
    {
      char *typestr = NULL;
      int err = 0;

      /* Emit the name, if we know it.  */
      if (name)
	{
	  if (asprintf (&str, "%s -> ", name) < 0)
	    goto oom;
	}
      else
	str = xstrdup ("");

      if ((typestr = ctf_type_aname (fp, id)) == NULL)
	{
	  if (id == 0 || ctf_errno (fp) == ECTF_NONREPRESENTABLE)
	    {
	      if (asprintf (&typestr, " (%s)", _("type not represented in CTF")) < 0)
		goto oom;

	      goto out;
	    }

	  if (asprintf (&typestr, _("error: %s"), ctf_errmsg (ctf_errno (fp))) < 0)
	    goto oom;

	  err = -1;
	  goto out;
	}

      str = str_append (str, typestr);
      str = str_append (str, "\n");
      ctf_dump_append (state, str);
      continue;

    oom:
      ctf_set_errno (fp, ENOMEM);
      ctf_next_destroy (i);
      return -1;
    out:
      str = str_append (str, typestr);
      free (typestr);
      ctf_dump_append (state, str);
      ctf_next_destroy (i);
      return err;				/* errno is set for us.  */
    }
  return 0;
}

/* Dump a single variable into the cds_items.  */
static int
ctf_dump_var (const char *name, ctf_id_t type, void *arg)
{
  char *str;
  char *typestr;
  ctf_dump_state_t *state = arg;

  if (asprintf (&str, "%s -> ", name) < 0)
    return (ctf_set_errno (state->cds_fp, errno));

  if ((typestr = ctf_dump_format_type (state->cds_fp, type,
				       CTF_ADD_ROOT)) == NULL)
    {
      free (str);
      return 0;			/* Swallow the error.  */
    }

  str = str_append (str, typestr);
  free (typestr);

  ctf_dump_append (state, str);
  return 0;
}

/* Report the number of digits in the hexadecimal representation of a type
   ID.  */

static int
type_hex_digits (ctf_id_t id)
{
  int i = 0;

  if (id == 0)
    return 1;

  for (; id > 0; id >>= 4, i++);
  return i;
}

/* Dump a single member into the string in the membstate.  */
static int
ctf_dump_member (const char *name, ctf_id_t id, unsigned long offset,
		 int depth, void *arg)
{
  ctf_dump_membstate_t *state = arg;
  char *typestr = NULL;
  char *bit = NULL;
  ctf_encoding_t ep;
  int has_encoding = 0;

  /* Align neatly.  */

  if (depth == 0)
    {
      if (asprintf (&state->cdm_toplevel_indent, "     %*s",
		    type_hex_digits (id), "") < 0)
	goto oom;
    }

  if (asprintf (&bit, "%s%*s", state->cdm_toplevel_indent, depth * 4, "") < 0)
    goto oom;
  *state->cdm_str = str_append (*state->cdm_str, bit);
  free (bit);

  if ((typestr = ctf_type_aname (state->cdm_fp, id)) == NULL)
    {
      if (id == 0 || ctf_errno (state->cdm_fp) == ECTF_NONREPRESENTABLE)
	{
	  if (asprintf (&bit, "[0x%lx] (type not represented in CTF)",
			offset) < 0)
	    goto oom;

	  *state->cdm_str = str_append (*state->cdm_str, bit);
	  free (typestr);
	  free (bit);
	  return 0;
	}

      return -1;				/* errno is set for us.  */
    }

  if (ctf_type_encoding (state->cdm_fp, id, &ep) == 0)
    {
      has_encoding = 1;
      ctf_type_encoding (state->cdm_fp, id, &ep);

      if (asprintf (&bit, "[0x%lx] (ID 0x%lx) (kind %i) %s%s%s:%i "
		    "(aligned at 0x%lx", offset, id,
		    ctf_type_kind (state->cdm_fp, id), typestr,
		    (name[0] != 0 && typestr[0] != 0) ? " " : "", name,
		    ep.cte_bits, (unsigned long) ctf_type_align (state->cdm_fp,
								 id)) < 0)
	goto oom;
    }
  else
    {
      if (asprintf (&bit, "[0x%lx] (ID 0x%lx) (kind %i) %s%s%s "
		    "(aligned at 0x%lx", offset, id,
		    ctf_type_kind (state->cdm_fp, id), typestr,
		    (name[0] != 0 && typestr[0] != 0) ? " " : "", name,
		    (unsigned long) ctf_type_align (state->cdm_fp, id)) < 0)
	goto oom;
    }

  *state->cdm_str = str_append (*state->cdm_str, bit);
  free (typestr);
  free (bit);
  typestr = NULL;
  bit = NULL;

  if (has_encoding)
    {
      if (asprintf (&bit, ", format 0x%x, offset:bits 0x%x:0x%x", ep.cte_format,
		    ep.cte_offset, ep.cte_bits) < 0)
	goto oom;
      *state->cdm_str = str_append (*state->cdm_str, bit);
      free (bit);
      bit = NULL;
    }

  *state->cdm_str = str_append (*state->cdm_str, ")\n");
  return 0;

 oom:
  free (typestr);
  free (bit);
  return (ctf_set_errno (state->cdm_fp, errno));
}

/* Dump a single type into the cds_items.  */
static int
ctf_dump_type (ctf_id_t id, int flag, void *arg)
{
  char *str;
  ctf_dump_state_t *state = arg;
  ctf_dump_membstate_t membstate = { &str, state->cds_fp, NULL };
  size_t len;

  if ((str = ctf_dump_format_type (state->cds_fp, id, flag)) == NULL)
    goto err;

  str = str_append (str, "\n");
  if ((ctf_type_visit (state->cds_fp, id, ctf_dump_member, &membstate)) < 0)
    {
      if (id == 0 || ctf_errno (state->cds_fp) == ECTF_NONREPRESENTABLE)
	{
	  ctf_dump_append (state, str);
	  return 0;
	}
      ctf_err_warn (state->cds_fp, 1, ctf_errno (state->cds_fp),
		    _("cannot visit members dumping type 0x%lx"), id);
      goto err;
    }
  free (membstate.cdm_toplevel_indent);

  /* Trim off the last linefeed added by ctf_dump_member().  */
  len = strlen (str);
  if (str[len-1] == '\n')
    str[len-1] = '\0';

  ctf_dump_append (state, str);
  return 0;

 err:
  free (membstate.cdm_toplevel_indent);
  free (str);
  return 0;				/* Swallow the error.  */
}

/* Dump the string table into the cds_items.  */

static int
ctf_dump_str (ctf_dict_t *fp, ctf_dump_state_t *state)
{
  const char *s = fp->ctf_str[CTF_STRTAB_0].cts_strs;

  for (; s < fp->ctf_str[CTF_STRTAB_0].cts_strs +
	 fp->ctf_str[CTF_STRTAB_0].cts_len;)
    {
      char *str;
      if (asprintf (&str, "0x%lx: %s",
		    (unsigned long) (s - fp->ctf_str[CTF_STRTAB_0].cts_strs),
		    s) < 0)
	return (ctf_set_errno (fp, errno));
      ctf_dump_append (state, str);
      s += strlen (s) + 1;
    }

  return 0;
}

/* Dump a particular section of a CTF file, in textual form.  Call with a
   pointer to a NULL STATE: each call emits a dynamically allocated string
   containing a description of one entity in the specified section, in order.
   Only the first call (with a NULL state) may vary SECT.  Once the CTF section
   has been entirely dumped, the call returns NULL and frees and annuls the
   STATE, ready for another section to be dumped.  The returned textual content
   may span multiple lines: between each call the FUNC is called with one
   textual line at a time, and should return a suitably decorated line (it can
   allocate a new one and return it if it likes).  */

char *
ctf_dump (ctf_dict_t *fp, ctf_dump_state_t **statep, ctf_sect_names_t sect,
	  ctf_dump_decorate_f *func, void *arg)
{
  char *str;
  char *line;
  ctf_dump_state_t *state = NULL;

  if (*statep == NULL)
    {
      /* Data collection.  Transforming a call-at-a-time iterator into a
	 return-at-a-time iterator in a language without call/cc is annoying. It
	 is easiest to simply collect everything at once and then return it bit
	 by bit.  The first call will take (much) longer than otherwise, but the
	 amortized time needed is the same.  */

      if ((*statep = malloc (sizeof (struct ctf_dump_state))) == NULL)
	{
	  ctf_set_errno (fp, ENOMEM);
	  goto end;
	}
      state = *statep;

      memset (state, 0, sizeof (struct ctf_dump_state));
      state->cds_fp = fp;
      state->cds_sect = sect;

      switch (sect)
	{
	case CTF_SECT_HEADER:
	  ctf_dump_header (fp, state);
	  break;
	case CTF_SECT_LABEL:
	  if (ctf_label_iter (fp, ctf_dump_label, state) < 0)
	    {
	      if (ctf_errno (fp) != ECTF_NOLABELDATA)
		goto end;		/* errno is set for us.  */
	      ctf_set_errno (fp, 0);
	    }
	  break;
	case CTF_SECT_OBJT:
	  if (ctf_dump_objts (fp, state, 0) < 0)
	    goto end;			/* errno is set for us.  */
	  break;
	case CTF_SECT_FUNC:
	  if (ctf_dump_objts (fp, state, 1) < 0)
	    goto end;			/* errno is set for us.  */
	  break;
	case CTF_SECT_VAR:
	  if (ctf_variable_iter (fp, ctf_dump_var, state) < 0)
	    goto end;			/* errno is set for us.  */
	  break;
	case CTF_SECT_TYPE:
	  if (ctf_type_iter_all (fp, ctf_dump_type, state) < 0)
	    goto end;			/* errno is set for us.  */
	  break;
	case CTF_SECT_STR:
	  ctf_dump_str (fp, state);
	  break;
	default:
	  ctf_set_errno (fp, ECTF_DUMPSECTUNKNOWN);
	  goto end;
	}
    }
  else
    {
      state = *statep;

      if (state->cds_sect != sect)
	{
	  ctf_set_errno (fp, ECTF_DUMPSECTCHANGED);
	  goto end;
	}
    }

  if (state->cds_current == NULL)
    state->cds_current = ctf_list_next (&state->cds_items);
  else
    state->cds_current = ctf_list_next (state->cds_current);

  if (state->cds_current == NULL)
    goto end;

  /* Hookery.  There is some extra complexity to preserve linefeeds within each
     item while removing linefeeds at the end.  */
  if (func)
    {
      size_t len;

      str = NULL;
      for (line = state->cds_current->cdi_item; line && *line; )
	{
	  char *nline = line;
	  char *ret;

	  nline = strchr (line, '\n');
	  if (nline)
	    nline[0] = '\0';

	  ret = func (sect, line, arg);
	  str = str_append (str, ret);
	  str = str_append (str, "\n");
	  if (ret != line)
	    free (ret);

	  if (nline)
	    {
	      nline[0] = '\n';
	      nline++;
	    }

	  line = nline;
	}

      len = strlen (str);

      if (str[len-1] == '\n')
	str[len-1] = '\0';
    }
  else
    {
      str = strdup (state->cds_current->cdi_item);
      if (!str)
	{
	  ctf_set_errno (fp, ENOMEM);
	  return str;
	}
    }

  ctf_set_errno (fp, 0);
  return str;

 end:
  ctf_dump_free (state);
  free (state);
  ctf_set_errno (fp, 0);
  *statep = NULL;
  return NULL;
}
