/*****************************************************************************
 * DikuMUD (C) 1990, 1991 by:                                                *
 *   Sebastian Hammer, Michael Seifert, Hans Henrik Staefeldt, Tom Madsen,   *
 *   and Katja Nyboe.                                                        *
 *---------------------------------------------------------------------------*
 * MERC 2.1 (C) 1992, 1993 by:                                               *
 *   Michael Chastain, Michael Quan, and Mitchell Tse.                       *
 *---------------------------------------------------------------------------*
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998 by: Derek Snider.                    *
 *   Team: Thoric, Altrag, Blodkai, Narn, Haus, Scryn, Rennard, Swordbearer, *
 *         gorog, Grishnakh, Nivek, Tricops, and Fireblade.                  *
 *---------------------------------------------------------------------------*
 * SMAUG 1.7 FUSS by: Samson and others of the SMAUG community.              *
 *                    Their contributions are greatly appreciated.           *
 *---------------------------------------------------------------------------*
 * LoP (C) 2006 - 2012 by: the LoP team.                                     *
 *---------------------------------------------------------------------------*
 * v. 0.9: 6/19/95:  Converts an ascii map to rooms.                         *
 * v. 1.0: 7/05/95:  Read/write maps to .are files.  Efficient storage.      *
 *	             Room qualities based on map code. Can add & remove rms  *
 *	                from a map. (Somewhat) intelligent exit decisions.   *
 * v. 1.1: 7/11/95:  Various display options.                                *
 *****************************************************************************/

#include <stdio.h>
#include "mud.h"

typedef struct map_index_data MAP_INDEX_DATA;   /* maps */
struct map_index_data
{
   MAP_INDEX_DATA *next;
   int vnum;                  /* vnum of the map */
   int map_of_vnums[49][81];  /* room vnums aranged as a map */
};
MAP_INDEX_DATA *first_map; /* maps */

/* Local Variables & Structs */
char text_map[MSL];
extern MAP_INDEX_DATA *first_map;   /* should be global */
struct map_stuff
{
   int vnum;
   int exits;
   int index;
   char code;
};

/* Useful Externals */
extern int top_exit;
void note_attach( CHAR_DATA * ch );

int num_rooms_avail( CHAR_DATA *ch );
void map_to_rooms( CHAR_DATA *ch, MAP_INDEX_DATA *m_index );

/* Be careful not to give
 * this an existing map_index
 */
MAP_INDEX_DATA *make_new_map_index( int vnum )
{
   MAP_INDEX_DATA *map_index;
   int i, j;

   CREATE( map_index, MAP_INDEX_DATA, 1 );
   map_index->vnum = vnum;
   for( i = 0; i < 49; i++ )
   {
      for( j = 0; j < 78; j++ )
         map_index->map_of_vnums[i][j] = -1;
   }
   map_index->next = first_map;
   first_map = map_index;
   return map_index;
}

char count_lines( char *txt )
{
   int i;
   char *c, buf[MSL];

   if( !txt )
      return ( char )'0';
   i = 1;
   for( c = txt; *c != '\0'; c++ )
      if( *c == '\n' )
         i++;
   if( i > 9 )
      return ( char )'+';
   snprintf( buf, sizeof( buf ), "%d", i );
   return ( buf[0] );
}

MAP_INDEX_DATA *get_map_index( int vnum )
{
   MAP_INDEX_DATA *map;

   for( map = first_map; map; map = map->next )
   {
      if( map->vnum == vnum )
         return map;
   }
   return NULL;
}


