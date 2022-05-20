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
 *                           Fishing Support                                 *
 *****************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "mud.h"

#define FNAMES_FILE  SYSTEM_DIR "fnames.dat"
typedef struct fish_data FISH_DATA;
typedef struct fname_data FNAME_DATA;

struct fname_data
{
   FNAME_DATA *next, *prev;
   const char *name;
   short minweight;
   short maxweight;
   short value; /* Default value, increase it by weight */
};

FNAME_DATA *first_fname, *last_fname;

struct fish_data
{
   FISH_DATA *next, *prev;
   OBJ_DATA *rod;
};

FISH_DATA *first_fish, *last_fish;

void remove_fname( FNAME_DATA *fname )
{
   if( !fname )
      return;
   UNLINK( fname, first_fname, last_fname, next, prev );
}

void free_fname( FNAME_DATA *fname )
{
   if( !fname )
      return;
   STRFREE( fname->name );
   DISPOSE( fname );
}

void add_fname( FNAME_DATA *fname )
{
   FNAME_DATA *tname;
   int match;

   if( !fname )
      return;
   if( !fname->name )
   {
      free_fname( fname );
      return;
   }
   for( tname = first_fname; tname; tname = tname->next )
   {
      if( !str_cmp( tname->name, fname->name ) )
      {
         free_fname( fname );
         return;
      }
      else if( ( match = strcmp( fname->name, tname->name ) ) < 0 )
      {
         INSERT( fname, tname, first_fname, next, prev );
         return;
      }
   }
   LINK( fname, first_fname, last_fname, next, prev );
}

void free_all_fnames( void )
{
   FNAME_DATA *fname, *fname_next;

   for( fname = first_fname; fname; fname = fname_next )
   {
      fname_next = fname->next;

      remove_fname( fname );
      free_fname( fname );
   }
}

void save_fnames( void )
{
   FNAME_DATA *fname;
   FILE *fp;

   if( !( fp = fopen( FNAMES_FILE, "w" ) ) )
      return;
   for( fname = first_fname; fname; fname = fname->next )
   {
      if( !fname->name || fname->name[0] == '\0' )
         continue;
      fprintf( fp, "%s", "#FISH\n" );
      fprintf( fp, "Name       %s~\n", fname->name );
      fprintf( fp, "MinWeight  %d\n", fname->minweight );
      fprintf( fp, "MaxWeight  %d\n", fname->maxweight );
      fprintf( fp, "Value      %d\n", fname->value );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   fclose( fp );
   fp = NULL;
}

void fread_fnames( FILE *fp )
{
   FNAME_DATA *fname;
   const char *word;
   bool fMatch;

   CREATE( fname, FNAME_DATA, 1 );

   for( ;; )
   {
      word = feof( fp ) ? "End" : fread_word( fp );
      fMatch = false;

      switch( UPPER( word[0] ) )
      {
         case '*':
            fMatch = true;
            fread_to_eol( fp );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               add_fname( fname );
               return;
	    }
	    break;

         case 'M':
            KEY( "MinWeight", fname->minweight, fread_number( fp ) );
            KEY( "MaxWeight", fname->maxweight, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", fname->name, fread_string( fp ) );
            break;

         case 'V':
            KEY( "Value", fname->value, fread_number( fp ) );
            break;
      }

      if( !fMatch )
      {
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
      }
   }
   free_fname( fname );
}

void load_fnames( void )
{
   FILE *fp;

   first_fname = last_fname = NULL;

   if( !( fp = fopen( FNAMES_FILE, "r" ) ) )
      return;
   for( ;; )
   {
      char letter;
      char *word;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }
      if( letter != '#' )
      {
         bug( "%s: # not found.", __FUNCTION__ );
         break;
      }
      word = fread_word( fp );
      if( !str_cmp( word, "FISH" ) )
      {
         fread_fnames( fp );
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: bad section (%s).", __FUNCTION__, word );
         fread_to_eol( fp );
         continue;
      }
   }
   fclose( fp );
   fp = NULL;
}

FNAME_DATA *get_fish_name( void )
{
   FNAME_DATA *fname, *uname = NULL;

   for( fname = first_fname; fname; fname = fname->next )
   {
      if( !fname->name )
         continue;
      if( uname && number_percent( ) < 75 )
         continue;
      uname = fname;
   }

   if( uname )
      return uname;
   return NULL;
}

