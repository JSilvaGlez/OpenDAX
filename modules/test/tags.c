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
 *
 */

#include <tags.h>


/* Returns a random datatype to the caller */
int get_random_type(void) {
    switch (rand()%15) {
    case 0:
        return DAX_BOOL;        
    case 1:
        return DAX_BYTE;        
    case 2:
        return DAX_SINT;        
    case 3:
        return DAX_WORD;        
    case 4:
        return DAX_INT;        
    case 5:
        return DAX_UINT;        
    case 6:
        return DAX_DWORD;        
    case 7:
        return DAX_DINT;        
    case 8:
        return DAX_UDINT;        
    case 9:
        return DAX_TIME;        
    case 10:
        return DAX_REAL;        
    case 11:
        return DAX_LWORD;        
    case 12:
        return DAX_LINT;        
    case 13:
        return DAX_ULINT;        
    case 14:
        return DAX_LREAL;        
    default:
        return 0;
    }
}

void add_random_tags(int tagcount) {
    static int index = 0;
    int n, count, data_type;
    handle_t result;
    char tagname[32];
    
    srand(12); /* Random but not so random */
    for(n = 0; n < tagcount; n++) {
        sprintf(tagname,"randtag%d",index++);
        if(rand()%4 == 0) { /* One of four tags will be an array */
            count = rand() % 100 + 1;
        } else {
            count = 1; /* the rest are just tags */
        }
        data_type = get_random_type();
        result = dax_tag_add(tagname, data_type, count);
        if(result < 0) {
            xlog(10, "Failed to add Tag %s %s[%d]", tagname, dax_get_type(data_type), count );
        } else {
          //  xlog(10, "%s added at handle 0x%8X", tagname, result);
        }
    }
}