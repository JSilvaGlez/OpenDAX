/*  opendcs - An open source distributed control system 
 *  Copyright (c) 1997 Phil Birkelbach
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
 
 * This is the header file for the tagname database handling routines
 */

#ifndef __TAGBASE_H
#define __TAGBASE_H

#ifndef DCS_TAGNAME_SIZE
 #define DCS_TAGNAME_SIZE 32
#endif

/* This is the initial size of the tagname list array */
#ifndef DCS_TAGLIST_SIZE
 #ifndef DCS_TAGLIST_SIZE 1024
#endif

/* This is the size that the tagname list array will grow when
   the size is exceeded */
#ifndef DCS_TAGLIST_INC
 #ifndef DCS_TAGLIST_INC 1024
#endif

/* The initial size of the database */
#ifndef DCS_DATABASE_SIZE
 #ifndef DCS_DATABASE_SIZE 1024
#endif

/* This is the increment by which the database will grow when
   the size is exceeded */
#ifndef DCS_DATABASE_INC
 #ifndef DCS_DATABASE_INC 1024
#endif



typedef struct Dcs_Tag {
    char [DCS_TAGNAME_SIZE +1];
    long int handle;
    unsigned int type;
    unsigned int count;
} dcs_tag;


void initialize_tagbase(void);
long int tagbase_add(char *name,unsigned int type, unsigned int count);
int tagbase_del(char *name);

#endif /* !__TAGBASE_H */