void map_stats( CHAR_DATA *ch, int *rooms, int *rows, int *cols )
{
   int row, col, n;
   int leftmost, rightmost;
   const char *l;
   char c;

   if( !ch->pnote )
   {
      bug( "%s: ch->pnote is NULL!", __FUNCTION__ );
      return;
   }
   n = 0;
   row = col = 0;
   leftmost = rightmost = 0;
   l = ch->pnote->text;
   do
   {
      c = l[0];
      switch( c )
      {
         case '\r':
            break;
         case '\n':
            col = 0;
            row++;
            break;
         case ' ':
            col++;
            break;
      }
      if( c == 'I' || c == 'C' || c == 'f' || c == 'F' || c == 'H' || c == 'M' || c == 's' || c == 'S'
      || c == 'A' || c == 'D' || c == 'O' || c == 'u' || c == 'U' || c == 'L' || c == 'W' || c == '#' )
      {
         /* This is a room */
         n++;

         /* This will later handle a letter etc */
         col++;

         if( col < leftmost )
            leftmost = col;
         if( col > rightmost )
            rightmost = col;
      }
      if( c == ' ' || c == '-' || c == '|' || c == '=' || c == '\\' || c == '/' || c == '^' || c == ':' || c == 'X' || c == '+' )
         col++;
      l++;
   }
   while( c != '\0' );
   *cols = ( rightmost - leftmost );
   *rows = row;   /* [sic.] */
   *rooms = n;
}

void do_mapout( CHAR_DATA* ch, const char* argument)
{
   char arg[MIL];
   OBJ_DATA *map_obj;   /* an obj made with map as an ed */
   OBJ_INDEX_DATA *map_obj_index;   /*    obj_index for previous     */
   EXTRA_DESCR_DATA *ed;   /*    the ed for it to go in     */
   int rooms, rows, cols, avail_rooms;

   if( !ch )
   {
      bug( "%s: null ch", __FUNCTION__ );
      return;
   }
   if( IS_NPC( ch ) )
   {
      send_to_char( "Not for mobs.\r\n", ch );
      return;
   }
   if( !ch->desc )
   {
      bug( "%s: no descriptor", __FUNCTION__ );
      return;
   }
   switch( ch->substate )
   {
      default:
         break;
      case SUB_WRITING_MAP:
         if( ch->dest_buf != ch->pnote )
            bug( "%s: sub_writing_map: ch->dest_buf != ch->pnote", __FUNCTION__ );
         STRFREE( ch->pnote->text );
         ch->pnote->text = copy_buffer( ch );
         stop_editing( ch );
         return;
   }

   set_char_color( AT_NOTE, ch );
   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "stat" ) )
   {
      if( !ch->pnote )
      {
         send_to_char( "You have no map in progress.\r\n", ch );
         return;
      }
      map_stats( ch, &rooms, &rows, &cols );
      ch_printf( ch, "Map represents %d rooms, %d rows, and %d columns\r\n", rooms, rows, cols );
      avail_rooms = num_rooms_avail( ch );
      ch_printf( ch, "You currently have %d unused rooms.\r\n", avail_rooms );
      act( AT_ACTION, "$n glances at an etherial map.", ch, NULL, NULL, TO_ROOM );
      return;
   }

   if( !str_cmp( arg, "write" ) )
   {
      note_attach( ch );
      ch->substate = SUB_WRITING_MAP;
      ch->dest_buf = ch->pnote;
      start_editing( ch, ch->pnote->text );
      return;
   }
   if( !str_cmp( arg, "clear" ) )
   {
      if( !ch->pnote )
      {
         send_to_char( "You have no map in progress\r\n", ch );
         return;
      }
      STRFREE( ch->pnote->text );
      STRFREE( ch->pnote->subject );
      STRFREE( ch->pnote->to_list );
      STRFREE( ch->pnote->sender );
      DISPOSE( ch->pnote );
      ch->pnote = NULL;
      send_to_char( "Map cleared.\r\n", ch );
      return;
   }
   if( !str_cmp( arg, "show" ) )
   {
      if( !ch->pnote )
      {
         send_to_char( "You have no map in progress.\r\n", ch );
         return;
      }
      send_to_char( ch->pnote->text, ch );
      do_mapout( ch, "stat" );
      return;
   }
   if( !str_cmp( arg, "create" ) )
   {
      if( !ch->pnote )
      {
         send_to_char( "You have no map in progress.\r\n", ch );
         return;
      }
      map_stats( ch, &rooms, &rows, &cols );
      avail_rooms = num_rooms_avail( ch );

      /* check for not enough rooms */
      if( rooms > avail_rooms )
      {
         send_to_char( "You don't have enough unused rooms allocated!\r\n", ch );
         return;
      }
      act( AT_ACTION, "$n warps the very dimensions of space!", ch, NULL, NULL, TO_ROOM );

      map_to_rooms( ch, NULL );  /* this does the grunt work */

      if( ( map_obj_index = get_obj_index( OBJ_VNUM_MAP ) ) )
      {
         if( !( map_obj = create_object( map_obj_index, 0 ) ) )
         {
            ch_printf( ch, "Couldn't give you a map. create_object failed for object vnum %d\r\n", OBJ_VNUM_MAP );
            return;
         }
         ed = SetOExtra( map_obj, (char *)"runes map scrawls" );
         STRSET( ed->description, ch->pnote->text );
         obj_to_char( map_obj, ch );
      }
      else
      {
         ch_printf( ch, "Couldn't give you a map. Need object vnum %d\r\n", OBJ_VNUM_MAP );
         return;
      }

      do_mapout( ch, "clear" );
      send_to_char( "Your written map has been turned into rooms in your assigned room vnum range.\r\n", ch );
      return;
   }
   send_to_char( "mapout write: create a map in edit buffer.\r\n", ch );
   send_to_char( "mapout stat: get information about a written, but not yet created map.\r\n", ch );
   send_to_char( "mapout clear: clear a written, but not yet created map.\r\n", ch );
   send_to_char( "mapout show: show a written, but not yet created map.\r\n", ch );
   send_to_char( "mapout create: turn a written map into rooms in your assigned room vnum range.\r\n", ch );
}

