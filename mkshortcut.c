/* mkshortcut.c -- create a Windows shortcut 
 *
 * Copyright (c) 2002 Joshua Daniel Franklin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * See the COPYING file for license information.
 *
 * Exit values
 *   1: user error (syntax error)
 *   2: system error (out of memory, etc.)
 *   3: windows error (interface failed)
 * 
 * Compile with: gcc -o prog.exe mkshortcut.c -lole32 -luuid
 *  (You'd need to uncomment the moved to common.h lines.)
 *
 */
#if HAVE_CONFIG_H
#  include "config.h"
#endif
#include "common.h"

#define NOCOMATTRIBUTE

#include <shlobj.h>
#include <olectlid.h>
/* moved to common.h */
/*
#include <stdio.h>
#include <getopt.h>
*/

static const char version[] = "$Revision: 1.2 $";
static const char versionID[] = "1.01.0";
/* for CVS */
static const char revID[] =
  "$Id: mkshortcut.c,v 1.2 2002/02/24 03:41:44 cwilson Exp $";
static const char copyrightID[] =
  "Copyright (c) 2002\nJoshua Daniel Franklin. All rights reserved.\nLicensed under GPL v2.0\n";

static char *prog_name;
static char *argument_arg, *name_arg;
static int icon_flag, unix_flag, windows_flag;
static int allusers_flag, desktop_flag, smprograms_flag;

static struct option long_options[] = {
  {"arguments", required_argument, NULL, 'a'},
  {"help", no_argument, NULL, 'h'},
  {"icon", required_argument, NULL, 'i'},
  {"iconoffset", required_argument, NULL, 'j'},
  {"name", required_argument, NULL, 'n'},
  {"version", no_argument, NULL, 'v'},
  {"allusers", no_argument, NULL, 'A'},
  {"desktop", no_argument, NULL, 'D'},
  {"smprograms", no_argument, NULL, 'P'},
  {NULL, 0, NULL, 0}
};

static void
usage (FILE * stream, int status)
{
  fprintf (stream, "\
Usage %s [OPTION]... TARGET \n\
NOTE: All filename arguments must be in unix (POSIX) format\n\
  -a|--arguments=ARGS	use arguments ARGS \n\
  -h|--help		output usage information and exit\n\
  -i|--icon		icon file for link to use\n\
  -j|--iconoffset	offset of icon in icon file (default is 0)\n\
  -n|--name		name for link (defaults to TARGET)\n\
  -v|--version		output version information and exit\n\
  -A|--allusers		use 'All Users' instead of current user for -D,-P\n\
  -D|--desktop		create link relative to 'Desktop' directory\n\
  -P|--smprograms	create link relative to Start Menu 'Programs' directory\
", prog_name);
  exit (status);
}

print_version ()
{
  const char *v = strchr (version, ':');
  int len;
  if (!v)
    {
      v = "?";
      len = 1;
    }
  else
    {
      v += 2;
      len = strchr (v, ' ') - v;
    }
  printf ("\
mkshortcut (cygutils) %.*s\n%s\
", len, v, copyrightID);
}

