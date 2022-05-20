/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *                          Player skills module                            *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "mud.h"

bool validate_spec_fun( const char *name );

const char *const spell_flag[] = { "water", "earth", "air", "astral", "area", "distant", "reverse",
   "noself", "_unused2_", "accumulative", "recastable", "noscribe",
   "nobrew", "group", "object", "character", "secretskill", "pksensitive",
   "stoponfail", "nofight", "nodispel", "randomtarget", "r2", "r3", "r4",
   "r5", "r6", "r7", "r8", "r9", "r10", "r11"
};

const char *const spell_saves[] = { "none", "poison_death", "wands", "para_petri", "breath", "spell_staff" };

const char *const spell_save_effect[] = { "none", "negate", "eightdam", "quarterdam", "halfdam", "3qtrdam", "reflect", "absorb" };

const char *const spell_damage[] = { "none", "fire", "water", "air", "earth", "life", "darkness", "poison" };

const char *const spell_action[] = { "none", "create", "destroy", "resist", "suscept", "divinate", "obscure", "change" };

const char *const spell_power[] = { "none", "minor", "greater", "major" };

const char *const spell_class[] = { "none", "lunar", "solar", "travel", "summon", "life", "death", "illusion" };

const char *const target_type[] = { "ignore", "offensive", "defensive", "self", "objinv" };


void show_char_to_char( CHAR_DATA * list, CHAR_DATA * ch );
int ris_save( CHAR_DATA * ch, int schance, int ris );
bool check_illegal_psteal( CHAR_DATA * ch, CHAR_DATA * victim );

/* from magic.c */
void failed_casting( struct skill_type *skill, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj );

void free_skill( SKILLTYPE * skill )
{
   SMAUG_AFF *aff, *aff_next;

   if( skill->first_affect )
   {
      for( aff = skill->first_affect; aff; aff = aff_next )
      {
         aff_next = aff->next;

         UNLINK( aff, skill->first_affect, skill->last_affect, next, prev );
         DISPOSE( aff->duration );
         DISPOSE( aff->modifier );
         DISPOSE( aff );
      }
   }
   DISPOSE( skill->name );
   DISPOSE( skill->noun_damage );
   DISPOSE( skill->msg_off );
   DISPOSE( skill->hit_char );
   DISPOSE( skill->hit_vict );
   DISPOSE( skill->hit_room );
   DISPOSE( skill->hit_dest );
   DISPOSE( skill->miss_char );
   DISPOSE( skill->miss_vict );
   DISPOSE( skill->miss_room );
   DISPOSE( skill->die_char );
   DISPOSE( skill->die_vict );
   DISPOSE( skill->die_room );
   DISPOSE( skill->imm_char );
   DISPOSE( skill->imm_vict );
   DISPOSE( skill->imm_room );
   DISPOSE( skill->dice );
   if( skill->spell_fun_name )
      DISPOSE( skill->spell_fun_name );
   if( skill->skill_fun_name )
      DISPOSE( skill->skill_fun_name );
   DISPOSE( skill->components );
   DISPOSE( skill->teachers );
   skill->spell_fun = NULL;
   skill->skill_fun = NULL;
   DISPOSE( skill );

   return;
}

void free_skills( void )
{
   SKILLTYPE *skill;
   int hash = 0;

   for( hash = 0; hash < num_skills; ++hash )
   {
      skill = skill_table[hash];
      free_skill( skill );

      skill_table_bytype[hash] = NULL;
   }

   for( hash = 0; hash < top_herb; hash++ )
   {
      skill = herb_table[hash];
      free_skill( skill );
   }
   return;
}

/*
 * Dummy function
 */
void skill_notfound( CHAR_DATA * ch, const char *argument )
{
   send_to_char( "Huh?\r\n", ch );
   return;
}


int get_ssave( const char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_saves ) / sizeof( spell_saves[0] ); x++ )
      if( !str_cmp( name, spell_saves[x] ) )
         return x;
   return -1;
}

int get_starget( const char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( target_type ) / sizeof( target_type[0] ); x++ )
      if( !str_cmp( name, target_type[x] ) )
         return x;
   return -1;
}

int get_sflag( const char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_flag ) / sizeof( spell_flag[0] ); x++ )
      if( !str_cmp( name, spell_flag[x] ) )
         return x;
   return -1;
}

int get_sdamage( const char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_damage ) / sizeof( spell_damage[0] ); x++ )
      if( !str_cmp( name, spell_damage[x] ) )
         return x;
   return -1;
}

int get_saction( const char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_action ) / sizeof( spell_action[0] ); x++ )
      if( !str_cmp( name, spell_action[x] ) )
         return x;
   return -1;
}

int get_ssave_effect( const char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_save_effect ) / sizeof( spell_save_effect[0] ); x++ )
      if( !str_cmp( name, spell_save_effect[x] ) )
         return x;
   return -1;
}

int get_spower( const char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_power ) / sizeof( spell_power[0] ); x++ )
      if( !str_cmp( name, spell_power[x] ) )
         return x;
   return -1;
}

int get_sclass( const char *name )
{
   unsigned int x;

   for( x = 0; x < sizeof( spell_class ) / sizeof( spell_class[0] ); x++ )
      if( !str_cmp( name, spell_class[x] ) )
         return x;
   return -1;
}

bool is_legal_kill( CHAR_DATA * ch, CHAR_DATA * vch )
{
   if( IS_NPC( ch ) || IS_NPC( vch ) )
      return TRUE;
   if( !IS_PKILL( ch ) || !IS_PKILL( vch ) )
      return FALSE;
   if( ch->pcdata->clan && ch->pcdata->clan == vch->pcdata->clan )
      return FALSE;
   return TRUE;
}

/*
 * Racial Skills Handling -- Kayle 7-8-07
 */
bool check_ability( CHAR_DATA * ch, char *command, char *argument )
{
   int sn, manacost, mana;
   struct timeval time_used;

   /*
    * bsearch for the ability
    */
   sn = find_ability( ch, command, TRUE );

   if( sn == -1 )
      return FALSE;

   // some additional checks
   if( !( skill_table[sn]->skill_fun || skill_table[sn]->spell_fun != spell_null ) || !can_use_skill( ch, 0, sn ) )
   {
      return FALSE;
   }

   if( !check_pos( ch, skill_table[sn]->minimum_position ) )
      return TRUE;

   if( IS_NPC( ch ) && ( IS_AFFECTED( ch, AFF_CHARM ) || IS_AFFECTED( ch, AFF_POSSESS ) ) )
   {
      send_to_char( "For some reason, you seem unable to perform that...\r\n", ch );
      act( AT_GREY, "$n wanders around aimlessly.", ch, NULL, NULL, TO_ROOM );
      return TRUE;
   }

   /*
    * check if mana is required
    */
   if( skill_table[sn]->min_mana )
   {
      manacost = ( skill_table[sn]->min_mana * 100 ) / ( 99 + ch->pcdata->learned[sn] ) ;
      mana = IS_NPC( ch ) ? 0 : UMIN( skill_table[sn]->min_mana, manacost );
      if( !IS_NPC( ch ) && ch->mana < mana )
      {
         send_to_char( "You don't have enough mana.\r\n", ch );
         return TRUE;
      }
   }
   else
      mana = 0;

   /*
    * Is this a real do-fun, or a really a spell?
    */
   if( !skill_table[sn]->skill_fun )
   {
      ch_ret retcode = rNONE;
      void *vo = NULL;
      CHAR_DATA *victim = NULL;
      OBJ_DATA *obj = NULL;

      target_name = "";

      switch ( skill_table[sn]->target )
      {
         default:
            bug( "Check_ability: bad target for sn %d.", sn );
            send_to_char( "Something went wrong...\r\n", ch );
            return TRUE;

         case TAR_IGNORE:
            vo = NULL;
            if( !argument || argument[0] == '\0' )
            {
               if( ( victim = who_fighting( ch ) ) != NULL )
                  target_name = victim->name;
            }
            else
               target_name = argument;
            break;

         case TAR_CHAR_OFFENSIVE:
         {
            if( ( !argument || argument[0] == '\0' ) && !( victim = who_fighting( ch ) ) )
            {
               ch_printf( ch, "Confusion overcomes you as your '%s' has no target.\r\n", skill_table[sn]->name );
               return TRUE;
            }
            else if( argument[0] != '\0' && !( victim = get_char_room( ch, argument ) ) )
            {
               send_to_char( "They aren't here.\r\n", ch );
               return TRUE;
            }
         }
            if( is_safe( ch, victim, TRUE ) )
               return TRUE;

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               send_to_char( "You can't target yourself!\r\n", ch );
               return TRUE;
            }

            if( !IS_NPC( ch ) )
            {
               if( !IS_NPC( victim ) )
               {
                  if( get_timer( ch, TIMER_PKILLED ) > 0 )
                  {
                     send_to_char( "You have been killed in the last 5 minutes.\r\n", ch );
                     return TRUE;
                  }

                  if( get_timer( victim, TIMER_PKILLED ) > 0 )
                  {
                     send_to_char( "This player has been killed in the last 5 minutes.\r\n", ch );
                     return TRUE;
                  }

                  if( victim != ch )
                     send_to_char( "You really shouldn't do this to another player...\r\n", ch );
               }

               if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
               {
                  send_to_char( "You can't do that on your own follower.\r\n", ch );
                  return TRUE;
               }
            }

            if( check_illegal_pk( ch, victim ) )
            {
               send_to_char( "You can't do that to another player!\r\n", ch );
               return TRUE;
            }
            vo = ( void * )victim;
            break;

         case TAR_CHAR_DEFENSIVE:
         {
            if( argument[0] != '\0' && !( victim = get_char_room( ch, argument ) ) )
            {
               send_to_char( "They aren't here.\r\n", ch );
               return TRUE;
            }
            if( !victim )
               victim = ch;
         }

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               send_to_char( "You can't target yourself!\r\n", ch );
               return TRUE;
            }
            vo = ( void * )victim;
            break;

         case TAR_CHAR_SELF:
            victim = ch;
            vo = ( void * )ch;
            break;

         case TAR_OBJ_INV:
         {
            if( !( obj = get_obj_carry( ch, argument ) ) )
            {
               send_to_char( "You can't find that.\r\n", ch );
               return TRUE;
            }
         }
            vo = ( void * )obj;
            break;
      }

      /*
       * waitstate
       */
      WAIT_STATE( ch, skill_table[sn]->beats );
      if( mana )
         ch->mana -= mana;
      start_timer( &time_used );
      retcode = ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, vo );
      end_timer( &time_used );
      update_userec( &time_used, &skill_table[sn]->userec );

      if( retcode == rCHAR_DIED || retcode == rERROR )
         return TRUE;

      if( char_died( ch ) )
         return TRUE;

      if( retcode == rSPELL_FAILED )
      {
         learn_from_failure( ch, sn );
         retcode = rNONE;
      }
      else
         ability_learn_from_success( ch, sn );

      if( skill_table[sn]->target == TAR_CHAR_OFFENSIVE && victim != ch && !char_died( victim ) )
      {
         CHAR_DATA *vch;
         CHAR_DATA *vch_next;

         for( vch = ch->in_room->first_person; vch; vch = vch_next )
         {
            vch_next = vch->next_in_room;
            if( victim == vch && !victim->fighting && victim->master != ch )
            {
               retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
               break;
            }
         }
      }
      return TRUE;
   }

   if( mana )
      ch->mana -= mana;
   ch->prev_cmd = ch->last_cmd;  /* haus, for automapping */
   ch->last_cmd = skill_table[sn]->skill_fun;
   start_timer( &time_used );
   ( *skill_table[sn]->skill_fun ) ( ch, argument );
   end_timer( &time_used );
   update_userec( &time_used, &skill_table[sn]->userec );

   tail_chain(  );
   return TRUE;
}

/*
 * Perform a binary search on a section of the skill table
 * Each different section of the skill table is sorted alphabetically
 * Only match skills player knows				-Thoric
 */
bool check_skill( CHAR_DATA * ch, char *command, char *argument )
{
   int sn, manacost, mana;
   struct timeval time_used;

   /*
    * bsearch for the skill
    */
   sn = find_skill( ch, command, TRUE );

   if( sn == -1 )
      return FALSE;

   // some additional checks
   if( !( skill_table[sn]->skill_fun || skill_table[sn]->spell_fun != spell_null ) || !can_use_skill( ch, 0, sn ) )
   {
      return FALSE;
   }

   if( !check_pos( ch, skill_table[sn]->minimum_position ) )
      return TRUE;

   if( IS_NPC( ch ) && ( IS_AFFECTED( ch, AFF_CHARM ) || IS_AFFECTED( ch, AFF_POSSESS ) ) )
   {
      send_to_char( "For some reason, you seem unable to perform that...\r\n", ch );
      act( AT_GREY, "$n wanders around aimlessly.", ch, NULL, NULL, TO_ROOM );
      return TRUE;
   }

   /*
    * check if mana is required
    */
   if( skill_table[sn]->min_mana )
   {
      manacost = ( skill_table[sn]->min_mana * 100 ) / ( 99 + ch->pcdata->learned[sn] ) ;
      mana = IS_NPC( ch ) ? 0 : UMIN( skill_table[sn]->min_mana, manacost );
      if( !IS_NPC( ch ) && ch->mana < mana )
      {
         send_to_char( "You don't have enough mana.\r\n", ch );
         return TRUE;
      }
   }
   else
      mana = 0;


   /*
    * Is this a real do-fun, or a really a spell?
    */
   if( !skill_table[sn]->skill_fun )
   {
      ch_ret retcode = rNONE;
      void *vo = NULL;
      CHAR_DATA *victim = NULL;
      OBJ_DATA *obj = NULL;

      target_name = "";

      switch ( skill_table[sn]->target )
      {
         default:
            bug( "Check_skill: bad target for sn %d.", sn );
            send_to_char( "Something went wrong...\r\n", ch );
            return TRUE;

         case TAR_IGNORE:
            vo = NULL;
            if( argument[0] == '\0' )
            {
               if( ( victim = who_fighting( ch ) ) != NULL )
                  target_name = victim->name;
            }
            else
               target_name = argument;
            break;

         case TAR_CHAR_OFFENSIVE:
         {
            if( argument[0] == '\0' && ( victim = who_fighting( ch ) ) == NULL )
            {
               ch_printf( ch, "Confusion overcomes you as your '%s' has no target.\r\n", skill_table[sn]->name );
               return TRUE;
            }
            else if( argument[0] != '\0' && ( victim = get_char_room( ch, argument ) ) == NULL )
            {
               send_to_char( "They aren't here.\r\n", ch );
               return TRUE;
            }
         }
            if( is_safe( ch, victim, TRUE ) )
               return TRUE;

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               send_to_char( "You can't target yourself!\r\n", ch );
               return TRUE;
            }

            if( !IS_NPC( ch ) )
            {
               if( !IS_NPC( victim ) )
               {
                  /*
                   * Sheesh! can't do anything
                   * send_to_char( "You can't do that on a player.\r\n", ch );
                   * return TRUE;
                   */
                  if( get_timer( ch, TIMER_PKILLED ) > 0 )
                  {
                     send_to_char( "You have been killed in the last 5 minutes.\r\n", ch );
                     return TRUE;
                  }

                  if( get_timer( victim, TIMER_PKILLED ) > 0 )
                  {
                     send_to_char( "This player has been killed in the last 5 minutes.\r\n", ch );
                     return TRUE;
                  }

                  if( victim != ch )
                     send_to_char( "You really shouldn't do this to another player...\r\n", ch );
               }

               if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master == victim )
               {
                  send_to_char( "You can't do that on your own follower.\r\n", ch );
                  return TRUE;
               }
            }

            check_illegal_pk( ch, victim );
            vo = ( void * )victim;
            break;

         case TAR_CHAR_DEFENSIVE:
         {
            if( argument[0] != '\0' && ( victim = get_char_room( ch, argument ) ) == NULL )
            {
               send_to_char( "They aren't here.\r\n", ch );
               return TRUE;
            }
            if( !victim )
               victim = ch;
         }

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               send_to_char( "You can't target yourself!\r\n", ch );
               return TRUE;
            }

            vo = ( void * )victim;
            break;

         case TAR_CHAR_SELF:
            vo = ( void * )ch;
            break;

         case TAR_OBJ_INV:
         {
            if( ( obj = get_obj_carry( ch, argument ) ) == NULL )
            {
               send_to_char( "You can't find that.\r\n", ch );
               return TRUE;
            }
         }
            vo = ( void * )obj;
            break;
      }

      /*
       * waitstate
       */
      WAIT_STATE( ch, skill_table[sn]->beats );
      if( mana )
         ch->mana -= mana;
      start_timer( &time_used );
      retcode = ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, vo );
      end_timer( &time_used );
      update_userec( &time_used, &skill_table[sn]->userec );

      if( retcode == rCHAR_DIED || retcode == rERROR )
         return TRUE;

      if( char_died( ch ) )
         return TRUE;

      if( retcode == rSPELL_FAILED )
      {
         learn_from_failure( ch, sn );
         retcode = rNONE;
      }
      else
         learn_from_success( ch, sn );

      if( skill_table[sn]->target == TAR_CHAR_OFFENSIVE && victim != ch && !char_died( victim ) )
      {
         CHAR_DATA *vch;
         CHAR_DATA *vch_next;

         for( vch = ch->in_room->first_person; vch; vch = vch_next )
         {
            vch_next = vch->next_in_room;
            if( victim == vch && !victim->fighting && victim->master != ch )
            {
               retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
               break;
            }
         }
      }
      return TRUE;
   }

   if( mana )
      ch->mana -= mana;
   ch->prev_cmd = ch->last_cmd;  /* haus, for automapping */
   ch->last_cmd = skill_table[sn]->skill_fun;
   start_timer( &time_used );
   ( *skill_table[sn]->skill_fun ) ( ch, argument );
   end_timer( &time_used );
   update_userec( &time_used, &skill_table[sn]->userec );

   tail_chain(  );
   return TRUE;
}

void do_skin( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *korps;
   OBJ_DATA *corpse;
   OBJ_DATA *obj;
   OBJ_DATA *skin;
   bool found;
   const char *name;
   char buf[MAX_STRING_LENGTH];
   found = FALSE;

   if( argument[0] == '\0' )
   {
      send_to_char( "Whose corpse do you wish to skin?\r\n", ch );
      return;
   }
   if( ( corpse = get_obj_here( ch, argument ) ) == NULL )
   {
      send_to_char( "You cannot find that here.\r\n", ch );
      return;
   }
   if( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL )
   {
      send_to_char( "You have no weapon with which to perform this deed.\r\n", ch );
      return;
   }
/*
   if( corpse->item_type != ITEM_CORPSE_PC )
   {
      send_to_char( "You can only skin the bodies of player characters.\r\n", ch );
      return;
   }
*/
   if( obj->value[3] != 1 && obj->value[3] != 2 && obj->value[3] != 3 && obj->value[3] != 11 )
   {
      send_to_char( "There is nothing you can do with this corpse.\r\n", ch );
      return;
   }
   if( get_obj_index( OBJ_VNUM_SKIN ) == NULL )
   {
      bug( "Vnum %d (OBJ_VNUM_SKIN) not found for do_skin!", OBJ_VNUM_SKIN );
      return;
   }
   korps = create_object( get_obj_index( OBJ_VNUM_CORPSE_PC ), 0 );
   skin = create_object( get_obj_index( OBJ_VNUM_SKIN ), 0 );
   name = IS_NPC( ch ) ? korps->short_descr : corpse->short_descr;
   snprintf( buf, MAX_STRING_LENGTH, skin->short_descr, name );
   STRFREE( skin->short_descr );
   skin->short_descr = STRALLOC( buf );
   snprintf( buf, MAX_STRING_LENGTH, skin->description, name );
   STRFREE( skin->description );
   skin->description = STRALLOC( buf );
   act( AT_BLOOD, "$n strips the skin from $p.", ch, corpse, NULL, TO_ROOM );
   act( AT_BLOOD, "You strip the skin from $p.", ch, corpse, NULL, TO_CHAR );
/*  act( AT_MAGIC, "\nThe skinless corpse is dragged through the ground by a strange force...", ch, corpse, NULL, TO_CHAR);
    act( AT_MAGIC, "\nThe skinless corpse is dragged through the ground by a strange force...", ch, corpse, NULL, TO_ROOM);
    extract_obj( corpse ); */
   obj_to_char( skin, ch );
   return;
}

/*
 * Lookup a skills information
 * High god command
 */
