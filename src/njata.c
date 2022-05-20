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
 *	                Additional Njata skills and commands                    *
 ***************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "mud.h"

/******************
 *    COMMANDS    *
 ******************/

/* Toggle "Mountable" flag. Used to allow players to become mountable */
void do_mountable( CHAR_DATA* ch, const char* argument)
{
   if( !IS_NPC( ch ) )
      if( ch->race == RACE_UNICORN || ch->race == RACE_DRAGON || ch->race == RACE_GRYPHON
      							   || ch->race == RACE_SPHINX || ch->race == RACE_CENTAUR )
      {
         if( xIS_SET( ch->act, PLR_MOUNTABLE ) )
         {
            xREMOVE_BIT( ch->act, PLR_MOUNTABLE );
            send_to_char( "You are no longer accepting riders.\r\n", ch );
            return;
         }
         else
         {
            xSET_BIT( ch->act, PLR_MOUNTABLE );
            send_to_char( "You will now allow another to ride upon your back.\r\n", ch );
            return;
         }
      }
      else
      {
		 send_to_char( "You're much too small to bear such a burden, but it's a kind thought.\r\n", ch );
		 return;
	  }
   else
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
}

/* Buck command. Allows players to get rid of their riders when they grow tired of them! */
void do_buck( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *victim;

   if( ch->race != RACE_UNICORN && ch->race != RACE_DRAGON && ch->race != RACE_GRYPHON
      					        && ch->race != RACE_SPHINX && ch->race != RACE_CENTAUR )
   {
      send_to_char( "You can't bear a rider, so what would be the point?\r\n", ch );
      return;
   }

   if( ( victim = ch->rider ) == NULL )
   {
      act( AT_SKILL, "You buck wildly about.", ch, NULL, NULL, TO_CHAR );
      act( AT_SKILL, "$n bucks wildly about.", ch, NULL, NULL, TO_ROOM );
      return;
   }

   act( AT_SKILL, "You buck and throw $N off!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n bucks and throws $N off.", ch, NULL, victim, TO_NOTVICT );
   act( AT_SKILL, "$n bucks and throws you off. Ouch!", ch, NULL, victim, TO_VICT );
   if ( IS_NPC( ch ) )
      xREMOVE_BIT( ch->act, ACT_MOUNTED );
   else
      xREMOVE_BIT( ch->act, PLR_MOUNTED );
   victim->mount = NULL;
   ch->rider = NULL;
   victim->position = POS_SITTING;
   return;
}

/* Allows certain races to start flying if they're on the ground */
void do_fly( CHAR_DATA* ch, const char* argument)
{
   if( !IS_NPC( ch ) )
      if( ch->race == RACE_FAIRY || ch->race == RACE_SPHINX || ch->race == RACE_GRYPHON || ch->race == RACE_DRAGON )
      {
         if( IS_AFFECTED( ch, AFF_FLYING ) )
         {
            send_to_char( "You are already flying. Perhaps you meant to land?\r\n", ch );
            return;
         }
         else
         {
            if( ch->race == RACE_DRAGON || ch->race == RACE_GRYPHON || ch->race == RACE_SPHINX )
            {
      		   act( AT_ACTION, "You leap into the air, beating your mighty wings, and begin to fly.", ch, NULL, NULL, TO_CHAR );
      		   act( AT_ACTION, "$n leaps into the air, beating $s wings, and begins to fly.", ch, NULL, NULL, TO_ROOM );
		    }
            else
            {
      		   act( AT_ACTION, "You leap into the air, wings beating, and flitter about.", ch, NULL, NULL, TO_CHAR );
      		   act( AT_ACTION, "$n leaps into the air, wings beating, and flitters about.", ch, NULL, NULL, TO_ROOM );
		    }
            xSET_BIT( ch->affected_by, AFF_FLYING );
            return;
         }
      }
      else
      {
		 act( AT_ACTION, "You flap your limbs, but try as you might, nothing happens.", ch, NULL, NULL, TO_CHAR );
      	 act( AT_ACTION, "$n flaps $s limbs, but nothing seems to happen.", ch, NULL, NULL, TO_ROOM );
		 return;
	  }
   else
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
}

/* Allows players to land on the ground if they're flying */
void do_land( CHAR_DATA* ch, const char* argument)
{
   if( !IS_NPC( ch ) )
      if( ch->race == RACE_FAIRY || ch->race == RACE_SPHINX || ch->race == RACE_GRYPHON || ch->race == RACE_DRAGON )
      {
         if( !IS_AFFECTED( ch, AFF_FLYING ) )
         {
            send_to_char( "You're already on the ground. It doesn't get much lower than that.\r\n", ch );
            return;
         }
         else
         {
            if( ch->race == RACE_FAIRY || ch->race == RACE_SPHINX || ch->race == RACE_GRYPHON )
            {
      		   act( AT_ACTION, "You alight noiselessly upon the ground.", ch, NULL, NULL, TO_CHAR );
      		   act( AT_ACTION, "$n alights noiselessly upon the ground.", ch, NULL, NULL, TO_ROOM );
		    }
            else
            {
      		   act( AT_ACTION, "You circle slowly to the ground and fold your wings upon your back.", ch, NULL, NULL, TO_CHAR );
      		   act( AT_ACTION, "$n circles slowly to the ground and folds $s wings upon $s back.", ch, NULL, NULL, TO_ROOM );
		    }
            xREMOVE_BIT( ch->affected_by, AFF_FLYING );
            affect_strip( ch, gsn_flight );
            return;
         }
      }
      else
      {
	     if( IS_AFFECTED( ch, AFF_FLYING ) )
	     {
	        xREMOVE_BIT( ch->affected_by, AFF_FLYING );
	        affect_strip( ch, gsn_flight );
	        act( AT_ACTION, "You softly float to the ground, ending the spell.", ch, NULL, NULL, TO_CHAR );
      		act( AT_ACTION, "$n softly floats to the ground.", ch, NULL, NULL, TO_ROOM );
		    return;
         }
         else
	  	 {
	  	    send_to_char( "You're not flying. Are you all right?\r\n", ch );
		    return;
	     }
	  }
   else
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
}