int add_new_room_to_map( CHAR_DATA * ch, char code )
{
   int i;
   ROOM_INDEX_DATA *location;

   /*
    * Get an unused room to copy to
    */
   for( i = ch->pcdata->area->low_r_vnum; i <= ch->pcdata->area->hi_r_vnum; i++ )
   {
      if( get_room_index( i ) == NULL )
      {
         if( !( location = make_room( i, ch->pcdata->area ) ) )
         {
            bug( "%s: make_room failed", __FUNCTION__ );
            return -1;
         }
         /*
          * Clones current room  (quietly)
          */
         location->area = ch->pcdata->area;
         location->name = STRALLOC( "Newly Created Room" );
         location->description = STRALLOC( "Newly Created Room\r\n" );
         xSET_BIT( location->room_flags, ROOM_PROTOTYPE );
         location->light = 0;
         if( code == 'I' )
            location->sector_type = SECT_INSIDE;
         else if( code == 'C' )
            location->sector_type = SECT_CITY;
         else if( code == 'f' )
            location->sector_type = SECT_FIELD;
         else if( code == 'F' )
            location->sector_type = SECT_FOREST;
         else if( code == 'H' )
            location->sector_type = SECT_HILLS;
         else if( code == 'M' )
            location->sector_type = SECT_MOUNTAIN;
         else if( code == 's' )
            location->sector_type = SECT_WATER_SWIM;
         else if( code == 'S' )
            location->sector_type = SECT_WATER_NOSWIM;
         else if( code == 'A' )
            location->sector_type = SECT_AIR;
         else if( code == 'D' )
            location->sector_type = SECT_DESERT;
         else if( code == 'o' )
            location->sector_type = SECT_OCEAN;
         else if( code == 'O' )
            location->sector_type = SECT_OCEANFLOOR;
         else if( code == 'u' )
            location->sector_type = SECT_UNDERGROUND;
         else if( code == 'U' )
            location->sector_type = SECT_UNDERWATER;
         else if( code == 'L' )
            location->sector_type = SECT_LAVA;
         else if( code == 'W' )
            location->sector_type = SECT_SWAMP;
         else
            location->sector_type = SECT_INSIDE;
         return i;
      }
   }
   send_to_char( "No available room in your vnums!", ch );
   return -1;
}