void do_slookup( CHAR_DATA* ch, const char* argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   int sn;
   int iClass;
   SKILLTYPE *skill = NULL;

   one_argument( argument, arg );
   if( arg[0] == '\0' )
   {
      send_to_char( "Slookup what?\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "slots" ) )
   {
      int firstfree = -1, lastfree = 0, found = 0, cnt = 0;
      bool firstloop = true;

      send_to_char( "Free slots:\r\n", ch );
      for( iClass = 0; iClass < MAX_SKILL; iClass++ )
      {
         found = 0;

         for( sn = 0; sn < num_skills && skill_table[sn] && skill_table[sn]->name; sn++ )
         {
            if( skill_table[sn]->slot == iClass )
            {
               /* On the first loop lets check for slots higher then MAX_SKILL */
               if( firstloop && skill_table[sn]->slot > MAX_SKILL )
                  ch_printf( ch, "SN %d has a slot higher then %d (MAX_SKILL).\r\n", sn, MAX_SKILL );
               found++;
            }
         }
         firstloop = false;
         /* Keep track of free slots */
         if( found == 0 )
         {
            if( firstfree == -1 )
               firstfree = iClass;
            lastfree = iClass;
            if( iClass < ( MAX_SKILL - 1 ) )
               continue;
         }
         if( firstfree != -1 )
         {
            if( lastfree != firstfree )
               snprintf( buf, sizeof( buf ), "[%4d - %-4d]", firstfree, lastfree );
            else
               snprintf( buf, sizeof( buf ), "[   %4d    ]", lastfree );
            ch_printf( ch, "%14s", buf );
            if( ++cnt == 6 )
            {
               cnt = 0;
               send_to_char( "\r\n", ch );
            }
         }
         else if( found > 1 && iClass != 0 ) /* 0 is basicaly not set */
         {
            if( cnt != 0 )
            {
               cnt = 0;
               send_to_char( "\r\n", ch );
            }
            ch_printf( ch, "%4d is used on more then one spell/skill.\r\n", iClass );
         }
         firstfree = -1;
         lastfree = -1;
      }
      if( cnt != 0 )
         send_to_char( "\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "all" ) )
   {
      for( sn = 0; sn < num_skills && skill_table[sn] && skill_table[sn]->name; ++sn )
         pager_printf( ch, "Sn: %4d Slot: %4d Skill/spell: '%-20s' Damtype: %s\r\n",
                       sn, skill_table[sn]->slot, skill_table[sn]->name, spell_damage[SPELL_DAMAGE( skill_table[sn] )] );
   }
   else if( !str_cmp( arg, "herbs" ) )
   {
      for( sn = 0; sn < top_herb && herb_table[sn] && herb_table[sn]->name; sn++ )
         pager_printf( ch, "%d) %s\r\n", sn, herb_table[sn]->name );
   }
   else
   {
      SMAUG_AFF *aff;
      int cnt = 0;

      if( arg[0] == 'h' && is_number( arg + 1 ) )
      {
         sn = atoi( arg + 1 );
         if( !IS_VALID_HERB( sn ) )
         {
            send_to_char( "Invalid herb.\r\n", ch );
            return;
         }
         skill = herb_table[sn];
      }
      else if( is_number( arg ) )
      {
         sn = atoi( arg );
         if( ( skill = get_skilltype( sn ) ) == NULL )
         {
            send_to_char( "Invalid sn.\r\n", ch );
            return;
         }
         sn %= 1000;
      }
      else if( ( sn = skill_lookup( arg ) ) >= 0 )
         skill = skill_table[sn];
      else if( ( sn = herb_lookup( arg ) ) >= 0 )
         skill = herb_table[sn];
      else
      {
         send_to_char( "No such skill, spell, proficiency or tongue.\r\n", ch );
         return;
      }
      if( !skill )
      {
         send_to_char( "Not created yet.\r\n", ch );
         return;
      }

      ch_printf( ch, "Sn: %4d Slot: %4d %s: '%-20s'\r\n", sn, skill->slot, skill_tname[skill->type], skill->name );
      if( skill->info )
         ch_printf( ch, "DamType: %s  ActType: %s   ClassType: %s   PowerType: %s\r\n",
                    spell_damage[SPELL_DAMAGE( skill )],
                    spell_action[SPELL_ACTION( skill )],
                    spell_class[SPELL_CLASS( skill )], spell_power[SPELL_POWER( skill )] );
      if( skill->flags )
      {
         int x;

         mudstrlcpy( buf, "Flags:", MAX_STRING_LENGTH );
         for( x = 0; x < 32; ++x )
            if( SPELL_FLAG( skill, 1 << x ) )
            {
               mudstrlcat( buf, " ", MAX_STRING_LENGTH );
               mudstrlcat( buf, spell_flag[x], MAX_STRING_LENGTH );
            }
         mudstrlcat( buf, "\r\n", MAX_STRING_LENGTH );
         send_to_char( buf, ch );
      }
      ch_printf( ch, "Attribute: %s  Saves: %s  SaveEffect: %s\r\n",
                 affect_loc_name( skill->attribute ), spell_saves[( int )skill->saves], spell_save_effect[SPELL_SAVE( skill )] );

      if( skill->difficulty != '\0' )
         ch_printf( ch, "Difficulty: %d\r\n", ( int )skill->difficulty );

      ch_printf( ch, "Type: %s  Target: %s  Minpos: %d  Mana: %d  Beats: %d  Range: %d\r\n",
                 skill_tname[skill->type],
                 target_type[URANGE( TAR_IGNORE, skill->target, TAR_OBJ_INV )],
                 skill->minimum_position, skill->min_mana, skill->beats, skill->range );
      ch_printf( ch, "Flags: %d  Guild: %d  Value: %d  Info: %d  Code: %s\r\n",
                 skill->flags,
                 skill->guild, skill->value, skill->info, skill->skill_fun ? skill->skill_fun_name : skill->spell_fun_name );
      ch_printf( ch, "Sectors Allowed: %s\n", skill->spell_sector ? flag_string( skill->spell_sector, sec_flags ) : "All" );
      ch_printf( ch, "Dammsg: %s\r\nWearoff: %s\n", skill->noun_damage, skill->msg_off ? skill->msg_off : "(none set)" );
      if( skill->req_skill && skill->req_skill != -1 )
      {
         int reqsn = skill->req_skill;

         ch_printf( ch, "Reqskill: %d", skill->req_skill );
         if( IS_VALID_SN( reqsn ) && skill_table[ reqsn ] )
            ch_printf( ch, "(%s)", skill_table[ reqsn ]->name );
         else
            send_to_char( "(UNKNOWN)", ch );
         send_to_char( "\r\n", ch );
      }
      if( skill->dice && skill->dice[0] != '\0' )
         ch_printf( ch, "Dice: %s\r\n", skill->dice );
      if( skill->teachers && skill->teachers[0] != '\0' )
         ch_printf( ch, "Teachers: %s\r\n", skill->teachers );
      if( skill->components && skill->components[0] != '\0' )
         ch_printf( ch, "Components: %s\r\n", skill->components );
      if( skill->participants )
         ch_printf( ch, "Participants: %d\r\n", ( int )skill->participants );
      if( skill->userec.num_uses )
         send_timer( &skill->userec, ch );
      for( aff = skill->first_affect; aff; aff = aff->next )
      {
         if( aff == skill->first_affect )
            send_to_char( "\r\n", ch );
         snprintf( buf, MAX_STRING_LENGTH, "Affect %d", ++cnt );
         if( aff->location )
         {
            mudstrlcat( buf, " modifies ", MAX_STRING_LENGTH );
            mudstrlcat( buf, a_types[aff->location % REVERSE_APPLY], MAX_STRING_LENGTH );
            mudstrlcat( buf, " by '", MAX_STRING_LENGTH );
            mudstrlcat( buf, aff->modifier, MAX_STRING_LENGTH );
            if( aff->bitvector != -1 )
               mudstrlcat( buf, "' and", MAX_STRING_LENGTH );
            else
               mudstrlcat( buf, "'", MAX_STRING_LENGTH );
         }
         if( aff->bitvector != -1 )
         {
            mudstrlcat( buf, " applies ", MAX_STRING_LENGTH );
            mudstrlcat( buf, a_flags[aff->bitvector], MAX_STRING_LENGTH );
         }
         if( aff->duration[0] != '\0' && aff->duration[0] != '0' )
         {
            mudstrlcat( buf, " for '", MAX_STRING_LENGTH );
            mudstrlcat( buf, aff->duration, MAX_STRING_LENGTH );
            mudstrlcat( buf, "' rounds", MAX_STRING_LENGTH );
         }
         if( aff->location >= REVERSE_APPLY )
            mudstrlcat( buf, " (affects caster only)", MAX_STRING_LENGTH );
         mudstrlcat( buf, "\r\n", MAX_STRING_LENGTH );
         send_to_char( buf, ch );

         if( !aff->next )
            send_to_char( "\r\n", ch );
      }

      if( skill->hit_char && skill->hit_char[0] != '\0' )
         ch_printf( ch, "Hitchar   : %s\r\n", skill->hit_char );
      if( skill->hit_vict && skill->hit_vict[0] != '\0' )
         ch_printf( ch, "Hitvict   : %s\r\n", skill->hit_vict );
      if( skill->hit_room && skill->hit_room[0] != '\0' )
         ch_printf( ch, "Hitroom   : %s\r\n", skill->hit_room );
      if( skill->hit_dest && skill->hit_dest[0] != '\0' )
         ch_printf( ch, "Hitdest   : %s\r\n", skill->hit_dest );
      if( skill->miss_char && skill->miss_char[0] != '\0' )
         ch_printf( ch, "Misschar  : %s\r\n", skill->miss_char );
      if( skill->miss_vict && skill->miss_vict[0] != '\0' )
         ch_printf( ch, "Missvict  : %s\r\n", skill->miss_vict );
      if( skill->miss_room && skill->miss_room[0] != '\0' )
         ch_printf( ch, "Missroom  : %s\r\n", skill->miss_room );
      if( skill->die_char && skill->die_char[0] != '\0' )
         ch_printf( ch, "Diechar   : %s\r\n", skill->die_char );
      if( skill->die_vict && skill->die_vict[0] != '\0' )
         ch_printf( ch, "Dievict   : %s\r\n", skill->die_vict );
      if( skill->die_room && skill->die_room[0] != '\0' )
         ch_printf( ch, "Dieroom   : %s\r\n", skill->die_room );
      if( skill->imm_char && skill->imm_char[0] != '\0' )
         ch_printf( ch, "Immchar   : %s\r\n", skill->imm_char );
      if( skill->imm_vict && skill->imm_vict[0] != '\0' )
         ch_printf( ch, "Immvict   : %s\r\n", skill->imm_vict );
      if( skill->imm_room && skill->imm_room[0] != '\0' )
         ch_printf( ch, "Immroom   : %s\r\n", skill->imm_room );
      if( skill->type != SKILL_HERB )
      {
         send_to_char( "--------------------------[CLASS USE]--------------------------\r\n", ch );
         for( iClass = 0; iClass < MAX_PC_CLASS; iClass++ )
         {
            mudstrlcpy( buf, class_table[iClass]->who_name, MAX_STRING_LENGTH );
            snprintf( buf + 3, MAX_STRING_LENGTH - 3, ") lvl: %3d max: %2d%%", skill->skill_level[iClass], skill->skill_adept[iClass] );
            if( iClass % 3 == 2 )
               mudstrlcat( buf, "\r\n", MAX_STRING_LENGTH );
            else
               mudstrlcat( buf, "  ", MAX_STRING_LENGTH );
            send_to_char( buf, ch );
         }
/*
         send_to_char( "\r\n--------------------------[RACE USE]--------------------------\r\n", ch );
         for( iRace = 0; iRace < MAX_PC_RACE; iRace++ )
         {
            snprintf( buf, MAX_STRING_LENGTH, "%8.8s) lvl: %3d max: %2d%%",
                      race_table[iRace]->race_name, skill->race_level[iRace], skill->race_adept[iRace] );
            if( !strcmp( race_table[iRace]->race_name, "" ) )
               snprintf( buf, MAX_STRING_LENGTH, "                           " );
            if( ( iRace > 0 ) && ( iRace % 2 == 1 ) )
               mudstrlcat( buf, "\r\n", MAX_STRING_LENGTH );
            else
               mudstrlcat( buf, "  ", MAX_STRING_LENGTH );
            send_to_char( buf, ch );
         }
*/
      }
      send_to_char( "\r\n", ch );
   }
   return;
}

/*
 * Set a skill's attributes or what skills a player has.
 * High god command, with support for creating skills/spells/herbs/etc
 */
void do_sset( CHAR_DATA* ch, const char* argument)
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int value;
   int sn, i;
   bool fAll;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Syntax: sset <victim> <skill> <value>\r\n", ch );
      send_to_char( "or:     sset <victim> all     <value>\r\n", ch );
      if( get_trust( ch ) > LEVEL_SUB_IMPLEM )
      {
         send_to_char( "or:     sset save skill table\r\n", ch );
         send_to_char( "or:     sset save herb table\r\n", ch );
         send_to_char( "or:     sset create skill 'new skill'\r\n", ch );
         send_to_char( "or:     sset create herb 'new herb'\r\n", ch );
         send_to_char( "or:     sset create ability 'new ability'\r\n", ch );
      }
      if( get_trust( ch ) > LEVEL_GREATER )
      {
         send_to_char( "or:     sset <sn>     <field> <value>\r\n", ch );
         send_to_char( "\r\nField being one of:\r\n", ch );
         send_to_char( "  name code target minpos slot mana beats dammsg wearoff guild minlevel\r\n", ch );
         send_to_char( "  type damtype acttype classtype powertype seffect flag dice value difficulty\r\n", ch );
         send_to_char( "  affect rmaffect level adept hit miss die imm (char/vict/room)\r\n", ch );
         send_to_char( "  components teachers racelevel raceadept reqskill\r\n", ch );
         send_to_char( "  sector attribute\r\n", ch );
         send_to_char( "Affect having the fields: <location> <modfifier> [duration] [bitvector]\r\n", ch );
         send_to_char( "(See AFFECTTYPES for location, and AFFECTED_BY for bitvector)\r\n", ch );
      }
      send_to_char( "Skill being any skill or spell.\r\n", ch );
      return;
   }

   if( get_trust( ch ) > LEVEL_SUB_IMPLEM && !str_cmp( arg1, "save" ) && !str_cmp( argument, "table" ) )
   {
      if( !str_cmp( arg2, "skill" ) )
      {
         send_to_char( "Saving skill table...\r\n", ch );
         save_skill_table(  );
         save_classes(  );
         save_races(  );
         return;
      }
      if( !str_cmp( arg2, "herb" ) )
      {
         send_to_char( "Saving herb table...\r\n", ch );
         save_herb_table(  );
         return;
      }
   }
   if( get_trust( ch ) > LEVEL_SUB_IMPLEM
       && !str_cmp( arg1, "create" )
       && ( !str_cmp( arg2, "skill" ) || !str_cmp( arg2, "herb" ) || !str_cmp( arg2, "ability" ) ) )
   {
      struct skill_type *skill;
      short type = SKILL_UNKNOWN;

      if( !str_cmp( arg2, "herb" ) )
      {
         type = SKILL_HERB;
         if( top_herb >= MAX_HERB )
         {
            ch_printf( ch, "The current top herb is %d, which is the maximum.  "
                       "To add more herbs,\r\nMAX_HERB will have to be "
                       "raised in mud.h, and the mud recompiled.\r\n", top_herb );
            return;
         }
      }
      else if( num_skills >= MAX_SKILL )
      {
         ch_printf( ch, "The current top sn is %d, which is the maximum.  "
                    "To add more skills,\r\nMAX_SKILL will have to be "
                    "raised in mud.h, and the mud recompiled.\r\n", num_skills );
         return;
      }
      CREATE( skill, struct skill_type, 1 );
      skill->slot = 0;
      if( type == SKILL_HERB )
      {
         int max, x;

         herb_table[top_herb++] = skill;
         for( max = x = 0; x < top_herb - 1; x++ )
            if( herb_table[x] && herb_table[x]->slot > max )
               max = herb_table[x]->slot;
         skill->slot = max + 1;
      }
      else  // add the skill. See db.c for why we are adding without sorting.
         skill_table[num_skills++] = skill;
      skill->min_mana = 0;
      skill->name = str_dup( argument );
      skill->noun_damage = str_dup( "" );
      skill->msg_off = str_dup( "" );
      skill->spell_fun = spell_smaug;
      skill->type = type;
      skill->spell_sector = 0;
      skill->guild = -1;
      if( !str_cmp( arg2, "ability" ) )
         skill->type = SKILL_RACIAL;

      for( i = 0; i < MAX_PC_CLASS; i++ )
      {
         skill->skill_level[i] = LEVEL_IMMORTAL;
         skill->skill_adept[i] = 95;
      }
      for( i = 0; i < MAX_PC_RACE; i++ )
      {
         skill->race_level[i] = LEVEL_IMMORTAL;
         skill->race_adept[i] = 95;
      }

      send_to_char( "Done.\r\n", ch );
      return;
   }

   if( arg1[0] == 'h' )
      sn = atoi( arg1 + 1 );
   else
      sn = atoi( arg1 );
   if( get_trust( ch ) > LEVEL_GREATER
       && ( ( arg1[0] == 'h' && is_number( arg1 + 1 ) && ( sn = atoi( arg1 + 1 ) ) >= 0 )
            || ( is_number( arg1 ) && ( sn = atoi( arg1 ) ) >= 0 ) ) )
   {
      struct skill_type *skill;

      if( arg1[0] == 'h' )
      {
         if( sn >= top_herb )
         {
            send_to_char( "Herb number out of range.\r\n", ch );
            return;
         }
         skill = herb_table[sn];
      }
      else
      {
         if( ( skill = get_skilltype( sn ) ) == NULL )
         {
            send_to_char( "Skill number out of range.\r\n", ch );
            return;
         }
         sn %= 1000;
      }

      if( !str_cmp( arg2, "difficulty" ) )
      {
         skill->difficulty = atoi( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "participants" ) )
      {
         skill->participants = atoi( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "damtype" ) )
      {
         int x = get_sdamage( argument );

         if( x == -1 )
            send_to_char( "Not a spell damage type.\r\n", ch );
         else
         {
            SET_SDAM( skill, x );
            send_to_char( "Ok.\r\n", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "acttype" ) )
      {
         int x = get_saction( argument );

         if( x == -1 )
            send_to_char( "Not a spell action type.\r\n", ch );
         else
         {
            SET_SACT( skill, x );
            send_to_char( "Ok.\r\n", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "classtype" ) )
      {
         int x = get_sclass( argument );

         if( x == -1 )
            send_to_char( "Not a spell class type.\r\n", ch );
         else
         {
            SET_SCLA( skill, x );
            send_to_char( "Ok.\r\n", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "powertype" ) )
      {
         int x = get_spower( argument );

         if( x == -1 )
            send_to_char( "Not a spell power type.\r\n", ch );
         else
         {
            SET_SPOW( skill, x );
            send_to_char( "Ok.\r\n", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "seffect" ) )
      {
         int x = get_ssave_effect( argument );

         if( x == -1 )
            send_to_char( "Not a spell save effect type.\r\n", ch );
         else
         {
            SET_SSAV( skill, x );
            send_to_char( "Ok.\r\n", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "flag" ) )
      {
         int x = get_sflag( argument );

         if( x == -1 )
            send_to_char( "Not a spell flag.\r\n", ch );
         else
         {
            TOGGLE_BIT( skill->flags, 1 << x );
            send_to_char( "Ok.\r\n", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "saves" ) )
      {
         int x = get_ssave( argument );

         if( x == -1 )
            send_to_char( "Not a saving type.\r\n", ch );
         else
         {
            skill->saves = x;
            send_to_char( "Ok.\r\n", ch );
         }
         return;
      }

      if( !str_cmp( arg2, "code" ) )
      {
         SPELL_FUN *spellfun;
         DO_FUN *dofun;

         if( !str_prefix( "do_", argument ) && ( dofun = skill_function( argument ) ) != skill_notfound )
         {
            skill->skill_fun = dofun;
            skill->spell_fun = NULL;
            DISPOSE( skill->skill_fun_name );
            skill->skill_fun_name = str_dup( argument );
         }
         else if( ( spellfun = spell_function( argument ) ) != spell_notfound )
         {
            skill->spell_fun = spellfun;
            skill->skill_fun = NULL;
            DISPOSE( skill->skill_fun_name );
            skill->spell_fun_name = str_dup( argument );
         }
         else if( validate_spec_fun( argument ) )
         {
            send_to_char( "Cannot use a spec_fun for skills or spells.\r\n", ch );
            return;
         }
         else
         {
            send_to_char( "Not a spell or skill.\r\n", ch );
            return;
         }
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "target" ) )
      {
         int x = get_starget( argument );

         if( x == -1 )
            send_to_char( "Not a valid target type.\r\n", ch );
         else
         {
            skill->target = x;
            send_to_char( "Ok.\r\n", ch );
         }
         return;
      }
      if( !str_cmp( arg2, "minpos" ) )
      {
         skill->minimum_position = URANGE( POS_DEAD, atoi( argument ), POS_DRAG );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "minlevel" ) )
      {
         skill->min_level = URANGE( 1, atoi( argument ), MAX_LEVEL );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "sector" ) )
      {
         char tmp_arg[MAX_STRING_LENGTH];

         while( argument[0] != '\0' )
         {
            argument = one_argument( argument, tmp_arg );
            value = get_secflag( tmp_arg );
            if( value < 0 || value > MAX_SECFLAG )
               ch_printf( ch, "Unknown flag: %s\r\n", tmp_arg );
            else
               TOGGLE_BIT( skill->spell_sector, ( 1 << value ) );
         }
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "slot" ) )
      {
         skill->slot = URANGE( 0, atoi( argument ), 30000 );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "mana" ) )
      {
         skill->min_mana = URANGE( 0, atoi( argument ), 2000 );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "beats" ) )
      {
         skill->beats = URANGE( 0, atoi( argument ), 120 );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "range" ) )
      {
         skill->range = URANGE( 0, atoi( argument ), 20 );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "attribute" ) )
      {
         skill->attribute = URANGE( 0, atoi( argument ), 6 );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "guild" ) )
      {
         skill->guild = atoi( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "value" ) )
      {
         skill->value = atoi( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "type" ) )
      {
         skill->type = get_skill( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      if( !str_cmp( arg2, "rmaffect" ) )
      {
         SMAUG_AFF *aff, *aff_next;
         int num = atoi( argument );
         int cnt = 0;

         if( !skill->first_affect )
         {
            send_to_char( "This spell has no special affects to remove.\r\n", ch );
            return;
         }
         for( aff = skill->first_affect; aff; aff = aff_next )
         {
            aff_next = aff->next;

            if( ++cnt == num )
            {
               UNLINK( aff, skill->first_affect, skill->last_affect, next, prev );
               DISPOSE( aff->duration );
               DISPOSE( aff->modifier );
               DISPOSE( aff );
               send_to_char( "Removed.\r\n", ch );
               return;
            }
         }
         send_to_char( "Not found.\r\n", ch );
         return;
      }
      /*
       * affect <location> <modifier> <duration> <bitvector>
       */
      if( !str_cmp( arg2, "affect" ) )
      {
         char location[MAX_INPUT_LENGTH];
         char modifier[MAX_INPUT_LENGTH];
         char duration[MAX_INPUT_LENGTH];
         int loc, bit, tmpbit;
         SMAUG_AFF *aff;

         argument = one_argument( argument, location );
         argument = one_argument( argument, modifier );
         argument = one_argument( argument, duration );

         if( location[0] == '!' )
            loc = get_atype( location + 1 ) + REVERSE_APPLY;
         else
            loc = get_atype( location );
         if( ( loc % REVERSE_APPLY ) < 0 || ( loc % REVERSE_APPLY ) >= MAX_APPLY_TYPE )
         {
            send_to_char( "Unknown affect location.  See AFFECTTYPES.\r\n", ch );
            return;
         }
         bit = -1;
         if( argument[0] != '\0' )
         {
            if( ( tmpbit = get_aflag( argument ) ) == -1 )
               ch_printf( ch, "Unknown bitvector: %s.  See AFFECTED_BY\r\n", argument );
            else
               bit = tmpbit;
         }
         CREATE( aff, SMAUG_AFF, 1 );
         if( !str_cmp( duration, "0" ) )
            duration[0] = '\0';
         if( !str_cmp( modifier, "0" ) )
            modifier[0] = '\0';
         aff->duration = str_dup( duration );
         aff->location = loc;
         if( loc == APPLY_AFFECT )
         {
            int modval = get_aflag( modifier );

            if( modval < 0 )
               modval = 0;
            snprintf( modifier, MAX_INPUT_LENGTH, "%d", modval );
         }
         else if( loc == APPLY_RESISTANT || loc == APPLY_IMMUNE || loc == APPLY_SUSCEPTIBLE )
         {
            int modval = get_risflag( modifier );
            if( modval < 0 )
               modval = 0;
            snprintf( modifier, MAX_INPUT_LENGTH, "%d", modval );
         }
         aff->modifier = str_dup( modifier );
         aff->bitvector = bit;
         LINK( aff, skill->first_affect, skill->last_affect, next, prev );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "level" ) )
      {
         char arg3[MAX_INPUT_LENGTH];
         int Class;

         argument = one_argument( argument, arg3 );
         Class = atoi( arg3 );
         if( Class >= MAX_PC_CLASS || Class < 0 )
            send_to_char( "Not a valid class.\r\n", ch );
         else
            skill->skill_level[Class] = URANGE( 0, atoi( argument ), MAX_LEVEL );
         return;
      }

      if( !str_cmp( arg2, "racelevel" ) )
      {
         char arg3[MAX_INPUT_LENGTH];
         int race;

         argument = one_argument( argument, arg3 );
         race = atoi( arg3 );
         if( race >= MAX_PC_RACE || race < 0 )
            send_to_char( "Not a valid race.\r\n", ch );
         else
            skill->race_level[race] = URANGE( 0, atoi( argument ), MAX_LEVEL );
         return;
      }

      if( !str_cmp( arg2, "adept" ) )
      {
         char arg3[MAX_INPUT_LENGTH];
         int Class;

         argument = one_argument( argument, arg3 );
         Class = atoi( arg3 );
         if( Class >= MAX_PC_CLASS || Class < 0 )
            send_to_char( "Not a valid class.\r\n", ch );
         else
            skill->skill_adept[Class] = URANGE( 0, atoi( argument ), 100 );
         return;
      }

      if( !str_cmp( arg2, "raceadept" ) )
      {
         char arg3[MAX_INPUT_LENGTH];
         int race;

         argument = one_argument( argument, arg3 );
         race = atoi( arg3 );
         if( race >= MAX_PC_RACE || race < 0 )
            send_to_char( "Not a valid race.\r\n", ch );
         else
            skill->race_adept[race] = URANGE( 0, atoi( argument ), 100 );
         return;
      }

      if( !str_cmp( arg2, "reqskill" ) )
	  {
	     int x;

		 if( is_number( argument ) )
		    x = atoi( argument );
		 else
	     {
		    if( ( x = skill_lookup( argument ) ) < 0 )
			{
			   ch_printf( ch, "No skill called (%s) to use.\r\n", argument );
				return;
			}
		 }
		 if( x != -1 && !IS_VALID_SN( x ) )
		 {
		 	send_to_char( "You can't set it to that.\r\n", ch );
			return;
		 }
		 skill->req_skill = URANGE( -1, x, num_skills );
		 if( skill->req_skill == -1 )
		    send_to_char( "Reqskill set to -1(Nothing).\r\n", ch );
		 else
		    ch_printf( ch, "Reqskill set to %d(%s).\r\n", skill->req_skill, skill_table[ skill->req_skill ]->name );
		 return;
	  }

      if( !str_cmp( arg2, "name" ) )
      {
         DISPOSE( skill->name );
         skill->name = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "dammsg" ) )
      {
         DISPOSE( skill->noun_damage );
         if( !str_cmp( argument, "clear" ) )
            skill->noun_damage = str_dup( "" );
         else
            skill->noun_damage = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "wearoff" ) )
      {
         DISPOSE( skill->msg_off );
         if( str_cmp( argument, "clear" ) )
            skill->msg_off = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "hitchar" ) )
      {
         if( skill->hit_char )
            DISPOSE( skill->hit_char );
         if( str_cmp( argument, "clear" ) )
            skill->hit_char = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "hitvict" ) )
      {
         if( skill->hit_vict )
            DISPOSE( skill->hit_vict );
         if( str_cmp( argument, "clear" ) )
            skill->hit_vict = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "hitroom" ) )
      {
         if( skill->hit_room )
            DISPOSE( skill->hit_room );
         if( str_cmp( argument, "clear" ) )
            skill->hit_room = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "hitdest" ) )
      {
         if( skill->hit_dest )
            DISPOSE( skill->hit_dest );
         if( str_cmp( argument, "clear" ) )
            skill->hit_dest = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "misschar" ) )
      {
         if( skill->miss_char )
            DISPOSE( skill->miss_char );
         if( str_cmp( argument, "clear" ) )
            skill->miss_char = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "missvict" ) )
      {
         if( skill->miss_vict )
            DISPOSE( skill->miss_vict );
         if( str_cmp( argument, "clear" ) )
            skill->miss_vict = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "missroom" ) )
      {
         if( skill->miss_room )
            DISPOSE( skill->miss_room );
         if( str_cmp( argument, "clear" ) )
            skill->miss_room = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "diechar" ) )
      {
         if( skill->die_char )
            DISPOSE( skill->die_char );
         if( str_cmp( argument, "clear" ) )
            skill->die_char = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "dievict" ) )
      {
         if( skill->die_vict )
            DISPOSE( skill->die_vict );
         if( str_cmp( argument, "clear" ) )
            skill->die_vict = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "dieroom" ) )
      {
         if( skill->die_room )
            DISPOSE( skill->die_room );
         if( str_cmp( argument, "clear" ) )
            skill->die_room = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "immchar" ) )
      {
         if( skill->imm_char )
            DISPOSE( skill->imm_char );
         if( str_cmp( argument, "clear" ) )
            skill->imm_char = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "immvict" ) )
      {
         if( skill->imm_vict )
            DISPOSE( skill->imm_vict );
         if( str_cmp( argument, "clear" ) )
            skill->imm_vict = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "immroom" ) )
      {
         if( skill->imm_room )
            DISPOSE( skill->imm_room );
         if( str_cmp( argument, "clear" ) )
            skill->imm_room = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "dice" ) )
      {
         if( skill->dice )
            DISPOSE( skill->dice );
         if( str_cmp( argument, "clear" ) )
            skill->dice = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "components" ) )
      {
         if( skill->components )
            DISPOSE( skill->components );
         if( str_cmp( argument, "clear" ) )
            skill->components = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }

      if( !str_cmp( arg2, "teachers" ) )
      {
         if( skill->teachers )
            DISPOSE( skill->teachers );
         if( str_cmp( argument, "clear" ) )
            skill->teachers = str_dup( argument );
         send_to_char( "Ok.\r\n", ch );
         return;
      }
      do_sset( ch, "" );
      return;
   }

   if( !( victim = get_char_world( ch, arg1 ) ) )
   {
      if( ( sn = skill_lookup( arg1 ) ) >= 0 )
      {
         snprintf( arg1, MAX_INPUT_LENGTH, "%d %s %s", sn, arg2, argument );
         do_sset( ch, arg1 );
      }
      else
         send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( IS_NPC( victim ) )
   {
      send_to_char( "Not on NPC's.\r\n", ch );
      return;
   }

   fAll = !str_cmp( arg2, "all" );
   sn = 0;
   if( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 )
   {
      send_to_char( "No such skill or spell.\r\n", ch );
      return;
   }

   /*
    * Snarf the value.
    */
   if( !is_number( argument ) )
   {
      send_to_char( "Value must be numeric.\r\n", ch );
      return;
   }

   value = atoi( argument );
   if( value < 0 || value > 100 )
   {
      send_to_char( "Value range is 0 to 100.\r\n", ch );
      return;
   }

   if( fAll )
   {
      for( sn = 0; sn < num_skills; ++sn )
      {
         /*
          * Fix by Narn to prevent ssetting skills the player shouldn't have.
          */
         if( victim->level >= skill_table[sn]->skill_level[victim->Class]
             || victim->level >= skill_table[sn]->race_level[victim->race] )
         {
            if( value == 100 && !IS_IMMORTAL( victim ) )
               victim->pcdata->learned[sn] = RANK_PARAMOUNT;
            else
               victim->pcdata->learned[sn] = value;
         }
      }
   }
   else
      victim->pcdata->learned[sn] = value;

   return;
}