/* Landwalk command for merfolk to grow a pair of legs */
void do_landwalk( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *tail;

   if( !IS_NPC( ch ) )
      if( ch->race == RACE_MERFOLK )
      {
         if( xIS_SET( ch->act, PLR_HASLEGS ) )
         {
            send_to_char( "You already have a pair of legs, maybe you meant to revert?\r\n", ch );
            return;
         }
         else
         {
            xSET_BIT( ch->act, PLR_HASLEGS );
            tail = get_eq_char( ch, WEAR_TAIL );
            if( tail )
            {
			   obj_from_char( tail );
               obj_to_room( tail, ch->in_room );
		    }
      		act( AT_ACTION, "Your tail shimmers and transforms into a pair of humanoid legs.", ch, NULL, NULL, TO_CHAR );
      		act( AT_ACTION, "$n transforms $s tail into a pair of humanoid legs.", ch, NULL, NULL, TO_ROOM );
      		return;
         }
      }
      else
      {
		 send_to_char( "You already possess the ability to walk on land!\r\n", ch );
		 return;
	  }
   else
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
}

/* Revert command allowing merfolk to ditch their legs and revert back */
void do_revert( CHAR_DATA* ch, const char* argument)
{
   OBJ_DATA *legs, *feet;

   if( !IS_NPC( ch ) )
      if( ch->race == RACE_MERFOLK )
      {
         if( xIS_SET( ch->act, PLR_HASLEGS ) )
         {
            xREMOVE_BIT( ch->act, PLR_HASLEGS );
            legs = get_eq_char( ch, WEAR_LEGS );
            feet = get_eq_char( ch, WEAR_FEET );
            if( legs )
            {
			   obj_from_char( legs );
               obj_to_room( legs, ch->in_room );
		    }
            if( feet )
            {
			   obj_from_char( feet );
               obj_to_room( feet, ch->in_room );
		    }
      		act( AT_ACTION, "Your legs glow and transform into a scaled merfolk tail.", ch, NULL, NULL, TO_CHAR );
      		act( AT_ACTION, "$n transforms $s legs into a scaled merfolk tail.", ch, NULL, NULL, TO_ROOM );
            return;
         }
         else
         {
      		send_to_char( "You're already in your natural form.\r\n", ch );
            return;
         }
      }
      else
      {
		 send_to_char( "You're already in your natural form.\r\n", ch );
		 return;
	  }
   else
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
}

/* Breathweapon command for dragons and salamanders */
void do_breathweapon( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't do that right now.\r\n", ch );
      return;
   }

   if( ch->race != RACE_SALAMANDER && ch->race != RACE_DRAGON )
   {
      send_to_char( "You lack the right esophagus for that.\r\n", ch );
      return;
   }

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      if( ( victim = who_fighting( ch ) ) == NULL )
      {
         send_to_char( "Use your breath weapon on whom?\r\n", ch );
         return;
      }

      WAIT_STATE( ch, skill_table[gsn_fireball]->beats );
///      if( can_use_skill( ch, number_percent(  ), gsn_kick ) )
///      {
///         learn_from_success( ch, gsn_kick );
      	 global_retcode = spell_fire_breath( skill_lookup( "fire breath" ), ch->level, ch, victim );
      	 check_illegal_pk( ch, victim );
//      }
//      else
//      {
//         learn_from_failure( ch, gsn_kick );
//         global_retcode = damage( ch, victim, 0, gsn_fireball );
//         check_illegal_pk( ch, victim );
//      }
   	  return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You're pretty sure that's not a good idea...\r\n", ch );
      return;
   }

   if( is_safe( ch, victim, TRUE ) )
      return;

   check_attacker( ch, victim );
   WAIT_STATE( ch, skill_table[gsn_fireball]->beats );
//   if( !IS_AWAKE( victim ) || can_use_skill( ch, percent, gsn_backstab ) )
//   {
//      learn_from_success( ch, gsn_fireball );
	  global_retcode = spell_fire_breath( skill_lookup( "fire breath" ), ch->level, ch, victim );
      check_illegal_pk( ch, victim );

//   }
//   else
//   {
//      learn_from_failure( ch, gsn_fireball );
//      global_retcode = damage( ch, victim, 0, gsn_fireball );
//      check_illegal_pk( ch, victim );
//   }
   return;
}

/* Toggle "PVP" flag. Let's players pvp each other */
void do_pvpflag( CHAR_DATA* ch, const char* argument)
{
   if( !IS_NPC( ch ) )
   {
      if( IS_SET( ch->pcdata->flags, PCFLAG_PVP ) )
      {
         REMOVE_BIT( ch->pcdata->flags, PCFLAG_PVP );
         send_to_char( "You are no longer in PVP mode.\r\n", ch );
         return;
      }
      else
      {
         SET_BIT( ch->pcdata->flags, PCFLAG_PVP );
         send_to_char( "You are now in PVP mode.\r\n", ch );
         return;
      }
   }
   else
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
}

/* Lets player set their hair description */
void do_hair( CHAR_DATA* ch, const char* argument)
{
   if( !IS_NPC( ch ) )
   {
      if( argument[0] == '\0' )
      {
         if( ch->pcdata->hair && ch->pcdata->hair[0] != '\0' )
            ch_printf( ch, "Your hair description is: %s\r\n", ch->pcdata->hair );
         else
            send_to_char( "You have no hair description.\r\n", ch );
         return;
      }

      if( !str_cmp( argument, "clear" ) )
      {
         if( ch->pcdata->hair )
            DISPOSE( ch->pcdata->hair );
         ch->pcdata->hair = str_dup( "" );
         send_to_char( "Hair description cleared.\r\n", ch );
         return;
      }

      char* newhair = str_dup(argument);
      smash_tilde(newhair);
      DISPOSE( ch->pcdata->hair );
      ch->pcdata->hair = newhair;
      ch_printf( ch, "Your hair description is: %s\r\n", ch->pcdata->hair );
   }
   return;
}

