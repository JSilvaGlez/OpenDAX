/*  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 2007 Phil Birkelbach
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 
 *  Header file for opendax configuration
 */

#ifndef __OPTIONS_H
#define __OPTIONS_H

#define _GNU_SOURCE

#include <common.h>
#include <dax/daxtypes.h>

#ifndef MAX_LINE_LENGTH
#  define MAX_LINE_LENGTH 100
#endif

#ifndef DEFAULT_PID
#  define DEFAULT_PID "/var/run/opendax.pid"
#endif

/* All this silliness is because the different distributions have the libraries
 and header files for lua in different places with different names.
 There has got to be a better way. */
#if defined(HAVE_LUA5_1_LUA_H)
#  include <lua5.1/lua.h>
#elif defined(HAVE_LUA51_LUA_H)
#  include <lua51/lua.h>
#elif defined(HAVE_LUA_LUA_H)
#  include <lua/lua.h>
#elif defined(HAVE_LUA_H)
#  include <lua.h>
#else
#  error Missing lua.h
#endif

#if defined(HAVE_LUA51_LAUXLIB_H)
#  include <lua51/lauxlib.h>
#elif defined(HAVE_LUA5_1_LAUXLIB_H)
#  include <lua5.1/lauxlib.h>
#elif defined(HAVE_LUA_LAUXLIB_H)
#  include <lua/lauxlib.h>
#elif defined(HAVE_LAUXLIB_H)
#  include <lauxlib.h>
#else
#  error Missing lauxlib.h
#endif

#if defined(HAVE_LUA51_LUALIB_H)
#  include <lua51/lualib.h>
#elif defined(HAVE_LUA5_1_LUALIB_H)
#  include <lua5.1/lualib.h>
#elif defined(HAVE_LUA_LUALIB_H)
#  include <lua/lualib.h>
#elif defined(HAVE_LUALIB_H)
#  include <lualib.h>
#else
#  error Missing lualib.h
#endif 


int dax_configure(int argc, const char *argv[]);

int get_daemonize(void);
char *get_statustag(void);
char *get_pidfile(void);
int get_maxstartup(void);

#endif /* !__OPTIONS_H */