/*
 * Racial Skills Learning -- Kayle 7-8-07
 */
void ability_learn_from_success( CHAR_DATA * ch, int sn )
{
   int adept, gain, sklvl, learn, percent, schance;

   if( IS_NPC( ch ) || ch->pcdata->learned[sn] <= 0 )
      return;

   adept = skill_table[sn]->race_adept[ch->race];

   sklvl = skill_table[sn]->race_level[ch->race];

   if( sklvl == 0 )
      sklvl = ch->level;
   if( ch->pcdata->learned[sn] < adept )
   {
      schance = ch->pcdata->learned[sn] + ( 5 * skill_table[sn]->difficulty );
      percent = number_percent(  );
      if( percent >= schance )
         learn = 2;
      else if( schance - percent > 25 )
         return;
      else
         learn = 1;
      ch->pcdata->learned[sn] = UMIN( adept, ch->pcdata->learned[sn] + learn );
      if( ch->pcdata->learned[sn] == adept ) /* fully learned! */
      {
         gain = 1000 * sklvl;
         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You are now an adept of %s!  You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
      }
      else
      {
         gain = 20 * sklvl;
         if( !ch->fighting && sn != gsn_hide && sn != gsn_sneak )
         {
            set_char_color( AT_WHITE, ch );
            ch_printf( ch, "You gain %d experience points from your success!\r\n", gain );
         }
      }
      gain_exp( ch, gain );
   }
}

void learn_from_success( CHAR_DATA * ch, int sn )
{
   int gain, sklvl, learn, percent, schance;

   if( IS_NPC( ch ) )
      return;
   sklvl = skill_table[sn]->skill_level[ch->Class];
   if( sklvl == 0 )
      sklvl = ch->level;
   if( ch->pcdata->learned[sn] < RANK_PARAMOUNT )
   {
      schance = ch->pcdata->learned[sn] + ( 5 * skill_table[sn]->difficulty );
      percent = number_percent(  );
      if( schance - percent > 25 )
         return;
      else
      {
         learn = 1;
         ch_printf( ch, "You get better at %s.\r\n", skill_table[sn]->name );
      }
      ch->pcdata->learned[sn] = UMIN( RANK_PARAMOUNT, ch->pcdata->learned[sn] + learn );
      if( ch->pcdata->learned[sn] == RANK_DISCIPLE )
      {
         gain = 100 * sklvl;
         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You are now a Disciple of %s!  You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
         gain_exp( ch, gain );
      }
      if( ch->pcdata->learned[sn] == RANK_PRACTITIONER )
      {
         gain = 100 * sklvl;
         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You are now a Practitioner of %s!  You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
         gain_exp( ch, gain );
      }
      if( ch->pcdata->learned[sn] == RANK_ARTISAN )
      {
         gain = 100 * sklvl;
         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You are now an Artisan of %s!  You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
         gain_exp( ch, gain );
      }
      if( ch->pcdata->learned[sn] == RANK_ADEPT )
      {
         gain = 100 * sklvl;
         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You are now an Adept of %s!  You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
         gain_exp( ch, gain );
      }
      if( ch->pcdata->learned[sn] == RANK_PROFESSIONAL )
      {
         gain = 100 * sklvl;
         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You are now a Professional of %s!  You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
         gain_exp( ch, gain );
      }

      if( ch->pcdata->learned[sn] == RANK_SAVANT )
      {
         gain = 100 * sklvl;
         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You are now a Savant of %s!  You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
         gain_exp( ch, gain );
      }
      if( ch->pcdata->learned[sn] == RANK_EXPERT )
      {
         gain = 100 * sklvl;
         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You are now an Expert of %s!  You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
         gain_exp( ch, gain );
      }
      if( ch->pcdata->learned[sn] == RANK_VIRTUOSO )
      {
         gain = 100 * sklvl;
         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You are now a Virtuoso of %s!  You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
         gain_exp( ch, gain );
      }
      if( ch->pcdata->learned[sn] == RANK_PARAMOUNT ) /* fully learned! */
      {
         gain = 1000 * sklvl;
         set_char_color( AT_WHITE, ch );
         ch_printf( ch, "You are now a Paramount of %s!  You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
         gain_exp( ch, gain );
      }
   }
}

void learn_from_failure( CHAR_DATA * ch, int sn )
{
   int schance;

   if( IS_NPC( ch ) || ch->pcdata->learned[sn] <= 0 )
      return;
   schance = ch->pcdata->learned[sn] + ( 5 * skill_table[sn]->difficulty );
   if( schance - number_percent(  ) > 25 )
      return;
/* No gain from failure (for now) -Danny
   if( ch->pcdata->learned[sn] < ( RANK_PARAMOUNT - 1 ) )
      ch->pcdata->learned[sn] = UMIN( RANK_PARAMOUNT, ch->pcdata->learned[sn] + 1 );
*/
}

void do_gouge( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   short dam;
   int schance;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !can_use_skill( ch, 0, gsn_gouge ) )
   {
      send_to_char( "You do not yet know of this skill.\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't get close enough while mounted.\r\n", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   schance = ( ( get_curr_dex( victim ) - get_curr_dex( ch ) ) * 10 ) + 10;
   if( !IS_NPC( ch ) && !IS_NPC( victim ) )
      schance += sysdata.gouge_plr_vs_plr;
   if( victim->fighting && victim->fighting->who != ch )
      schance += sysdata.gouge_nontank;
   dam = number_range( 5, ch->level );
   global_retcode = damage( ch, victim, dam, gsn_gouge );
   if( global_retcode == rNONE )
   {
      if( !IS_AFFECTED( victim, AFF_BLIND ) )
      {
         af.type = gsn_blindness;
         af.location = APPLY_HITROLL;
         af.modifier = -6;
         if( !IS_NPC( victim ) && !IS_NPC( ch ) )
            af.duration = ( ch->level + 10 ) / get_curr_con( victim );
         else
            af.duration = 3 + ( ch->level / 15 );
         af.bitvector = meb( AFF_BLIND );
         affect_to_char( victim, &af );
         act( AT_SKILL, "You can't see a thing!", victim, NULL, NULL, TO_CHAR );
      }
      WAIT_STATE( ch, PULSE_VIOLENCE );
      if( !IS_NPC( ch ) && !IS_NPC( victim ) )
      {
         if( number_bits( 1 ) == 0 )
         {
            ch_printf( ch, "%s looks momentarily dazed.\r\n", victim->name );
            send_to_char( "You are momentarily dazed ...\r\n", victim );
            WAIT_STATE( victim, PULSE_VIOLENCE );
         }
      }
      else
         WAIT_STATE( victim, PULSE_VIOLENCE );
         /*
          * Taken out by request - put back in by Thoric
          * * This is how it was designed.  You'd be a tad stunned
          * * if someone gouged you in the eye.
          * * Mildly modified by Blodkai, Feb 1998 at request of
          * * of pkill Conclave (peaceful use remains the same)
          */
   }
   else if( global_retcode == rVICT_DIED )
   {
      act( AT_BLOOD, "Your fingers plunge into your victim's brain, causing immediate death!", ch, NULL, NULL, TO_CHAR );
   }
   if( global_retcode != rCHAR_DIED && global_retcode != rBOTH_DIED )
      learn_from_success( ch, gsn_gouge );
   return;
}

void do_detrap( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   OBJ_DATA *trap;
   int percent;
   bool found = FALSE;

   switch ( ch->substate )
   {
      default:
         if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg );
         if( !can_use_skill( ch, 0, gsn_detrap ) )
         {
            send_to_char( "You do not yet know of this skill.\r\n", ch );
            return;
         }
         if( arg[0] == '\0' )
         {
            send_to_char( "Detrap what?\r\n", ch );
            return;
         }
         if( ms_find_obj( ch ) )
            return;
         found = FALSE;
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\r\n", ch );
            return;
         }
         if( !ch->in_room->first_content )
         {
            send_to_char( "You can't find that here.\r\n", ch );
            return;
         }
         for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
         {
            if( can_see_obj( ch, obj ) && nifty_is_name( arg, obj->name ) )
            {
               found = TRUE;
               break;
            }
         }
         if( !found )
         {
            send_to_char( "You can't find that here.\r\n", ch );
            return;
         }
         act( AT_ACTION, "You carefully begin your attempt to remove a trap from $p...", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n carefully attempts to remove a trap from $p...", ch, obj, NULL, TO_ROOM );
         ch->alloc_ptr = str_dup( obj->name );
         add_timer( ch, TIMER_DO_FUN, 3, do_detrap, 1 );
/*	    WAIT_STATE( ch, skill_table[gsn_detrap]->beats ); */
         return;
      case 1:
         if( !ch->alloc_ptr )
         {
            send_to_char( "Your detrapping was interrupted!\r\n", ch );
            bug( "%s", "do_detrap: ch->alloc_ptr NULL!" );
            return;
         }
         mudstrlcpy( arg, ch->alloc_ptr, MAX_INPUT_LENGTH );
         DISPOSE( ch->alloc_ptr );
         ch->alloc_ptr = NULL;
         ch->substate = SUB_NONE;
         break;
      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You carefully stop what you were doing.\r\n", ch );
         return;
   }

   if( !ch->in_room->first_content )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return;
   }
   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
   {
      if( can_see_obj( ch, obj ) && nifty_is_name( arg, obj->name ) )
      {
         found = TRUE;
         break;
      }
   }
   if( !found )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return;
   }
   if( ( trap = get_trap( obj ) ) == NULL )
   {
      send_to_char( "You find no trap on that.\r\n", ch );
      return;
   }

   percent = number_percent(  ) - ( get_curr_lck( ch ) );

   separate_obj( obj );
   if( !can_use_skill( ch, percent, gsn_detrap ) )
   {
      send_to_char( "Ooops!\r\n", ch );
      spring_trap( ch, trap );
      learn_from_failure( ch, gsn_detrap );
      return;
   }

   extract_obj( trap );

   send_to_char( "You successfully remove a trap.\r\n", ch );
   learn_from_success( ch, gsn_detrap );
   return;
}

void do_dig( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   OBJ_DATA *startobj;
   bool found, shovel;
   EXIT_DATA *pexit;

   switch ( ch->substate )
   {
      default:
         if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\r\n", ch );
            return;
         }
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\r\n", ch );
            return;
         }
         one_argument( argument, arg );
         if( arg[0] != '\0' )
         {
            if( ( pexit = find_door( ch, arg, TRUE ) ) == NULL && get_dir( arg ) == -1 )
            {
               send_to_char( "What direction is that?\r\n", ch );
               return;
            }
            if( pexit )
            {
               if( !IS_SET( pexit->exit_info, EX_DIG ) && !IS_SET( pexit->exit_info, EX_CLOSED ) )
               {
                  send_to_char( "There is no need to dig out that exit.\r\n", ch );
                  return;
               }
            }
         }
         else
         {
            switch ( ch->in_room->sector_type )
            {
               case SECT_CITY:
               case SECT_INSIDE:
                  send_to_char( "The floor is too hard to dig through.\r\n", ch );
                  return;
               case SECT_WATER_SWIM:
               case SECT_WATER_NOSWIM:
               case SECT_OCEAN:
               case SECT_UNDERWATER:
                  send_to_char( "You cannot dig here.\r\n", ch );
                  return;
               case SECT_AIR:
                  send_to_char( "What?  In the air?!\r\n", ch );
                  return;
            }
         }
         add_timer( ch, TIMER_DO_FUN, UMIN( skill_table[gsn_dig]->beats / 10, 3 ), do_dig, 1 );
         ch->alloc_ptr = str_dup( arg );
         send_to_char( "You begin digging...\r\n", ch );
         act( AT_PLAIN, "$n begins digging...", ch, NULL, NULL, TO_ROOM );
         return;

      case 1:
         if( !ch->alloc_ptr )
         {
            send_to_char( "Your digging was interrupted!\r\n", ch );
            act( AT_PLAIN, "$n's digging was interrupted!", ch, NULL, NULL, TO_ROOM );
            bug( "%s", "do_dig: alloc_ptr NULL" );
            return;
         }
         mudstrlcpy( arg, ch->alloc_ptr, MAX_INPUT_LENGTH );
         DISPOSE( ch->alloc_ptr );
         break;

      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You stop digging...\r\n", ch );
         act( AT_PLAIN, "$n stops digging...", ch, NULL, NULL, TO_ROOM );
         return;
   }

   ch->substate = SUB_NONE;

   /*
    * not having a shovel makes it harder to succeed
    */
   shovel = FALSE;
   for( obj = ch->first_carrying; obj; obj = obj->next_content )
      if( obj->item_type == ITEM_SHOVEL )
      {
         shovel = TRUE;
         break;
      }

   /*
    * dig out an EX_DIG exit...
    */
   if( arg[0] != '\0' )
   {
      if( ( pexit = find_door( ch, arg, TRUE ) ) != NULL
          && IS_SET( pexit->exit_info, EX_DIG ) && IS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         /*
          * 4 times harder to dig open a passage without a shovel
          */

         if( ch->pcdata->learned[gsn_dig] > 0 )
         {
            REMOVE_BIT( pexit->exit_info, EX_CLOSED );
            send_to_char( "You dig open a passageway!\r\n", ch );
            act( AT_PLAIN, "$n digs open a passageway!", ch, NULL, NULL, TO_ROOM );
            learn_from_success( ch, gsn_dig );
            return;
         }
      }
      learn_from_failure( ch, gsn_dig );
      send_to_char( "Your dig did not discover any exit...\r\n", ch );
      act( AT_PLAIN, "$n's dig did not discover any exit...", ch, NULL, NULL, TO_ROOM );
      return;
   }

   startobj = ch->in_room->first_content;
   found = FALSE;

   for( obj = startobj; obj; obj = obj->next_content )
   {
      /*
       * twice as hard to find something without a shovel
       */
      if( IS_OBJ_STAT( obj, ITEM_BURIED ) )
      {
         found = TRUE;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "Your dig uncovered nothing.\r\n", ch );
      act( AT_PLAIN, "$n's dig uncovered nothing.", ch, NULL, NULL, TO_ROOM );
      learn_from_failure( ch, gsn_dig );
      return;
   }

   separate_obj( obj );
   xREMOVE_BIT( obj->extra_flags, ITEM_BURIED );
   act( AT_SKILL, "Your dig uncovered $p!", ch, obj, NULL, TO_CHAR );
   act( AT_SKILL, "$n's dig uncovered $p!", ch, obj, NULL, TO_ROOM );
   learn_from_success( ch, gsn_dig );
   if( obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC )
      adjust_favor( ch, 14, 1 );

   return;
}


