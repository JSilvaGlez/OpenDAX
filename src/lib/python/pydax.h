/*  PyDAX - A Python extension module for OpenDAX 
 *  OpenDAX - An open source data acquisition and control system 
 *  Copyright (c) 2011 Phil Birkelbach
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
 * 
 *  This is the main header file for the extension module
 */

#include <Python.h>
#include <opendax.h>

struct iter_udata {
    PyObject *po;
    void *data;
    int error;
};

/* We use this structure to hold the Python callback function as
 * well as the PyOjbect that the Python function gave us during
 * the add_event() call.  We pass this object to the Python function
 * when our callback is called */
struct callback_data {
    PyObject *callback;
    PyObject *data;
};