/* Lets player set their eyes description */
void do_eyes( CHAR_DATA* ch, const char* argument)
{
   if( !IS_NPC( ch ) )
   {
      if( argument[0] == '\0' )
      {
         if( ch->pcdata->eyes && ch->pcdata->eyes[0] != '\0' )
            ch_printf( ch, "Your eyes description is: %s\r\n", ch->pcdata->eyes );
         else
            send_to_char( "You have no eyes description.\r\n", ch );
         return;
      }

      if( !str_cmp( argument, "clear" ) )
      {
         if( ch->pcdata->eyes )
            DISPOSE( ch->pcdata->eyes );
         ch->pcdata->eyes = str_dup( "" );
         send_to_char( "Eyes description cleared.\r\n", ch );
         return;
      }

      char* neweyes = str_dup(argument);
      smash_tilde(neweyes);
      DISPOSE( ch->pcdata->eyes );
      ch->pcdata->eyes = neweyes;
      ch_printf( ch, "Your eyes description is: %s\r\n", ch->pcdata->eyes );
   }
   return;
}



/******************
 *     SKILLS     *
 ******************/

void do_forage( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   int percent;

   switch ( ch->substate )
   {
      default:
         if( IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
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
         add_timer( ch, TIMER_DO_FUN, UMIN( skill_table[gsn_mount]->beats / 10, 3 ), do_forage, 1 );
         send_to_char( "You begin foraging...\r\n", ch );
         ch->alloc_ptr = str_dup( arg );
         return;

      case 1:
         if( !ch->alloc_ptr )
         {
            send_to_char( "Your foraging was interrupted!\r\n", ch );
            bug( "%s", "do_forage: alloc_ptr NULL" );
            return;
         }
         mudstrlcpy( arg, ch->alloc_ptr, MAX_INPUT_LENGTH );
         DISPOSE( ch->alloc_ptr );
         break;
      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You stop foraging.\r\n", ch );
         return;
   }
   ch->substate = SUB_NONE;

   percent = number_percent(  ) + number_percent(  ) - ( ch->level / 10 );

   if( ch->pcdata->learned[gsn_forage] > 0 )
   {
      if( ch->in_room->sector_type == SECT_FOREST )
      {
         OBJ_DATA *sticks;

         sticks = create_object( get_obj_index( OBJ_VNUM_PILE_OF_STICKS ), 0 );
         act( AT_PLAIN, "Through clever foraging, $n has found $p.", ch, sticks, NULL, TO_ROOM );
         act( AT_PLAIN, "Through clever foraging, you find $p.", ch, sticks, NULL, TO_CHAR );
         sticks = obj_to_room( sticks, ch->in_room );
         learn_from_success( ch, gsn_forage );
         return;
	  }
      send_to_char( "You find nothing.\r\n", ch );
      return;
   }
   send_to_char( "You find nothing.\r\n", ch );
   return;
}

void do_make( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   int vnum;
   bool checkflint, checktinder;
   OBJ_DATA *obj;
   OBJ_INDEX_DATA *pObjIndex;

   switch ( ch->substate )
   {
      default:
         if( IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
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

         if( !str_cmp( arg, "fire" ) )
         {
            checkflint = FALSE;
            checktinder = FALSE;

            for( obj = ch->last_carrying; obj; obj = obj->prev_content )
            {
               if(obj->item_type == ITEM_FLINT)
                  checkflint = TRUE;
               if (obj->item_type == ITEM_TINDER)
                  checktinder = TRUE;
            }

            if( !checkflint && !checktinder )
            {
               send_to_char( "You need some flint and tinder to build a fire.\n\r", ch);
               return;
            }
            else if( !checkflint )
            {
               send_to_char( "You need some flint to build a fire with.\n\r", ch);
               return;
	        }
            else if( !checktinder )
            {
               send_to_char( "You need some wood to burn to build a fire.\n\r", ch);
               return;
            }
         }
         else
         {
            send_to_char( "You can't make that, see &Ghelp make &wfor the complete list.\n\r", ch);
            return;
         }
         add_timer( ch, TIMER_DO_FUN, UMIN( skill_table[gsn_mount]->beats / 10, 3 ), do_make, 1 );
         send_to_char( "You kneel down and begin to build a fire.\n\r", ch);
    	 act( AT_PLAIN, "$n takes $s flint and tinder and begins to build a fire.", ch, NULL, argument , TO_ROOM );
         ch->alloc_ptr = str_dup( arg );
         return;

      case 1:
         if( !ch->alloc_ptr )
         {
            send_to_char( "Your fire building was interrupted!\r\n", ch );
            bug( "%s", "do_make: alloc_ptr NULL" );
            return;
         }
         mudstrlcpy( arg, ch->alloc_ptr, MAX_INPUT_LENGTH );
         DISPOSE( ch->alloc_ptr );
         break;
      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You stop building a fire.\r\n", ch );
         return;
   }
   ch->substate = SUB_NONE;

   if( !str_cmp( arg, "fire" ) )
   {
	  vnum = 29;

      checkflint = FALSE;
      checktinder = FALSE;

      for ( obj = ch->last_carrying; obj; obj = obj->prev_content )
      {
         if (obj->item_type == ITEM_FLINT)
            checkflint = TRUE;
         if (obj->item_type == ITEM_TINDER && checktinder == FALSE)
         {
            checktinder = TRUE;
            separate_obj( obj );
            obj_from_char( obj );
         }
      }
   }

/* The argument's been checked, now make the item! -Danny */
   if ( ( pObjIndex = get_obj_index( vnum ) ) == NULL )
   {
      send_to_char( "The item you are trying to create is missing from the database.\n\rPlease inform the administration of this error.\n\r", ch );
      return;
   }

   OBJ_DATA *item;

   item = create_object( pObjIndex, 0 );
   act( AT_PLAIN, "$n finishes making $p.", ch, item, NULL, TO_ROOM );
   act( AT_PLAIN, "You successfully make $p.", ch, item, NULL, TO_CHAR );
   item = obj_to_room( item, ch->in_room );

//    learn_from_success( ch, gsn_make );
   return;
}

void do_forge( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_INPUT_LENGTH];
   bool checkhammer, checkingot;
   OBJ_DATA *obj;
   OBJ_DATA *material;
   int gain;

   switch ( ch->substate )
   {
      default:
         if( IS_NPC( ch ) || IS_AFFECTED( ch, AFF_CHARM ) )
         {
            send_to_char( "You can't concentrate enough for that.\r\n", ch );
            return;
         }
         if( ch->mount )
         {
            send_to_char( "You can't do that while mounted.\r\n", ch );
            return;
         }
         if( !xIS_SET( ch->in_room->room_flags, ROOM_FORGE ) )
         {
            send_to_char( "You need to be at a forge to do that.\n\r", ch);
            return;
         }
         argument = one_argument( argument, arg );

         if( !str_cmp( arg, "dagger" ) )
         {
            checkhammer = FALSE;
            checkingot = FALSE;

            for( obj = ch->last_carrying; obj; obj = obj->prev_content )
            {
               if( obj->item_type == ITEM_HAMMER )
                  checkhammer = TRUE;
               if (obj->item_type == ITEM_INGOT)
                  checkingot = TRUE;
            }

            if( !checkhammer && !checkingot )
            {
               send_to_char( "You need a hammer and some metal to forge that.\n\r", ch);
               return;
            }
            else if( !checkhammer )
            {
               send_to_char( "You need a hammer to hit the metal with.\n\r", ch);
               return;
	        }
            else if( !checkingot )
            {
               send_to_char( "You need some metal to make that item.\n\r", ch);
               return;
            }
         }
         else
         {
            send_to_char( "You can't make that, see &Ghelp forge &wfor the complete list.\n\r", ch);
            return;
         }
         add_timer( ch, TIMER_DO_FUN, UMIN( skill_table[gsn_mount]->beats / 10, 3 ), do_forge, 1 );
         send_to_char( "You stoke the fire and begin to heat the metal.\n\r", ch);
    	 act( AT_PLAIN, "$n stokes the fire and begins to heat the metal.", ch, NULL, argument , TO_ROOM );
         ch->alloc_ptr = str_dup( arg );
         return;

      case 1:
         if( !ch->alloc_ptr )
         {
            send_to_char( "Your forging was interrupted!\r\n", ch );
            bug( "%s", "do_forge: alloc_ptr NULL" );
            return;
         }
         mudstrlcpy( arg, ch->alloc_ptr, MAX_INPUT_LENGTH );
         DISPOSE( ch->alloc_ptr );
         break;
      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         send_to_char( "You stop forging.\r\n", ch );
         return;
   }
   ch->substate = SUB_NONE;

   if( !str_cmp( arg, "dagger" ) )
   {
	  checkhammer = FALSE;
      checkingot = FALSE;

      for( obj = ch->last_carrying; obj; obj = obj->prev_content )
      {
         if( obj->item_type == ITEM_HAMMER)
            checkhammer = TRUE;
         if( obj->item_type == ITEM_INGOT && checkingot == FALSE)
         {
            checkingot = TRUE;
            separate_obj( obj );
            obj_from_char( obj );
            material = obj;
         }
      }
   }

/* The argument's been checked, now make the item! -Danny */
   OBJ_DATA *item;
   item = material;
   item->item_type = ITEM_WEAPON;
   item->level = 1;
   strcpy( buf, item->name );
//   STRFREE( item->name );
   strcat( buf, " dagger" );
   item->name = STRALLOC( buf );
//   STRFREE( item->short_descr );
//   strcpy( buf, " dagger" );
   item->short_descr = STRALLOC( buf );
   STRFREE( item->description );
   strcat( buf, " is lying here." );
   item->description = STRALLOC( buf );
   item->value[3] = 2;
   item->value[4] = 2;
   item->cost *= 2;
   gain = item->level * 100;
   act( AT_PLAIN, "$n finishes making $p.", ch, item, NULL, TO_ROOM );
   act( AT_PLAIN, "You successfully make $p.", ch, item, NULL, TO_CHAR );
   ch_printf( ch, "You receive %d experience points for crafting!\r\n", gain );
   item = obj_to_char( item, ch );
   learn_from_success( ch, gsn_forge );
   gain_exp( ch, gain );
   return;
}

void do_makefire( CHAR_DATA* ch, const char* argument )
{
   OBJ_INDEX_DATA *findex = NULL;
   OBJ_DATA *fire = NULL, *obj, *wood = NULL;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }
   if( ch->fighting )
   {
      send_to_char( "You're to busy at this moment.\r\n", ch );
      return;
   }
   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }

   if( ch->in_room->sector_type == SECT_WATER_SWIM || ch->in_room->sector_type == SECT_WATER_NOSWIM
   || ch->in_room->sector_type == SECT_UNDERWATER || ch->in_room->sector_type == SECT_OCEANFLOOR )
   {
      send_to_char( "You can't create a fire here.\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_NODROP ) )
   {
      send_to_char( "A magical force prevents you from creating that here.\r\n", ch );
      return;
   }

   for( obj = ch->first_carrying; obj; obj = obj->next_content )
   {
      if( obj->wear_loc != WEAR_NONE )
         continue;
      if( obj->item_type == ITEM_WOOD )
      {
         wood = obj;
         break;
      }
   }
   if( !wood )
   {
      send_to_char( "You have no wood to use.\r\n", ch );
      return;
   }

   for( obj = ch->in_room->first_content; obj; obj = obj->next_content )
   {
      if( obj->pIndexData->vnum == OBJ_VNUM_WOODFIRE
      || obj->pIndexData->vnum == OBJ_VNUM_FIRE )
      {
         fire = obj;
         break;
      }
   }

   separate_obj( wood );
   if( fire )
   {
      act( AT_MAGIC, "$n tosses another piece of wood on the fire!", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You toss another piece of wood on the fire!", ch, NULL, NULL, TO_CHAR );
      fire->timer = UMAX( fire->timer, ( fire->timer + wood->value[0] ) );
   }
   else
   {
//      if( !can_use_skill( ch, number_percent( ), gsn_makefire ) )
//      {
//         act( AT_ACTION, "You use up the wood, but fail to make a fire.", ch, NULL, NULL, TO_CHAR );
//         act( AT_ACTION, "$n uses up the wood, but fails to make a fire.", ch, NULL, NULL, TO_ROOM );
//         learn_from_failure( ch, gsn_makefire );
//         extract_obj( wood );
//         return;
//      }

      if( !( findex = get_obj_index( OBJ_VNUM_WOODFIRE ) ) )
      {
         send_to_char( "Something happened in makefire and the fire couldn't be created.\r\n", ch );
         bug( "%s: Object vnum %d couldn't be found", __FUNCTION__, OBJ_VNUM_WOODFIRE );
         return;
      }

      if( !( fire = create_object( findex, 0 ) ) )
      {
         send_to_char( "Something happened in makefire and the fire couldn't be created.\r\n", ch );
         bug( "%s: Object vnum %d couldn't be created", __FUNCTION__, OBJ_VNUM_WOODFIRE );
         return;
      }
//      learn_from_success( ch, gsn_makefire );
      fire->timer = wood->value[0];
      act( AT_MAGIC, "$n uses a piece of wood to start a fire.", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You use a piece of wood to start a fire.", ch, NULL, NULL, TO_CHAR );
      obj_to_room( fire, ch->in_room );
   }
   extract_obj( wood );
}

void do_chop( CHAR_DATA* ch, const char* argument )
{
   OBJ_INDEX_DATA *windex = NULL;
   OBJ_DATA *obj, *axe;
   int moveloss = 10;

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You can't concentrate enough for that.\r\n", ch );
      return;
   }
   if( ch->fighting )
   {
      send_to_char( "You're to busy at this moment.\r\n", ch );
      return;
   }
   if( ch->mount )
   {
      send_to_char( "You can't do that while mounted.\r\n", ch );
      return;
   }
   if( !( axe = get_eq_hold( ch, ITEM_AXE ) ) )
   {
      send_to_char( "You aren't holding an axe!\r\n", ch );
      return;
   }
   if( ch->in_room->sector_type != SECT_FOREST )
   {
      send_to_char( "You aren't in a forest!\r\n", ch );
      return;
   }
   if( ch->move < moveloss )
   {
      send_to_char( "You're to exhausted to chop wood.\r\n", ch );
      return;
   }
   separate_obj( axe );
   if( axe->value[0] <= 0 )
   {
      send_to_char( "The axe is to dull to cut.\r\n", ch );
      return;
   }

   switch( ch->substate )
   {
      default:
         add_timer( ch, TIMER_DO_FUN, number_range( 2, 4 ), do_chop, 1 );
         act( AT_ACTION, "You begin chopping some wood...", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n begins chopping some wood...", ch, NULL, NULL, TO_ROOM );
         return;

      case 1:
         ch->substate = SUB_NONE;
         ch->move -= moveloss;
         axe->value[0]--;
//         if( !can_use_skill( ch, number_percent( ), gsn_chop ) )
//         {
//            act( AT_ACTION, "You slip and hit yourself with the axe.", ch, NULL, NULL, TO_CHAR );
//            act( AT_ACTION, "$n slips and hits $mself with the axe.", ch, NULL, NULL, TO_ROOM );
//            learn_from_failure( ch, gsn_chop );
//            damage( ch, ch, NULL, number_range( 1, ch->level ), gsn_chop, true );
//            return;
//         }
         if( !( windex = get_obj_index( OBJ_VNUM_WOOD ) ) )
         {
            send_to_char( "There was an issue in chopping, so no wood was created.\r\n", ch );
            bug( "%s: couldn't find object vnum %d.", __FUNCTION__, OBJ_VNUM_WOOD );
            return;
         }
         if( !( obj = create_object( windex, 0 ) ) )
         {
            send_to_char( "There was an issue in chopping, so no wood was created.\r\n", ch );
            bug( "%s: couldn't create an object using vnum %d.", __FUNCTION__, OBJ_VNUM_WOOD );
            return;
         }
         act( AT_ACTION, "You finish chopping wood.", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n finishes chopping wood.", ch, NULL, NULL, TO_ROOM );
//         learn_from_success( ch, gsn_chop );
         obj->value[0] = number_range( 5, 50 );
         obj_to_room( obj, ch->in_room );
         return;

      case SUB_TIMER_DO_ABORT:
         ch->substate = SUB_NONE;
         act( AT_ACTION, "You stop chopping wood...", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n stops chopping wood...", ch, NULL, NULL, TO_ROOM );
         return;
   }
}

int stat_bonus( int stat )
{
	int bonus;

	bonus = ( stat - 10 ) / 2;
	if( stat == 3 || stat == 5 || stat == 7 || stat == 9 )
		--bonus;

	return bonus;
}

int get_init( CHAR_DATA* ch )
{
	int dexmod;
	int halflevel;

	dexmod = stat_bonus( ch->perm_dex );
	halflevel = ch->level / 2;
	return (10 + dexmod + halflevel );
}

int get_movespeed( CHAR_DATA* ch )
{
	if( ch->race == RACE_ELF )
		return 7;
	else if( ch->race == RACE_DWARF /* || ch->race == RACE_HALFLING */ )
		return 5;
	else
		return 6;
}

int get_fort( CHAR_DATA* ch )
{
	int strmod;
	int conmod;
	int halflevel;

	strmod = stat_bonus( ch->perm_str );
	conmod = stat_bonus( ch->perm_con );
	halflevel = ch->level / 2;

	if( strmod > conmod )
		return (10 + strmod + halflevel );
	else
		return (10 + conmod + halflevel );
}

int get_ref( CHAR_DATA* ch )
{
	int dexmod;
	int intmod;
	int halflevel;

	dexmod = stat_bonus( ch->perm_dex );
	intmod = stat_bonus( ch->perm_int );
	halflevel = ch->level / 2;

	if( intmod > dexmod )
		return (10 + intmod + halflevel );
	else
		return (10 + dexmod + halflevel );
}

int get_will( CHAR_DATA* ch )
{
	int wismod;
	int chamod;
	int halflevel;

	wismod = stat_bonus( ch->perm_wis );
	chamod = stat_bonus( ch->perm_cha );
	halflevel = ch->level / 2;

	if( chamod > wismod )
		return (10 + chamod + halflevel );
	else
		return (10 + wismod + halflevel );
}

int get_perception( CHAR_DATA* ch )
{
	int wismod;
	int halflevel;

	wismod = stat_bonus( ch->perm_wis );
	halflevel = ch->level / 2;

	return (10 + wismod + halflevel );
}

int get_insight( CHAR_DATA* ch )
{
	int wismod;
	int halflevel;

	wismod = stat_bonus( ch->perm_wis );
	halflevel = ch->level / 2;

	return (10 + wismod + halflevel );
}

int get_actionpoints( CHAR_DATA* ch )
{
	if( ch->level > 20 )
		return 3;
	else if( ch->level > 10 )
		return 2;
	else
		return 1;
}

const char* pos_name( int pos )
{
    switch( pos )
    {
    	case POS_DEAD: 		return "DEAD";
    	case POS_MORTAL: 	return "mortally wounded";
    	case POS_INCAP: 	return "incapacitated";
    	case POS_STUNNED: 	return "stunned";
//    	case POS_IMMOBILE: 	return "immobile";
    	case POS_SLEEPING: 	return "sleeping";
//    	case POS_PRONE: 	return "prone";
    	case POS_RESTING: 	return "resting";
//    	case POS_WRIGGLING: return "wriggling";
//    	case POS_CRAWLING: 	return "crawling";
//    	case POS_SITTING: 	return "sitting";
    	case POS_STANDING: 	return "standing";
    	case POS_FIGHTING: 	return "fighting";
    	case POS_EVASIVE: 	return "fighting (evasive)";
    	case POS_DEFENSIVE: return "fighting (defensive)";
    	case POS_AGGRESSIVE:return "fighting (aggressive)";
    	case POS_BERSERK: 	return "fighting (berserk)";
    	case POS_MOUNTED: 	return "mounted";
//    	case POS_SHOVE: 	return "being shoved";
//    	case POS_DRAG: 		return "being dragged";
    }
    return "invalid";
}

const char* align_name( int align )
{
    if( align > 600 )       return "Lawful Good";
    else if( align > 200 )  return "Good";
    else if( align > -200 ) return "Unaligned";
    else if( align > -600 ) return "Evil";
    else                  	return "Chaotic Evil";
}

const char* gender_name( int gender )
{
    if( gender == 2 )		return "Female";
	else if( gender == 1 )	return "Male";
    else    				return "Neuter";
}

/* Placeholder for paragon paths */
const char* paragon_name( int path )
{
    return "None";
}

/* Placeholder for epic destinies */
const char* epic_name( int destiny )
{
    return "None";
}


/* Samples score

Name  : Zoie            Level: 5                Heroic Class: Warlock     *
Race  : Tiefling        Align: Chaotic Evil     Paragon Path: None
Gender: Female          Deity: Tiamat           Epic Destiny: None
--------------------------------------------------------------------------
Init: 15                AC   : 16               Speed         : 6

STR : 06(-2)            FORT : 14               Insight       : 16
CON : 14(+2)
DEX : 16(+3)            REF  : 15               Perception    : 12
INT : 12(+1)
WIS : 10(+0)            WILL : 12               Action Points : 1
CHA : 18(+4)

HP  : 0024 of 0030      Pos'n: standing         Items : 0 (max 12800)
Mana: 0000 of 0000      Wimpy: 0                Weight: 0 (max 1000000)

EXP  : 196608000
GOLD : 51,130

You feel great.
Languages: common elvish abyssal infernal draconic
---------------------------------------------------------------------------

*/

/* New 4E-style Score Command - Zoie */
void do_4escore( CHAR_DATA* ch, const char* argument)
{
    int iLang;

	if( IS_NPC( ch ) )
    {
		send_to_char( "Huh?", ch );
		return;
    }
    set_pager_color( AT_SCORE, ch );
	send_to_pager_color("&W--------------------------------------------------------------------------\n\r", ch);

    pager_printf_color( ch, "&CName  : &W%-16s&CLevel: &W%-17d&CHeroic Class: &W%s\r\n", ch->name, ch->level, capitalize(get_class(ch)) );
    pager_printf_color( ch, "&CRace  : &W%-16s&CAlign: &W%-17s&CParagon Path: &W%s\r\n", capitalize(get_race(ch)), align_name(-1000), paragon_name(0) );
    pager_printf_color( ch, "&CGender: &W%-16s&CDeity: &W%-17s&CEpic Destiny: &W%s\r\n\r\n", gender_name(ch->sex), ch->pcdata->deity? ch->pcdata->deity->name : "None", epic_name(0) );

    pager_printf_color( ch, "&CInit: &W%-18d&CAC   : &W%-17d&CSpeed         : &W%d\r\n\r\n", get_init(ch), GET_AC(ch), get_movespeed(ch) );

	if( stat_bonus( ch->perm_str ) >= 0)
    	pager_printf_color( ch, "&CSTR : &W%-2d&C(&W+%d&C)            &CFORT : &W%-17d&CInsight       : &W%d\r\n", ch->perm_str, stat_bonus( ch->perm_str ), get_fort(ch), get_insight(ch) );
	else
    	pager_printf_color( ch, "&CSTR : &W%-2d&C(&W%-2d&C)            &CFORT : &W%-17d&CInsight       : &W%d\r\n", ch->perm_str, stat_bonus( ch->perm_str ), get_fort(ch), get_insight(ch) );
	if( stat_bonus( ch->perm_con ) >= 0)
    	pager_printf_color( ch, "&CCON : &W%-2d&C(&W+%d&C)\r\n", ch->perm_con, stat_bonus( ch->perm_con ) );
	else
    	pager_printf_color( ch, "&CCON : &W%-2d&C(&W%-2d&C)\r\n", ch->perm_con, stat_bonus( ch->perm_con ) );
	if( stat_bonus( ch->perm_dex ) >= 0)
    	pager_printf_color( ch, "&CDEX : &W%-2d&C(&W+%d&C)            &CREF  : &W%-17d&CPerception    : &W%d\r\n", ch->perm_dex, stat_bonus( ch->perm_dex ), get_ref(ch), get_perception(ch) );
	else
    	pager_printf_color( ch, "&CDEX : &W%-2d&C(&W%-2d&C)            &CREF  : &W%-17d&CPerception    : &W%d\r\n", ch->perm_dex, stat_bonus( ch->perm_dex ), get_ref(ch), get_perception(ch) );
	if( stat_bonus( ch->perm_int ) >= 0)
    	pager_printf_color( ch, "&CINT : &W%-2d&C(&W+%d&C)\r\n", ch->perm_int, stat_bonus( ch->perm_int ) );
	else
    	pager_printf_color( ch, "&CINT : &W%-2d&C(&W%-2d&C)\r\n", ch->perm_int, stat_bonus( ch->perm_int ) );
	if( stat_bonus( ch->perm_wis ) >= 0)
	    pager_printf_color( ch, "&CWIS : &W%-2d&C(&W+%d&C)            &CWILL : &W%-17d&CAction Points : &W%d\r\n", ch->perm_wis, stat_bonus( ch->perm_wis ), get_will(ch), get_actionpoints(ch) );
	else
	    pager_printf_color( ch, "&CWIS : &W%-2d&C(&W%-2d&C)            &CWILL : &W%-17d&CAction Points : &W%d\r\n", ch->perm_wis, stat_bonus( ch->perm_wis ), get_will(ch), get_actionpoints(ch) );
	if( stat_bonus( ch->perm_cha ) >= 0)
	    pager_printf_color( ch, "&CCHA : &W%-2d&C(&W+%d&C)\r\n\r\n", ch->perm_cha, stat_bonus( ch->perm_cha ) );
	else
	    pager_printf_color( ch, "&CCHA : &W%-2d&C(&W%-2d&C)\r\n\r\n", ch->perm_cha, stat_bonus( ch->perm_cha ) );

    pager_printf_color(ch, "&CHP  : &G%-4d &Cof &g%4d      &CPos'n: &W%-17s&CItems:  &W%d &C(max &W%d&C)\n\r", ch->hit, ch->max_hit, pos_name(ch->position), ch->carry_number, can_carry_n(ch) );
    pager_printf_color(ch, "&CMana: &B%-4d &Cof &b%4d      &CWimpy: &W%-17d&CWeight: &W%d &C(max &W%d&C)\n\r\r\n", ch->mana, ch->max_mana, ch->wimpy, ch->carry_weight, can_carry_w(ch) );

    pager_printf_color( ch, "&CEXP  : &W%-9d\r\n", ch->exp );
	pager_printf_color( ch, "&CGOLD : &Y%-13s\r\n\r\n", num_punct(ch->gold) );

    set_pager_color( AT_SCORE, ch );

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
	send_to_pager("You are drunk.\n\r", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] == 0)
	send_to_pager("You are in danger of dehydrating.\n\r", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] == 0)
	send_to_pager("You are starving to death.\n\r", ch);
    if ( ch->position != POS_SLEEPING )
	switch( ch->mental_state / 10 )
	{
	    default:   send_to_pager( "You're completely messed up!\n\r", ch );	break;
	    case -10:  send_to_pager( "You're barely conscious.\n\r", ch );	break;
	    case  -9:  send_to_pager( "You can barely keep your eyes open.\n\r", ch );	break;
	    case  -8:  send_to_pager( "You're extremely drowsy.\n\r", ch );	break;
	    case  -7:  send_to_pager( "You feel very unmotivated.\n\r", ch );	break;
	    case  -6:  send_to_pager( "You feel sedated.\n\r", ch );		break;
	    case  -5:  send_to_pager( "You feel sleepy.\n\r", ch );		break;
	    case  -4:  send_to_pager( "You feel tired.\n\r", ch );		break;
	    case  -3:  send_to_pager( "You could use a rest.\n\r", ch );		break;
	    case  -2:  send_to_pager( "You feel a little under the weather.\n\r", ch );	break;
	    case  -1:  send_to_pager( "You feel fine.\n\r", ch );		break;
	    case   0:  send_to_pager( "You feel great.\n\r", ch );		break;
	    case   1:  send_to_pager( "You feel energetic.\n\r", ch );	break;
	    case   2:  send_to_pager( "Your mind is racing.\n\r", ch );	break;
	    case   3:  send_to_pager( "You can't think straight.\n\r", ch );	break;
	    case   4:  send_to_pager( "Your mind is going 100 miles an hour.\n\r", ch );	break;
	    case   5:  send_to_pager( "You're high as a kite.\n\r", ch );	break;
	    case   6:  send_to_pager( "Your mind and body are slipping apart.\n\r", ch );	break;
	    case   7:  send_to_pager( "Reality is slipping away.\n\r", ch );	break;
	    case   8:  send_to_pager( "You have no idea what is real, and what is not.\n\r", ch );	break;
	    case   9:  send_to_pager( "You feel immortal.\n\r", ch );	break;
	    case  10:  send_to_pager( "You are a Supreme Entity.\n\r", ch );	break;
	}
    else
    if ( ch->mental_state >45 )
	send_to_pager( "Your sleep is filled with strange and vivid dreams.\n\r", ch );
    else
    if ( ch->mental_state >25 )
	send_to_pager( "Your sleep is uneasy.\n\r", ch );
    else
    if ( ch->mental_state <-35 )
	send_to_pager( "You are deep in a much needed sleep.\n\r", ch );
    else
    if ( ch->mental_state <-25 )
	send_to_pager( "You are in deep slumber.\n\r", ch );
    send_to_pager_color("\r\n&CLanguages: &W", ch );
    for ( iLang = 0; lang_array[iLang] != LANG_UNKNOWN; iLang++ )
	if ( knows_language( ch, lang_array[iLang], ch ) ||  (IS_NPC(ch) && ch->speaks == 0) )
	{
	    if ( lang_array[iLang] & ch->speaking || (IS_NPC(ch) && !ch->speaking) )
		   set_pager_color( AT_RED, ch );
	    send_to_pager( lang_names[iLang], ch );
	    send_to_pager( " ", ch );
	    set_pager_color( AT_WHITE, ch );
	}
    send_to_pager_color( "&C\n\r", ch );

    if ( ch->pcdata->bestowments && ch->pcdata->bestowments[0] != '\0' )
	pager_printf_color(ch, "&CYou are bestowed with the command(s): &Y%s\n\r",
		ch->pcdata->bestowments );

    if ( CAN_PKILL( ch ) )
    {
		send_to_pager_color("&W--------------------------------------------------------------------------&C\n\r", ch);
		pager_printf_color(ch, "&CPKILL DATA:  Pkills (&W%d&C)     Illegal Pkills (&W%d&C)     Pdeaths (&W%d&C)\n\r",
		 ch->pcdata->pkills, ch->pcdata->illegal_pk, ch->pcdata->pdeaths );
    }

    if (ch->pcdata->clan && ch->pcdata->clan->clan_type != CLAN_ORDER  && ch->pcdata->clan->clan_type != CLAN_GUILD )
    {
		pager_printf_color(ch, "&CCLAN STATS:  &W%-14.14s  &CClan AvPkills : &W%-5d  &CClan NonAvpkills : &W%-5d\n\r",
		  ch->pcdata->clan->name, ch->pcdata->clan->pkills[5],
		  (ch->pcdata->clan->pkills[0]+ch->pcdata->clan->pkills[1]+
		  ch->pcdata->clan->pkills[2]+ch->pcdata->clan->pkills[3]+
		  ch->pcdata->clan->pkills[4]) );
 		pager_printf_color(ch, "&C                             Clan AvPdeaths: &W%-5d  &CClan NonAvpdeaths: &W%-5d\n\r",
		  ch->pcdata->clan->pdeaths[5],
		  ( ch->pcdata->clan->pdeaths[0] + ch->pcdata->clan->pdeaths[1] +
		  ch->pcdata->clan->pdeaths[2] + ch->pcdata->clan->pdeaths[3] +
		  ch->pcdata->clan->pdeaths[4] ) );
    }

    if (ch->pcdata->clan && ch->pcdata->clan->clan_type == CLAN_ORDER )
    {
        send_to_pager_color( "&W--------------------------------------------------------------------------&C\n\r", ch);
		  pager_printf_color(ch, "&COrder:  &W%-20s  &COrder Mkills:  &W%-6d   &COrder MDeaths:  &W%-6d\n\r",
		  ch->pcdata->clan->name, ch->pcdata->clan->mkills, ch->pcdata->clan->mdeaths);
    }
    if (ch->pcdata->clan && ch->pcdata->clan->clan_type == CLAN_GUILD )
    {
        send_to_pager_color( "&W--------------------------------------------------------------------------&C\n\r", ch);
		pager_printf_color( ch, "&CGuild:  &W%-20s  &CGuild Mkills:  &W%-6d   &CGuild MDeaths:  &W%-6d\n\r",
   		  				    ch->pcdata->clan->name, ch->pcdata->clan->mkills, ch->pcdata->clan->mdeaths);
    }

    if( IS_IMMORTAL( ch ) )
    {
       send_to_pager( "&W--------------------------------------------------------------------------&C\r\n", ch );

       pager_printf( ch, "&CIMMORTAL DATA:  Wizinvis [&W%s&C]  Wizlevel (&W%d&C)\r\n",
                     xIS_SET( ch->act, PLR_WIZINVIS ) ? "X" : " ", ch->pcdata->wizinvis );

       pager_printf( ch, "&CBamfin:  &W%s %s&C\r\n", ch->name, ( ch->pcdata->bamfin[0] != '\0' )
                     ? ch->pcdata->bamfin : "appears in a swirling mist." );
       pager_printf( ch, "&CBamfout: &W%s %s&C\r\n", ch->name, ( ch->pcdata->bamfout[0] != '\0' )
                     ? ch->pcdata->bamfout : "leaves in a swirling mist." );


      /*
       * Area Loaded info - Scryn 8/11
       */
       if( ch->pcdata->area )
       {
          pager_printf( ch, "&CVnums:   Room (&W%-5.5d - %-5.5d&C)   Object (&W%-5.5d - %-5.5d&C)   Mob (&W%-5.5d - %-5.5d)&C\r\n",
                        ch->pcdata->area->low_r_vnum, ch->pcdata->area->hi_r_vnum,
                        ch->pcdata->area->low_o_vnum, ch->pcdata->area->hi_o_vnum,
                        ch->pcdata->area->low_m_vnum, ch->pcdata->area->hi_m_vnum );
          pager_printf( ch, "&CArea Loaded [&W%s&C]\r\n", ( IS_SET( ch->pcdata->area->status, AREA_LOADED ) ) ? "yes" : "no" );
       }
    }

    send_to_pager("\n\r", ch);
    return;
}