int num_rooms_avail( CHAR_DATA * ch )
{
   int i, n;

   n = 0;
   for( i = ch->pcdata->area->low_r_vnum; i <= ch->pcdata->area->hi_r_vnum; i++ )
      if( !get_room_index( i ) )
         n++;
   return n;
}

/*
 * This function takes the character string in ch->pnote and
 *  creates rooms laid out in the appropriate configuration.
 */
void map_to_rooms( CHAR_DATA *ch, MAP_INDEX_DATA *m_index )
{
   struct map_stuff map[49][78]; /* size of edit buffer */
   int row, col, i, n, x, y, tvnum, leftmost, rightmost;
   int newx, newy, start, end;
   const char *l;
   char c;
   ROOM_INDEX_DATA *newrm;
   MAP_INDEX_DATA *map_index = NULL, *tmp;
   EXIT_DATA *xit;   /* these are for exits */

   if( !ch->pnote )
   {
      bug( "%s: ch->pnote is NULL!", __FUNCTION__ );
      return;
   }

   n = 0;
   row = col = 0;
   leftmost = rightmost = 0;

   start = ch->pcdata->area->low_r_vnum;
   end = ch->pcdata->area->hi_r_vnum;

   /*
    * Check to make sure map_index exists.
    * If not, then make a new one.
    */
   if( !m_index )
   {  /* Make a new vnum */
      for( i = start; i <= end; i++ )
      {
         if( !( tmp = get_map_index( i ) ) )
         {
            map_index = make_new_map_index( i );
            break;
         }
      }
   }
   else
   {
      map_index = m_index;
   }

   if( !map_index )
   {
      send_to_char( "Couldn't find or make a map_index for you!\r\n", ch );
      bug( "%s: Couldn't find or make a map_index.", __FUNCTION__ );
      return;
   }

   for( x = 0; x < 49; x++ )
   {
      for( y = 0; y < 78; y++ )
      {
         map[x][y].vnum = 0;
         map[x][y].exits = 0;
         map[x][y].index = 0;
      }
   }

   l = ch->pnote->text;
   do
   {
      c = l[0];
      switch( c )
      {
         case '\r':
            break;
         case '\n':
            col = 0;
            row++;
            break;
      }
      if( c != ' ' && c != '-' && c != '|' && c != '=' && c != '\\' && c != '/' && c != '^'
      && c != ':' && c != '^' && c != 'X' && c != '+' && c != 'W' && c != 'L' && c != 'U'
      && c != 'u' && c != 'O' && c != 'D' && c != 'A' && c != 'S' && c != 's' && c != 'M'
      && c != 'H' && c != 'F' && c != 'f' && c != 'C' && c != 'I' && c != '#' )
      {
         l++;
         continue;
      }
      if( c == 'W' || c == 'L' || c == 'U' || c == 'u' || c == 'O' || c == 'D' || c == 'A'
      || c == 'S' || c == 's' || c == 'M' || c == 'H' || c == 'F' || c == 'f' || c == 'C'
      || c == 'I' || c == '#' )
      {
         n++;

         /* Actual room info */
         map[row][col].vnum = add_new_room_to_map( ch, c );
         map_index->map_of_vnums[row][col] = map[row][col].vnum;
      }
      else
      {
         map_index->map_of_vnums[row][col] = 0;
         map[row][col].vnum = 0;
         map[row][col].exits = 0;
      }
      map[row][col].code = c;
      col++;
      l++;
   }
   while( c != '\0' );

   for( y = 0; y < ( row + 1 ); y++ )
   {  /* rows */
      for( x = 0; x < 78; x++ )
      {  /* cols (78, i think) */

         if( map[y][x].vnum == 0 )
            continue;

         /* Continue if no newrm */
         if( !( newrm = get_room_index( map[y][x].vnum ) ) )
            continue;

         /* Check up */
         if( y > 1 )
         {
            newx = x;
            newy = y;
            newy--;
            while( newy >= 0 && ( map[newy][x].code == '^' ) )
               newy--;
            if( map[y][x].vnum != map[newy][x].vnum && ( tvnum = map[newy][x].vnum ) != 0 )
            {
               xit = make_exit( newrm, get_room_index( tvnum ), DIR_UP );
               xit->keyword = STRALLOC( "" );
               xit->description = STRALLOC( "" );
               xit->key = -1;
               xit->exit_info = 0;
            }
         }

         /* Check down */
         if( y < 48 )
         {
            newx = x;
            newy = y;
            newy++;
            while( newy <= 48 && ( map[newy][x].code == '^' ) )
               newy++;
            if( map[y][x].vnum != map[newy][x].vnum && ( tvnum = map[newy][x].vnum ) != 0 )
            {
               xit = make_exit( newrm, get_room_index( tvnum ), DIR_DOWN );
               xit->keyword = STRALLOC( "" );
               xit->description = STRALLOC( "" );
               xit->key = -1;
               xit->exit_info = 0;
            }
         }

         /* Check north */
         if( y > 1 )
         {
            newx = x;
            newy = y;
            newy--;
            while( newy >= 0 && ( map[newy][x].code == '|' || map[newy][x].code == ':' || map[newy][x].code == '='
            || map[newy][x].code == '+' ) )
               newy--;
            if( map[y][x].vnum != map[newy][x].vnum && ( tvnum = map[newy][x].vnum ) != 0 )
            {
               xit = make_exit( newrm, get_room_index( tvnum ), DIR_NORTH );
               xit->keyword = STRALLOC( "" );
               xit->description = STRALLOC( "" );
               xit->key = -1;
               if( map[newy + 1][x].code == ':' || map[newy + 1][x].code == '=' )
               {
                  SET_BIT( xit->exit_info, EX_ISDOOR );
                  SET_BIT( xit->exit_info, EX_CLOSED );
               }
               else
                  xit->exit_info = 0;
            }
         }

         /* Check south */
         if( y < 48 )
         {
            newx = x;
            newy = y;
            newy++;
            while( newy <= 48 && ( map[newy][x].code == '|' || map[newy][x].code == ':' || map[newy][x].code == '='
            || map[newy][x].code == '+' ) )
               newy++;
            if( map[y][x].vnum != map[newy][x].vnum && ( tvnum = map[newy][x].vnum ) != 0 )
            {
               xit = make_exit( newrm, get_room_index( tvnum ), DIR_SOUTH );
               xit->keyword = STRALLOC( "" );
               xit->description = STRALLOC( "" );
               xit->key = -1;
               if( map[newy - 1][x].code == ':' || map[newy - 1][x].code == '=' )
               {
                  SET_BIT( xit->exit_info, EX_ISDOOR );
                  SET_BIT( xit->exit_info, EX_CLOSED );
               }
               else
                  xit->exit_info = 0;
            }
         }

         /* Check east */
         if( x < 79 )
         {
            newx = x;
            newy = y;
            newx++;
            while( newx <= 79 && ( map[y][newx].code == '-' || map[y][newx].code == ':' || map[y][newx].code == '='
            || map[y][newx].code == '+' ) )
               newx++;
            if( map[y][x].vnum != map[y][newx].vnum && ( tvnum = map[y][newx].vnum ) != 0 )
            {
               xit = make_exit( newrm, get_room_index( tvnum ), DIR_EAST );
               xit->keyword = STRALLOC( "" );
               xit->description = STRALLOC( "" );
               xit->key = -1;
               if( map[y][newx - 2].code == ':' || map[y][newx - 2].code == '=' )
               {
                  SET_BIT( xit->exit_info, EX_ISDOOR );
                  SET_BIT( xit->exit_info, EX_CLOSED );
               }
               else
                  xit->exit_info = 0;
            }
         }

         /* Check west */
         if( x > 1 )
         {
            newx = x;
            newy = y;
            newx--;
            while( newx >= 0 && ( map[y][newx].code == '-' || map[y][newx].code == ':' || map[y][newx].code == '='
            || map[y][newx].code == '+' ) )
               newx--;
            if( map[y][x].vnum != map[y][newx].vnum && ( tvnum = map[y][newx].vnum ) != 0 )
            {
               xit = make_exit( newrm, get_room_index( tvnum ), DIR_WEST );
               xit->keyword = STRALLOC( "" );
               xit->description = STRALLOC( "" );
               xit->key = -1;
               if( map[y][newx + 2].code == ':' || map[y][newx + 2].code == '=' )
               {
                  SET_BIT( xit->exit_info, EX_ISDOOR );
                  SET_BIT( xit->exit_info, EX_CLOSED );
               }
               else
                  xit->exit_info = 0;
            }
         }

         /* Check southeast */
         if( y < 48 && x < 79 )
         {
            newx = x;
            newy = y;
            newx++;
            newy++;
            while( newx <= 79 && newy <= 48
            && ( map[newy][newx].code == '\\' || map[newy][newx].code == ':'
            || map[newy][newx].code == '=' || map[newy][newx].code == 'X' ) )
            {
               newx++;
               newy++;
            }
            if( map[y][x].vnum != map[newy][newx].vnum && ( tvnum = map[newy][newx].vnum ) != 0 )
            {
               xit = make_exit( newrm, get_room_index( tvnum ), DIR_SOUTHEAST );
               xit->keyword = STRALLOC( "" );
               xit->description = STRALLOC( "" );
               xit->key = -1;
               xit->exit_info = 0;
            }
         }

         /* Check northeast */
         if( y > 1 && x < 79 )
         {
            newx = x;
            newy = y;
            newx++;
            newy--;
            while( newx >= 0 && newy <= 48
            && ( map[newy][newx].code == '/' || map[newy][newx].code == ':'
            || map[newy][newx].code == '=' || map[newy][newx].code == 'X' ) )
            {
               newx++;
               newy--;
            }
            if( map[y][x].vnum != map[newy][newx].vnum && ( tvnum = map[newy][newx].vnum ) != 0 )
            {
               xit = make_exit( newrm, get_room_index( tvnum ), DIR_NORTHEAST );
               xit->keyword = STRALLOC( "" );
               xit->description = STRALLOC( "" );
               xit->key = -1;
               xit->exit_info = 0;
            }
         }

         /* Check northwest */
         if( y > 1 && x > 1 )
         {
            newx = x;
            newy = y;
            newx--;
            newy--;
            while( newx >= 0 && newy >= 0
            && ( map[newy][newx].code == '\\' || map[newy][newx].code == ':'
            || map[newy][newx].code == '=' || map[newy][newx].code == 'X' ) )
            {
               newx--;
               newy--;
            }
            if( map[y][x].vnum != map[newy][newx].vnum && ( tvnum = map[newy][newx].vnum ) != 0 )
            {
               xit = make_exit( newrm, get_room_index( tvnum ), DIR_NORTHWEST );
               xit->keyword = STRALLOC( "" );
               xit->description = STRALLOC( "" );
               xit->key = -1;
               xit->exit_info = 0;
            }
         }

         /* Check southwest */
         if( y < 48 && x > 1 )
         {
            newx = x;
            newy = y;
            newx--;
            newy++;
            while( newx >= 0 && newy <= 48
            && ( map[newy][newx].code == '/' || map[newy][newx].code == ':'
            || map[newy][newx].code == '=' || map[newy][newx].code == 'X' ) )
            {
               newx--;
               newy++;
            }
            if( map[y][x].vnum != map[newy][newx].vnum && ( tvnum = map[newy][newx].vnum ) != 0 )
            {
               xit = make_exit( newrm, get_room_index( tvnum ), DIR_SOUTHWEST );
               xit->keyword = STRALLOC( "" );
               xit->description = STRALLOC( "" );
               xit->key = -1;
               xit->exit_info = 0;
            }
         }
      }
   }
}