FNAME_DATA *find_fname( const char *name )
{
   FNAME_DATA *fname;

   if( !name || name[0] == '\0' )
      return NULL;

   for( fname = first_fname; fname; fname = fname->next )
   {
      if( !fname->name )
         continue;
      if( !str_cmp( fname->name, name ) )
         return fname;
   }
   return NULL;
}

void do_fname( CHAR_DATA* ch, const char* argument)
{
   FNAME_DATA *fname;
   char arg[MSL], arg2[MSL];
   short cnt = 0;
   bool found = false;

   if( !argument || argument[0] == '\0' )
   {
      for( fname = first_fname; fname; fname = fname->next )
      {
         if( !found )
         {
            ch_printf( ch, "%20s %4s %4s %4s  %20s %4s %4s %4s\r\n", "Fish Name", "Min", "Max", "Cost", "Fish Name", "Min", "Max", "Cost" );
            send_to_char( "-------------------- ---- ---- ----  -------------------- ---- ---- ----\r\n", ch );
         }
         ch_printf( ch, "%20s %4d %4d %4d", fname->name, fname->minweight, fname->maxweight, fname->value );
         if( ++cnt == 2 )
         {
            send_to_char( "\r\n", ch );
            cnt = 0;
         }
         else
            send_to_char( "  ", ch );
         found = true;
      }
      if( !found )
         send_to_char( "No fish names to display.\r\n", ch );
      else if( cnt != 0 )
         send_to_char( "\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   fname = find_fname( arg );

   if( !str_cmp( arg2, "create" ) )
   {
      if( fname )
      {
         send_to_char( "That fish name is already being used.\r\n", ch );
         return;
      }
      CREATE( fname, FNAME_DATA, 1 );
      STRSET( fname->name, arg );
      fname->minweight = 1;
      fname->maxweight = 5;
      fname->value = 5;
      add_fname( fname );
      save_fnames( );
      send_to_char( "That fish name has been added.\r\n", ch );
      return;
   }

   if( !fname )
   {
      send_to_char( "No such fish name exist.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      remove_fname( fname );
      free_fname( fname );
      save_fnames( );
      send_to_char( "That fish name has been deleted.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "What fish name would you like to use for it instead?\r\n", ch );
         return;
      }
      if( find_fname( argument ) )
      {
         send_to_char( "That fish name is already being used.\r\n", ch );
         return;
      }
      STRSET( fname->name, argument );
      save_fnames( );
      send_to_char( "The fish name has been changed.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "minweight" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "What minweight would you like to set it to?\r\n", ch );
         return;
      }
      fname->minweight = UMAX( 1, atoi( argument ) );
      save_fnames( );
      send_to_char( "The minweight has been set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "maxweight" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         send_to_char( "What maxweight would you like to set it to?\r\n", ch );
         return;
      }
      fname->maxweight = UMAX( 1, atoi( argument ) );
      save_fnames( );
      send_to_char( "The maxweight has been set.\r\n", ch );
      return;
   }

   if( !str_cmp( arg2, "value" ) )
   {
      if( !argument || argument[0] == '\0' || !is_number( argument ) )
      {
         send_to_char( "What value would you like to set it to?\r\n", ch );
         return;
      }
      fname->value = UMAX( 1, atoi( argument ) );
      save_fnames( );
      send_to_char( "The value has been set.\r\n", ch );
      return;
   }

   send_to_char( "Usage: fname <fname> create/delete\r\n", ch );
   send_to_char( "Usage: fname <fname> name <new fname>\r\n", ch );
   send_to_char( "Usage: fname <fname> minweight/maxweight/value <#>\r\n", ch );
}

void remove_fish( FISH_DATA *fish )
{
   if( !fish )
      return;
   UNLINK( fish, first_fish, last_fish, next, prev );
}

void add_fish( FISH_DATA *fish )
{
   if( !fish )
      return;
   LINK( fish, first_fish, last_fish, next, prev );
}

void free_fish( FISH_DATA *fish )
{
   if( !fish )
      return;
   fish->rod = NULL;
   DISPOSE( fish );
}

void free_all_fish( void )
{
   FISH_DATA *fish, *fish_next;

   for( fish = first_fish; fish; fish = fish_next )
   {
      fish_next = fish->next;

      remove_fish( fish );
      free_fish( fish );
   }
}

bool create_fish( OBJ_DATA *rod )
{
   FISH_DATA *fish;

   if( !rod )
      return false;
   CREATE( fish, FISH_DATA, 1 );
   if( !fish )
   {
      bug( "%s: failed to create fish.", __FUNCTION__ );
      return false;
   }
   fish->rod = rod;
   add_fish( fish );
   return true;
}

bool catch_fish( FISH_DATA *fish )
{
   OBJ_INDEX_DATA *findex;
   OBJ_DATA *nfish;
   FNAME_DATA *fname;
   const char *name;
   char buf[MSL];

   if( !fish || !fish->rod )
      return false;
   if( ( findex = get_obj_index( OBJ_VNUM_FISH ) ) )
   {
      nfish = create_object( findex, 0 );

      if( !( fname = get_fish_name( ) ) )
      {
         name = (char *)"catfish"; /* Need some default in case lol*/
         nfish->weight = number_range( 1, 10 );
         nfish->cost = number_range( 5, 10 );
         nfish->cost *= nfish->weight;
      }
      else
      {
         name = fname->name;
         nfish->weight = number_range( fname->minweight, fname->maxweight );
         nfish->cost = ( fname->value * nfish->weight );
      }
      if( nfish->name )
         snprintf( buf, sizeof( buf ), nfish->name, name );
      else
         snprintf( buf, sizeof( buf ), "%s fish", name );
      STRSET( nfish->name, buf );

      if( nfish->short_descr )
         snprintf( buf, sizeof( buf ), nfish->short_descr, name );
      else
         snprintf( buf, sizeof( buf ), "%s", name );
      STRSET( nfish->short_descr, aoran( buf ) );

      if( nfish->description )
         snprintf( buf, sizeof( buf ), nfish->description, name );
      else
         snprintf( buf, sizeof( buf ), "%s lies here on the ground.", name );
      STRSET( nfish->description, capitalize( aoran( buf ) ) );

      /* Value for when you cook and eat it */
      nfish->value[0] = UMAX( 1, nfish->weight );
      nfish->value[1] = number_range( 10, 100 );
      /* It is raw after all */
      nfish->value[2] = 0;
      /* Maybe sometime make it so some fish are poisonious */
      nfish->value[3] = 0;

//      obj_to_obj( nfish, fish->rod );

      if( fish->rod->carried_by )
      {
         act( AT_ACTION, "You reel in your line to find $p.", fish->rod->carried_by, nfish, NULL, TO_CHAR );
         act( AT_ACTION, "$n reels in $s line to find $p.", fish->rod->carried_by, nfish, NULL, TO_ROOM );
         obj_to_char( nfish, fish->rod->carried_by );
      }
      remove_fish( fish );
      free_fish( fish );
      return true;
   }
   else
      bug( "%s: couldn't find object vnum %d.", __FUNCTION__, OBJ_VNUM_FISH );
   return false;
}

bool stop_obj_fishing( OBJ_DATA *obj )
{
   FISH_DATA *fish, *fish_next;
   bool found = false;

   if( !obj || obj->item_type != ITEM_FISHINGPOLE )
      return false;

   for( fish = first_fish; fish; fish = fish_next )
   {
      fish_next = fish->next;

      if( fish->rod != obj )
         continue;
      remove_fish( fish );
      free_fish( fish );
      found = true;
   }
   return found;
}

bool stop_fishing( CHAR_DATA *ch )
{
   OBJ_DATA *fishingpole;

   if( !ch )
      return false;

   if( !( fishingpole = get_eq_hold( ch, ITEM_FISHINGPOLE ) ) )
      return false;

   return stop_obj_fishing( fishingpole );
}

bool is_fishing( CHAR_DATA *ch )
{
   FISH_DATA *fish;
   OBJ_DATA *fishingpole;

   if( !ch )
      return false;

   if( !( fishingpole = get_eq_hold( ch, ITEM_FISHINGPOLE ) ) )
      return false;

   separate_obj( fishingpole );

   for( fish = first_fish; fish; fish = fish->next )
   {
      if( fish->rod == fishingpole )
         return true;
   }
   return false;
}

int fish_update = 0;

void update_fishing( void )
{
   FISH_DATA *fish, *next_fish;

   if( --fish_update > 0 )
      return;

   fish_update = number_range( 80, 100 );

   for( fish = first_fish; fish; fish = next_fish )
   {
      next_fish = fish->next;

      /* If no rod no point in keeping it */
      if( !fish->rod )
      {
         remove_fish( fish );
         free_fish( fish );
         continue;
      }

      /* Baited? */
      if( !fish->rod->first_content )
         continue;

      /* Nothing happens */
      if( number_percent( ) < 75 )
         continue;

      /* Looses bait */
      if( number_percent( ) < 50 )
      {
         extract_obj( fish->rod->first_content );
         if( fish->rod->carried_by )
         {
            act( AT_ACTION, "You reel in your line to find all the bait gone.", fish->rod->carried_by, NULL, NULL, TO_CHAR );
            act( AT_ACTION, "$n reels in $s line to find all the bait gone.", fish->rod->carried_by, NULL, NULL, TO_ROOM );
         }
         remove_fish( fish );
         free_fish( fish );
         continue;
      }

      /* Catches a fish */
      /* Remove bait first, fish becomes new bait */
      extract_obj( fish->rod->first_content );
      catch_fish( fish );
   }
}

void do_fish( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *fishingpole;

   if( !( fishingpole = get_eq_hold( ch, ITEM_FISHINGPOLE ) ) )
   {
      send_to_char( "You must be holding a fishing pole.\r\n", ch );
      return;
   }

   separate_obj( fishingpole );

   if( ch->in_room->sector_type != SECT_WATER_SWIM && ch->in_room->sector_type != SECT_WATER_NOSWIM
    && ch->in_room->sector_type != SECT_OCEAN )
   {
      send_to_char( "Water is needed to fish!\r\n", ch );
      return;
   }

   if( !str_cmp( argument, "stop" ) )
   {
      if( stop_fishing( ch ) )
      {
         act( AT_ACTION, "You reel in your line and stop fishing.", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n reels in $s line and stops fishing.", ch, NULL, NULL, TO_ROOM );
      }
      else
         send_to_char( "You aren't fishing.\r\n", ch );
      return;
   }

   if( is_fishing( ch ) )
   {
      send_to_char( "You're already fishing.\r\n", ch );
      return;
   }

   if( !fishingpole->first_content )
   {
      send_to_char( "That pole doesn't have any bait to attract fish.\r\n", ch );
      return;
   }

   if( create_fish( fishingpole ) )
   {
      act( AT_ACTION, "You cast your line into the water.", ch, NULL, NULL, TO_CHAR );
      act( AT_ACTION, "$n cast $s line into the water.", ch, NULL, NULL, TO_ROOM );
   }
   else
      send_to_char( "You failed to start fishing.\r\n", ch );
}

void do_bait( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *fishingpole, *bait;

   if( !( fishingpole = get_eq_hold( ch, ITEM_FISHINGPOLE ) ) )
   {
      send_to_char( "You must be holding a fishing pole.\r\n", ch );
      return;
   }

   if( is_fishing( ch ) )
   {
      send_to_char( "You can't do anything to it while your fishing.\r\n", ch );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "What would you like to bait the hook with?\r\n", ch );
      return;
   }

   separate_obj( fishingpole );

   if( !str_cmp( argument, "remove" ) )
   {
      if( !( bait = fishingpole->first_content ) )
      {
         send_to_char( "There is nothing on the hook to remove.\r\n", ch );
         return;
      }
      obj_from_obj( bait );
      obj_to_char( bait, ch );
      act( AT_ACTION, "You remove $P from $p's hook.", ch, fishingpole, bait, TO_CHAR );
      act( AT_ACTION, "$n removes $P from $p's hook.", ch, fishingpole, bait, TO_ROOM );
      return;
   }

   if( !( bait = get_obj_carry( ch, argument ) ) )
   {
      send_to_char( "You don't have that item.\r\n", ch );
      return;
   }

   if( fishingpole->first_content )
   {
      send_to_char( "That pole is already baited.\r\n", ch );
      return;
   }

   separate_obj( bait );

   if( bait->item_type != ITEM_FISH )
   {
      send_to_char( "You can't bait a hook with that.\r\n", ch );
      return;
   }

   obj_from_char( bait );
   obj_to_obj( bait, fishingpole );
   save_char_obj( ch );
   act( AT_ACTION, "You bait $p's hook with $P.", ch, fishingpole, bait, TO_CHAR );
   act( AT_ACTION, "$n baits $p's hook with $P.", ch, fishingpole, bait, TO_ROOM );
}