void do_search( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   OBJ_DATA *container;
   OBJ_DATA *startobj;
   int percent, door;

   door = -1;
   switch ( ch->substate )
   {
      default:
         if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\r\n", ch );
            return;
         }
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\r\n", ch );
            return;
         }
         argument = one_argument( argument, arg );
         if( arg[0] != '\0' && ( door = get_door( arg ) ) == -1 )
         {
            container = get_obj_here( ch, arg );
            if( !container )
            {
               send_to_char( "You can't find that here.\r\n", ch );
               return;
            }
            if( container->item_type != ITEM_CONTAINER )
            {
               send_to_char( "You can't search in that!\r\n", ch );
               return;
            }
            if( IS_SET( container->value[1], CONT_CLOSED ) )
            {
               send_to_char( "It is closed.\r\n", ch );
               return;
            }
         }
         add_timer( ch, TIMER_DO_FUN, UMIN( 6 / 10, 3 ), do_search, 1 );
         send_to_char( "You begin your search...\r\n", ch );
         ch->alloc_ptr = str_dup( arg );
         return;

      case 1:
         if( !ch->alloc_ptr )
         {
            send_to_char( "Your search was interrupted!\r\n", ch );
            bug( "%s", "do_search: alloc_ptr NULL" );
            return;
         }
         mudstrlcpy( arg, ch->alloc_ptr, MAX_INPUT_LENGTH );
         DISPOSE( ch->alloc_ptr );
         break;
      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You stop your search...\r\n", ch );
         return;
   }
   ch->substate = SUB_NONE;
   if( arg[0] == '\0' )
      startobj = ch->in_room->first_content;
   else
   {
      if( ( door = get_door( arg ) ) != -1 )
         startobj = NULL;
      else
      {
         container = get_obj_here( ch, arg );
         if( !container )
         {
            send_to_char( "You can't find that here.\r\n", ch );
            return;
         }
         startobj = container->first_content;
      }
   }

   if( ( !startobj && door == -1 ) || IS_NPC( ch ) )
   {
      send_to_char( "You find nothing.\r\n", ch );
      return;
   }

   percent = number_percent(  ) + number_percent(  ) - ( ch->level / 10 );

   if( door != -1 )
   {
      EXIT_DATA *pexit;

      if( ( pexit = get_exit( ch->in_room, door ) ) != NULL
          && IS_SET( pexit->exit_info, EX_SECRET )
          && IS_SET( pexit->exit_info, EX_xSEARCHABLE ) )
      {
         act( AT_SKILL, "Your search reveals the $d!", ch, NULL, pexit->keyword, TO_CHAR );
         act( AT_SKILL, "$n finds the $d!", ch, NULL, pexit->keyword, TO_ROOM );
         REMOVE_BIT( pexit->exit_info, EX_SECRET );
         learn_from_success( ch, gsn_search );
         return;
      }
   }
   else
      for( obj = startobj; obj; obj = obj->next_content )
      {
         if( IS_OBJ_STAT( obj, ITEM_HIDDEN ) )
         {
            separate_obj( obj );
            xREMOVE_BIT( obj->extra_flags, ITEM_HIDDEN );
            act( AT_SKILL, "Your search reveals $p!", ch, obj, NULL, TO_CHAR );
            act( AT_SKILL, "$n finds $p!", ch, obj, NULL, TO_ROOM );
            learn_from_success( ch, gsn_search );
            return;
         }
      }

   send_to_char( "You find nothing.\r\n", ch );
   return;
}

void do_steal( CHAR_DATA* ch, const char* argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CHAR_DATA *victim, *mst;
   OBJ_DATA *obj;
   int percent;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Steal what from whom?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( victim = get_char_room( ch, arg2 ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "That's pointless.\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "A magical force interrupts you.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && !IS_NPC( victim ) )
   {
      set_char_color( AT_IMMORT, ch );
      send_to_char( "The gods forbid theft between players.\r\n", ch );
      return;
   }

   if( xIS_SET( victim->act, ACT_PACIFIST ) )   /* Gorog */
   {
      send_to_char( "They are a pacifist - Shame on you!\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_steal]->beats );
   percent = number_percent(  ) + ( IS_AWAKE( victim ) ? 10 : -50 )
      - ( get_curr_lck( ch ) ) + ( get_curr_lck( victim ) );

   /*
    * Changed the level check, made it 10 levels instead of five and made the
    * victim not attack in the case of a too high level difference.  This is
    * to allow mobprogs where the mob steals eq without having to put level
    * checks into the progs.  Also gave the mobs a 10% chance of failure.
    */
   if( ch->level + 10 < victim->level )
   {
      send_to_char( "You really don't want to try that!\r\n", ch );
      return;
   }

   if( victim->position == POS_FIGHTING )
   {
      /*
       * Failure.
       */
      send_to_char( "Oops...\r\n", ch );
      act( AT_ACTION, "$n tried to steal from you!\r\n", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$n tried to steal from $N.\r\n", ch, NULL, victim, TO_NOTVICT );

      snprintf( buf, MAX_STRING_LENGTH, "%s is a bloody thief!", ch->name );
      do_yell( victim, buf );

      learn_from_failure( ch, gsn_steal );
      if( !IS_NPC( ch ) )
      {
         if( legal_loot( ch, victim ) )
         {
            global_retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
         }
         else
         {
            /*
             * log_string( buf );
             */
            if( IS_NPC( ch ) )
            {
               if( ( mst = ch->master ) == NULL )
                  return;
            }
            else
               mst = ch;
            if( IS_NPC( mst ) )
               return;
            if( !xIS_SET( mst->act, PLR_THIEF ) )
            {
               xSET_BIT( mst->act, PLR_THIEF );
               set_char_color( AT_WHITE, ch );
               send_to_char( "A strange feeling grows deep inside you, and a tingle goes up your spine...\r\n", ch );
               set_char_color( AT_IMMORT, ch );
               send_to_char( "A deep voice booms inside your head, 'Thou shall now be known as a lowly thief!'\r\n", ch );
               set_char_color( AT_WHITE, ch );
               send_to_char( "You feel as if your soul has been revealed for all to see.\r\n", ch );
               save_char_obj( mst );
            }
         }
      }
      return;
   }

   if( !str_cmp( arg1, "coin" ) || !str_cmp( arg1, "coins" ) || !str_cmp( arg1, "gold" ) )
   {
      int amount;

      amount = ( int )( victim->gold * number_range( 1, 10 ) / 100 );
      if( amount <= 0 )
      {
         send_to_char( "You couldn't get any gold.\r\n", ch );
         learn_from_failure( ch, gsn_steal );
         return;
      }

      ch->gold += amount;
      victim->gold -= amount;
      ch_printf( ch, "Aha!  You got %d gold coins.\r\n", amount );
      learn_from_success( ch, gsn_steal );
      return;
   }

   if( ( obj = get_obj_carry( victim, arg1 ) ) == NULL )
   {
      send_to_char( "You can't seem to find it.\r\n", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   if( !can_drop_obj( ch, obj )
       || IS_OBJ_STAT( obj, ITEM_INVENTORY ) || IS_OBJ_STAT( obj, ITEM_PROTOTYPE ) || obj->level > ch->level )
   {
      send_to_char( "You can't manage to pry it away.\r\n", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   if( ch->carry_number + ( get_obj_number( obj ) / obj->count ) > can_carry_n( ch ) )
   {
      send_to_char( "You have your hands full.\r\n", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   if( ch->carry_weight + ( get_obj_weight( obj ) / obj->count ) > can_carry_w( ch ) )
   {
      send_to_char( "You can't carry that much weight.\r\n", ch );
      learn_from_failure( ch, gsn_steal );
      return;
   }

   separate_obj( obj );
   obj_from_char( obj );
   obj_to_char( obj, ch );
   send_to_char( "Ok.\r\n", ch );
   learn_from_success( ch, gsn_steal );
   adjust_favor( ch, 9, 1 );
   return;
}

void do_backstab( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   int percent;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't do that right now.\r\n", ch );
      return;
   }

   one_argument( argument, arg );

   if( ch->mount )
   {
      send_to_char( "You can't get close enough while mounted.\r\n", ch );
      return;
   }

   if( arg[0] == '\0' )
   {
      send_to_char( "Backstab whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "How can you sneak up on yourself?\r\n", ch );
      return;
   }

   if( is_safe( ch, victim, TRUE ) )
      return;

   if( !IS_NPC( ch ) && !IS_NPC( victim ) && xIS_SET( ch->act, PLR_NICE ) )
   {
      send_to_char( "You are too nice to do that.\r\n", ch );
      return;
   }

   /* Added stabbing weapon. -Narn */
   if( !( obj = get_eq_char( ch, WEAR_WIELD ) ) )
   {
      send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
      return;
   }

   if( obj->value[4] != WEP_DAGGER )
   {
      if( ( obj->value[4] == WEP_SWORD && obj->value[3] != DAM_PIERCE ) || obj->value[4] != WEP_SWORD )
      {
         send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
         return;
      }
   }

   if( victim->fighting )
   {
      send_to_char( "You can't backstab someone who is in combat.\r\n", ch );
      return;
   }

   /*
    * Can backstab a char even if it's hurt as long as it's sleeping. -Narn
    */
   if( victim->hit < victim->max_hit && IS_AWAKE( victim ) )
   {
      act( AT_PLAIN, "$N is hurt and suspicious ... you can't sneak up.", ch, NULL, victim, TO_CHAR );
      return;
   }

   percent = number_percent(  ) - ( get_curr_lck( ch ) ) + ( get_curr_lck( victim ) );

   check_attacker( ch, victim );
   WAIT_STATE( ch, skill_table[gsn_backstab]->beats );
   if( !IS_AWAKE( victim ) || ch->pcdata->learned[gsn_backstab] > 0 )
   {
      learn_from_success( ch, gsn_backstab );
      global_retcode = multi_hit( ch, victim, gsn_backstab );
      adjust_favor( ch, 10, 1 );
      check_illegal_pk( ch, victim );

   }
   else
   {
      learn_from_failure( ch, gsn_backstab );
      global_retcode = damage( ch, victim, 0, gsn_backstab );
      check_illegal_pk( ch, victim );
   }
   return;
}


void do_rescue( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   CHAR_DATA *fch;
   int percent;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      send_to_char( "You aren't thinking clearly...\r\n", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg[0] == '\0' )
   {
      send_to_char( "Rescue whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "How about fleeing instead?\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && IS_NPC( victim ) )
   {
      send_to_char( "They don't need your help!\r\n", ch );
      return;
   }

   if( !ch->fighting )
   {
      send_to_char( "Too late...\r\n", ch );
      return;
   }

   if( ( fch = who_fighting( victim ) ) == NULL )
   {
      send_to_char( "They are not fighting right now.\r\n", ch );
      return;
   }

   if( who_fighting( victim ) == ch )
   {
      send_to_char( "Just running away would be better...\r\n", ch );
      return;
   }

   if( IS_AFFECTED( victim, AFF_BERSERK ) )
   {
      send_to_char( "Stepping in front of a berserker would not be an intelligent decision.\r\n", ch );
      return;
   }

   percent = number_percent(  ) - ( get_curr_lck( ch ) - 10 ) - ( get_curr_lck( victim ) - 10 );

   WAIT_STATE( ch, skill_table[gsn_rescue]->beats );
   if( ch->pcdata->learned[gsn_rescue] <= 0 )
   {
      send_to_char( "You fail the rescue.\r\n", ch );
      act( AT_SKILL, "$n tries to rescue you!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "$n tries to rescue $N!", ch, NULL, victim, TO_NOTVICT );
      return;
   }

   act( AT_SKILL, "You rescue $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n rescues you!", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "$n moves in front of $N!", ch, NULL, victim, TO_NOTVICT );

   learn_from_success( ch, gsn_rescue );
   adjust_favor( ch, 8, 1 );
   stop_fighting( fch, FALSE );
   stop_fighting( victim, FALSE );
   if( ch->fighting )
      stop_fighting( ch, FALSE );

   set_fighting( ch, fch );
   set_fighting( fch, ch );
   return;
}



void do_kick( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_kick]->beats );
   if( IS_NPC( ch ) )
	  global_retcode = damage( ch, victim, 0, gsn_kick );
   else
   {
      if( ch->pcdata->learned[gsn_kick] > 0 )
      {
         learn_from_success( ch, gsn_kick );
         global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_kick );
      }
      else
         global_retcode = damage( ch, victim, 0, gsn_kick );
   }
   return;
}

void do_punch( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_punch]->skill_level[ch->Class] )
   {
      send_to_char( "You better leave the martial arts to fighters.\r\n", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_punch]->beats );
   if( IS_NPC( ch ) )
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_punch );
   else
   {
	  if( ch->pcdata->learned[gsn_punch] > 0 )
      {
         learn_from_success( ch, gsn_punch );
         global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_punch );
      }
      else
         global_retcode = damage( ch, victim, 0, gsn_punch );
   }
   return;
}


void do_bite( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( ch->race != RACE_WOLFFOLK && ch->race != RACE_NAGA && ch->race != RACE_DRYAD
    && ch->race != RACE_LIZARDFOLK && ch->race != RACE_DRAGON )
   {
      send_to_char( "That isn't quite one of your natural skills.\r\n", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_bite]->beats );
//   if( can_use_skill( ch, number_percent(  ), gsn_bite ) )
//   {
//      learn_from_success( ch, gsn_bite );
   global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_bite );
//   }
//   else
//   {
//      learn_from_failure( ch, gsn_bite );
//      global_retcode = damage( ch, victim, 0, gsn_bite );
//   }
   return;
}


void do_claw( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_claw]->skill_level[ch->Class] )
   {
      send_to_char( "That isn't quite one of your natural skills.\r\n", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_claw]->beats );
   if( IS_NPC( ch ) )
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_claw );
   else
   {
      if( ch->pcdata->learned[gsn_claw] > 0 )
      {
         learn_from_success( ch, gsn_claw );
         global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_claw );
      }
      else
         global_retcode = damage( ch, victim, 0, gsn_claw );
   }
   return;
}


void do_sting( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_sting]->skill_level[ch->Class] )
   {
      send_to_char( "That isn't quite one of your natural skills.\r\n", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_sting]->beats );
   if( IS_NPC( ch ) )
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_sting );
   else
   {
	  if( ch->pcdata->learned[gsn_sting] > 0 )
      {
         learn_from_success( ch, gsn_sting );
         global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_sting );
      }
      else
         global_retcode = damage( ch, victim, 0, gsn_sting );
   }
   return;
}


void do_tail( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_tail]->skill_level[ch->Class] )
   {
      send_to_char( "That isn't quite one of your natural skills.\r\n", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_tail]->beats );
   if( IS_NPC( ch ) )
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_tail );
   else
   {
      if( ch->pcdata->learned[gsn_tail] > 0 )
      {
         learn_from_success( ch, gsn_tail );
         global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_tail );
      }
      else
         global_retcode = damage( ch, victim, 0, gsn_tail );
   }
   return;
}


void do_bash( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;
   int schance;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_bash]->skill_level[ch->Class] )
   {
      send_to_char( "You better leave the martial arts to fighters.\r\n", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   schance = ( ( ( get_curr_dex( victim ) + get_curr_str( victim ) )
                 - ( get_curr_dex( ch ) + get_curr_str( ch ) ) ) * 10 ) + 10;
   if( !IS_NPC( ch ) && !IS_NPC( victim ) )
      schance += sysdata.bash_plr_vs_plr;
   if( victim->fighting && victim->fighting->who != ch )
      schance += sysdata.bash_nontank;
   WAIT_STATE( ch, skill_table[gsn_bash]->beats );
   if( IS_NPC( ch ) )
   {
	  WAIT_STATE( victim, 2 * PULSE_VIOLENCE );
      victim->position = POS_SITTING;
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_bash );
   }
   else
   {
      if( ch->pcdata->learned[gsn_bash] > 0 )
      {
         learn_from_success( ch, gsn_bash );
         /*
          * do not change anything here!  -Thoric
          */
         WAIT_STATE( victim, 2 * PULSE_VIOLENCE );
         victim->position = POS_SITTING;
         global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_bash );
      }
      else
         global_retcode = damage( ch, victim, 0, gsn_bash );
   }
   return;
}