int
main (int argc, char **argv)
{
  int opt, tmp, offset;
  char *args, *tmp_str, *buf_str;
  char link_name[MAX_PATH];
  char exe_name[MAX_PATH];
  char dir_name[MAX_PATH];
  char icon_name[MAX_PATH];

  /* For OLE interface */
  LPITEMIDLIST id;
  HRESULT hres;
  IShellLink *shell_link;
  IPersistFile *persist_file;
  WCHAR widepath[MAX_PATH];

  buf_str = (char *) malloc (PATH_MAX);
  if (buf_str == NULL)
    {
      fprintf (stderr, "%s: out of memory\n", prog_name);
      exit (2);
    }

  offset = 0;
  icon_flag = 0;
  unix_flag = 0;
  windows_flag = 0;
  allusers_flag = 0;
  desktop_flag = 0;
  smprograms_flag = 0;

  prog_name = strrchr (argv[0], '/');
  if (prog_name == NULL)
    prog_name = strrchr (argv[0], '\\');
  if (prog_name == NULL)
    prog_name = argv[0];
  else
    prog_name++;

  while ((opt =
	  getopt_long (argc, argv, (char *) "a:i:j:n:hvADP", long_options,
		       NULL)) != EOF)
    {
      switch (opt)
	{
	case 'a':
	  argument_arg = optarg;
	  break;
	case 'h':
	  usage (stdout, 0);
	  break;
	case 'i':
	  icon_flag = 1;
	  cygwin_conv_to_full_win32_path (optarg, icon_name);
	  break;
	case 'j':
	  if (!icon_flag)
	    usage (stderr, 1);
	  offset = atoi (optarg);
	  break;
	case 'n':
	  name_arg = optarg;
	  break;
	case 'v':
	  print_version ();
	  exit (0);
	case 'A':
	  allusers_flag = 1;
	  break;
	case 'D':
	  if (smprograms_flag)
	    usage (stderr, 1);
	  desktop_flag = 1;
	  break;
	case 'P':
	  if (desktop_flag)
	    usage (stderr, 1);
	  smprograms_flag = 1;
	  break;
	default:
	  usage (stderr, 1);
	  break;
	}
    }

  if (optind != argc - 1)
    usage (stderr, 1);

  /*  If there's a colon in the TARGET, it should be a URL */
  if (strchr (argv[optind], ':') != NULL)
    {
      /*  Nope, somebody's trying a W32 path  */
      if (argv[optind][1] == ':')
	usage (stderr, 1);
      strcpy (exe_name, argv[optind]);
      dir_name[0] = '\0';	/* No working dir for URL */
    }
  /* Convert TARGET to win32 path */
  else
    {
      strcpy (buf_str, argv[optind]);
      cygwin_conv_to_full_win32_path (buf_str, exe_name);

      /*  Get a working dir from the exepath */
      tmp_str = strrchr (exe_name, '\\');
      tmp = strlen (exe_name) - strlen (tmp_str);
      strncpy (dir_name, exe_name, tmp);
      dir_name[tmp] = '\0';
    }

  /*  Generate a name for the link if not given */
  if (name_arg == NULL)
    {
      /*  Strip trailing /'s if any */
      strcpy (buf_str, argv[optind]);
      tmp_str = buf_str;
      tmp = strlen (buf_str) - 1;
      while (strrchr (buf_str, '/') == (buf_str + tmp))
	{
	  buf_str[tmp] = '\0';
	  tmp--;
	}
      /*  Get basename */
      while (*buf_str)
	{
	  if (*buf_str == '/')
	    tmp_str = buf_str + 1;
	  buf_str++;
	}
      strcpy (link_name, tmp_str);
    }
  /*  User specified a name, so check it and convert  */
  else
    {
      if (desktop_flag || smprograms_flag)
	{
	  /*  Cannot have absolute path relative to Desktop/SM Programs */
	  if (name_arg[0] == '/')
	    usage (stderr, 1);
	}
      /*  Sigh. Another W32 path */
      if (strchr (name_arg, ':') != NULL)
	usage (stderr, 1);
      cygwin_conv_to_win32_path (name_arg, link_name);
    }

  /*  Add suffix to link name if necessary */
  if (strlen (link_name) > 4)
    {
      tmp = strlen (link_name) - 4;
      if (strncmp (link_name + tmp, ".lnk", 4) != 0)
	strcat (link_name, ".lnk");
    }
  else
    strcat (link_name, ".lnk");

  /*  Prepend relative path if necessary  */
  if (desktop_flag)
    {
      strcpy (buf_str, link_name);
      if (!allusers_flag)
	SHGetSpecialFolderLocation (NULL, CSIDL_DESKTOPDIRECTORY, &id);
      else
	SHGetSpecialFolderLocation (NULL, CSIDL_COMMON_DESKTOPDIRECTORY, &id);
      SHGetPathFromIDList (id, link_name);
      /*  Make sure Win95 without "All Users" has output  */
      if (strlen (link_name) == 0)
	{
	  SHGetSpecialFolderLocation (NULL, CSIDL_DESKTOPDIRECTORY, &id);
	  SHGetPathFromIDList (id, link_name);
	}
      strcat (link_name, "\\");
      strcat (link_name, buf_str);
    }

  if (smprograms_flag)
    {
      strcpy (buf_str, link_name);
      if (!allusers_flag)
	SHGetSpecialFolderLocation (NULL, CSIDL_PROGRAMS, &id);
      else
	SHGetSpecialFolderLocation (NULL, CSIDL_COMMON_PROGRAMS, &id);
      SHGetPathFromIDList (id, link_name);
      /*  Make sure Win95 without "All Users" has output  */
      if (strlen (link_name) == 0)
	{
	  SHGetSpecialFolderLocation (NULL, CSIDL_PROGRAMS, &id);
	  SHGetPathFromIDList (id, link_name);
	}
      strcat (link_name, "\\");
      strcat (link_name, buf_str);
    }

  /*  Beginning of Windows interface */
  hres = OleInitialize (NULL);
  if (hres != S_FALSE && hres != S_OK)
    {
      fprintf (stderr, "%s: Could not initialize OLE interface\n", prog_name);
      exit (3);
    }

  hres =
    CoCreateInstance (&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		      &IID_IShellLink, (void **) &shell_link);
  if (SUCCEEDED (hres))
    {
      hres =
	shell_link->lpVtbl->QueryInterface (shell_link, &IID_IPersistFile,
					    (void **) &persist_file);
      if (SUCCEEDED (hres))
	{
	  shell_link->lpVtbl->SetPath (shell_link, exe_name);
	  /* Put the POSIX path in the "Description", just to be nice */
	  cygwin_conv_to_full_posix_path (exe_name, buf_str);
	  shell_link->lpVtbl->SetDescription (shell_link, buf_str);
	  shell_link->lpVtbl->SetWorkingDirectory (shell_link, dir_name);
	  if (argument_arg)
	    shell_link->lpVtbl->SetArguments (shell_link, argument_arg);
	  if (icon_flag)
	    shell_link->lpVtbl->SetIconLocation (shell_link, icon_name,
						 offset);

	  /*  Make link name Unicode-compliant  */
	  hres =
	    MultiByteToWideChar (CP_ACP, 0, link_name, -1, widepath,
				 MAX_PATH);
	  if (!SUCCEEDED (hres))
	    {
	      fprintf (stderr, "%s: Unicode translation failed%d\n",
		       prog_name, hres);
	      exit (3);
	    }
	  hres = persist_file->lpVtbl->Save (persist_file, widepath, TRUE);
	  if (!SUCCEEDED (hres))
	    {
	      fprintf (stderr,
		       "%s: Saving \"%s\" failed; does the target directory exist?\n",
		       prog_name, link_name);
	      exit (3);
	    }
	  persist_file->lpVtbl->Release (persist_file);
	  shell_link->lpVtbl->Release (shell_link);
	}
      else
	{
	  fprintf (stderr, "%s: QueryInterface failed\n", prog_name);
	  exit (3);
	}
    }
  else
    {
      fprintf (stderr, "%s: CoCreateInstance failed\n", prog_name);
      exit (3);
    }

  exit (0);
}
