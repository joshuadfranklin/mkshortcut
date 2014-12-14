/* mkshortcut.c -- create a Windows shortcut 
 *
 * Version 1.01
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

static const char versionID[] =
    "1.01.0";
/* for CVS */
static const char revID[] =
  "$Id: mkshortcut.c,v 1.2 2002/02/24 03:41:44 cwilson Exp $";
static const char copyrightID[] =
    "Copyright (c) 2001,...\nJoshua Daniel Franklin. All rights reserved.\nLicensed under GPL v2.0\n";

static char *prog_name;
static char *argument_arg, *name_arg;
static int icon_flag, unix_flag, windows_flag;
static int allusers_flag, desktop_flag, smprograms_flag;

static struct option long_options[] =
{
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

int
main (int argc, char **argv)
{
  LPITEMIDLIST id;
  HRESULT hres;
  IShellLink *sl;
  IPersistFile *pf;
  WCHAR widepath[MAX_PATH];

  int opt, tmp, offset;
  char *args, *str2, *str1;
  char lname[MAX_PATH], exename[MAX_PATH], dirname[MAX_PATH], iconname[MAX_PATH];

  prog_name = strrchr (argv[0], '/');
  if (prog_name == NULL)
    prog_name = strrchr (argv[0], '\\');
  if (prog_name == NULL)
    prog_name = argv[0];
  else
    prog_name++;

  offset=0;
  icon_flag=0;
  unix_flag=0;
  windows_flag=0;
  allusers_flag=0;
  desktop_flag=0;
  smprograms_flag=0;

  while ( (opt = getopt_long(argc, argv, (char *) "a:i:j:n:hvADP", long_options, NULL) ) != EOF)
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
	cygwin_conv_to_full_win32_path(optarg, iconname);
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
	printf ("Cygwin shortcut creator version 1.01\n");
	printf ("Copyright 2002 Joshua Daniel Franklin.\n");
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

  // If there's a colon in the TARGET, it should be a URL
  if (strchr (argv[optind], ':') != NULL)
    {
    // Nope, somebody's trying a W32 path 
      if (argv[optind][1] == ':')
	usage (stderr, 1);
    strcpy(exename, argv[optind]);
    dirname[0]='\0';  //No working dir for URL
    }
  else
    {
    strcpy(str1, argv[optind]);
    cygwin_conv_to_full_win32_path(str1, exename);

    // Get a working dir from the exepath
    str1 = strrchr(exename, '\\');
    tmp = strlen (exename) - strlen (str1);
    strncpy (dirname, exename, tmp);
    dirname[tmp]='\0';
    }

  // Generate a name for the link if not given
  if (name_arg == NULL)
    {
    // Strip trailing /'s if any
    str2 = str1 = argv[optind];
    tmp = strlen(str1) - 1;
    while ( strrchr(str1,'/') == (str1+tmp) )
	{
      str1[tmp]='\0';
	  tmp--;
	}
    // Get basename
    while (*str1)
	{
      if (*str1 == '/')
        str2 = str1 + 1;
      str1++;
	}
    }
  else
    {
    // User specified a name, so check it and convert 
      if (desktop_flag || smprograms_flag)
	{
      // Absolute path not allowed on Desktop/SM Programs
	  if (name_arg[0] == '/')
	    usage (stderr, 1);
	}
    // Sigh. Another W32 path
      if (strchr (name_arg, ':') != NULL)
	usage (stderr, 1);
    cygwin_conv_to_win32_path(name_arg, str2);
    }

  // Add suffix to link name if necessary
  if (strlen(str2) > 4)
    {
    tmp = strlen(str2) - 4;
    if (strncmp (str2+tmp, ".lnk", 4) != 0 )
      strcat (str2, ".lnk");
    }
  else strcat (str2, ".lnk");
  strcpy (lname, str2);

  // Prepend relative path if necessary 
  if (desktop_flag)
    {
      if (!allusers_flag)
	SHGetSpecialFolderLocation (NULL, CSIDL_DESKTOPDIRECTORY, &id);
      else
	SHGetSpecialFolderLocation (NULL, CSIDL_COMMON_DESKTOPDIRECTORY, &id);
    SHGetPathFromIDList(id, lname);
    // Make sure Win95 without "All Users" has output 
    if ( strlen(lname) == 0 ) {
	  SHGetSpecialFolderLocation (NULL, CSIDL_DESKTOPDIRECTORY, &id);
      SHGetPathFromIDList(id, lname);
	}
    strcat(lname, "\\");
    strcat(lname, str2);
    }

  if (smprograms_flag)
    {
      if (!allusers_flag)
	SHGetSpecialFolderLocation (NULL, CSIDL_PROGRAMS, &id);
      else
	SHGetSpecialFolderLocation (NULL, CSIDL_COMMON_PROGRAMS, &id);
    SHGetPathFromIDList(id, lname);
    // Make sure Win95 without "All Users" has output 
    if ( strlen(lname) == 0 ) {
	  SHGetSpecialFolderLocation (NULL, CSIDL_PROGRAMS, &id);
      SHGetPathFromIDList(id, lname);
	}
    strcat(lname, "\\");
    strcat(lname, str2);
    }

  // Beginning of Windows interface
  hres = OleInitialize (NULL);
  if (hres != S_FALSE && hres != S_OK)
    {
      fprintf (stderr, "%s: Could not initialize OLE interface\n", prog_name);
      exit (3);
    }

  hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (void **) &sl);
  if (SUCCEEDED (hres))
    {
    hres = sl->lpVtbl->QueryInterface(sl, &IID_IPersistFile, (void **) &pf);
      if (SUCCEEDED (hres))
	{
      sl->lpVtbl->SetPath (sl, exename);
      cygwin_conv_to_full_posix_path(exename, str1);
      sl->lpVtbl->SetDescription (sl, str1);
      sl->lpVtbl->SetWorkingDirectory (sl, dirname);
      if (argument_arg) sl->lpVtbl->SetArguments (sl, argument_arg);
      if (icon_flag) sl->lpVtbl->SetIconLocation (sl, iconname, offset);

      // Make link name Unicode-compliant 
      MultiByteToWideChar (CP_ACP, 0, lname, -1, widepath, MAX_PATH);
      hres = pf->lpVtbl->Save (pf, widepath, TRUE);
	  if (!SUCCEEDED (hres))
	    {
        fprintf(stderr, "%s: Save to persistant storage failed (Does the directory you are writing to exist?)\n", prog_name);
	      exit (3);
	    }
      pf->lpVtbl->Release (pf);
      sl->lpVtbl->Release (sl);
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
}