void do_stun( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   int schance;
   bool fail;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_stun]->skill_level[ch->Class] )
   {
      send_to_char( "You better leave the martial arts to fighters.\r\n", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->move < ch->max_move / 10 )
   {
      set_char_color( AT_SKILL, ch );
      send_to_char( "You are far too tired to do that.\r\n", ch );
      return;  /* missing return fixed March 11/96 */
   }

   WAIT_STATE( ch, skill_table[gsn_stun]->beats );
   fail = FALSE;
   schance = ris_save( victim, ch->level, RIS_PARALYSIS );
   if( schance == 1000 )
      fail = TRUE;
   else
      fail = saves_fortitude( schance, victim );

   schance = ( ( ( get_curr_dex( victim ) + get_curr_str( victim ) )
                 - ( get_curr_dex( ch ) + get_curr_str( ch ) ) ) * 10 ) + 10;
   /*
    * harder for player to stun another player
    */
   if( !IS_NPC( ch ) && !IS_NPC( victim ) )
      schance += sysdata.stun_plr_vs_plr;
   else
      schance += sysdata.stun_regular;
   if( !fail && ch->pcdata->learned[gsn_stun] > 0 )
   {
      learn_from_success( ch, gsn_stun );
      /*
       * DO *NOT* CHANGE!    -Thoric
       */
      if( !IS_NPC( ch ) )
         ch->move -= ch->max_move / 10;
      WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
      WAIT_STATE( victim, PULSE_VIOLENCE );
      act( AT_SKILL, "$N smashes into you, leaving you stunned!", victim, NULL, ch, TO_CHAR );
      act( AT_SKILL, "You smash into $N, leaving $M stunned!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n smashes into $N, leaving $M stunned!", ch, NULL, victim, TO_NOTVICT );
      if( !IS_AFFECTED( victim, AFF_PARALYSIS ) )
      {
         af.type = gsn_stun;
         af.location = APPLY_AC;
         af.modifier = 20;
         af.duration = 3;
         af.bitvector = meb( AFF_PARALYSIS );
         affect_to_char( victim, &af );
         update_pos( victim );
      }
   }
   else
   {
      WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
      if( !IS_NPC( ch ) )
         ch->move -= ch->max_move / 15;
      act( AT_SKILL, "$n charges at you screaming, but you dodge out of the way.", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You try to stun $N, but $E dodges out of the way.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n charges screaming at $N, but keeps going right on past.", ch, NULL, victim, TO_NOTVICT );
   }
   return;
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 * Check for loyalty flag (weapon disarms to inventory) for pkillers -Blodkai
 */
void disarm( CHAR_DATA * ch, CHAR_DATA * victim )
{
   OBJ_DATA *obj, *tmpobj;

   if( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
      return;

   if( ( tmpobj = get_eq_char( victim, WEAR_DUAL_WIELD ) ) != NULL && number_bits( 1 ) == 0 )
      obj = tmpobj;

   if( get_eq_char( ch, WEAR_WIELD ) == NULL && number_bits( 1 ) == 0 )
   {
      learn_from_failure( ch, gsn_disarm );
      return;
   }

   if( IS_NPC( ch ) && !can_see_obj( ch, obj ) && number_bits( 1 ) == 0 )
   {
      learn_from_failure( ch, gsn_disarm );
      return;
   }

   if( check_grip( ch, victim ) )
   {
      learn_from_failure( ch, gsn_disarm );
      return;
   }

   act( AT_SKILL, "$n DISARMS you!", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "You disarm $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n disarms $N!", ch, NULL, victim, TO_NOTVICT );
   learn_from_success( ch, gsn_disarm );

   if( obj == get_eq_char( victim, WEAR_WIELD ) && ( tmpobj = get_eq_char( victim, WEAR_DUAL_WIELD ) ) != NULL )
      tmpobj->wear_loc = WEAR_WIELD;

   obj_from_char( obj );
   if( !IS_NPC( victim ) && CAN_PKILL( victim ) && !IS_OBJ_STAT( obj, ITEM_LOYAL ) )
   {
      SET_BIT( obj->magic_flags, ITEM_PKDISARMED );
      obj->value[5] = victim->level;
   }
   if( IS_NPC( victim ) || ( IS_OBJ_STAT( obj, ITEM_LOYAL ) && IS_PKILL( victim ) && !IS_NPC( ch ) ) )
      obj_to_char( obj, victim );
   else
      obj_to_room( obj, victim->in_room );

   return;
}


void do_disarm( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   int percent;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_disarm]->skill_level[ch->Class] )
   {
      send_to_char( "You don't know how to disarm opponents.\r\n", ch );
      return;
   }

   if( get_eq_char( ch, WEAR_WIELD ) == NULL )
   {
      send_to_char( "You must wield a weapon to disarm.\r\n", ch );
      return;
   }

   if( ( victim = who_fighting( ch ) ) == NULL )
   {
      send_to_char( "You aren't fighting anyone.\r\n", ch );
      return;
   }

   if( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
   {
      send_to_char( "Your opponent is not wielding a weapon.\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_disarm]->beats );
   percent = number_percent(  ) + victim->level - ch->level - ( get_curr_lck( ch ) ) + ( get_curr_lck( victim ) );
   if( !can_see_obj( ch, obj ) )
      percent += 10;
   if( ch->pcdata->learned[gsn_disarm] > 0 )
      disarm( ch, victim );
   else
      send_to_char( "You failed.\r\n", ch );
   return;
}


/*
 * Trip a creature.
 * Caller must check for successful attack.
 */
void trip( CHAR_DATA * ch, CHAR_DATA * victim )
{
   if( IS_AFFECTED( victim, AFF_FLYING ) || IS_AFFECTED( victim, AFF_FLOATING ) )
      return;
   if( victim->mount )
   {
      if( IS_AFFECTED( victim->mount, AFF_FLYING ) || IS_AFFECTED( victim->mount, AFF_FLOATING ) )
         return;
      act( AT_SKILL, "$n trips your mount and you fall off!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You trip $N's mount and $N falls off!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n trips $N's mount and $N falls off!", ch, NULL, victim, TO_NOTVICT );
      if ( IS_NPC( victim->mount ) )
         xREMOVE_BIT( victim->mount->act, ACT_MOUNTED );
      else
         xREMOVE_BIT( victim->mount->act, PLR_MOUNTED );
      victim->mount = NULL;
      WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
      WAIT_STATE( victim, 2 * PULSE_VIOLENCE );
      victim->position = POS_RESTING;
      return;
   }
   if( victim->wait == 0 )
   {
      act( AT_SKILL, "$n trips you and you go down!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You trip $N and $N goes down!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n trips $N and $N goes down!", ch, NULL, victim, TO_NOTVICT );

      WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
      WAIT_STATE( victim, 2 * PULSE_VIOLENCE );
      victim->position = POS_RESTING;
   }

   return;
}

void do_pick( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *gch;
   OBJ_DATA *obj;
   EXIT_DATA *pexit;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Pick what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_pick_lock]->beats );

   /*
    * look for guards
    */
   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( IS_NPC( gch ) && IS_AWAKE( gch ) && ch->level + 5 < gch->level )
      {
         act( AT_PLAIN, "$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR );
         return;
      }
   }

   if( ch->pcdata->learned[gsn_pick_lock] <= 0 )
   {
      send_to_char( "You failed.\r\n", ch );
/*        for ( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
        {
          if ( IS_NPC(gch) && IS_AWAKE(gch) && xIS_SET(gch->act, ACT_GUARDIAN ) )
            multi_hit( gch, ch, TYPE_UNDEFINED );
        }
*/
      return;
   }

   if( ( pexit = find_door( ch, arg, TRUE ) ) != NULL )
   {
      /*
       * 'pick door'
       */
      /*
       * ROOM_INDEX_DATA *to_room;
       *//*
       * Unused
       */
      EXIT_DATA *pexit_rev;

      if( !IS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         send_to_char( "It's not closed.\r\n", ch );
         return;
      }
      if( pexit->key < 0 )
      {
         send_to_char( "It can't be picked.\r\n", ch );
         return;
      }
      if( !IS_SET( pexit->exit_info, EX_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\r\n", ch );
         return;
      }
      if( IS_SET( pexit->exit_info, EX_PICKPROOF ) )
      {
         send_to_char( "You failed.\r\n", ch );
         learn_from_failure( ch, gsn_pick_lock );
         check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
         return;
      }

      REMOVE_BIT( pexit->exit_info, EX_LOCKED );
      send_to_char( "*Click*\r\n", ch );
      act( AT_ACTION, "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
      learn_from_success( ch, gsn_pick_lock );
      adjust_favor( ch, 9, 1 );
      /*
       * pick the other side
       */
      if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
      {
         REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
      }
      check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
      return;
   }

   if( ( obj = get_obj_here( ch, arg ) ) != NULL )
   {
      /*
       * 'pick object'
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         send_to_char( "That's not a container.\r\n", ch );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         send_to_char( "It's not closed.\r\n", ch );
         return;
      }
      if( obj->value[2] < 0 )
      {
         send_to_char( "It can't be unlocked.\r\n", ch );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         send_to_char( "It's already unlocked.\r\n", ch );
         return;
      }
      if( IS_SET( obj->value[1], CONT_PICKPROOF ) )
      {
         send_to_char( "You failed.\r\n", ch );
         learn_from_failure( ch, gsn_pick_lock );
         check_for_trap( ch, obj, TRAP_PICK );
         return;
      }

      separate_obj( obj );
      REMOVE_BIT( obj->value[1], CONT_LOCKED );
      send_to_char( "*Click*\r\n", ch );
      act( AT_ACTION, "$n picks $p.", ch, obj, NULL, TO_ROOM );
      learn_from_success( ch, gsn_pick_lock );
      adjust_favor( ch, 9, 1 );
      check_for_trap( ch, obj, TRAP_PICK );
      return;
   }

   ch_printf( ch, "You see no %s here.\r\n", arg );
   return;
}



void do_sneak( CHAR_DATA* ch, const char* argument)
{
   AFFECT_DATA af;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   send_to_char( "You attempt to move silently.\r\n", ch );
   affect_strip( ch, gsn_sneak );

   if( ch->pcdata->learned[gsn_sneak] > 0 )
   {
      af.type = gsn_sneak;
      af.duration = ( int )( ch->level * DUR_CONV );
      af.location = APPLY_NONE;
      af.modifier = 0;
      af.bitvector = meb( AFF_SNEAK );
      affect_to_char( ch, &af );
      learn_from_success( ch, gsn_sneak );
   }

   return;
}



void do_hide( CHAR_DATA* ch, const char* argument)
{
   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   send_to_char( "You attempt to hide.\r\n", ch );

   if( IS_AFFECTED( ch, AFF_HIDE ) )
      xREMOVE_BIT( ch->affected_by, AFF_HIDE );

   if( ch->pcdata->learned[gsn_hide] > 0 )
   {
      xSET_BIT( ch->affected_by, AFF_HIDE );
      learn_from_success( ch, gsn_hide );
   }

   return;
}



/*
 * Contributed by Alander.
 */
void do_visible( CHAR_DATA* ch, const char* argument)
{
   affect_strip( ch, gsn_invis );
   affect_strip( ch, gsn_mass_invis );
   affect_strip( ch, gsn_sneak );
   xREMOVE_BIT( ch->affected_by, AFF_HIDE );
   xREMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
   xREMOVE_BIT( ch->affected_by, AFF_SNEAK );
   send_to_char( "Ok.\r\n", ch );
   return;
}


void do_recall( CHAR_DATA* ch, const char* argument)
{
   ROOM_INDEX_DATA *location;
   CHAR_DATA *opponent;

   location = NULL;

   if( !IS_NPC( ch ) && ch->pcdata->clan )
      location = get_room_index( ch->pcdata->clan->recall );

   if( !IS_NPC( ch ) && !location && ch->level >= 5 && IS_SET( ch->pcdata->flags, PCFLAG_PVP ) )
      location = get_room_index( ROOM_VNUM_PVP );

   /*
    * 1998-01-02, h
    */
   if( !location )
      location = get_room_index( race_table[ch->race]->race_recall );

   if( !location )
      location = get_room_index( ROOM_VNUM_TEMPLE );

   if( !location )
   {
      send_to_char( "You are completely lost.\r\n", ch );
      return;
   }

   if( ch->in_room == location )
      return;

   if( xIS_SET( ch->in_room->room_flags, ROOM_NO_RECALL ) )
   {
      send_to_char( "For some strange reason... nothing happens.\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_CURSE ) )
   {
      send_to_char( "You are cursed and cannot recall!\r\n", ch );
      return;
   }

   if( ( opponent = who_fighting( ch ) ) != NULL )
   {
      int lose;

      if( number_bits( 1 ) == 0 || ( !IS_NPC( opponent ) && number_bits( 3 ) > 1 ) )
      {
         WAIT_STATE( ch, 4 );
         lose = ( int )( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) * 0.1 );
         if( ch->desc )
            lose /= 2;
         gain_exp( ch, 0 - lose );
         ch_printf( ch, "You failed!  You lose %d exps.\r\n", lose );
         return;
      }

      lose = ( int )( ( exp_level( ch, ch->level + 1 ) - exp_level( ch, ch->level ) ) * 0.2 );
      if( ch->desc )
         lose /= 2;
      gain_exp( ch, 0 - lose );
      ch_printf( ch, "You recall from combat!  You lose %d exps.\r\n", lose );
      stop_fighting( ch, TRUE );
   }

   act( AT_ACTION, "$n disappears in a swirl of smoke.", ch, NULL, NULL, TO_ROOM );
   char_from_room( ch );
   char_to_room( ch, location );
   if( ch->mount )
   {
      char_from_room( ch->mount );
      char_to_room( ch->mount, location );
   }
   act( AT_ACTION, "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
   do_look( ch, "auto" );

   return;
}


void do_aid( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int percent;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   one_argument( argument, arg );
   if( arg[0] == '\0' )
   {
      send_to_char( "Aid whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( IS_NPC( victim ) )  /* Gorog */
   {
      send_to_char( "Not on mobs.\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Aid yourself?\r\n", ch );
      return;
   }

   if( victim->position > POS_STUNNED )
   {
      act( AT_PLAIN, "$N doesn't need your help.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->hit <= -11 )
   {
      act( AT_PLAIN, "$N's condition is beyond your aiding ability.", ch, NULL, victim, TO_CHAR );
      return;
   }

   percent = number_percent(  ) - ( get_curr_lck( ch ) - 10 );
   WAIT_STATE( ch, skill_table[gsn_aid]->beats );
   if( ch->pcdata->learned[gsn_aid] <= 0 )
   {
      send_to_char( "You fail.\r\n", ch );
      return;
   }

   act( AT_SKILL, "You aid $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n aids $N!", ch, NULL, victim, TO_NOTVICT );
   learn_from_success( ch, gsn_aid );
   adjust_favor( ch, 8, 1 );
   if( victim->hit < 1 )
      victim->hit = 1;

   update_pos( victim );
   act( AT_SKILL, "$n aids you!", ch, NULL, victim, TO_VICT );
   return;
}

/*
 * Modified to accomodate mounting other players. -Danny-
 */
void do_mount( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;

   if( ch->race == RACE_UNICORN || ch->race == RACE_DRAGON || ch->race == RACE_GRYPHON
      							|| ch->race == RACE_SPHINX || ch->race == RACE_CENTAUR )
   {
      send_to_char( "You cannot mount other creatures!\r\n", ch );
      return;
   }

   if( ch->mount )
   {
      send_to_char( "You're already mounted!\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, argument ) ) == NULL )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "Mount yourself?\r\n", ch );
      return;
   }

   if( ( !xIS_SET( victim->act, ACT_MOUNTABLE ) ) && ( !xIS_SET( victim->act, PLR_MOUNTABLE ) ) )
   {
      send_to_char( "You can't mount that!\r\n", ch );
      return;
   }

   if( xIS_SET( victim->act, ACT_MOUNTED ) || xIS_SET( victim->act, PLR_MOUNTED ) )
   {
      send_to_char( "That mount already has a rider.\r\n", ch );
      return;
   }

   if( victim->position < POS_STANDING )
   {
      send_to_char( "Your mount must be standing.\r\n", ch );
      return;
   }

   if( victim->position == POS_FIGHTING || victim->fighting )
   {
      send_to_char( "Your mount is moving around too much.\r\n", ch );
      return;
   }

   if( IS_AFFECTED( victim, AFF_FLYING ) && !IS_AFFECTED( ch, AFF_FLYING ) )
   {
      send_to_char( "You'd need to fly up to climb on their back!\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_mount]->beats );
   if( ch->pcdata->learned[gsn_mount] >= 0 )
   {
      if ( IS_NPC( victim ) )
         xSET_BIT( victim->act, ACT_MOUNTED );
      else
         xSET_BIT( victim->act, PLR_MOUNTED );
      ch->mount = victim;
      victim->rider = ch;
      act( AT_SKILL, "You mount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n skillfully mounts $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n mounts you.", ch, NULL, victim, TO_VICT );
      learn_from_success( ch, gsn_mount );
      ch->position = POS_MOUNTED;
   }
   else
   {
      act( AT_SKILL, "You unsuccessfully try to mount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n unsuccessfully attempts to mount $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n tries to mount you.", ch, NULL, victim, TO_VICT );
   }
   return;
}


void do_dismount( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;

   if( ( victim = ch->mount ) == NULL )
   {
      send_to_char( "You're not mounted.\r\n", ch );
      return;
   }

   WAIT_STATE( ch, skill_table[gsn_mount]->beats );
   if( ch->pcdata->learned[gsn_mount] >= 0 )
   {
      act( AT_SKILL, "You dismount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n skillfully dismounts $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n dismounts you.  Whew!", ch, NULL, victim, TO_VICT );
      if ( IS_NPC( victim ) )
         xREMOVE_BIT( victim->act, ACT_MOUNTED );
      else
         xREMOVE_BIT( victim->act, PLR_MOUNTED );
      ch->mount = NULL;
      victim->rider = NULL;
      ch->position = POS_STANDING;
      learn_from_success( ch, gsn_mount );
   }
   else
   {
      act( AT_SKILL, "You fall off while dismounting $N.  Ouch!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n falls off of $N while dismounting.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n falls off your back.", ch, NULL, victim, TO_VICT );
      if ( IS_NPC( victim ) )
         xREMOVE_BIT( victim->act, ACT_MOUNTED );
      else
         xREMOVE_BIT( victim->act, PLR_MOUNTED );
      ch->mount = NULL;
      victim->rider = NULL;
      ch->position = POS_SITTING;
      global_retcode = damage( ch, ch, 1, TYPE_UNDEFINED );
   }
   return;
}

/* study by Absalom */
void do_study( CHAR_DATA *ch, const char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int sn = 0;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
	send_to_char( "Study what?\r\n", ch );
	return;
    }

/*  if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
	send_to_char( "You do not have that item.\r\n", ch );
	return;
    }
*/
    if ( ( obj = get_obj_here( ch, arg ) ) == NULL )
    {
	send_to_char( "You do not see that here.\r\n", ch );
	return;
    }

    if ( obj->item_type != ITEM_STAFF && obj->item_type != ITEM_WAND &&
	     obj->item_type != ITEM_SCROLL && obj->item_type != ITEM_FURNITURE)
    {
	send_to_char( "You can only study relics, scrolls, wands, and staves.\r\n", ch );
	return;
    }

/* Can't study furniture without a spell in it (v3 = 0 ) -Danny */
    if ( obj->item_type == ITEM_FURNITURE && obj->value[3] == 0 )
    {
	send_to_char( "You cannot learn anything from studying this.\r\n", ch );
	return;
    }

    act( AT_MAGIC, "$n studies $p.", ch, obj, NULL, TO_ROOM );
    act( AT_MAGIC, "You study $p.", ch, obj, NULL, TO_CHAR );

    if (obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
    {
	sn = obj->value[3];
	if ( sn < 0 || sn >= MAX_SKILL || skill_table[sn]->spell_fun == spell_null )
	{
	  bug( "Do_study: bad sn %d.", sn );
	  return;
	}
	WAIT_STATE( ch, skill_table[gsn_study]->beats );
	if ( number_percent() >= 55 + ch->pcdata->learned[gsn_study] * 4/5)
	{
	  send_to_char("You cannot glean any knowledge from it.\r\n",ch);
	  learn_from_failure( ch, gsn_study );
	  act( AT_FIRE, "$p burns brightly and is gone.", ch, obj, NULL, TO_CHAR );
	  separate_obj( obj );
	  extract_obj( obj );
	  return;
	}
	if ( ch->pcdata->learned[sn])
	{
	  send_to_char("You already know that spell!\r\n",ch);
	  return;
	}
	ch->pcdata->learned[sn] = 1;
	act( AT_MAGIC, "You have learned the art of $t!", ch ,skill_table[sn]->name, NULL, TO_CHAR);
	learn_from_success( ch, gsn_study );
	act( AT_FIRE, "$p burns brightly and is gone.", ch, obj, NULL, TO_CHAR );
      separate_obj( obj );
	extract_obj( obj );
	return;
    }

/*  Code for studying 'furniture' items. For stuff in ruins that can't be moved and is reuseable -Danny */
    if (obj->item_type == ITEM_FURNITURE )
    {
	sn = obj->value[3]; /* SN goes in value3 */
	if ( sn < 0 || sn >= MAX_SKILL || skill_table[sn]->spell_fun == spell_null )
	{
	  bug( "Do_study: bad sn %d.", sn );
	  return;
	}
	WAIT_STATE( ch, skill_table[gsn_study]->beats );
	if ( number_percent() >= 55 + ch->pcdata->learned[gsn_study] * 4/5)
	{
	  send_to_char("You cannot glean any knowledge from it.\r\n",ch);
	  learn_from_failure( ch, gsn_study );
	  return;
	}
	if ( ch->pcdata->learned[sn])
	{
	  send_to_char("You already know that spell!\r\n",ch);
	  return;
	}
	ch->pcdata->learned[sn] = 1;
	act( AT_MAGIC, "You have learned the art of $t!", ch ,skill_table[sn]->name, NULL, TO_CHAR);
	learn_from_success( ch, gsn_study );
	return;
    }

    if (obj->item_type == ITEM_SCROLL)
    {
	sn = obj->value[1];
	if ( sn < 0 || sn >= MAX_SKILL || skill_table[sn]->spell_fun == spell_null )
	{
	  bug( "Do_study: bad sn %d.", sn );
	  return;
	}
	if ( number_percent() >= 15 + ch->pcdata->learned[gsn_study] * 4/5)
	{
	  send_to_char("You cannot glean any knowledge from it.\r\n",ch);
	  learn_from_failure( ch, gsn_study );
	  act( AT_FIRE, "$p burns brightly and is gone.", ch, obj, NULL, TO_CHAR );
	  separate_obj( obj );
	  extract_obj( obj );
	  return;
	}
	if ( ch->pcdata->learned[sn])
	{
	  send_to_char("You already know that spell!\r\n",ch);
	  return;
	}
	ch->pcdata->learned[sn] = 1;
	act( AT_MAGIC, "You have learned the art of $t!", ch, skill_table[sn]->name, NULL, TO_CHAR);
	learn_from_success( ch, gsn_study );
	act( AT_FIRE, "$p burns brightly and is gone.", ch, obj, NULL, TO_CHAR );
	separate_obj( obj );
	extract_obj( obj );
	return;
    }

}


/**************************************************************************/


/*
 * Check for parry.
 */
bool check_parry( CHAR_DATA * ch, CHAR_DATA * victim )
{
   int chances;

   if( !IS_AWAKE( victim ) )
      return FALSE;

   if( IS_NPC( victim ) && !xIS_SET( victim->defenses, DFND_PARRY ) )
      return FALSE;

   if( IS_NPC( victim ) )
   {
      /*
       * Tuan was here.  :)
       */
      chances = UMIN( 60, 2 * victim->level );
   }
   else
   {
      if( get_eq_char( victim, WEAR_WIELD ) == NULL )
         return FALSE;
      chances = ( int )( LEARNED( victim, gsn_parry ) / sysdata.parry_mod );
   }

   /*
    * Put in the call to chance() to allow penalties for misaligned
    * clannies.
    */
   if( chances != 0 && victim->morph )
      chances += victim->morph->parry;

   if( !chance( victim, chances + victim->level - ch->level ) )
   {
      learn_from_failure( victim, gsn_parry );
      return FALSE;
   }
   if( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
       /*SB*/ act( AT_SKILL, "You parry $n's attack.", ch, NULL, victim, TO_VICT );

   if( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )   /* SB */
      act( AT_SKILL, "$N parries your attack.", ch, NULL, victim, TO_CHAR );

   learn_from_success( victim, gsn_parry );
   return TRUE;
}

/*
 * Check for shield block.
 */
bool check_block( CHAR_DATA * ch, CHAR_DATA * victim )
{
   int chances;

   if( !IS_AWAKE( victim ) )
      return FALSE;

   if( IS_NPC( victim ) && !xIS_SET( victim->defenses, DFND_BLOCK ) )
      return FALSE;

   if( IS_NPC( victim ) )
   {
      /*
       * Tuan was here.  :)
       */
      chances = UMIN( 60, 2 * victim->level );
   }
   else
   {
      if( get_eq_char( victim, WEAR_SHIELD ) == NULL )
         return FALSE;
      chances = ( int )( LEARNED( victim, gsn_block ) / sysdata.parry_mod );
   }

   if( !chance( victim, chances + victim->level - ch->level ) )
   {
      learn_from_failure( victim, gsn_block );
      return FALSE;
   }
   if( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
       /*SB*/ act( AT_SKILL, "You block $n's attack with your shield.", ch, NULL, victim, TO_VICT );

   if( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )   /* SB */
      act( AT_SKILL, "$N blocks your attack with $S shield.", ch, NULL, victim, TO_CHAR );

   learn_from_success( victim, gsn_block );
   return TRUE;
}

/*
 * Check for dodge.
 */
bool check_dodge( CHAR_DATA * ch, CHAR_DATA * victim )
{
   int chances;

   if( !IS_AWAKE( victim ) )
      return FALSE;

   if( IS_NPC( victim ) && !xIS_SET( victim->defenses, DFND_DODGE ) )
      return FALSE;

   if( IS_NPC( victim ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = ( int )( LEARNED( victim, gsn_dodge ) / sysdata.dodge_mod );

   if( chances != 0 && victim->morph != NULL )
      chances += victim->morph->dodge;

   /*
    * Consider luck as a factor
    */
   if( !chance( victim, chances + victim->level - ch->level ) )
   {
      learn_from_failure( victim, gsn_dodge );
      return FALSE;
   }

   if( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "You dodge $n's attack.", ch, NULL, victim, TO_VICT );

   if( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "$N dodges your attack.", ch, NULL, victim, TO_CHAR );

   learn_from_success( victim, gsn_dodge );
   return TRUE;
}

bool check_tumble( CHAR_DATA * ch, CHAR_DATA * victim )
{
   int chances;

   if( victim->Class != CLASS_ROGUE || !IS_AWAKE( victim ) )
      return FALSE;
   if( !IS_NPC( victim ) && !victim->pcdata->learned[gsn_tumble] > 0 )
      return FALSE;
   if( IS_NPC( victim ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = ( int )( LEARNED( victim, gsn_tumble ) / sysdata.tumble_mod + ( get_curr_dex( victim ) - 10 ) );
   if( chances != 0 && victim->morph )
      chances += victim->morph->tumble;
   if( !chance( victim, chances + victim->level - ch->level ) )
      return FALSE;
   if( !IS_NPC( victim ) && !IS_SET( victim->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "You tumble away from $n's attack.", ch, NULL, victim, TO_VICT );
   if( !IS_NPC( ch ) && !IS_SET( ch->pcdata->flags, PCFLAG_GAG ) )
      act( AT_SKILL, "$N tumbles away from your attack.", ch, NULL, victim, TO_CHAR );
   learn_from_success( victim, gsn_tumble );
   return TRUE;
}

void do_poison_weapon( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *obj;
   OBJ_DATA *pobj;
   OBJ_DATA *wobj;
   char arg[MAX_INPUT_LENGTH];
   int percent;

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_poison_weapon]->skill_level[ch->Class] )
   {
      send_to_char( "What do you think you are, a thief?\r\n", ch );
      return;
   }

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "What are you trying to poison?\r\n", ch );
      return;
   }
   if( ch->fighting )
   {
      send_to_char( "While you're fighting?  Nice try.\r\n", ch );
      return;
   }
   if( ms_find_obj( ch ) )
      return;

   if( !( obj = get_obj_carry( ch, arg ) ) )
   {
      send_to_char( "You do not have that weapon.\r\n", ch );
      return;
   }
   if( obj->item_type != ITEM_WEAPON )
   {
      send_to_char( "That item is not a weapon.\r\n", ch );
      return;
   }
   if( IS_OBJ_STAT( obj, ITEM_POISONED ) )
   {
      send_to_char( "That weapon is already poisoned.\r\n", ch );
      return;
   }
   if( IS_OBJ_STAT( obj, ITEM_CLANOBJECT ) )
   {
      send_to_char( "It doesn't appear to be fashioned of a poisonable material.\r\n", ch );
      return;
   }
   /*
    * Now we have a valid weapon...check to see if we have the powder.
    */
   for( pobj = ch->first_carrying; pobj; pobj = pobj->next_content )
   {
      if( pobj->pIndexData->vnum == OBJ_VNUM_BLACK_POWDER )
         break;
   }
   if( !pobj )
   {
      send_to_char( "You do not have the black poison powder.\r\n", ch );
      return;
   }
   /*
    * Okay, we have the powder...do we have water?
    */
   for( wobj = ch->first_carrying; wobj; wobj = wobj->next_content )
   {
      if( wobj->item_type == ITEM_DRINK_CON && wobj->value[1] > 0 && wobj->value[2] == 0 )
         break;
   }
   if( !wobj )
   {
      send_to_char( "You have no water to mix with the powder.\r\n", ch );
      return;
   }
   /*
    * Great, we have the ingredients...but is the thief smart enough?
    */
   if( !IS_NPC( ch ) && get_curr_wis( ch ) < 16 )
   {
      send_to_char( "You can't quite remember what to do...\r\n", ch );
      return;
   }
   /*
    * And does the thief have steady enough hands?
    */
   if( !IS_NPC( ch ) && ( ( get_curr_dex( ch ) < 17 ) || ch->pcdata->condition[COND_DRUNK] > 0 ) )
   {
      send_to_char( "Your hands aren't steady enough to properly mix the poison.\r\n", ch );
      return;
   }
   WAIT_STATE( ch, skill_table[gsn_poison_weapon]->beats );

   percent = ( number_percent(  ) - get_curr_lck( ch ) - 10 );

   /*
    * Check the skill percentage
    */
   separate_obj( pobj );
   separate_obj( wobj );
   if( ch->pcdata->learned[gsn_poison_weapon] <= 0 )
   {
      set_char_color( AT_RED, ch );
      send_to_char( "You failed and spill some on yourself.  Ouch!\r\n", ch );
      set_char_color( AT_GREY, ch );
      damage( ch, ch, ch->level, gsn_poison_weapon );
      act( AT_RED, "$n spills the poison all over!", ch, NULL, NULL, TO_ROOM );
      extract_obj( pobj );
      extract_obj( wobj );
      return;
   }
   separate_obj( obj );
   /*
    * Well, I'm tired of waiting.  Are you?
    */
   act( AT_RED, "You mix $p in $P, creating a deadly poison!", ch, pobj, wobj, TO_CHAR );
   act( AT_RED, "$n mixes $p in $P, creating a deadly poison!", ch, pobj, wobj, TO_ROOM );
   act( AT_GREEN, "You pour the poison over $p, which glistens wickedly!", ch, obj, NULL, TO_CHAR );
   act( AT_GREEN, "$n pours the poison over $p, which glistens wickedly!", ch, obj, NULL, TO_ROOM );
   xSET_BIT( obj->extra_flags, ITEM_POISONED );
   obj->cost *= 2;
   /*
    * Set an object timer.  Don't want proliferation of poisoned weapons
    */
   obj->timer = UMIN( obj->level, ch->level );

   if( IS_OBJ_STAT( obj, ITEM_BLESS ) )
      obj->timer *= 2;

   if( IS_OBJ_STAT( obj, ITEM_MAGIC ) )
      obj->timer *= 2;

   /*
    * WHAT?  All of that, just for that one bit?  How lame. ;)
    */
   act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj, NULL, TO_CHAR );
   act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj, NULL, TO_ROOM );
   extract_obj( pobj );
   extract_obj( wobj );
   learn_from_success( ch, gsn_poison_weapon );
   return;
}

void do_scribe( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *scroll;
   int sn;
   char buf[MAX_STRING_LENGTH];
   int manacost, mana;

   if( IS_NPC( ch ) )
      return;

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_scribe]->skill_level[ch->Class] )
   {
      send_to_char( "A skill such as this requires more magical ability than that of your class.\r\n", ch );
      return;
   }

   if( argument[0] == '\0' || !str_cmp( argument, "" ) )
   {
      send_to_char( "Scribe what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, TRUE ) ) < 0 )
   {
      send_to_char( "You have not learned that spell.\r\n", ch );
      return;
   }

   if( skill_table[sn]->spell_fun == spell_null )
   {
      send_to_char( "That's not a spell!\r\n", ch );
      return;
   }

   if( SPELL_FLAG( skill_table[sn], SF_NOSCRIBE ) )
   {
      send_to_char( "You cannot scribe that spell.\r\n", ch );
      return;
   }

   manacost = ( skill_table[sn]->min_mana * 100 ) / ( 99 + ch->pcdata->learned[sn] ) ;
   mana = IS_NPC( ch ) ? 0 : UMIN( skill_table[sn]->min_mana, manacost );

   mana *= 5;

   if( !IS_NPC( ch ) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\r\n", ch );
      return;
   }

   if( ( scroll = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
   {
      send_to_char( "You must be holding a blank scroll to scribe it.\r\n", ch );
      return;
   }

   if( scroll->pIndexData->vnum != OBJ_VNUM_SCROLL_SCRIBING )
   {
      send_to_char( "You must be holding a blank scroll to scribe it.\r\n", ch );
      return;
   }

   if( ( scroll->value[1] != -1 ) && ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING ) )
   {
      send_to_char( "That scroll has already been inscribed.\r\n", ch );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      learn_from_failure( ch, gsn_scribe );
      ch->mana -= ( mana / 2 );
      return;
   }

   if( ch->pcdata->learned[gsn_scribe] <= 0 )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "You failed.\r\n", ch );
      ch->mana -= ( mana / 2 );
      return;
   }

   scroll->value[1] = sn;
   scroll->value[0] = ch->level;
   snprintf( buf, MAX_STRING_LENGTH, "%s scroll", skill_table[sn]->name );
   STRFREE( scroll->short_descr );
   scroll->short_descr = STRALLOC( aoran( buf ) );

   snprintf( buf, MAX_STRING_LENGTH, "A glowing scroll inscribed '%s' lies in the dust.", skill_table[sn]->name );
   STRFREE( scroll->description );
   scroll->description = STRALLOC( buf );

   snprintf( buf, MAX_STRING_LENGTH, "scroll scribing %s", skill_table[sn]->name );
   STRFREE( scroll->name );
   scroll->name = STRALLOC( buf );

   act( AT_MAGIC, "$n magically scribes $p.", ch, scroll, NULL, TO_ROOM );
   act( AT_MAGIC, "You magically scribe $p.", ch, scroll, NULL, TO_CHAR );

   learn_from_success( ch, gsn_scribe );

   ch->mana -= mana;

}

void do_brew( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *potion;
   OBJ_DATA *fire;
   int sn;
   char buf[MAX_STRING_LENGTH];
   int manacost, mana;
   bool found;

   if( IS_NPC( ch ) )
      return;

   if( !IS_NPC( ch ) && ch->level < skill_table[gsn_brew]->skill_level[ch->Class] )
   {
      send_to_char( "A skill such as this requires more magical ability than that of your class.\r\n", ch );
      return;
   }

   if( argument[0] == '\0' || !str_cmp( argument, "" ) )
   {
      send_to_char( "Brew what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, TRUE ) ) < 0 )
   {
      send_to_char( "You have not learned that spell.\r\n", ch );
      return;
   }

   if( skill_table[sn]->spell_fun == spell_null )
   {
      send_to_char( "That's not a spell!\r\n", ch );
      return;
   }

   if( SPELL_FLAG( skill_table[sn], SF_NOBREW ) )
   {
      send_to_char( "You cannot brew that spell.\r\n", ch );
      return;
   }

   manacost = ( skill_table[sn]->min_mana * 100 ) / ( 99 + ch->pcdata->learned[sn] ) ;
   mana = IS_NPC( ch ) ? 0 : UMIN( skill_table[sn]->min_mana, manacost );

   mana *= 4;

   if( !IS_NPC( ch ) && ch->mana < mana )
   {
      send_to_char( "You don't have enough mana.\r\n", ch );
      return;
   }

   found = FALSE;

   for( fire = ch->in_room->first_content; fire; fire = fire->next_content )
   {
      if( fire->item_type == ITEM_FIRE )
      {
         found = TRUE;
         break;
      }
   }

   if( !found )
   {
      send_to_char( "There must be a fire in the room to brew a potion.\r\n", ch );
      return;
   }

   if( ( potion = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
   {
      send_to_char( "You must be holding an empty flask to brew a potion.\r\n", ch );
      return;
   }

   if( potion->pIndexData->vnum != OBJ_VNUM_FLASK_BREWING )
   {
      send_to_char( "You must be holding an empty flask to brew a potion.\r\n", ch );
      return;
   }

   if( ( potion->value[1] != -1 ) && ( potion->pIndexData->vnum == OBJ_VNUM_FLASK_BREWING ) )
   {
      send_to_char( "That's not an empty flask.\r\n", ch );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      learn_from_failure( ch, gsn_brew );
      ch->mana -= ( mana / 2 );
      return;
   }

   if( ch->pcdata->learned[gsn_brew] <= 0 )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "You failed.\r\n", ch );
      learn_from_failure( ch, gsn_brew );
      ch->mana -= ( mana / 2 );
      return;
   }

   potion->value[1] = sn;
   potion->value[0] = ch->level;
   snprintf( buf, MAX_STRING_LENGTH, "%s potion", skill_table[sn]->name );
   STRFREE( potion->short_descr );
   potion->short_descr = STRALLOC( aoran( buf ) );

   snprintf( buf, MAX_STRING_LENGTH, "A strange potion labelled '%s' sizzles in a glass flask.", skill_table[sn]->name );
   STRFREE( potion->description );
   potion->description = STRALLOC( buf );

   snprintf( buf, MAX_STRING_LENGTH, "flask potion %s", skill_table[sn]->name );
   STRFREE( potion->name );
   potion->name = STRALLOC( buf );

   act( AT_MAGIC, "$n brews up $p.", ch, potion, NULL, TO_ROOM );
   act( AT_MAGIC, "You brew up $p.", ch, potion, NULL, TO_CHAR );

   learn_from_success( ch, gsn_brew );

   ch->mana -= mana;

}

bool check_grip( CHAR_DATA * ch, CHAR_DATA * victim )
{
   int schance;

   if( !IS_AWAKE( victim ) )
      return FALSE;

   if( IS_NPC( victim ) && !xIS_SET( victim->defenses, DFND_GRIP ) )
      return FALSE;

   if( IS_NPC( victim ) )
      schance = UMIN( 60, 2 * victim->level );
   else
      schance = ( int )( LEARNED( victim, gsn_grip ) / 2 );

   /*
    * Consider luck as a factor
    */
   schance += ( 2 * ( get_curr_lck( victim ) - 10 ) );

   if( number_percent(  ) >= schance + victim->level - ch->level )
   {
      learn_from_failure( victim, gsn_grip );
      return FALSE;
   }
   act( AT_SKILL, "You evade $n's attempt to disarm you.", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "$N holds $S weapon strongly, and is not disarmed.", ch, NULL, victim, TO_CHAR );
   learn_from_success( victim, gsn_grip );
   return TRUE;
}

void do_circle( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   int percent;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }

   one_argument( argument, arg );

   if( ch->mount )
   {
      send_to_char( "You can't circle while mounted.\r\n", ch );
      return;
   }

   if( arg[0] == '\0' )
   {
      send_to_char( "Circle around whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "How can you sneak up on yourself?\r\n", ch );
      return;
   }

   if( is_safe( ch, victim, TRUE ) )
      return;

   if( !( obj = get_eq_char( ch, WEAR_WIELD ) ) )
   {
      send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
      return;
   }

   if( obj->value[4] != WEP_DAGGER )
   {
      if( ( obj->value[4] == WEP_SWORD && obj->value[3] != DAM_PIERCE ) || obj->value[4] != WEP_SWORD )
      {
         send_to_char( "You need to wield a piercing or stabbing weapon.\r\n", ch );
         return;
      }
   }

   if( !ch->fighting )
   {
      send_to_char( "You can't circle when you aren't fighting.\r\n", ch );
      return;
   }

   if( !victim->fighting )
   {
      send_to_char( "You can't circle around a person who is not fighting.\r\n", ch );
      return;
   }

   if( victim->num_fighting < 2 )
   {
      act( AT_PLAIN, "You can't circle around them without a distraction.", ch, NULL, victim, TO_CHAR );
      return;
   }

   percent = number_percent(  ) - ( get_curr_lck( ch ) ) + ( get_curr_lck( victim ) );

   check_attacker( ch, victim );
   WAIT_STATE( ch, skill_table[gsn_circle]->beats );
   if( ch->pcdata->learned[gsn_circle] > 0 )
   {
      learn_from_success( ch, gsn_circle );
      WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
      global_retcode = multi_hit( ch, victim, gsn_circle );
      adjust_favor( ch, 10, 1 );
      check_illegal_pk( ch, victim );
   }
   else
   {
      WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
      global_retcode = damage( ch, victim, 0, gsn_circle );
   }
   return;
}

/* Berserk and HitAll. -- Altrag */
void do_berserk( CHAR_DATA* ch, const char* argument)
{
   short percent;
   AFFECT_DATA af;

   if( !ch->fighting )
   {
      send_to_char( "But you aren't fighting!\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_BERSERK ) )
   {
      send_to_char( "Your rage is already at its peak!\r\n", ch );
      return;
   }

   percent = LEARNED( ch, gsn_berserk );
   WAIT_STATE( ch, skill_table[gsn_berserk]->beats );
   if( !chance( ch, percent ) )
   {
      send_to_char( "You couldn't build up enough rage.\r\n", ch );
      learn_from_failure( ch, gsn_berserk );
      return;
   }
   af.type = gsn_berserk;
   /*
    * Hmmm.. 10-20 combat rounds at level 50.. good enough for most mobs,
    * and if not they can always go berserk again.. shrug.. maybe even
    * too high. -- Altrag
    */
   af.duration = number_range( ch->level / 5, ch->level * 2 / 5 );
   /*
    * Hmm.. you get stronger when yer really enraged.. mind over matter
    * type thing..
    */
   af.location = APPLY_STR;
   af.modifier = 1;
   af.bitvector = meb( AFF_BERSERK );
   affect_to_char( ch, &af );
   send_to_char( "You start to lose control..\r\n", ch );
   learn_from_success( ch, gsn_berserk );
   return;
}

/* External from fight.c */
ch_ret one_hit args( ( CHAR_DATA * ch, CHAR_DATA * victim, int dt ) );
void do_hitall( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *vch;
   CHAR_DATA *vch_next;
   short nvict = 0;
   short nhit = 0;
   short percent;

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      send_to_char_color( "&BA godly force prevents you.\r\n", ch );
      return;
   }

   if( !ch->in_room->first_person )
   {
      send_to_char( "There's no one else here!\r\n", ch );
      return;
   }
   percent = LEARNED( ch, gsn_hitall );
   for( vch = ch->in_room->first_person; vch; vch = vch_next )
   {
      vch_next = vch->next_in_room;
      if( is_same_group( ch, vch ) || !is_legal_kill( ch, vch ) || !can_see( ch, vch ) || is_safe( ch, vch, TRUE ) )
         continue;
      if( ++nvict > ch->level / 5 )
         break;
      check_illegal_pk( ch, vch );
      if( chance( ch, percent ) )
      {
         nhit++;
         global_retcode = one_hit( ch, vch, TYPE_UNDEFINED );
      }
      else
         global_retcode = damage( ch, vch, 0, TYPE_UNDEFINED );
      /*
       * Fireshield, etc. could kill ch too.. :>.. -- Altrag
       */
      if( global_retcode == rCHAR_DIED || global_retcode == rBOTH_DIED || char_died( ch ) )
         return;
   }
   if( !nvict )
   {
      send_to_char( "There's no one else here!\r\n", ch );
      return;
   }
   ch->move = UMAX( 0, ch->move - nvict * 3 + nhit );
   if( nhit )
      learn_from_success( ch, gsn_hitall );
   else
      learn_from_failure( ch, gsn_hitall );
   return;
}



bool check_illegal_psteal( CHAR_DATA * ch, CHAR_DATA * victim )
{
/*   char log_buf[MAX_STRING_LENGTH]; */

   if( !IS_NPC( victim ) && !IS_NPC( ch ) )
   {
      if( ( !IS_SET( victim->pcdata->flags, PCFLAG_PVP )
            || ch->level - victim->level > 10
            || !IS_SET( ch->pcdata->flags, PCFLAG_PVP ) )
          && ( ch->in_room->vnum < 29 || ch->in_room->vnum > 43 ) && ch != victim )
      {
         /*
          * snprintf( log_buf, MAX_STRING_LENGTH, "%s illegally stealing from %s at %d",
          * (IS_NPC(ch) ? ch->short_descr : ch->name),
          * victim->name,
          * victim->in_room->vnum );
          * log_string( log_buf );
          * to_channel( log_buf, CHANNEL_MONITOR, "Monitor", LEVEL_IMMORTAL );
          */
         return TRUE;
      }
   }
   return FALSE;
}

void do_scan( CHAR_DATA* ch, const char* argument)
{
   ROOM_INDEX_DATA *was_in_room;
   EXIT_DATA *pexit;
   short dir = -1;
   short dist;
   short max_dist = 8;

   set_char_color( AT_ACTION, ch );

   if( IS_AFFECTED( ch, AFF_BLIND ) && ( !IS_AFFECTED( ch, AFF_TRUESIGHT ) ||
                                         ( !IS_NPC( ch ) && !xIS_SET( ch->act, PLR_HOLYLIGHT ) ) ) )
   {
      send_to_char( "Everything looks the same when you're blind...\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Scan in a direction...\r\n", ch );
      return;
   }

   if( ( dir = get_door( argument ) ) == -1 )
   {
      send_to_char( "Scan in WHAT direction?\r\n", ch );
      return;
   }

   was_in_room = ch->in_room;
   act( AT_GREY, "Scanning $t...", ch, dir_name[dir], NULL, TO_CHAR );
   act( AT_GREY, "$n scans $t.", ch, dir_name[dir], NULL, TO_ROOM );

   if( ch->pcdata->learned[gsn_scan] <= 0 )
   {
      act( AT_GREY, "You stop scanning $t as your vision blurs.", ch, dir_name[dir], NULL, TO_CHAR );
      return;
   }

   if( IS_VAMPIRE( ch ) )
   {
      if( time_info.hour < 21 && time_info.hour > 5 )
      {
         send_to_char( "You have trouble seeing clearly through all the " "light.\r\n", ch );
         max_dist = 1;
      }
   }

   if( ( pexit = get_exit( ch->in_room, dir ) ) == NULL )
   {
      act( AT_GREY, "You can't see $t.", ch, dir_name[dir], NULL, TO_CHAR );
      return;
   }

   if( ch->level < 50 )
      --max_dist;
   if( ch->level < 40 )
      --max_dist;
   if( ch->level < 30 )
      --max_dist;

   for( dist = 1; dist <= max_dist; )
   {
      if( IS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         if( IS_SET( pexit->exit_info, EX_SECRET ) || IS_SET( pexit->exit_info, EX_DIG ) )
            act( AT_GREY, "Your view $t is blocked by a wall.", ch, dir_name[dir], NULL, TO_CHAR );
         else
            act( AT_GREY, "Your view $t is blocked by a door.", ch, dir_name[dir], NULL, TO_CHAR );
         break;
      }
      if( room_is_private( pexit->to_room ) && ch->level < LEVEL_GREATER )
      {
         act( AT_GREY, "Your view $t is blocked by a private room.", ch, dir_name[dir], NULL, TO_CHAR );
         break;
      }
      char_from_room( ch );
      char_to_room( ch, pexit->to_room );
      set_char_color( AT_RMNAME, ch );
      send_to_char( ch->in_room->name, ch );
      send_to_char( "\r\n", ch );
      show_list_to_char( ch->in_room->first_content, ch, FALSE, FALSE );
      show_char_to_char( ch->in_room->first_person, ch );

      switch ( ch->in_room->sector_type )
      {
         default:
            dist++;
            break;
         case SECT_AIR:
            if( number_percent(  ) < 80 )
               dist++;
            break;
         case SECT_INSIDE:
         case SECT_FIELD:
         case SECT_UNDERGROUND:
            dist++;
            break;
         case SECT_FOREST:
         case SECT_CITY:
         case SECT_DESERT:
         case SECT_HILLS:
            dist += 2;
            break;
         case SECT_WATER_SWIM:
         case SECT_WATER_NOSWIM:
         case SECT_OCEAN:
            dist += 3;
            break;
         case SECT_MOUNTAIN:
         case SECT_UNDERWATER:
         case SECT_OCEANFLOOR:
            dist += 4;
            break;
      }

      if( dist >= max_dist )
      {
         act( AT_GREY, "Your vision blurs with distance and you see no " "farther $t.", ch, dir_name[dir], NULL, TO_CHAR );
         break;
      }
      if( ( pexit = get_exit( ch->in_room, dir ) ) == NULL )
      {
         act( AT_GREY, "Your view $t is blocked by a wall.", ch, dir_name[dir], NULL, TO_CHAR );
         break;
      }
   }

   char_from_room( ch );
   char_to_room( ch, was_in_room );
   learn_from_success( ch, gsn_scan );

   return;
}


/*
 * Basically the same guts as do_scan() from above (please keep them in
 * sync) used to find the victim we're firing at.	-Thoric
 */
CHAR_DATA *scan_for_victim( CHAR_DATA * ch, EXIT_DATA * pexit, const char *name )
{
   CHAR_DATA *victim;
   ROOM_INDEX_DATA *was_in_room;
   short dist, dir;
   short max_dist = 8;

   if( IS_AFFECTED( ch, AFF_BLIND ) || !pexit )
      return NULL;

   was_in_room = ch->in_room;
   if( IS_VAMPIRE( ch ) && time_info.hour < 21 && time_info.hour > 5 )
      max_dist = 1;

   if( ch->level < 50 )
      --max_dist;
   if( ch->level < 40 )
      --max_dist;
   if( ch->level < 30 )
      --max_dist;

   for( dist = 1; dist <= max_dist; )
   {
      if( IS_SET( pexit->exit_info, EX_CLOSED ) )
         break;

      if( room_is_private( pexit->to_room ) && ch->level < LEVEL_GREATER )
         break;

      char_from_room( ch );
      char_to_room( ch, pexit->to_room );

      if( ( victim = get_char_room( ch, name ) ) != NULL )
      {
         char_from_room( ch );
         char_to_room( ch, was_in_room );
         return victim;
      }

      switch ( ch->in_room->sector_type )
      {
         default:
            dist++;
            break;
         case SECT_AIR:
            if( number_percent(  ) < 80 )
               dist++;
            break;
         case SECT_INSIDE:
         case SECT_FIELD:
         case SECT_UNDERGROUND:
            dist++;
            break;
         case SECT_FOREST:
         case SECT_CITY:
         case SECT_DESERT:
         case SECT_HILLS:
            dist += 2;
            break;
         case SECT_WATER_SWIM:
         case SECT_WATER_NOSWIM:
         case SECT_OCEAN:
            dist += 3;
            break;
         case SECT_MOUNTAIN:
         case SECT_UNDERWATER:
         case SECT_OCEANFLOOR:
            dist += 4;
            break;
      }

      if( dist >= max_dist )
         break;

      dir = pexit->vdir;
      if( ( pexit = get_exit( ch->in_room, dir ) ) == NULL )
         break;
   }

   char_from_room( ch );
   char_to_room( ch, was_in_room );

   return NULL;
}

/*
 * Search inventory for an appropriate projectile to fire.
 * Also search open quivers.					-Thoric
 */
OBJ_DATA *find_projectile( CHAR_DATA * ch, int type )
{
   OBJ_DATA *obj, *obj2;

   for( obj = ch->last_carrying; obj; obj = obj->prev_content )
   {
      if( can_see_obj( ch, obj ) )
      {
         if( obj->item_type == ITEM_QUIVER && !IS_SET( obj->value[1], CONT_CLOSED ) )
         {
            for( obj2 = obj->last_content; obj2; obj2 = obj2->prev_content )
            {
               if( obj2->item_type == ITEM_PROJECTILE && obj2->value[4] == type )
                  return obj2;
            }
         }
         if( obj->item_type == ITEM_PROJECTILE && obj->value[4] == type )
            return obj;
      }
   }

   return NULL;
}


ch_ret spell_attack( int, int, CHAR_DATA *, void * );

/*
 * Perform the actual attack on a victim			-Thoric
 */
ch_ret ranged_got_target( CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * weapon,
                          OBJ_DATA * projectile, short dist, short dt, const char *stxt, short color )
{
   /* added wtype for check to determine skill used for ranged attacks - Grimm */
   short wtype = 0;

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      /*
       * safe room, bubye projectile
       */
      if( projectile )
      {
         ch_printf( ch, "Your %s is blasted from existance by a godly presense.", myobj( projectile ) );
         act( color, "A godly presence smites $p!", ch, projectile, NULL, TO_ROOM );
         extract_obj( projectile );
      }
      else
      {
         ch_printf( ch, "Your %s is blasted from existance by a godly presense.", stxt );
         act( color, "A godly presence smites $t!", ch, aoran( stxt ), NULL, TO_ROOM );
      }
      return rNONE;
   }

   if( IS_NPC( victim ) && xIS_SET( victim->act, ACT_SENTINEL ) && ch->in_room != victim->in_room )
   {
      /*
       * letsee, if they're high enough.. attack back with fireballs
       * long distance or maybe some minions... go herne! heh..
       *
       * For now, just always miss if not in same room  -Thoric
       */

      if( projectile )
      {
         /* check dam type of projectile to determine skill to use - Grimm */
         switch( projectile->value[3] )
         {
            case 13:
            case 14:
               learn_from_failure( ch, gsn_archery );
               break;

            case 15:
               learn_from_failure( ch, gsn_blowguns );
               break;

            case 16:
               learn_from_failure( ch, gsn_slings );
               break;
         }

         /* 50% chance of projectile getting lost */
         if( number_percent() < 50 )
            extract_obj(projectile);
         else
         {
            if( projectile->in_obj )
               obj_from_obj( projectile );
            if( projectile->carried_by )
               obj_from_char( projectile );
            obj_to_room( projectile, victim->in_room );
         }
      }
      return damage( ch, victim, 0, dt );
   }

   /* check dam type of projectile to determine value of wtype
    * wtype points to same "sh_int" as the skill assigned to that
    * range by the code and as such the proper skill will be used.
    * Grimm
    */
   switch( projectile->value[3] )
   {
      case 13: case 14: wtype = gsn_archery; break;
      case 15: wtype = gsn_blowguns; break;
      case 16: wtype = gsn_slings; break;
   }

   if( number_percent() > 50 || (projectile && weapon && can_use_skill(ch, number_percent(), wtype)) )
   {
      if( projectile )
         global_retcode = projectile_hit( ch, victim, weapon, projectile, dist );
      else
         global_retcode = spell_attack( dt, ch->level, ch, victim );
   }
   else
   {
      switch( projectile->value[3] )
      {
         case 13:
         case 14:
            learn_from_failure( ch, gsn_archery );
            break;

         case 15:
            learn_from_failure( ch, gsn_blowguns );
            break;

         case 16:
            learn_from_failure( ch, gsn_slings );
            break;
      }
      global_retcode = damage( ch, victim, 0, dt );

      if( projectile )
      {
         /*
          * 50% chance of getting lost
          */
         if( number_percent(  ) < 50 )
            extract_obj( projectile );
         else
         {
            if( projectile->in_obj )
               obj_from_obj( projectile );
            if( projectile->carried_by )
               obj_from_char( projectile );
            obj_to_room( projectile, victim->in_room );
         }
      }
   }
   return global_retcode;
}

/*
 * Generic use ranged attack function			-Thoric & Tricops
 */
ch_ret ranged_attack( CHAR_DATA * ch, const char *argument, OBJ_DATA * weapon, OBJ_DATA * projectile, short dt, short range )
{
   CHAR_DATA *victim, *vch;
   EXIT_DATA *pexit;
   ROOM_INDEX_DATA *was_in_room;
   char arg[MAX_INPUT_LENGTH];
   char arg1[MAX_INPUT_LENGTH];
   char temp[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   SKILLTYPE *skill = NULL;
   short dir = -1, dist = 0, color = AT_GREY;
   const char *dtxt = "somewhere";
   const char *stxt = "burst of energy";
   int count;


   if( argument && argument[0] != '\0' && argument[0] == '\'' )
   {
      one_argument( argument, temp );
      argument = temp;
   }

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg1 );

   if( arg[0] == '\0' )
   {
      send_to_char( "Where?  At who?\r\n", ch );
      return rNONE;
   }

   victim = NULL;

   /*
    * get an exit or a victim
    */
   if( ( pexit = find_door( ch, arg, TRUE ) ) == NULL )
   {
      if( ( victim = get_char_room( ch, arg ) ) == NULL )
      {
         send_to_char( "Aim in what direction?\r\n", ch );
         return rNONE;
      }
      else
      {
         if( who_fighting( ch ) == victim )
         {
            send_to_char( "They are too close to release that type of attack!\r\n", ch );
            return rNONE;
         }
         /*
          * Taken out because of waitstate
          * if ( !IS_NPC(ch) && !IS_NPC(victim) )
          * {
          * send_to_char("Pkill like a real pkiller.\r\n", ch );
          * return rNONE;
          * }
          */
      }
   }
   else
      dir = pexit->vdir;

   /*
    * check for ranged attacks from private rooms, etc
    */
   if( !victim )
   {
      if( xIS_SET( ch->in_room->room_flags, ROOM_PRIVATE ) || xIS_SET( ch->in_room->room_flags, ROOM_SOLITARY ) )
      {
         send_to_char( "You cannot perform a ranged attack from a private room.\r\n", ch );
         return rNONE;
      }
      if( ch->in_room->tunnel > 0 )
      {
         count = 0;
         for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
            ++count;
         if( count >= ch->in_room->tunnel )
         {
            send_to_char( "This room is too cramped to perform such an attack.\r\n", ch );
            return rNONE;
         }
      }
   }

   if( IS_VALID_SN( dt ) )
      skill = skill_table[dt];

   if( pexit && !pexit->to_room )
   {
      send_to_char( "Are you expecting to fire through a wall!?\r\n", ch );
      return rNONE;
   }

   /*
    * Check for obstruction
    */
   if( pexit && IS_SET( pexit->exit_info, EX_CLOSED ) )
   {
      if( IS_SET( pexit->exit_info, EX_SECRET ) || IS_SET( pexit->exit_info, EX_DIG ) )
         send_to_char( "Are you expecting to fire through a wall!?\r\n", ch );
      else
         send_to_char( "Are you expecting to fire through a door!?\r\n", ch );
      return rNONE;
   }

   vch = NULL;
   if( pexit && arg1[0] != '\0' )
   {
      if( ( vch = scan_for_victim( ch, pexit, arg1 ) ) == NULL )
      {
         send_to_char( "You cannot see your target.\r\n", ch );
         return rNONE;
      }

      /*
       * don't allow attacks on mobs stuck in another area?
       * if ( IS_NPC(vch) && xIS_SET(vch->act, ACT_STAY_AREA)
       * &&   ch->in_room->area != vch->in_room->area) )
       * {
       * }
       */
      /*
       * don't allow attacks on mobs that are in a no-missile room --Shaddai
       */
      if( xIS_SET( vch->in_room->room_flags, ROOM_NOMISSILE ) )
      {
         send_to_char( "You can't get a clean shot off.\r\n", ch );
         return rNONE;
      }
      /*
       * Taken out cause of wait state
       * if ( !IS_NPC(ch) && !IS_NPC(vch) )
       * {
       * send_to_char("Pkill like a real pkiller.\r\n", ch );
       * return rNONE;
       * }
       */

      /*
       * can't properly target someone heavily in battle
       */
      if( vch->num_fighting > MAX_FIGHT )
      {
         send_to_char( "There is too much activity there for you to get a clear shot.\r\n", ch );
         return rNONE;
      }
   }
   if( vch )
   {
      if( !IS_NPC( vch ) && !IS_NPC( ch ) && xIS_SET( ch->act, PLR_NICE ) )
      {
         send_to_char( "Your too nice to do that!\r\n", ch );
         return rNONE;
      }
      if( vch && is_safe( ch, vch, TRUE ) )
         return rNONE;
   }
   was_in_room = ch->in_room;

   if( projectile )
   {
      separate_obj( projectile );
      if( pexit )
      {
         if( weapon )
         {
            act( AT_GREY, "You fire $p $T.", ch, projectile, dir_name[dir], TO_CHAR );
            act( AT_GREY, "$n fires $p $T.", ch, projectile, dir_name[dir], TO_ROOM );
         }
         else
         {
            act( AT_GREY, "You throw $p $T.", ch, projectile, dir_name[dir], TO_CHAR );
            act( AT_GREY, "$n throw $p $T.", ch, projectile, dir_name[dir], TO_ROOM );
         }
      }
      else
      {
         if( weapon )
         {
            act( AT_GREY, "You fire $p at $N.", ch, projectile, victim, TO_CHAR );
            act( AT_GREY, "$n fires $p at $N.", ch, projectile, victim, TO_NOTVICT );
            act( AT_GREY, "$n fires $p at you!", ch, projectile, victim, TO_VICT );
         }
         else
         {
            act( AT_GREY, "You throw $p at $N.", ch, projectile, victim, TO_CHAR );
            act( AT_GREY, "$n throws $p at $N.", ch, projectile, victim, TO_NOTVICT );
            act( AT_GREY, "$n throws $p at you!", ch, projectile, victim, TO_VICT );
         }
      }
   }
   else if( skill )
   {
      if( skill->noun_damage && skill->noun_damage[0] != '\0' )
         stxt = skill->noun_damage;
      else
         stxt = skill->name;
      /*
       * a plain "spell" flying around seems boring
       */
      if( !str_cmp( stxt, "spell" ) )
         stxt = "magical burst of energy";
      if( skill->type == SKILL_SPELL )
      {
         color = AT_MAGIC;
         if( pexit )
         {
            act( AT_MAGIC, "You release $t $T.", ch, aoran( stxt ), dir_name[dir], TO_CHAR );
            act( AT_MAGIC, "$n releases $s $t $T.", ch, stxt, dir_name[dir], TO_ROOM );
         }
         else
         {
            act( AT_MAGIC, "You release $t at $N.", ch, aoran( stxt ), victim, TO_CHAR );
            act( AT_MAGIC, "$n releases $s $t at $N.", ch, stxt, victim, TO_NOTVICT );
            act( AT_MAGIC, "$n releases $s $t at you!", ch, stxt, victim, TO_VICT );
         }
      }
   }
   else
   {
      bug( "Ranged_attack: no projectile, no skill dt %d", dt );
      return rNONE;
   }

   /*
    * victim in same room
    */
   if( victim )
   {
      check_illegal_pk( ch, victim );
      check_attacker( ch, victim );
      return ranged_got_target( ch, victim, weapon, projectile, 0, dt, stxt, color );
   }

   /*
    * assign scanned victim
    */
   victim = vch;

   /*
    * reverse direction text from move_char
    */
   dtxt = rev_exit( pexit->vdir );

   while( dist <= range )
   {
      char_from_room( ch );
      char_to_room( ch, pexit->to_room );

      if( IS_SET( pexit->exit_info, EX_CLOSED ) )
      {
         /*
          * whadoyahknow, the door's closed
          */
         if( projectile )
            snprintf( buf, MAX_STRING_LENGTH, "You see your %s pierce a door in the distance to the %s.",
                      myobj( projectile ), dir_name[dir] );
         else
            snprintf( buf, MAX_STRING_LENGTH, "You see your %s hit a door in the distance to the %s.", stxt, dir_name[dir] );
         act( color, buf, ch, NULL, NULL, TO_CHAR );
         if( projectile )
         {
            snprintf( buf, MAX_STRING_LENGTH, "$p flies in from %s and implants itself solidly in the %sern door.", dtxt,
                      dir_name[dir] );
            act( color, buf, ch, projectile, NULL, TO_ROOM );
         }
         else
         {
            snprintf( buf, MAX_STRING_LENGTH, "%s flies in from %s and implants itself solidly in the %sern door.",
                      aoran( stxt ), dtxt, dir_name[dir] );
            buf[0] = UPPER( buf[0] );
            act( color, buf, ch, NULL, NULL, TO_ROOM );
         }
         break;
      }


      /*
       * no victim? pick a random one
       */
      if( !victim )
      {
         for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
         {
            if( ( ( IS_NPC( ch ) && !IS_NPC( vch ) ) || ( !IS_NPC( ch ) && IS_NPC( vch ) ) ) && number_bits( 1 ) == 0 )
            {
               victim = vch;
               break;
            }
         }
         if( victim && is_safe( ch, victim, TRUE ) )
         {
            char_from_room( ch );
            char_to_room( ch, was_in_room );
            return rNONE;
         }
      }

      /*
       * In the same room as our victim?
       */
      if( victim && ch->in_room == victim->in_room )
      {
         if( projectile )
            act( color, "$p flies in from $T.", ch, projectile, dtxt, TO_ROOM );
         else
            act( color, "$t flies in from $T.", ch, aoran( stxt ), dtxt, TO_ROOM );

         /*
          * get back before the action starts
          */
         char_from_room( ch );
         char_to_room( ch, was_in_room );

         check_illegal_pk( ch, victim );
         check_attacker( ch, victim );
         return ranged_got_target( ch, victim, weapon, projectile, dist, dt, stxt, color );
      }

      if( dist == range )
      {
         if( projectile )
         {
            act( color, "Your $t falls harmlessly to the ground to the $T.",
                 ch, myobj( projectile ), dir_name[dir], TO_CHAR );
            act( color, "$p flies in from $T and falls harmlessly to the ground here.", ch, projectile, dtxt, TO_ROOM );
            if( projectile->in_obj )
               obj_from_obj( projectile );
            if( projectile->carried_by )
               obj_from_char( projectile );
            obj_to_room( projectile, ch->in_room );
         }
         else
         {
            act( color, "Your $t fizzles out harmlessly to the $T.", ch, stxt, dir_name[dir], TO_CHAR );
            act( color, "$t flies in from $T and fizzles out harmlessly.", ch, aoran( stxt ), dtxt, TO_ROOM );
         }
         break;
      }

      if( ( pexit = get_exit( ch->in_room, dir ) ) == NULL )
      {
         if( projectile )
         {
            act( color, "Your $t hits a wall and bounces harmlessly to the ground to the $T.",
                 ch, myobj( projectile ), dir_name[dir], TO_CHAR );
            act( color, "$p strikes the $Tsern wall and falls harmlessly to the ground.",
                 ch, projectile, dir_name[dir], TO_ROOM );
            if( projectile->in_obj )
               obj_from_obj( projectile );
            if( projectile->carried_by )
               obj_from_char( projectile );
            obj_to_room( projectile, ch->in_room );
         }
         else
         {
            act( color, "Your $t harmlessly hits a wall to the $T.", ch, stxt, dir_name[dir], TO_CHAR );
            act( color, "$t strikes the $Tsern wall and falls harmlessly to the ground.",
                 ch, aoran( stxt ), dir_name[dir], TO_ROOM );
         }
         break;
      }
      if( projectile )
         act( color, "$p flies in from $T.", ch, projectile, dtxt, TO_ROOM );
      else
         act( color, "$t flies in from $T.", ch, aoran( stxt ), dtxt, TO_ROOM );
      dist++;
   }

   char_from_room( ch );
   char_to_room( ch, was_in_room );

   return rNONE;
}

/*
 * Fire <direction> <target>
 *
 * Fire a projectile from a missile weapon (bow, crossbow, etc)
 *
 * Design by Thoric, coding by Thoric and Tricops.
 *
 * Support code (see projectile_hit(), quiver support, other changes to
 * fight.c, etc by Thoric.
 */
void do_fire( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *arrow;
   OBJ_DATA *bow;
   short max_dist;

   if( argument[0] == '\0' || !str_cmp( argument, " " ) )
   {
      send_to_char( "Fire where at who?\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
   {
      set_char_color( AT_MAGIC, ch );
      send_to_char( "A magical force prevents you from attacking.\r\n", ch );
      return;
   }

   /*
    * Find the projectile weapon
    */
   if( ( bow = get_eq_char( ch, WEAR_MISSILE_WIELD ) ) != NULL )
      if( !( bow->item_type == ITEM_MISSILE_WEAPON ) )
         bow = NULL;

   if( !bow )
   {
      send_to_char( "You are not wielding a missile weapon.\r\n", ch );
      return;
   }

   /*
    * modify maximum distance based on bow-type and ch's class/str/etc
    */
   max_dist = URANGE( 1, bow->value[4], 10 );

   if( !( arrow = find_projectile( ch, bow->value[5] ) ) )
   {
      const char *msg = "You have nothing to fire...\r\n";

      switch( bow->value[5] )
      {
         case PROJ_BOLT:  msg = "You have no bolts...\r\n";	break;
         case PROJ_ARROW: msg = "You have no arrows...\r\n";	break;
         case PROJ_DART:  msg = "You have no darts...\r\n";	break;
         case PROJ_STONE: msg = "You have no slingstones...\r\n";	break;
      }
      send_to_char( msg, ch );
      return;
   }

   /*
    * Add wait state to fire for pkill, etc...
    */
   WAIT_STATE( ch, 6 );

   /*
    * handle the ranged attack
    */
   ranged_attack( ch, argument, bow, arrow, TYPE_HIT + arrow->value[3], max_dist );

   return;
}

/*
 * Attempt to fire at a victim.
 * Returns FALSE if no attempt was made
 */
bool mob_fire( CHAR_DATA * ch, const char *name )
{
   OBJ_DATA *arrow;
   OBJ_DATA *bow;
   short max_dist;

   if( xIS_SET( ch->in_room->room_flags, ROOM_SAFE ) )
      return FALSE;

   if( ( bow = get_eq_char( ch, WEAR_MISSILE_WIELD ) ) != NULL )
      if( !( bow->item_type == ITEM_MISSILE_WEAPON ) )
         bow = NULL;

   if( !bow )
      return FALSE;

   /*
    * modify maximum distance based on bow-type and ch's class/str/etc
    */
   max_dist = URANGE( 1, bow->value[4], 10 );

   if( ( arrow = find_projectile( ch, bow->value[5] ) ) == NULL )
      return FALSE;

   ranged_attack( ch, name, bow, arrow, TYPE_HIT + arrow->value[3], max_dist );

   return TRUE;
}

/* -- working on --
 * Syntaxes: throw object  (assumed already fighting)
 *	     throw object direction target  (all needed args for distance
 *	          throwing)
 *	     throw object  (assumed same room throw)

void do_throw( CHAR_DATA* ch, const char* argument)
{
  ROOM_INDEX_DATA *was_in_room;
  CHAR_DATA *victim;
  OBJ_DATA *throw_obj;
  EXIT_DATA *pexit;
  short dir;
  short dist;
  short max_dist = 3;
  char arg[MAX_INPUT_LENGTH];
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  argument = one_argument( argument, arg );
  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  for ( throw_obj = ch->last_carrying; throw_obj;
	throw_obj = throw_obj=>prev_content )
  {
---    if ( can_see_obj( ch, throw_obj )
	&& ( throw_obj->wear_loc == WEAR_HELD || throw_obj->wear_loc ==
	WEAR_WIELDED || throw_obj->wear_loc == WEAR_DUAL_WIELDED )
	&& nifty_is_name( arg, throw_obj->name ) )
      break;
 ----
    if ( can_see_obj( ch, throw_obj ) && nifty_is_name( arg, throw_obj->name )
      break;
  }

  if ( !throw_obj )
  {
    send_to_char( "You aren't holding or wielding anything like that.\r\n", ch );
    return;
  }

----
  if ( ( throw_obj->item_type != ITEM_WEAPON)
  {
    send_to_char("You can only throw weapons.\r\n", ch );
    return;
  }
----

  if (get_obj_weight( throw_obj ) - ( 3 * (get_curr_str(ch) - 15) ) > 0)
  {
    send_to_char("That is too heavy for you to throw.\r\n", ch);
    if (!number_range(0,10))
      learn_from_failure( ch, gsn_throw );
    return;
  }

  if ( ch->fighting )
    victim = ch->fighting;
   else
    {
      if ( ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
  	&& ( arg2[0] == '\0' ) )
      {
        act( AT_GREY, "Throw $t at whom?", ch, obj->short_descr, NULL,
	  TO_CHAR );
        return;
      }
    }
}*/

void do_slice( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *corpse;
   OBJ_DATA *obj;
   OBJ_DATA *slice;
   bool found;
   MOB_INDEX_DATA *pMobIndex;
   char buf[MAX_STRING_LENGTH];
   found = FALSE;

   if( !IS_NPC( ch ) && !IS_IMMORTAL( ch ) && ch->level < skill_table[gsn_slice]->skill_level[ch->Class] )
   {
      send_to_char( "You are not learned in this skill.\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "From what do you wish to slice meat?\r\n", ch );
      return;
   }


   if( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL
       || ( obj->value[3] != 1 && obj->value[3] != 2 && obj->value[3] != 3 && obj->value[3] != 11 ) )
   {
      send_to_char( "You need to wield a sharp weapon.\r\n", ch );
      return;
   }

   if( ( corpse = get_obj_here( ch, argument ) ) == NULL )
   {
      send_to_char( "You can't find that here.\r\n", ch );
      return;
   }

   if( corpse->item_type != ITEM_CORPSE_NPC || corpse->value[3] < 75 )
   {
      send_to_char( "That is not a suitable source of meat.\r\n", ch );
      return;
   }

   if( ( pMobIndex = get_mob_index( ( short )-( corpse->value[2] ) ) ) == NULL )
   {
      bug( "%s", "Can not find mob for value[2] of corpse, do_slice" );
      return;
   }

   if( get_obj_index( OBJ_VNUM_SLICE ) == NULL )
   {
      bug( "Vnum %d not found for do_slice!", OBJ_VNUM_SLICE );
      return;
   }

   slice = create_object( get_obj_index( OBJ_VNUM_SLICE ), 0 );

   snprintf( buf, MAX_STRING_LENGTH, "meat fresh slice %s", pMobIndex->player_name );
   STRFREE( slice->name );
   slice->name = STRALLOC( buf );

   snprintf( buf, MAX_STRING_LENGTH, "a slice of raw meat from %s", pMobIndex->short_descr );
   STRFREE( slice->short_descr );
   slice->short_descr = STRALLOC( buf );

   snprintf( buf, MAX_STRING_LENGTH, "A slice of raw meat from %s lies on the ground.", pMobIndex->short_descr );
   STRFREE( slice->description );
   slice->description = STRALLOC( buf );

   act( AT_BLOOD, "$n cuts a slice of meat from $p.", ch, corpse, NULL, TO_ROOM );
   act( AT_BLOOD, "You cut a slice of meat from $p.", ch, corpse, NULL, TO_CHAR );

   obj_to_char( slice, ch );
   corpse->value[3] -= 25;
   learn_from_success( ch, gsn_slice );
   return;
}

/*------------------------------------------------------------
 *  Fighting Styles - haus
 */ void do_style( CHAR_DATA * ch, const char *argument )
{
   char arg[MAX_INPUT_LENGTH];
/*  char buf[MAX_INPUT_LENGTH];
    int percent; */

   if( IS_NPC( ch ) )
      return;

   one_argument( argument, arg );
   if( arg[0] == '\0' )
   {
      ch_printf_color( ch, "&wAdopt which fighting style?  (current:  %s&w)\r\n",
                       ch->style == STYLE_BERSERK ? "&Rberserk" :
                       ch->style == STYLE_AGGRESSIVE ? "&Raggressive" :
                       ch->style == STYLE_DEFENSIVE ? "&Ydefensive" :
                       ch->style == STYLE_EVASIVE ? "&Yevasive" : "standard" );
      return;
   }

   if( !str_prefix( arg, "evasive" ) )
   {
         /*
          * success
          */
         if( ch->fighting )
         {
            ch->position = POS_EVASIVE;
            if( IS_PKILL( ch ) )
               act( AT_ACTION, "$n falls back into an evasive stance.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_EVASIVE;
         send_to_char( "You adopt an evasive fighting style.\r\n", ch );
         return;
   }
   else if( !str_prefix( arg, "defensive" ) )
   {
         /*
          * success
          */
         if( ch->fighting )
         {
            ch->position = POS_DEFENSIVE;
            if( IS_PKILL( ch ) )
               act( AT_ACTION, "$n moves into a defensive posture.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_DEFENSIVE;
         send_to_char( "You adopt a defensive fighting style.\r\n", ch );
         return;
   }
   else if( !str_prefix( arg, "standard" ) )
   {
         /*
          * success
          */
         if( ch->fighting )
         {
            ch->position = POS_FIGHTING;
            if( IS_PKILL( ch ) )
               act( AT_ACTION, "$n switches to a standard fighting style.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_FIGHTING;
         send_to_char( "You adopt a standard fighting style.\r\n", ch );
         return;
   }
   else if( !str_prefix( arg, "aggressive" ) )
   {
         /*
          * success
          */
         if( ch->fighting )
         {
            ch->position = POS_AGGRESSIVE;
            if( IS_PKILL( ch ) )
               act( AT_ACTION, "$n assumes an aggressive stance.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_AGGRESSIVE;
         send_to_char( "You adopt an aggressive fighting style.\r\n", ch );
         return;
   }
   else if( !str_prefix( arg, "berserk" ) )
   {
         /*
          * success
          */
         if( ch->fighting )
         {
            ch->position = POS_BERSERK;
            if( IS_PKILL( ch ) )
               act( AT_ACTION, "$n enters a wildly aggressive style.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_BERSERK;
         send_to_char( "You adopt a berserk fighting style.\r\n", ch );
         return;
   }

   send_to_char( "Adopt which fighting style?\r\n", ch );

   return;
}

/*  New check to see if you can use skills to support morphs --Shaddai */
bool can_use_skill( CHAR_DATA * ch, int percent, int gsn )
{
   bool check = FALSE;
   if( IS_NPC( ch ) && percent < 85 )
      check = TRUE;
   else if( !IS_NPC( ch ) && percent < LEARNED( ch, gsn ) )
      check = TRUE;
   else if( ch->morph && ch->morph->morph && ch->morph->morph->skills &&
            ch->morph->morph->skills[0] != '\0' &&
            is_name( skill_table[gsn]->name, ch->morph->morph->skills ) && percent < 85 )
      check = TRUE;
   if( ch->morph && ch->morph->morph && ch->morph->morph->no_skills &&
       ch->morph->morph->no_skills[0] != '\0' && is_name( skill_table[gsn]->name, ch->morph->morph->no_skills ) )
      check = FALSE;
   return check;
}

/*
 * Cook was coded by Blackmane and heavily modified by Shaddai
 */

void do_cook( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *food, *fire;
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];

   one_argument( argument, arg );
   if( IS_NPC( ch ) || ch->level < skill_table[gsn_cook]->skill_level[ch->Class] )
   {
      send_to_char( "That skill is beyond your understanding.\r\n", ch );
      return;
   }
   if( arg[0] == '\0' )
   {
      send_to_char( "Cook what?\r\n", ch );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( food = get_obj_carry( ch, arg ) ) == NULL )
   {
      send_to_char( "You do not have that item.\r\n", ch );
      return;
   }
   if( food->item_type != ITEM_COOK )
   {
      send_to_char( "How can you cook that?\r\n", ch );
      return;
   }
   if( food->value[2] > 2 )
   {
      send_to_char( "That is already burnt to a crisp.\r\n", ch );
      return;
   }
   for( fire = ch->in_room->first_content; fire; fire = fire->next_content )
   {
      if( fire->item_type == ITEM_FIRE )
         break;
   }
   if( !fire )
   {
      send_to_char( "There is no fire here!\r\n", ch );
      return;
   }
   separate_obj( food );   /* Bug catch by Tchaika from SMAUG list */
/*
   if( number_percent(  ) > LEARNED( ch, gsn_cook ) )
   {
      food->timer = food->timer / 2;
      food->value[0] = 0;
      food->value[2] = 3;
      act( AT_MAGIC, "$p catches on fire burning it to a crisp!\r\n", ch, food, NULL, TO_CHAR );
      act( AT_MAGIC, "$n catches $p on fire burning it to a crisp.", ch, food, NULL, TO_ROOM );
      snprintf( buf, MAX_STRING_LENGTH, "a burnt %s", food->pIndexData->name );
      STRFREE( food->short_descr );
      food->short_descr = STRALLOC( buf );
      snprintf( buf, MAX_STRING_LENGTH, "A burnt %s.", food->pIndexData->name );
      STRFREE( food->description );
      food->description = STRALLOC( buf );
      return;
   }
*/
   if( LEARNED( ch, gsn_cook ) < 50 )
   {
      food->timer = food->timer * 3;
      food->value[2] += 2;
      act( AT_MAGIC, "$n overcooks a $p.", ch, food, NULL, TO_ROOM );
      act( AT_MAGIC, "You overcook a $p.", ch, food, NULL, TO_CHAR );
      snprintf( buf, MAX_STRING_LENGTH, "an overcooked %s", food->pIndexData->name );
      STRFREE( food->short_descr );
      food->short_descr = STRALLOC( buf );
      snprintf( buf, MAX_STRING_LENGTH, "An overcooked %s.", food->pIndexData->name );
      STRFREE( food->description );
      food->description = STRALLOC( buf );
   }
   else
   {
      food->timer = food->timer * 4;
      food->value[0] *= 2;
      act( AT_MAGIC, "$n roasts a $p.", ch, food, NULL, TO_ROOM );
      act( AT_MAGIC, "You roast a $p.", ch, food, NULL, TO_CHAR );
      snprintf( buf, MAX_STRING_LENGTH, "a roasted %s", food->pIndexData->name );
      STRFREE( food->short_descr );
      food->short_descr = STRALLOC( buf );
      snprintf( buf, MAX_STRING_LENGTH, "A roasted %s.", food->pIndexData->name );
      STRFREE( food->description );
      food->description = STRALLOC( buf );
      food->value[2]++;
   }
   learn_from_success( ch, gsn_cook );
}

void do_makecontainer( CHAR_DATA* ch, const char* argument )
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int level = 0;
    int vnum;
//    int chance;
    bool checksew, checkfab;
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObjIndex;
    int value;



    argument = one_argument( argument, arg );
    strcpy( arg2 , argument );

    if( !str_cmp( arg, "eyes" )
     || !str_cmp( arg, "ears" )
     || !str_cmp( arg, "finger" )
     || !str_cmp( arg, "neck" )
     || !str_cmp( arg, "wrist" ) )
    {
    	send_to_char( "You cannot make a container for that body part.\n\r", ch);
        send_to_char( "Try MAKEJEWELRY.\n\r", ch);
        return;
    }
    if( !str_cmp( arg, "feet" )
     || !str_cmp( arg, "hands" )
     || !str_cmp( arg, "face" ) )
    {
        send_to_char( "You cannot make a container for that body part.\n\r", ch);
        send_to_char( "Try MAKEARMOR.\n\r", ch);
        return;
    }
    if( !str_cmp( arg, "shield" ) )
    {
        send_to_char( "You cannot make a container a shield.\n\r", ch);
        send_to_char( "Try MAKESHIELD.\n\r", ch);
        return;
    }
    if ( !str_cmp( arg, "wield" ) )
    {
        send_to_char( "Are you going to fight with a container?\n\r", ch);
        send_to_char( "Try MAKEBLADE...\n\r", ch);
        return;
    }

    switch( ch->substate )
    {
    	default:
			if ( arg2[0] == '\0' )
            {
            	send_to_char( "Usage: Makecontainer <wearloc> <name>\n\r", ch);
            	return;
            }
            if( is_profane( arg2 ) )
            {
                send_to_char("Your name has been tested and found to be profane in some way!\r\n", ch);
                send_to_char("If this was not profane, e-mail keylord16@gmail.com\r\n", ch);
                return;
            }

/*  Starts by setting variables to FALSE, for testing purposes, TRUE instead -Danny */
    	    checksew = TRUE;
            checkfab = TRUE;

/*	Checks if room is a factory -Danny

			if ( !IS_SET( ch->in_room->room_flags, ROOM_FACTORY ) )
          	{
            	send_to_char( "You need to be in a factory or workshop to do that.\n\r", ch);
                return;
            }
*/

            if( strlen( arg2 ) > 70)
            {
            	send_to_char("You can't make items with names longer than 70 characters!\r\n", ch);
            	return;
            }

/*	Looks for ITEM_FABRIC(ITEM_MATERIAL_FABRIC?) and ITEM_THREAD -Danny
            for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
            {
            	if (obj->item_type == ITEM_FABRIC)
                    checkfab = TRUE;
                if (obj->item_type == ITEM_THREAD)
          	    	checksew = TRUE;
            }
*/
            if( !checkfab )
            {
            	send_to_char( "You need some sort of fabric or material.\n\r", ch);
           		return;
    		}

       		if( !checksew )
        	{
          		send_to_char( "You need a needle and some thread.\n\r", ch);
         		return;
         	}
/*	Checks to see if your skill is high enough -Danny
    		chance = IS_NPC(ch) ? ch->top_level : (int) (ch->pcdata->learned[gsn_makecontainer]);
         	if( number_percent( ) < chance )
    		{
*/
    		   	send_to_char( "You begin the long process of creating a bag.\n\r", ch);
    		   	act( AT_PLAIN, "$n takes $s sewing kit and some material and begins to work.", ch, NULL, argument , TO_ROOM );
		   		add_timer ( ch , TIMER_DO_FUN , 10 , do_makecontainer , 1 );
    		  	ch->dest_buf = str_dup(arg);
    		  	ch->dest_buf_2 = str_dup(arg2);
    		  	return;
/*	Can't fail, this is only a test. -Danny
	        }
	     	send_to_char("&RYou can't figure out what to do.\n\r",ch);
	      	learn_from_failure( ch, gsn_makecontainer );
    	  	return;
*/
		case 1:
    		if( !ch->dest_buf )
    			return;
    		if( !ch->dest_buf_2 )
    		    return;
    		strcpy(arg, ( char * )ch->dest_buf);
    		DISPOSE( ch->dest_buf);
    		strcpy(arg2, ( char * )ch->dest_buf_2);
    		DISPOSE( ch->dest_buf_2);
    		break;

  		case SUB_TIMER_DO_ABORT:
    		DISPOSE( ch->dest_buf );
    		DISPOSE( ch->dest_buf_2 );
    		ch->substate = SUB_NONE;
    	        send_to_char("You are interupted and fail to finish your work.\n\r", ch);
    	        return;
    }

    ch->substate = SUB_NONE;
    vnum = 123;

    if ( ( pObjIndex = get_obj_index( vnum ) ) == NULL )
    {
		send_to_char( "The item you are trying to create is missing from the database.\n\rPlease inform the administration of this error.\n\r", ch );
  		return;
    }

//    level = IS_NPC(ch) ? ch->top_level : (int) (ch->pcdata->learned[gsn_makecontainer]);

/*	Checks for needle and fabric again for some reason -Danny
    checksew = FALSE;
    checkfab = FALSE;

    for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
    {
       if (obj->item_type == ITEM_THREAD)
          checksew = TRUE;
       if (obj->item_type == ITEM_FABRIC && checkfab == FALSE)
       {
          checkfab = TRUE;
          separate_obj( obj );
          obj_from_char( obj );
          material = obj;
       }
    }
*/
/*
    chance = IS_NPC(ch) ? ch->top_level : (int) (ch->pcdata->learned[gsn_makecontainer]) ;

    if( number_percent( ) > chance*2 )
    {
       send_to_char( "You hold up your newly created container.\n\r", ch);
       send_to_char( "It suddenly dawns upon you that you have created the most useless\n\r", ch);
       send_to_char( "container you've ever seen. You quickly hide your mistake...\n\r", ch);
       learn_from_failure( ch, gsn_makecontainer );
       return;
    }
*/
    obj = create_object( pObjIndex, level );

    obj->item_type = ITEM_CONTAINER;
    SET_BIT( obj->wear_flags, ITEM_TAKE );
    value = get_wflag( arg );
    if ( value < 0 || value > 31 )
        SET_BIT( obj->wear_flags, ITEM_HOLD );
    else
        SET_BIT( obj->wear_flags, 1 << value );
//    obj->level = level;
    STRFREE( obj->name );
    strcpy( buf, arg2 );
    obj->name = STRALLOC( buf );
    strcpy( buf, arg2 );
    STRFREE( obj->short_descr );
    obj->short_descr = STRALLOC( buf );
    STRFREE( obj->description );
    strcat( buf, " was dropped here." );
    obj->description = STRALLOC( buf );
    obj->value[0] = level;
    obj->value[1] = 0;
    obj->value[2] = 0;
    obj->value[3] = 10;
    obj->cost *= 2;

    obj = obj_to_char( obj, ch );

    send_to_char( "&GYou finish your work and hold up your newly created container.&w\n\r", ch);
    act( AT_PLAIN, "$n finishes sewing a new container.", ch, NULL, argument , TO_ROOM );
//    learn_from_success( ch, gsn_makecontainer );
}

/* New command to view a player's skills - Samson 4-13-98 */
void do_viewskills( CHAR_DATA * ch, const char *argument )
{
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *victim;
   int sn, col;

   if( !argument || argument[0] == '\0' )
   {
      send_to_char( "&zSyntax: skills <player>.\r\n", ch );
      return;
   }

   if( !( victim = get_char_world( ch, argument ) ) )
   {
      send_to_char( "No such person in the game.\r\n", ch );
      return;
   }

   col = 0;

   if( !IS_NPC( victim ) )
   {
      set_char_color( AT_MAGIC, ch );
      for( sn = 0; sn < num_skills && skill_table[sn] && skill_table[sn]->name; sn++ )
      {
         if( skill_table[sn]->name == NULL )
            break;
         if( victim->pcdata->learned[sn] == 0 )
            continue;

         snprintf( buf, MAX_STRING_LENGTH, "%20s %3d%% ", skill_table[sn]->name, victim->pcdata->learned[sn] );
         send_to_char( buf, ch );

         if( ++col % 3 == 0 )
            send_to_char( "\r\n", ch );
      }
   }
   return;
}

