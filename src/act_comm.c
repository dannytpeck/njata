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
 *			   Player communication module                            *
 ****************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"

/*
 * Local functions.
 */
void talk_channel( CHAR_DATA * ch, const char *argument, int channel, const char *verb );
char *scramble( const char *argument, int modifier );
const char *drunk_speech( const char *argument, CHAR_DATA * ch );

/* Text scrambler -- Altrag */
char *scramble( const char *argument, int modifier )
{
   static char arg[MAX_INPUT_LENGTH];
   short position;
   short conversion = 0;

   modifier %= number_range( 80, 300 );   /* Bitvectors get way too large #s */
   for( position = 0; position < MAX_INPUT_LENGTH; position++ )
   {
      if( argument[position] == '\0' )
      {
         arg[position] = '\0';
         return arg;
      }
      else if( argument[position] >= 'A' && argument[position] <= 'Z' )
      {
         conversion = -conversion + position - modifier + argument[position] - 'A';
         conversion = number_range( conversion - 5, conversion + 5 );
         while( conversion > 25 )
            conversion -= 26;
         while( conversion < 0 )
            conversion += 26;
         arg[position] = conversion + 'A';
      }
      else if( argument[position] >= 'a' && argument[position] <= 'z' )
      {
         conversion = -conversion + position - modifier + argument[position] - 'a';
         conversion = number_range( conversion - 5, conversion + 5 );
         while( conversion > 25 )
            conversion -= 26;
         while( conversion < 0 )
            conversion += 26;
         arg[position] = conversion + 'a';
      }
      else if( argument[position] >= '0' && argument[position] <= '9' )
      {
         conversion = -conversion + position - modifier + argument[position] - '0';
         conversion = number_range( conversion - 2, conversion + 2 );
         while( conversion > 9 )
            conversion -= 10;
         while( conversion < 0 )
            conversion += 10;
         arg[position] = conversion + '0';
      }
      else
         arg[position] = argument[position];
   }
   arg[position] = '\0';
   return arg;
}

/* I'll rewrite this later if its still needed.. -- Altrag
char *translate( CHAR_DATA *ch, CHAR_DATA *victim, const char *argument )
{
	return "";
}
*/

LANG_DATA *get_lang( const char *name )
{
   LANG_DATA *lng;

   for( lng = first_lang; lng; lng = lng->next )
      if( !str_cmp( lng->name, name ) )
         return lng;
   return NULL;
}

/* percent = percent knowing the language. */
char *translate( int percent, const char *in, const char *name )
{
   LCNV_DATA *cnv;
   static char buf[256];
   char buf2[256];
   const char *pbuf;
   char *pbuf2 = buf2;
   LANG_DATA *lng;
   static char log_buf[MAX_STRING_LENGTH];

   if( percent > 99 )
   {
      mudstrlcpy( log_buf, in, MAX_STRING_LENGTH );
      return log_buf;
   }

   /*
    * If we don't know this language... use "default"
    */
   if( !( lng = get_lang( name ) ) )
      if( !( lng = get_lang( "default" ) ) )
      {
         mudstrlcpy( log_buf, in, MAX_STRING_LENGTH );
         return log_buf;
      }

   for( pbuf = in; *pbuf; )
   {
      for( cnv = lng->first_precnv; cnv; cnv = cnv->next )
      {
         if( !str_prefix( cnv->old, pbuf ) )
         {
            if( percent && ( rand(  ) % 100 ) < percent )
            {
               strncpy( pbuf2, pbuf, cnv->olen );
               pbuf2[cnv->olen] = '\0';
               pbuf2 += cnv->olen;
            }
            else
            {
               mudstrlcpy( pbuf2, cnv->lnew, 256 );
               pbuf2 += cnv->nlen;
            }
            pbuf += cnv->olen;
            break;
         }
      }
      if( !cnv )
      {
         if( isalpha( *pbuf ) && ( !percent || ( rand(  ) % 100 ) > percent ) )
         {
            *pbuf2 = lng->alphabet[LOWER( *pbuf ) - 'a'];
            if( isupper( *pbuf ) )
               *pbuf2 = UPPER( *pbuf2 );
         }
         else
            *pbuf2 = *pbuf;
         pbuf++;
         pbuf2++;
      }
   }
   *pbuf2 = '\0';
   for( pbuf = buf2, pbuf2 = buf; *pbuf; )
   {
      for( cnv = lng->first_cnv; cnv; cnv = cnv->next )
         if( !str_prefix( cnv->old, pbuf ) )
         {
            mudstrlcpy( pbuf2, cnv->lnew, 256 );
            pbuf += cnv->olen;
            pbuf2 += cnv->nlen;
            break;
         }
      if( !cnv )
         *( pbuf2++ ) = *( pbuf++ );
   }
   *pbuf2 = '\0';
   return buf;
}

const char *drunk_speech( const char *argument, CHAR_DATA * ch )
{
   const char *arg = argument;
   static char buf[MAX_INPUT_LENGTH * 2];
   char buf1[MAX_INPUT_LENGTH * 2];
   short drunk;
   char *txt;
   char *txt1;

   if( IS_NPC( ch ) || !ch->pcdata )
   {
      mudstrlcpy( buf, argument, MAX_INPUT_LENGTH * 2 );
      return buf;
   }

   drunk = ch->pcdata->condition[COND_DRUNK];

   if( drunk <= 0 )
   {
      mudstrlcpy( buf, argument, MAX_INPUT_LENGTH * 2 );
      return buf;
   }

   buf[0] = '\0';
   buf1[0] = '\0';

   if( !argument )
   {
      bug( "%s", "Drunk_speech: NULL argument" );
      return "";
   }

   /*
    * if ( *arg == '\0' )
    * return (char *) argument;
    */

   txt = buf;
   txt1 = buf1;

   while( *arg != '\0' )
   {
      if( toupper( *arg ) == 'T' )
      {
         if( number_percent(  ) < ( drunk * 2 ) )  /* add 'h' after an 'T' */
         {
            *txt++ = *arg;
            *txt++ = 'h';
         }
         else
            *txt++ = *arg;
      }
      else if( toupper( *arg ) == 'X' )
      {
         if( number_percent(  ) < ( drunk * 2 / 2 ) )
         {
            *txt++ = 'c', *txt++ = 's', *txt++ = 'h';
         }
         else
            *txt++ = *arg;
      }
      else if( number_percent(  ) < ( drunk * 2 / 5 ) )  /* slurred letters */
      {
         short slurn = number_range( 1, 2 );
         short currslur = 0;

         while( currslur < slurn )
            *txt++ = *arg, currslur++;
      }
      else
         *txt++ = *arg;

      arg++;
   };

   *txt = '\0';

   txt = buf;

   while( *txt != '\0' )   /* Let's mess with the string's caps */
   {
      if( number_percent(  ) < ( 2 * drunk / 2.5 ) )
      {
         if( isupper( *txt ) )
            *txt1 = tolower( *txt );
         else if( islower( *txt ) )
            *txt1 = toupper( *txt );
         else
            *txt1 = *txt;
      }
      else
         *txt1 = *txt;

      txt1++, txt++;
   };

   *txt1 = '\0';
   txt1 = buf1;
   txt = buf;

   while( *txt1 != '\0' )  /* Let's make them stutter */
   {
      if( *txt1 == ' ' )   /* If there's a space, then there's gotta be a */
      {  /* along there somewhere soon */

         while( *txt1 == ' ' )   /* Don't stutter on spaces */
            *txt++ = *txt1++;

         if( ( number_percent(  ) < ( 2 * drunk / 4 ) ) && *txt1 != '\0' )
         {
            short offset = number_range( 0, 2 );
            short pos = 0;

            while( *txt1 != '\0' && pos < offset )
               *txt++ = *txt1++, pos++;

            if( *txt1 == ' ' )   /* Make sure not to stutter a space after */
            {  /* the initial offset into the word */
               *txt++ = *txt1++;
               continue;
            }

            pos = 0;
            offset = number_range( 2, 4 );
            while( *txt1 != '\0' && pos < offset )
            {
               *txt++ = *txt1;
               pos++;
               if( *txt1 == ' ' || pos == offset ) /* Make sure we don't stick */
               {  /* A hyphen right before a space */
                  txt1--;
                  break;
               }
               *txt++ = '-';
            }
            if( *txt1 != '\0' )
               txt1++;
         }
      }
      else
         *txt++ = *txt1++;
   }

   *txt = '\0';

   return buf;
}

/*
 * Generic channel function.
 */
void talk_channel( CHAR_DATA * ch, const char *argument, int channel, const char *verb )
{
   char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;
   int position;
#ifndef SCRAMBLE
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif

   if( IS_NPC( ch ) && channel == CHANNEL_CLAN )
   {
      send_to_char( "Mobs can't be in clans.\r\n", ch );
      return;
   }
   if( IS_NPC( ch ) && channel == CHANNEL_ORDER )
   {
      send_to_char( "Mobs can't be in orders.\r\n", ch );
      return;
   }

   if( IS_NPC( ch ) && channel == CHANNEL_COUNCIL )
   {
      send_to_char( "Mobs can't be in councils.\r\n", ch );
      return;
   }

   if( IS_NPC( ch ) && channel == CHANNEL_GUILD )
   {
      send_to_char( "Mobs can't be in guilds.\r\n", ch );
      return;
   }

   if( !IS_PKILL( ch ) && channel == CHANNEL_PVP )
   {
      if( !IS_IMMORTAL( ch ) )
      {
         send_to_char( "Peacefuls have no need to use the PVP channel.\r\n", ch );
         return;
      }
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return;
   }

   if( IS_NPC( ch ) && IS_AFFECTED( ch, AFF_CHARM ) )
   {
      if( ch->master )
         send_to_char( "I don't think so...\r\n", ch->master );
      return;
   }

   if( argument[0] == '\0' )
   {
      snprintf( buf, MAX_STRING_LENGTH, "%s what?\r\n", verb );
      buf[0] = UPPER( buf[0] );
      send_to_char( buf, ch );   /* where'd this line go? */
      return;
   }

   if( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_SILENCE ) )
   {
      ch_printf( ch, "You can't %s.\r\n", verb );
      return;
   }

   REMOVE_BIT( ch->deaf, channel );

   switch ( channel )
   {
      default:
         set_char_color( AT_GOSSIP, ch );
         ch_printf( ch, "You %s '%s'\r\n", verb, argument );
         snprintf( buf, MAX_STRING_LENGTH, "$n %ss '$t'", verb );
         break;
      case CHANNEL_CHAT:
         set_char_color( AT_GOSSIP, ch );
         ch_printf( ch, "&GYou (&W%s&G): &W%s\r\n", verb, argument );
         snprintf( buf, MAX_STRING_LENGTH, "&G$n (&W%s&G): &W$t", verb );
         break;
      case CHANNEL_LFG:
         set_char_color( AT_GOSSIP, ch );
         ch_printf( ch, "&pYou (&W%s&p): &W%s\r\n", verb, argument );
         snprintf( buf, MAX_STRING_LENGTH, "&p$n (&W%s&p): &W$t", verb );
         break;
      case CHANNEL_PVP:
         set_char_color( AT_GOSSIP, ch );
         ch_printf( ch, "&RYou (&W%s&R): &W%s\r\n", verb, argument );
         snprintf( buf, MAX_STRING_LENGTH, "&R$n (&W%s&R): &W$t", verb );
         break;
      case CHANNEL_IMMTALK:
         set_char_color( AT_GOSSIP, ch );
         ch_printf( ch, "&YYou (&W%s&Y): &W%s\r\n", verb, argument );
         snprintf( buf, MAX_STRING_LENGTH, "&Y$n (&W%s&Y): &W$t", verb );
         break;
      case CHANNEL_QUEST:
         set_char_color( AT_GOSSIP, ch );
         ch_printf( ch, "&cYou (&W%s&c): &W%s\r\n", verb, argument );
         snprintf( buf, MAX_STRING_LENGTH, "&c$n (&W%s&c): &W$t", verb );
         break;
      case CHANNEL_ASK:
         set_char_color( AT_GOSSIP, ch );
         ch_printf( ch, "&CYou (&W%s&C): &W%s\r\n", verb, argument );
         snprintf( buf, MAX_STRING_LENGTH, "&C$n (&W%s&C): &W$t", verb );
         break;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf2, MAX_STRING_LENGTH, "%s: %s (%s)", IS_NPC( ch ) ? ch->short_descr : ch->name, argument, verb );
      append_to_file( LOG_FILE, buf2 );
   }

   for( d = first_descriptor; d; d = d->next )
   {
      CHAR_DATA *och;
      CHAR_DATA *vch;

      och = d->original ? d->original : d->character;
      vch = d->character;

      if( d->connected == CON_PLAYING && vch != ch && !IS_SET( och->deaf, channel ) )
      {
         const char *sbuf = argument;
         char lbuf[MAX_INPUT_LENGTH + 4]; /* invis level string + buf */

         /*
          * fix by Gorog os that players can ignore others' channel talk
          */
         if( is_ignoring( och, ch ) && get_trust( ch ) <= get_trust( och ) )
            continue;

         if( channel == CHANNEL_IMMTALK && !IS_IMMORTAL( och ) )
            continue;
         if( channel == CHANNEL_PVP && NOT_AUTHED( och ) )
            continue;
         if( xIS_SET( vch->in_room->room_flags, ROOM_SILENCE ) )
            continue;
         if( channel == CHANNEL_YELL && vch->in_room->area != ch->in_room->area )
            continue;

         if( channel == CHANNEL_CLAN || channel == CHANNEL_ORDER || channel == CHANNEL_GUILD )
         {
            if( IS_NPC( vch ) )
               continue;
            if( vch->pcdata->clan != ch->pcdata->clan )
               continue;
         }

         if( channel == CHANNEL_COUNCIL )
         {
            if( IS_NPC( vch ) )
               continue;
            if( vch->pcdata->council != ch->pcdata->council )
               continue;
         }

         if( xIS_SET( ch->act, PLR_WIZINVIS ) && can_see( vch, ch ) && IS_IMMORTAL( vch ) )
         {
            snprintf( lbuf, MAX_INPUT_LENGTH + 4, "(%d) ", ( !IS_NPC( ch ) ) ? ch->pcdata->wizinvis : ch->mobinvis );
         }
         else
         {
            lbuf[0] = '\0';
         }

         position = vch->position;
         if( channel != CHANNEL_SHOUT && channel != CHANNEL_YELL )
            vch->position = POS_STANDING;
#ifndef SCRAMBLE
         if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
         {
            int speakswell = UMIN( knows_language( vch, ch->speaking, ch ),
                                   knows_language( ch, ch->speaking, vch ) );

            if( speakswell < 85 )
               sbuf = translate( speakswell, argument, lang_names[speaking] );
         }
#else
         if( !knows_language( vch, ch->speaking, ch ) && ( !IS_NPC( ch ) || ch->speaking != 0 ) )
            sbuf = scramble( argument, ch->speaking );
#endif
         MOBtrigger = FALSE;
         mudstrlcat( lbuf, buf, MAX_STRING_LENGTH );
         if( channel == CHANNEL_IMMTALK )
            act( AT_IMMORT, lbuf, ch, sbuf, vch, TO_VICT );
         else if( channel == CHANNEL_PVP )
            act( AT_WARTALK, lbuf, ch, sbuf, vch, TO_VICT );
         else
            act( AT_GOSSIP, lbuf, ch, sbuf, vch, TO_VICT );
         vch->position = position;
      }
   }
   return;
}

void to_channel( const char *argument, int channel, const char *verb, short level )
{
   char buf[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;

   if( !first_descriptor || argument[0] == '\0' )
      return;

   snprintf( buf, MAX_STRING_LENGTH, "%s: %s\r\n", verb, argument );

   for( d = first_descriptor; d; d = d->next )
   {
      CHAR_DATA *och;
      CHAR_DATA *vch;

      och = d->original ? d->original : d->character;
      vch = d->character;

      if( !och || !vch )
         continue;
      if( !IS_IMMORTAL( vch )
          || ( get_trust( vch ) < sysdata.build_level && channel == CHANNEL_BUILD )
          || ( get_trust( vch ) < sysdata.log_level
               && ( channel == CHANNEL_LOG || channel == CHANNEL_HIGH ||
                    channel == CHANNEL_WARN || channel == CHANNEL_COMM ) ) )
         continue;

      if( d->connected == CON_PLAYING && !IS_SET( och->deaf, channel ) && get_trust( vch ) >= level )
      {
         set_char_color( AT_LOG, vch );
         send_to_char_color( buf, vch );
      }
   }

   return;
}


void do_chat( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, argument, CHANNEL_CHAT, "Chat" );
   return;
}

void do_clantalk( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( IS_NPC( ch ) || !ch->pcdata->clan
       || ch->pcdata->clan->clan_type == CLAN_ORDER || ch->pcdata->clan->clan_type == CLAN_GUILD )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, argument, CHANNEL_CLAN, "Clantalk" );
}

void do_ot( CHAR_DATA* ch, const char* argument)
{
   do_ordertalk( ch, argument );
}

void do_ordertalk( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( IS_NPC( ch ) || !ch->pcdata->clan || ch->pcdata->clan->clan_type != CLAN_ORDER )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, argument, CHANNEL_ORDER, "Ordertalk" );
}

void do_counciltalk( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( IS_NPC( ch ) || !ch->pcdata->council )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, argument, CHANNEL_COUNCIL, "Counciltalk" );
   return;
}

void do_guildtalk( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   if( IS_NPC( ch ) || !ch->pcdata->clan || ch->pcdata->clan->clan_type != CLAN_GUILD )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, argument, CHANNEL_GUILD, "Guildtalk" );
   return;
}

void do_quest( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, argument, CHANNEL_QUEST, "Quest" );
   return;
}

void do_ask( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, argument, CHANNEL_ASK, "Ask" );
   return;
}

void do_answer( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, argument, CHANNEL_ASK, "Answer" );
   return;
}

void do_lfg( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, argument, CHANNEL_LFG, "LFG" );
   return;
}

void do_shout( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, drunk_speech( argument, ch ), CHANNEL_SHOUT, "shout" );
   WAIT_STATE( ch, 12 );
   return;
}

void do_yell( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, drunk_speech( argument, ch ), CHANNEL_YELL, "yell" );
   return;
}

void do_immtalk( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   talk_channel( ch, argument, CHANNEL_IMMTALK, "Imm" );
   return;
}

void do_say( CHAR_DATA* ch, const char* argument)
{
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *vch;
   EXT_BV actflags;
#ifndef SCRAMBLE
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif

   if( argument[0] == '\0' )
   {
      send_to_char( "Say what?\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return;
   }

   actflags = ch->act;
   if( IS_NPC( ch ) )
      xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      const char *sbuf = argument;

      if( vch == ch )
         continue;

      /*
       * Check to see if character is ignoring speaker
       */
      if( is_ignoring( vch, ch ) )
      {
         /*
          * continue unless speaker is an immortal
          */
         if( !IS_IMMORTAL( ch ) || get_trust( vch ) > get_trust( ch ) )
            continue;
         else
         {
            set_char_color( AT_IGNORE, vch );
            ch_printf( vch, "You attempt to ignore %s, but" " are unable to do so.\r\n", ch->name );
         }
      }

#ifndef SCRAMBLE
      if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
      {
         int speakswell = UMIN( knows_language( vch, ch->speaking, ch ),
                                knows_language( ch, ch->speaking, vch ) );

         if( speakswell < 75 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );
      }
#else
      if( !knows_language( vch, ch->speaking, ch ) && ( !IS_NPC( ch ) || ch->speaking != 0 ) )
         sbuf = scramble( argument, ch->speaking );
#endif
      sbuf = drunk_speech( sbuf, ch );

      MOBtrigger = FALSE;
      act( AT_SAY, "$n says '$t'", ch, sbuf, vch, TO_VICT );

   }
   ch->act = actflags;
   MOBtrigger = FALSE;
   act( AT_SAY, "You say '$T'", ch, NULL, drunk_speech( argument, ch ), TO_CHAR );
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, MAX_STRING_LENGTH, "%s: %s", IS_NPC( ch ) ? ch->short_descr : ch->name, argument );
      append_to_file( LOG_FILE, buf );
   }
//   mprog_speech_trigger( argument, ch ); Use sayto to talk to mobs -Danny
   if( char_died( ch ) )
      return;
   oprog_speech_trigger( argument, ch );
   if( char_died( ch ) )
      return;
   rprog_speech_trigger( argument, ch );
   return;
}

/* Clone of do_say that lets players sing~ -Danny */
void do_sing( CHAR_DATA* ch, const char* argument)
{
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *vch;
   EXT_BV actflags;
#ifndef SCRAMBLE
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif

   if( argument[0] == '\0' )
   {
      send_to_char( "Sing what?\r\n", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return;
   }

   actflags = ch->act;
   if( IS_NPC( ch ) )
      xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      const char *sbuf = argument;

      if( vch == ch )
         continue;

      /*
       * Check to see if character is ignoring speaker
       */
      if( is_ignoring( vch, ch ) )
      {
         /*
          * continue unless speaker is an immortal
          */
         if( !IS_IMMORTAL( ch ) || get_trust( vch ) > get_trust( ch ) )
            continue;
         else
         {
            set_char_color( AT_IGNORE, vch );
            ch_printf( vch, "You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
         }
      }

#ifndef SCRAMBLE
      if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
      {
         int speakswell = UMIN( knows_language( vch, ch->speaking, ch ),
                                knows_language( ch, ch->speaking, vch ) );

         if( speakswell < 75 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );
      }
#else
      if( !knows_language( vch, ch->speaking, ch ) && ( !IS_NPC( ch ) || ch->speaking != 0 ) )
         sbuf = scramble( argument, ch->speaking );
#endif
      sbuf = drunk_speech( sbuf, ch );

      MOBtrigger = FALSE;
      act( AT_SAY, "$n sings '$t'", ch, sbuf, vch, TO_VICT );
   }
   ch->act = actflags;
   MOBtrigger = FALSE;
   act( AT_SAY, "You sing '$T'", ch, NULL, drunk_speech( argument, ch ), TO_CHAR );
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, MAX_STRING_LENGTH, "%s: %s", IS_NPC( ch ) ? ch->short_descr : ch->name, argument );
      append_to_file( LOG_FILE, buf );
   }
   mprog_speech_trigger( argument, ch );
   if( char_died( ch ) )
      return;
   oprog_speech_trigger( argument, ch );
   if( char_died( ch ) )
      return;
   rprog_speech_trigger( argument, ch );
   return;
}


/* Sayto command -Danny */
void do_sayto( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   char *sbuf;
   char lastchar;
   CHAR_DATA *victim;
   EXT_BV actflags;
   int arglen;
#ifndef SCRAMBLE
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Say what to whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL
    || ( IS_NPC(victim) && victim->in_room != ch->in_room )
    || (!NOT_AUTHED(ch) && NOT_AUTHED(victim) && !IS_IMMORTAL(ch) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You have a nice conversation with yourself. Did it help?\n\r", ch );
      return;
   }

   arglen = strlen( argument ) - 1;
   /* Remove whitespace and tabs. */
   while( argument[arglen] == ' ' || argument[arglen] == '\t' )
       --arglen;
   lastchar = argument[arglen];

   actflags = ch->act;
   if( IS_NPC(ch) )
      xREMOVE_BIT( ch->act, ACT_SECRETIVE );

   sbuf = (char*)argument;

   /* Check to see if character is ignoring speaker */
   if( is_ignoring( victim, ch ) )
   {
      /* continue unless speaker is an immortal */
      if( !IS_IMMORTAL(ch) || get_trust(victim) > get_trust(ch) )
         return;
      else
      {
         set_char_color(AT_IGNORE, victim );
         ch_printf( victim, "You attempt to ignore %s, but are unable to do so.\n\r", ch->name );
      }
   }

#ifndef SCRAMBLE
   if( speaking != -1 && (!IS_NPC(ch) || ch->speaking) )
   {
      int speakswell = UMIN( knows_language(victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );
      if( speakswell < 75 )
         sbuf = translate( speakswell, argument, lang_names[speaking] );
   }
#else
   if( !knows_language( victim, ch->speaking, ch ) && ( !IS_NPC(ch) || ch->speaking != 0 ) )
      sbuf = scramble( argument, ch->speaking );
#endif
   sbuf = (char*)drunk_speech( sbuf, ch );

   ch->act = actflags;
   MOBtrigger = FALSE;

   switch ( lastchar )
   {
      case '?':
         act( AT_SAY, "You ask $N, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_CHAR );
         act( AT_SAY, "$n asks $N, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_NOTVICT );
         act( AT_SAY, "$n asks you '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_VICT );
         break;

      case '!':
         act( AT_SAY, "You exclaim at $N, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_CHAR );
         act( AT_SAY, "$n exclaims at $N, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_NOTVICT );
         act( AT_SAY, "$n exclaims to you, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_VICT );
         break;

      default:
         act( AT_SAY, "You say to $N '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_CHAR );
         act( AT_SAY, "$n says to $N '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_NOTVICT );
         act( AT_SAY, "$n says to you '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_VICT );
         break;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, MAX_STRING_LENGTH, "%s: %s", IS_NPC( ch ) ? ch->short_descr : ch->name, argument );
      append_to_file( LOG_FILE, buf );
   }
   mprog_speech_trigger( argument, ch );
   if( char_died(ch) )
      return;
   oprog_speech_trigger( argument, ch );
   if( char_died(ch) )
      return;
   rprog_speech_trigger( argument, ch );
   return;
}

/* Clone of do_sayto that lets players sing~ -Danny */
void do_singto( CHAR_DATA* ch, const char* argument )
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   char *sbuf;
   char lastchar;
   CHAR_DATA *victim;
   EXT_BV actflags;
   int arglen;
#ifndef SCRAMBLE
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Sing what to whom?\n\r", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL
    || ( IS_NPC(victim) && victim->in_room != ch->in_room )
    || (!NOT_AUTHED(ch) && NOT_AUTHED(victim) && !IS_IMMORTAL(ch) ) )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\n\r", ch );
      return;
   }

   if( victim == ch )
   {
      send_to_char( "You sing a lovely song to yourself. Did it help?\n\r", ch );
      return;
   }

   arglen = strlen( argument ) - 1;
   /* Remove whitespace and tabs. */
   while( argument[arglen] == ' ' || argument[arglen] == '\t' )
       --arglen;
   lastchar = argument[arglen];

   actflags = ch->act;
   if( IS_NPC(ch) )
      xREMOVE_BIT( ch->act, ACT_SECRETIVE );

   sbuf = (char*)argument;

   /* Check to see if character is ignoring speaker */
   if( is_ignoring( victim, ch ) )
   {
      /* continue unless speaker is an immortal */
      if( !IS_IMMORTAL(ch) || get_trust(victim) > get_trust(ch) )
         return;
      else
      {
         set_char_color(AT_IGNORE, victim );
         ch_printf( victim, "You attempt to ignore %s but are unable to do so.\n\r", ch->name );
      }
   }

#ifndef SCRAMBLE
   if( speaking != -1 && (!IS_NPC(ch) || ch->speaking) )
   {
      int speakswell = UMIN( knows_language(victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );
      if( speakswell < 75 )
         sbuf = translate( speakswell, argument, lang_names[speaking] );
   }
#else
   if( !knows_language( victim, ch->speaking, ch ) && ( !IS_NPC(ch) || ch->speaking != 0 ) )
      sbuf = scramble( argument, ch->speaking );
#endif
   sbuf = (char*)drunk_speech( sbuf, ch );

   ch->act = actflags;
   MOBtrigger = FALSE;

   switch ( lastchar )
   {
      case '?':
         act( AT_SAY, "You melodiously question $N, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_CHAR );
         act( AT_SAY, "$n melodiously questions $N, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_NOTVICT );
         act( AT_SAY, "$n melodiously questions you '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_VICT );
         break;

      case '!':
         act( AT_SAY, "You belt at $N, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_CHAR );
         act( AT_SAY, "$n belts at $N, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_NOTVICT );
         act( AT_SAY, "$n belts to you, '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_VICT );
         break;

      default:
         act( AT_SAY, "You sing to $N '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_CHAR );
         act( AT_SAY, "$n sings to $N '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_NOTVICT );
         act( AT_SAY, "$n sings to you '$t&c'", ch, drunk_speech( argument, ch ), victim, TO_VICT );
         break;
   }

   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, MAX_STRING_LENGTH, "%s: %s", IS_NPC( ch ) ? ch->short_descr : ch->name, argument );
      append_to_file( LOG_FILE, buf );
   }
   mprog_speech_trigger( argument, ch );
   if( char_died(ch) )
      return;
   oprog_speech_trigger( argument, ch );
   if( char_died(ch) )
      return;
   rprog_speech_trigger( argument, ch );
   return;
}

void do_whisper( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int position;
   int speaking = -1, lang;

#ifndef SCRAMBLE
   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif

   REMOVE_BIT( ch->deaf, CHANNEL_WHISPER );

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Whisper to whom what?\r\n", ch );
      return;
   }


   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ch == victim )
   {
      send_to_char( "You have a nice little chat with yourself.\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) && ( victim->switched ) && !IS_AFFECTED( victim->switched, AFF_POSSESS ) )
   {
      send_to_char( "That player is switched.\r\n", ch );
      return;
   }
   else if( !IS_NPC( victim ) && ( !victim->desc ) )
   {
      send_to_char( "That player is link-dead.\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_AFK ) )
   {
      send_to_char( "That player is afk.\r\n", ch );
      return;
   }

   if( IS_SET( victim->deaf, CHANNEL_WHISPER ) && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
   {
      act( AT_PLAIN, "$E has $S whispers turned off.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_SILENCE ) )
      send_to_char( "That player is silenced.  They will receive your message but can not respond.\r\n", ch );

   if( victim->desc  /* make sure desc exists first  -Thoric */
       && victim->desc->connected == CON_EDITING && get_trust( ch ) < LEVEL_GOD )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer.  Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
   }

   /*
    * Check to see if target of tell is ignoring the sender
    */
   if( is_ignoring( victim, ch ) )
   {
      /*
       * If the sender is an imm then they cannot be ignored
       */
      if( !IS_IMMORTAL( ch ) || get_trust( victim ) > get_trust( ch ) )
      {
         set_char_color( AT_IGNORE, ch );
         ch_printf( ch, "%s is ignoring you.\r\n", victim->name );
         return;
      }
      else
      {
         set_char_color( AT_IGNORE, victim );
         ch_printf( victim, "You attempt to ignore %s, but " "are unable to do so.\r\n", ch->name );
      }
   }

   act( AT_WHISPER, "You whisper to $N '$t'", ch, argument, victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;
#ifndef SCRAMBLE
   if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
   {
      int speakswell = UMIN( knows_language( victim, ch->speaking, ch ),
                             knows_language( ch, ch->speaking, victim ) );

      if( speakswell < 85 )
         act( AT_WHISPER, "$n whispers to you '$t'", ch,
              translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
      else
         act( AT_WHISPER, "$n whispers to you '$t'", ch, argument, victim, TO_VICT );
   }
   else
      act( AT_WHISPER, "$n whispers to you '$t'", ch, argument, victim, TO_VICT );
#else
   if( !knows_language( vch, ch->speaking, ch ) && ( !IS_NPC( ch ) || ch->speaking != 0 ) )
      act( AT_WHISPER, "$n whispers to you '$t'", ch,
           translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
   else
      act( AT_WHISPER, "$n whispers to you '$t'", ch, argument, victim, TO_VICT );
#endif

   if( !xIS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
      act( AT_WHISPER, "$n whispers something to $N.", ch, argument, victim, TO_NOTVICT );

   victim->position = position;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, MAX_INPUT_LENGTH, "%s: %s (whisper to) %s.",
                IS_NPC( ch ) ? ch->short_descr : ch->name, argument, IS_NPC( victim ) ? victim->short_descr : victim->name );
      append_to_file( LOG_FILE, buf );
   }

   mprog_tell_trigger( argument, ch );
   return;
}

void do_tell( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int position;
   CHAR_DATA *switched_victim = NULL;
#ifndef SCRAMBLE
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif

   REMOVE_BIT( ch->deaf, CHANNEL_TELLS );
   if( xIS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && ( xIS_SET( ch->act, PLR_SILENCE ) || xIS_SET( ch->act, PLR_NO_TELL ) ) )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Tell whom what?\r\n", ch );
      return;
   }

   if( ( victim = get_char_world( ch, arg ) ) == NULL
       || ( IS_NPC( victim ) && victim->in_room != ch->in_room )
       || ( !NOT_AUTHED( ch ) && NOT_AUTHED( victim ) && !IS_IMMORTAL( ch ) ) )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ch == victim )
   {
      send_to_char( "You have a nice little chat with yourself.\r\n", ch );
      return;
   }

   if( NOT_AUTHED( ch ) && !NOT_AUTHED( victim ) && !IS_IMMORTAL( victim ) )
   {
      send_to_char( "They can't hear you because you are not authorized.\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) && ( victim->switched )
       && ( get_trust( ch ) > LEVEL_AVATAR ) && !IS_AFFECTED( victim->switched, AFF_POSSESS ) )
   {
      send_to_char( "That player is switched.\r\n", ch );
      return;
   }

   else if( !IS_NPC( victim ) && ( victim->switched ) && IS_AFFECTED( victim->switched, AFF_POSSESS ) )
      switched_victim = victim->switched;

   else if( !IS_NPC( victim ) && ( !victim->desc ) )
   {
      send_to_char( "That player is link-dead.\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_AFK ) )
   {
      send_to_char( "That player is afk.\r\n", ch );
      return;
   }

   if( IS_SET( victim->deaf, CHANNEL_TELLS ) && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
   {
      act( AT_PLAIN, "$E has $S tells turned off.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_SILENCE ) )
      send_to_char( "That player is silenced.  They will receive your message but can not respond.\r\n", ch );

   if( ( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) ) )
   {
      act( AT_PLAIN, "$E is too tired to discuss such matters with you now.", ch, 0, victim, TO_CHAR );
      return;
   }

   if( !IS_NPC( victim ) && xIS_SET( victim->in_room->room_flags, ROOM_SILENCE ) )
   {
      act( AT_PLAIN, "A magic force prevents your message from being heard.", ch, 0, victim, TO_CHAR );
      return;
   }

   if( victim->desc  /* make sure desc exists first  -Thoric */
       && victim->desc->connected == CON_EDITING && get_trust( ch ) < LEVEL_GOD )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer.  Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
   }

   /*
    * Check to see if target of tell is ignoring the sender
    */
   if( is_ignoring( victim, ch ) )
   {
      /*
       * If the sender is an imm then they cannot be ignored
       */
      if( !IS_IMMORTAL( ch ) || get_trust( victim ) > get_trust( ch ) )
      {
         set_char_color( AT_IGNORE, ch );
         ch_printf( ch, "%s is ignoring you.\r\n", victim->name );
         return;
      }
      else
      {
         set_char_color( AT_IGNORE, victim );
         ch_printf( victim, "You attempt to ignore %s, but " "are unable to do so.\r\n", ch->name );
      }
   }

   ch->retell = victim;

   if( !IS_NPC( victim ) && IS_IMMORTAL( victim ) && victim->pcdata->tell_history &&
       isalpha( IS_NPC( ch ) ? ch->short_descr[0] : ch->name[0] ) )
   {
      snprintf( buf, MAX_INPUT_LENGTH, "%s told you '%s'\r\n", capitalize( IS_NPC( ch ) ? ch->short_descr : ch->name ),
                argument );

      /*
       * get lasttell index... assumes names begin with characters
       */
      victim->pcdata->lt_index = tolower( IS_NPC( ch ) ? ch->short_descr[0] : ch->name[0] ) - 'a';

      /*
       * get rid of old messages
       */
      if( victim->pcdata->tell_history[victim->pcdata->lt_index] )
         STRFREE( victim->pcdata->tell_history[victim->pcdata->lt_index] );

      /*
       * store the new message
       */
      victim->pcdata->tell_history[victim->pcdata->lt_index] = STRALLOC( buf );
   }

   if( switched_victim )
      victim = switched_victim;

   /*
    * Bug fix by guppy@wavecomputers.net
    */
   MOBtrigger = FALSE;
   act( AT_TELL, "You tell $N '$t'", ch, argument, victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;
   if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
   {
      int speakswell = UMIN( knows_language( victim, ch->speaking, ch ),
                             knows_language( ch, ch->speaking, victim ) );

      if( speakswell < 85 )
         act( AT_TELL, "$n tells you '$t'", ch, translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
      else
         act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
   }
   else
      act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );

   MOBtrigger = TRUE;

   victim->position = position;
   victim->reply = ch;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, MAX_INPUT_LENGTH, "%s: %s (tell to) %s.",
                IS_NPC( ch ) ? ch->short_descr : ch->name, argument, IS_NPC( victim ) ? victim->short_descr : victim->name );
      append_to_file( LOG_FILE, buf );
   }

   mprog_tell_trigger( argument, ch );
   return;
}

void do_reply( CHAR_DATA* ch, const char* argument)
{
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *victim;
   int position;
#ifndef SCRAMBLE
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif


   REMOVE_BIT( ch->deaf, CHANNEL_TELLS );
   if( xIS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_SILENCE ) )
   {
      send_to_char( "Your message didn't get through.\r\n", ch );
      return;
   }

   if( ( victim = ch->reply ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) && ( victim->switched ) && can_see( ch, victim ) && ( get_trust( ch ) > LEVEL_AVATAR ) )
   {
      send_to_char( "That player is switched.\r\n", ch );
      return;
   }
   else if( !IS_NPC( victim ) && ( !victim->desc ) )
   {
      send_to_char( "That player is link-dead.\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_AFK ) )
   {
      send_to_char( "That player is afk.\r\n", ch );
      return;
   }

   if( IS_SET( victim->deaf, CHANNEL_TELLS ) && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
   {
      act( AT_PLAIN, "$E has $S tells turned off.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( ( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) )
       || ( !IS_NPC( victim ) && xIS_SET( victim->in_room->room_flags, ROOM_SILENCE ) ) )
   {
      act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
      return;
   }

   if( victim->desc  /* make sure desc exists first  -Thoric */
       && victim->desc->connected == CON_EDITING && get_trust( ch ) < LEVEL_GOD )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer.  Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
   }

   /*
    * Check to see if the receiver is ignoring the sender
    */
   if( is_ignoring( victim, ch ) )
   {
      /*
       * If the sender is an imm they cannot be ignored
       */
      if( !IS_IMMORTAL( ch ) || get_trust( victim ) > get_trust( ch ) )
      {
         set_char_color( AT_IGNORE, ch );
         ch_printf( ch, "%s is ignoring you.\r\n", victim->name );
         return;
      }
      else
      {
         set_char_color( AT_IGNORE, victim );
         ch_printf( victim, "You attempt to ignore %s, but " "are unable to do so.\r\n", ch->name );
      }
   }

   act( AT_TELL, "You tell $N '$t'", ch, argument, victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;
#ifndef SCRAMBLE
   if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
   {
      int speakswell = UMIN( knows_language( victim, ch->speaking, ch ),
                             knows_language( ch, ch->speaking, victim ) );

      if( speakswell < 85 )
         act( AT_TELL, "$n tells you '$t'", ch, translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
      else
         act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
   }
   else
      act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
#else
   if( knows_language( victim, ch->speaking, ch ) || ( IS_NPC( ch ) && !ch->speaking ) )
      act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
   else
      act( AT_TELL, "$n tells you '$t'", ch, scramble( argument, ch->speaking ), victim, TO_VICT );
#endif
   victim->position = position;
   victim->reply = ch;
   ch->retell = victim;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, MAX_STRING_LENGTH, "%s: %s (reply to) %s.",
                IS_NPC( ch ) ? ch->short_descr : ch->name, argument, IS_NPC( victim ) ? victim->short_descr : victim->name );
      append_to_file( LOG_FILE, buf );
   }

   if( !IS_NPC( victim ) && IS_IMMORTAL( victim ) && victim->pcdata->tell_history &&
       isalpha( IS_NPC( ch ) ? ch->short_descr[0] : ch->name[0] ) )
   {
      snprintf( buf, MAX_STRING_LENGTH, "%s told you '%s'\r\n", capitalize( IS_NPC( ch ) ? ch->short_descr : ch->name ),
                argument );

      /*
       * get lasttell index... assumes names begin with characters
       */
      victim->pcdata->lt_index = tolower( IS_NPC( ch ) ? ch->short_descr[0] : ch->name[0] ) - 'a';

      /*
       * get rid of old messages
       */
      if( victim->pcdata->tell_history[victim->pcdata->lt_index] )
         STRFREE( victim->pcdata->tell_history[victim->pcdata->lt_index] );

      /*
       * store the new message
       */
      victim->pcdata->tell_history[victim->pcdata->lt_index] = STRALLOC( buf );
   }
   mprog_tell_trigger( argument, ch );
   return;
}

void do_retell( CHAR_DATA* ch, const char* argument)
{
   char buf[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int position;
   CHAR_DATA *switched_victim = NULL;
#ifndef SCRAMBLE
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif
   REMOVE_BIT( ch->deaf, CHANNEL_TELLS );
   if( xIS_SET( ch->in_room->room_flags, ROOM_SILENCE ) )
   {
      send_to_char( "You can't do that here.\r\n", ch );
      return;
   }

   if( !IS_NPC( ch ) && ( xIS_SET( ch->act, PLR_SILENCE ) || xIS_SET( ch->act, PLR_NO_TELL ) ) )
   {
      send_to_char( "You can't do that.\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      ch_printf( ch, "What message do you wish to send?\r\n" );
      return;
   }

   victim = ch->retell;

   if( !victim )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) && ( victim->switched ) &&
       ( get_trust( ch ) > LEVEL_AVATAR ) && !IS_AFFECTED( victim->switched, AFF_POSSESS ) )
   {
      send_to_char( "That player is switched.\r\n", ch );
      return;
   }
   else if( !IS_NPC( victim ) && ( victim->switched ) && IS_AFFECTED( victim->switched, AFF_POSSESS ) )
   {
      switched_victim = victim->switched;
   }
   else if( !IS_NPC( victim ) && ( !victim->desc ) )
   {
      send_to_char( "That player is link-dead.\r\n", ch );
      return;
   }

   if( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_AFK ) )
   {
      send_to_char( "That player is afk.\r\n", ch );
      return;
   }

   if( IS_SET( victim->deaf, CHANNEL_TELLS ) && ( !IS_IMMORTAL( ch ) || ( get_trust( ch ) < get_trust( victim ) ) ) )
   {
      act( AT_PLAIN, "$E has $S tells turned off.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( !IS_NPC( victim ) && xIS_SET( victim->act, PLR_SILENCE ) )
      send_to_char( "That player is silenced. They will receive your message, but can not respond.\r\n", ch );

   if( ( !IS_IMMORTAL( ch ) && !IS_AWAKE( victim ) ) ||
       ( !IS_NPC( victim ) && xIS_SET( victim->in_room->room_flags, ROOM_SILENCE ) ) )
   {
      act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
      return;
   }

   if( victim->desc && victim->desc->connected == CON_EDITING && get_trust( ch ) < LEVEL_GOD )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer. Please " "try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
   }

   /*
    * check to see if the target is ignoring the sender
    */
   if( is_ignoring( victim, ch ) )
   {
      /*
       * if the sender is an imm then they cannot be ignored
       */
      if( !IS_IMMORTAL( ch ) || get_trust( victim ) > get_trust( ch ) )
      {
         set_char_color( AT_IGNORE, ch );
         ch_printf( ch, "%s is ignoring you.\r\n", victim->name );
         return;
      }
      else
      {
         set_char_color( AT_IGNORE, victim );
         ch_printf( victim, "You attempy to ignore %s, but " "are unable to do so.\r\n", ch->name );
      }
   }

   /*
    * store tell history for victim
    */
   if( !IS_NPC( victim ) && IS_IMMORTAL( victim ) && victim->pcdata->tell_history &&
       isalpha( IS_NPC( ch ) ? ch->short_descr[0] : ch->name[0] ) )
   {
      snprintf( buf, MAX_INPUT_LENGTH, "%s told you '%s'\r\n", capitalize( IS_NPC( ch ) ? ch->short_descr : ch->name ),
                argument );

      /*
       * get lasttel index... assumes names begin with chars
       */
      victim->pcdata->lt_index = tolower( IS_NPC( ch ) ? ch->short_descr[0] : ch->name[0] ) - 'a';

      /*
       * get rid of old messages
       */
      if( victim->pcdata->tell_history[victim->pcdata->lt_index] )
         STRFREE( victim->pcdata->tell_history[victim->pcdata->lt_index] );

      /*
       * store the new messagec
       */
      victim->pcdata->tell_history[victim->pcdata->lt_index] = STRALLOC( buf );
   }

   if( switched_victim )
      victim = switched_victim;

   act( AT_TELL, "You tell $N '$t'", ch, argument, victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;
#ifndef SCRAMBLE
   if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
   {
      int speakswell = UMIN( knows_language( victim, ch->speaking, ch ),
                             knows_language( ch, ch->speaking, victim ) );

      if( speakswell < 85 )
         act( AT_TELL, "$n tells you '$t'", ch, translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
      else
         act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
   }
   else
      act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
#else
   if( knows_language( victim, ch->speaking, ch ) || ( IS_NPC( ch ) && !ch->speaking ) )
   {
      act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
   }
   else
   {
      act( AT_TELL, "$n tells you '$t'", ch, scramble( argument, ch->speaking ), victim, TO_VICT );
   }
#endif
   victim->position = position;
   victim->reply = ch;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, MAX_INPUT_LENGTH, "%s: %s (retell to) %s.",
                IS_NPC( ch ) ? ch->short_descr : ch->name, argument, IS_NPC( victim ) ? victim->short_descr : victim->name );
      append_to_file( LOG_FILE, buf );
   }

   mprog_tell_trigger( argument, ch );
   return;
}

void do_repeat( CHAR_DATA* ch, const char* argument)
{
   int tindex;

   if( IS_NPC( ch ) || !IS_IMMORTAL( ch ) || !ch->pcdata->tell_history )
   {
      ch_printf( ch, "Huh?\r\n" );
      return;
   }

   if( argument[0] == '\0' )
   {
      tindex = ch->pcdata->lt_index;
   }
   else if( isalpha( argument[0] ) && argument[1] == '\0' )
   {
      tindex = tolower( argument[0] ) - 'a';
   }
   else
   {
      ch_printf( ch, "You may only index your tell history using " "a single letter.\r\n" );
      return;
   }

   if( ch->pcdata->tell_history[tindex] )
   {
      set_char_color( AT_TELL, ch );
      send_to_char( ch->pcdata->tell_history[tindex], ch );
   }
   else
   {
      send_to_char( "No one like that has sent you a tell.\r\n", ch );
   }
   return;
}

void do_emote( CHAR_DATA* ch, const char* argument)
{
   char buf[MAX_STRING_LENGTH];
   const char *plast;
   CHAR_DATA *vch;
   EXT_BV actflags;
#ifndef SCRAMBLE
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif

   if( !IS_NPC( ch ) && xIS_SET( ch->act, PLR_NO_EMOTE ) )
   {
      send_to_char( "You can't show your emotions.\r\n", ch );
      return;
   }

   if( argument[0] == '\0' )
   {
      send_to_char( "Emote what?\r\n", ch );
      return;
   }

   actflags = ch->act;
   if( IS_NPC( ch ) )
      xREMOVE_BIT( ch->act, ACT_SECRETIVE );
   for( plast = argument; *plast != '\0'; plast++ )
      ;

   mudstrlcpy( buf, argument, MAX_STRING_LENGTH );
   if( isalpha( plast[-1] ) )
      mudstrlcat( buf, ".", MAX_STRING_LENGTH );
   for( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
   {
      char *sbuf = buf;

      /*
       * Check to see if character is ignoring emoter
       */
      if( is_ignoring( vch, ch ) )
      {
         /*
          * continue unless emoter is an immortal
          */
         if( !IS_IMMORTAL( ch ) || get_trust( vch ) > get_trust( ch ) )
            continue;
         else
         {
            set_char_color( AT_IGNORE, vch );
            ch_printf( vch, "You attempt to ignore %s, but" " are unable to do so.\r\n", ch->name );
         }
      }
#ifndef SCRAMBLE
      if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
      {
         int speakswell = UMIN( knows_language( vch, ch->speaking, ch ),
                                knows_language( ch, ch->speaking, vch ) );

         if( speakswell < 85 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );
      }
#else
      if( !knows_language( vch, ch->speaking, ch ) && ( !IS_NPC( ch ) && ch->speaking != 0 ) )
         sbuf = scramble( buf, ch->speaking );
#endif
      MOBtrigger = FALSE;
      act( AT_ACTION, "$n $t", ch, sbuf, vch, ( vch == ch ? TO_CHAR : TO_VICT ) );
   }
/*    MOBtrigger = FALSE;
    act( AT_ACTION, "$n $T", ch, NULL, buf, TO_ROOM );
    MOBtrigger = FALSE;
    act( AT_ACTION, "$n $T", ch, NULL, buf, TO_CHAR );*/
   ch->act = actflags;
   if( xIS_SET( ch->in_room->room_flags, ROOM_LOGSPEECH ) )
   {
      snprintf( buf, MAX_STRING_LENGTH, "%s %s (emote)", IS_NPC( ch ) ? ch->short_descr : ch->name, argument );
      append_to_file( LOG_FILE, buf );
   }
   return;
}

void do_ide( CHAR_DATA* ch, const char* argument)
{
   set_char_color( AT_PLAIN, ch );
   send_to_char( "\r\nIf you want to send an idea, type 'idea <message>'.\r\n", ch );
   send_to_char( "If you want to identify an object, use the identify spell.\r\n", ch );
   return;
}

void do_bug(CHAR_DATA * ch, const char *argument)
{
   	char buf[MAX_STRING_LENGTH];
   	char arg[MAX_INPUT_LENGTH];
   	const char *argument1;
   	struct tm *t = localtime( &current_time );

   	argument1 = one_argument(argument, arg);

   	if (arg[0] == '\0')
   	{
   		set_char_color( AT_PLAIN, ch );
      	send_to_char("Usage: bug <message>\n\r", ch);
      	send_to_char("Usage: bug <# of entries OR \'-1\' for all entries>\n\r", ch);
      	return;
   	}

   	if (isdigit(arg[0]) || atoi(arg) == -1 || !str_cmp(arg, "today")) //View list instead of players
   	{
   		send_to_char("&w&RVnum    Date    Name         Bug Message\n\r&c&w---------------------------------------------------------------------------\n\r", ch);
      	if (!str_cmp(arg, "today"))
         	read_last_file(ch, -2, NULL, 1);
      	else
         	read_last_file(ch, atoi(arg), NULL, 1);
      	return;
   	}

    sprintf( buf, "[%5d] %-2.2d/%-2.2d   %-12s %s", ch->in_room ? ch->in_room->vnum : 0, t->tm_mon + 1, t->tm_mday, ch->name, argument );
    write_last_file( buf, 1 );
   	send_to_char( "Thanks! your bug notice has been recorded.\r\n", ch );
   	return;
}

void do_idea(CHAR_DATA * ch, const char *argument)
{
   	char buf[MAX_STRING_LENGTH];
   	char arg[MAX_INPUT_LENGTH];
   	const char *argument1;
   	struct tm *t = localtime( &current_time );

   	argument1 = one_argument(argument, arg);

   	if (arg[0] == '\0')
   	{
   		set_char_color( AT_PLAIN, ch );
      	send_to_char("Usage: idea <message>\n\r", ch);
      	send_to_char("Usage: idea <# of entries OR \'-1\' for all entries>\n\r", ch);
      	return;
   	}

   	if (isdigit(arg[0]) || atoi(arg) == -1 || !str_cmp(arg, "today")) //View list instead of players
   	{
   		send_to_char("&w&RDate    Name         Idea Message\n\r&c&w---------------------------------------------------------------------------\n\r", ch);
      	if (!str_cmp(arg, "today"))
         	read_last_file(ch, -2, NULL, 2);
      	else
         	read_last_file(ch, atoi(arg), NULL, 2);
      	return;
   	}

    sprintf( buf, "%-2.2d/%-2.2d   %-12s %s", t->tm_mon + 1, t->tm_mday, ch->name, argument );
    write_last_file( buf, 2 );
   	send_to_char( "Thanks for your idea contribution!\r\n", ch );
   	return;
}

void do_typo(CHAR_DATA * ch, const char *argument)
{
   	char buf[MAX_STRING_LENGTH];
   	char arg[MAX_INPUT_LENGTH];
   	const char *argument1;
   	struct tm *t = localtime( &current_time );

   	argument1 = one_argument(argument, arg);

   	if (arg[0] == '\0')
   	{
   		set_char_color( AT_PLAIN, ch );
      	send_to_char("Usage: typo <message>\n\r", ch);
      	send_to_char("Usage: typo <# of entries OR \'-1\' for all entries>\n\r", ch);
      	return;
   	}

   	if (isdigit(arg[0]) || atoi(arg) == -1 || !str_cmp(arg, "today")) //View list instead of players
   	{
   		send_to_char("&w&RVnum    Date    Name         Typo Message\n\r&c&w---------------------------------------------------------------------------\n\r", ch);
      	if (!str_cmp(arg, "today"))
         	read_last_file(ch, -2, NULL, 3);
      	else
         	read_last_file(ch, atoi(arg), NULL, 3);
      	return;
   	}

    sprintf( buf, "[%5d] %-2.2d/%-2.2d   %-12s %s", ch->in_room ? ch->in_room->vnum : 0, t->tm_mon + 1, t->tm_mday, ch->name, argument );
    write_last_file( buf, 3 );
   	send_to_char( "Thanks! Your typo notice has been recorded.\r\n", ch );
   	return;
}

/* Old bug, idea and typo commands - Danny
void do_bug( CHAR_DATA* ch, const char* argument)
{
   char buf[MAX_STRING_LENGTH];
   struct tm *t = localtime( &current_time );

   set_char_color( AT_PLAIN, ch );
   if( argument[0] == '\0' )
   {
      send_to_char( "\r\nUsage:  'bug <message>'  (your location is automatically recorded)\r\n", ch );
      return;
   }
   snprintf( buf, MAX_STRING_LENGTH, "(%-2.2d/%-2.2d):  %s", t->tm_mon + 1, t->tm_mday, argument );
   append_file( ch, PBUG_FILE, buf );
   send_to_char( "Thanks, your bug notice has been recorded.\r\n", ch );
   return;
}

void do_idea( CHAR_DATA* ch, const char* argument)
{
   set_char_color( AT_PLAIN, ch );
   if( argument[0] == '\0' )
   {
      send_to_char( "\r\nUsage:  'idea <message>'\r\n", ch );
      return;
   }
   append_file( ch, IDEA_FILE, argument );
   send_to_char( "Thanks, your idea has been recorded.\r\n", ch );
   return;
}

void do_typo( CHAR_DATA* ch, const char* argument)
{
   set_char_color( AT_PLAIN, ch );
   if( argument[0] == '\0' )
   {
      send_to_char( "\r\nUsage:  'typo <message>'  (your location is automatically recorded)\r\n", ch );
      if( get_trust( ch ) >= LEVEL_ASCENDANT )
         send_to_char( "Usage:  'typo list' or 'typo clear now'\r\n", ch );
      return;
   }
   if( !str_cmp( argument, "clear now" ) && get_trust( ch ) >= LEVEL_ASCENDANT )
   {
      FILE *fp = fopen( TYPO_FILE, "w" );
      if( fp )
         fclose( fp );
      send_to_char( "Typo file cleared.\r\n", ch );
      return;
   }
   if( !str_cmp( argument, "list" ) && get_trust( ch ) >= LEVEL_ASCENDANT )
   {
      send_to_char( "\r\n VNUM \r\n.......\r\n", ch );
      show_file( ch, TYPO_FILE );
   }
   else
   {
      append_file( ch, TYPO_FILE, argument );
      send_to_char( "Thanks, your typo notice has been recorded.\r\n", ch );
   }
   return;
}
*/

void do_rent( CHAR_DATA* ch, const char* argument)
{
   set_char_color( AT_WHITE, ch );
   send_to_char( "There is no rent here.  Just save and quit.\r\n", ch );
   return;
}

void do_qui( CHAR_DATA* ch, const char* argument)
{
   set_char_color( AT_RED, ch );
   send_to_char( "If you want to QUIT, you have to spell it out.\r\n", ch );
   return;
}

void do_quit( CHAR_DATA* ch, const char* argument)
{
   char log_buf[MAX_STRING_LENGTH];
   int x, y;
   int level;

   if( IS_NPC( ch ) )
      return;

   if( ch->position == POS_FIGHTING
       || ch->position == POS_EVASIVE
       || ch->position == POS_DEFENSIVE || ch->position == POS_AGGRESSIVE || ch->position == POS_BERSERK )
   {
      set_char_color( AT_RED, ch );
      send_to_char( "No way! You are fighting.\r\n", ch );
      return;
   }

   if( ch->position < POS_STUNNED )
   {
      set_char_color( AT_BLOOD, ch );
      send_to_char( "You're not DEAD yet.\r\n", ch );
      return;
   }

   if( get_timer( ch, TIMER_RECENTFIGHT ) > 0 && !IS_IMMORTAL( ch ) )
   {
      set_char_color( AT_RED, ch );
      send_to_char( "Your adrenaline is pumping too hard to quit now!\r\n", ch );
      return;
   }

   if( auction->item != NULL && ( ( ch == auction->buyer ) || ( ch == auction->seller ) ) )
   {
      send_to_char( "Wait until you have bought/sold the item on auction.\r\n", ch );
      return;

   }

   if( IS_PKILL( ch ) && ch->wimpy > ( int )ch->max_hit / 2.25 )
   {
      send_to_char( "Your wimpy has been adjusted to the maximum level for deadlies.\r\n", ch );
      do_wimpy( ch, "max" );
   }
   /*
    * Get 'em dismounted until we finish mount saving -- Blodkai, 4/97
    */
   if( ch->position == POS_MOUNTED )
      do_dismount( ch, "" );
   set_char_color( AT_WHITE, ch );
   send_to_char
      ( "Your surroundings begin to fade as a mystical swirling vortex of colors\r\nenvelops your body... When you come to, things are not as they were.\r\n\r\n",
        ch );
   act( AT_SAY, "A strange voice says, 'We await your return, $n...'", ch, NULL, NULL, TO_CHAR );
   act( AT_BYE, "$n has left the game.", ch, NULL, NULL, TO_CANSEE );
   set_char_color( AT_GREY, ch );

   snprintf( log_buf, MAX_STRING_LENGTH, "%s has quit (Room %d).", ch->name, ( ch->in_room ? ch->in_room->vnum : -1 ) );
   quitting_char = ch;
   save_char_obj( ch );

   if( sysdata.save_pets && ch->pcdata->pet )
   {
      act( AT_BYE, "$N follows $S master into the Void.", ch, NULL, ch->pcdata->pet, TO_ROOM );
      extract_char( ch->pcdata->pet, TRUE );
   }

   if( ch->pcdata->in_progress )
      free_global_note( ch->pcdata->in_progress );

   /*
    * Synch clandata up only when clan member quits now. --Shaddai
    */
   if( ch->pcdata->clan )
      save_clan( ch->pcdata->clan );

   saving_char = NULL;

   level = get_trust( ch );
   /*
    * After extract_char the ch is no longer valid!
    */
   extract_char( ch, TRUE );
   for( x = 0; x < MAX_WEAR; x++ )
      for( y = 0; y < MAX_LAYERS; y++ )
         save_equipment[x][y] = NULL;

   log_string_plus( log_buf, LOG_COMM, level );
   return;
}

void send_rip_screen( CHAR_DATA * ch )
{
   FILE *rpfile;
   int num = 0;
   char BUFF[MAX_STRING_LENGTH * 2];

   if( ( rpfile = fopen( RIPSCREEN_FILE, "r" ) ) != NULL )
   {
      while( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
         num++;
      fclose( rpfile );
      BUFF[num] = 0;
      write_to_buffer( ch->desc, BUFF, num );
   }
}

void send_rip_title( CHAR_DATA * ch )
{
   FILE *rpfile;
   int num = 0;
   char BUFF[MAX_STRING_LENGTH * 2];

   if( ( rpfile = fopen( RIPTITLE_FILE, "r" ) ) != NULL )
   {
      while( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
         num++;
      fclose( rpfile );
      BUFF[num] = 0;
      write_to_buffer( ch->desc, BUFF, num );
   }
}

void send_ansi_title( CHAR_DATA * ch )
{
   FILE *rpfile;
   int num = 0;
   char BUFF[MAX_STRING_LENGTH * 2];

   if( ( rpfile = fopen( ANSITITLE_FILE, "r" ) ) != NULL )
   {
      while( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
         num++;
      fclose( rpfile );
      BUFF[num] = 0;
      write_to_buffer( ch->desc, BUFF, num );
   }
}

void send_ascii_title( CHAR_DATA * ch )
{
   FILE *rpfile;
   int num = 0;
   char BUFF[MAX_STRING_LENGTH];

   if( ( rpfile = fopen( ASCTITLE_FILE, "r" ) ) != NULL )
   {
      while( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
         num++;
      fclose( rpfile );
      BUFF[num] = 0;
      write_to_buffer( ch->desc, BUFF, num );
   }
}

void do_rip( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Rip ON or OFF?\r\n", ch );
      return;
   }
   if( ( strcmp( arg, "on" ) == 0 ) || ( strcmp( arg, "ON" ) == 0 ) )
   {
      send_rip_screen( ch );
      xSET_BIT( ch->act, PLR_RIP );
      xSET_BIT( ch->act, PLR_ANSI );
      return;
   }

   if( ( strcmp( arg, "off" ) == 0 ) || ( strcmp( arg, "OFF" ) == 0 ) )
   {
      xREMOVE_BIT( ch->act, PLR_RIP );
      send_to_char( "!|*\r\nRIP now off...\r\n", ch );
      return;
   }
}

void do_ansi( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "ANSI ON or OFF?\r\n", ch );
      return;
   }
   if( ( strcmp( arg, "on" ) == 0 ) || ( strcmp( arg, "ON" ) == 0 ) )
   {
      xSET_BIT( ch->act, PLR_ANSI );
      set_char_color( AT_WHITE + AT_BLINK, ch );
      send_to_char( "ANSI ON!!!\r\n", ch );
      return;
   }

   if( ( strcmp( arg, "off" ) == 0 ) || ( strcmp( arg, "OFF" ) == 0 ) )
   {
      xREMOVE_BIT( ch->act, PLR_ANSI );
      send_to_char( "Okay... ANSI support is now off\r\n", ch );
      return;
   }
}

void do_save( CHAR_DATA* ch, const char* argument)
{
   if( IS_NPC( ch ) )
      return;

   WAIT_STATE( ch, 2 ); /* For big muds with save-happy players, like RoD */
   update_aris( ch );   /* update char affects and RIS */
   save_char_obj( ch );
   saving_char = NULL;
   send_to_char( "Saved...\r\n", ch );
   return;
}


/*
 * Something from original DikuMUD that Merc yanked out.
 * Used to prevent following loops, which can cause problems if people
 * follow in a loop through an exit leading back into the same room
 * (Which exists in many maze areas)			-Thoric
 */
bool circle_follow( CHAR_DATA * ch, CHAR_DATA * victim )
{
   CHAR_DATA *tmp;

   for( tmp = victim; tmp; tmp = tmp->master )
      if( tmp == ch )
         return TRUE;
   return FALSE;
}


void do_dismiss( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Dismiss whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ( IS_AFFECTED( victim, AFF_CHARM ) ) && ( IS_NPC( victim ) ) && ( victim->master == ch ) )
   {
      stop_follower( victim );
      stop_hating( victim );
      stop_hunting( victim );
      stop_fearing( victim );
      act( AT_ACTION, "$n dismisses $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "You dismiss $N.", ch, NULL, victim, TO_CHAR );
   }
   else
   {
      send_to_char( "You cannot dismiss them.\r\n", ch );
   }

   return;
}

void do_follow( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Follow whom?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_CHARM ) && ch->master )
   {
      act( AT_PLAIN, "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
      return;
   }

   if( victim == ch )
   {
      if( !ch->master )
      {
         send_to_char( "You already follow yourself.\r\n", ch );
         return;
      }
      stop_follower( ch );
      return;
   }

   if( ( ch->level - victim->level < -10 || ch->level - victim->level > 10 )
       && !IS_HERO( ch ) && !( ch->level < 15 && !IS_NPC( victim )
                               && victim->pcdata->council && !str_cmp( victim->pcdata->council->name, "Newbie Council" ) ) )
   {
      send_to_char( "You are not of the right caliber to follow.\r\n", ch );
      return;
   }

   if( circle_follow( ch, victim ) )
   {
      send_to_char( "Following in loops is not allowed... sorry.\r\n", ch );
      return;
   }

   if( ch->master )
      stop_follower( ch );

   add_follower( ch, victim );
   return;
}



void add_follower( CHAR_DATA * ch, CHAR_DATA * master )
{
   if( ch->master )
   {
      bug( "%s", "Add_follower: non-null master." );
      return;
   }

   ch->master = master;
   ch->leader = NULL;

   /*
    * Support for saving pets --Shaddai
    */
   if( IS_NPC( ch ) && xIS_SET( ch->act, ACT_PET ) && !IS_NPC( master ) )
      master->pcdata->pet = ch;

   if( can_see( master, ch ) )
      act( AT_ACTION, "$n now follows you.", ch, NULL, master, TO_VICT );

   act( AT_ACTION, "You now follow $N.", ch, NULL, master, TO_CHAR );

   return;
}



void stop_follower( CHAR_DATA * ch )
{
   if( !ch->master )
   {
      bug( "%s", "Stop_follower: null master." );
      return;
   }

   if( IS_NPC( ch ) && !IS_NPC( ch->master ) && ch->master->pcdata->pet == ch )
      ch->master->pcdata->pet = NULL;

   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      xREMOVE_BIT( ch->affected_by, AFF_CHARM );
      affect_strip( ch, gsn_charm_person );
      if( !IS_NPC( ch->master ) )
         ch->master->pcdata->charmies--;
   }

   if( can_see( ch->master, ch ) )
      if( !( !IS_NPC( ch->master ) && IS_IMMORTAL( ch ) && !IS_IMMORTAL( ch->master ) ) )
         act( AT_ACTION, "$n stops following you.", ch, NULL, ch->master, TO_VICT );
   act( AT_ACTION, "You stop following $N.", ch, NULL, ch->master, TO_CHAR );

   ch->master = NULL;
   ch->leader = NULL;
   return;
}



void die_follower( CHAR_DATA * ch )
{
   CHAR_DATA *fch;

   if( ch->master )
      stop_follower( ch );

   ch->leader = NULL;

   for( fch = first_char; fch; fch = fch->next )
   {
      if( fch->master == ch )
         stop_follower( fch );
      if( fch->leader == ch )
         fch->leader = fch;
   }
}

void do_order( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   char argbuf[MAX_INPUT_LENGTH];
   char log_buf[MAX_STRING_LENGTH];
   CHAR_DATA *victim;
   CHAR_DATA *och;
   CHAR_DATA *och_next;
   bool found;
   bool fAll;

   mudstrlcpy( argbuf, argument, MAX_INPUT_LENGTH );
   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Order whom to do what?\r\n", ch );
      return;
   }

   if( IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "You feel like taking, not giving, orders.\r\n", ch );
      return;
   }

   if( strstr( argument, "mp" ) != NULL )
   {
      send_to_char( "No.. I don't think so.\r\n", ch );
      return;
   }

   if( !str_cmp( arg, "all" ) )
   {
      fAll = TRUE;
      victim = NULL;
   }
   else
   {
      fAll = FALSE;
      if( ( victim = get_char_room( ch, arg ) ) == NULL )
      {
         send_to_char( "They aren't here.\r\n", ch );
         return;
      }

      if( victim == ch )
      {
         send_to_char( "Aye aye, right away!\r\n", ch );
         return;
      }

      if( !IS_AFFECTED( victim, AFF_CHARM ) || victim->master != ch )
      {
         send_to_char( "Do it yourself!\r\n", ch );
         return;
      }
   }

   found = FALSE;
   for( och = ch->in_room->first_person; och; och = och_next )
   {
      och_next = och->next_in_room;

      if( IS_AFFECTED( och, AFF_CHARM ) && och->master == ch && ( fAll || och == victim ) && !IS_IMMORTAL( och ) )
      {
         found = TRUE;
         act( AT_ACTION, "$n orders you to '$t'.", ch, argument, och, TO_VICT );
         interpret( och, argument );
      }
   }

   if( found )
   {
      snprintf( log_buf, MAX_STRING_LENGTH, "%s: order %s.", ch->name, argbuf );
      log_string_plus( log_buf, LOG_NORMAL, ch->level );
      send_to_char( "Ok.\r\n", ch );
      WAIT_STATE( ch, 12 );
   }
   else
      send_to_char( "You have no followers here.\r\n", ch );
}

/* Overhauled 2/97 -- Blodkai */
void do_group( CHAR_DATA* ch, const char* argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      CHAR_DATA *gch;
      CHAR_DATA *leader;

      leader = ch->leader ? ch->leader : ch;
      set_char_color( AT_DGREEN, ch );
      ch_printf( ch, "\r\nFollowing %-12.12s     [hitpnts]   [ magic ] [mst] [mvs] [race]%s\r\n",
                 PERS( leader, ch ), ch->level < LEVEL_AVATAR ? " [to lvl]" : "" );
      for( gch = first_char; gch; gch = gch->next )
      {
         if( is_same_group( gch, ch ) )
         {
            set_char_color( AT_DGREEN, ch );
            if( IS_AFFECTED( gch, AFF_POSSESS ) )  /*reveal no information */
               ch_printf( ch,
                          "[%2d %s] %-16s %4s/%4s hp %4s/%4s %s %4s/%4s mv %5s xp\r\n",
                          gch->level,
                          IS_NPC( gch ) ? "Mob" : class_table[gch->Class]->who_name,
                          capitalize( PERS( gch, ch ) ),
                          "????", "????", "????", "????", IS_VAMPIRE( gch ) ? "bp" : "mana", "????", "????", "?????" );
            else if( gch->alignment > 750 )
               snprintf( buf, MAX_STRING_LENGTH, "%s", " A" );
            else if( gch->alignment > 350 )
               snprintf( buf, MAX_STRING_LENGTH, "%s", "-A" );
            else if( gch->alignment > 150 )
               snprintf( buf, MAX_STRING_LENGTH, "%s", "+N" );
            else if( gch->alignment > -150 )
               snprintf( buf, MAX_STRING_LENGTH, "%s", " N" );
            else if( gch->alignment > -350 )
               snprintf( buf, MAX_STRING_LENGTH, "%s", "-N" );
            else if( gch->alignment > -750 )
               snprintf( buf, MAX_STRING_LENGTH, "%s", "+S" );
            else
               snprintf( buf, MAX_STRING_LENGTH, "%s", " S" );
            set_char_color( AT_DGREEN, ch );
            send_to_char( "[", ch );
            set_char_color( AT_GREEN, ch );
            ch_printf( ch, "%-2d %2.2s %3.3s", gch->level, buf, IS_NPC( gch ) ? "Mob" : class_table[gch->Class]->who_name );
            set_char_color( AT_DGREEN, ch );
            send_to_char( "]  ", ch );
            set_char_color( AT_GREEN, ch );
            ch_printf( ch, "%-12.12s ", capitalize( PERS( gch, ch ) ) );
            if( gch->hit < gch->max_hit / 4 )
               set_char_color( AT_DANGER, ch );
            else if( gch->hit < gch->max_hit / 2.5 )
               set_char_color( AT_YELLOW, ch );
            else
               set_char_color( AT_GREY, ch );
            ch_printf( ch, "%5d", gch->hit );
            set_char_color( AT_GREY, ch );
            ch_printf( ch, "/%-5d ", gch->max_hit );
            if( IS_VAMPIRE( gch ) )
               set_char_color( AT_BLOOD, ch );
            else
               set_char_color( AT_LBLUE, ch );
            if( gch->Class != CLASS_WARRIOR )
               ch_printf( ch, "%5d/%-5d ",
                          IS_VAMPIRE( gch ) ? gch->pcdata->condition[COND_BLOODTHIRST] : gch->mana,
                          IS_VAMPIRE( gch ) ? 10 + gch->level : gch->max_mana );
            else
               send_to_char( "            ", ch );
            if( gch->mental_state < -25 || gch->mental_state > 25 )
               set_char_color( AT_YELLOW, ch );
            else
               set_char_color( AT_GREEN, ch );
            ch_printf( ch, "%3.3s  ",
                       gch->mental_state > 75 ? "+++" :
                       gch->mental_state > 50 ? "=++" :
                       gch->mental_state > 25 ? "==+" :
                       gch->mental_state > -25 ? "===" :
                       gch->mental_state > -50 ? "-==" : gch->mental_state > -75 ? "--=" : "---" );
            set_char_color( AT_DGREEN, ch );
            ch_printf( ch, "%5d ", gch->move );
            ch_printf( ch, "%6s ",
                       gch->race == 0 ? "human" :
                       gch->race == 1 ? "dwarf" :
                       gch->race == 2 ? "elf" :
                       gch->race == 3 ? "half-elf" :
                       gch->race == 4 ? "gnome" :
                       gch->race == 5 ? "catfolk" :
                       gch->race == 6 ? "wolfolk" :
                       gch->race == 7 ? "cyclops" :
                       gch->race == 8 ? "firebird" :
                       gch->race == 9 ? "benn" :
                       gch->race == 10 ? "merfolk" :
                       gch->race == 11 ? "naga" :
                       gch->race == 12 ? "dryad" :
                       gch->race == 13 ? "naiad" :
                       gch->race == 14 ? "lizardfolk" :
                       gch->race == 15 ? "dollfolk" :
                       gch->race == 16 ? "fairy" :
                       gch->race == 17 ? "satyr" :
                       gch->race == 18 ? "minotaur" :
                       gch->race == 19 ? "centaur" :
                       gch->race == 20 ? "sphinx" :
                       gch->race == 21 ? "salamander" :
                       gch->race == 22 ? "gryphon" :
                       gch->race == 23 ? "dragon" :
                       gch->race == 24 ? "unicorn" :
                       gch->race == 25 ? "slime" : "" );
            set_char_color( AT_GREEN, ch );
            if( gch->level < LEVEL_AVATAR )
               ch_printf( ch, "%8d ", exp_level( gch, gch->level + 1 ) - gch->exp );
            send_to_char( "\r\n", ch );
         }
      }
      return;
   }

   if( !strcmp( arg, "disband" ) )
   {
      CHAR_DATA *gch;
      int count = 0;

      if( ch->leader || ch->master )
      {
         send_to_char( "You cannot disband a group if you're following someone.\r\n", ch );
         return;
      }

      for( gch = first_char; gch; gch = gch->next )
      {
         if( is_same_group( ch, gch ) && ( ch != gch ) )
         {
            gch->leader = NULL;
            gch->master = NULL;
            count++;
            send_to_char( "Your group is disbanded.\r\n", gch );
         }
      }

      if( count == 0 )
         send_to_char( "You have no group members to disband.\r\n", ch );
      else
         send_to_char( "You disband your group.\r\n", ch );

      return;
   }

   if( !strcmp( arg, "all" ) )
   {
      CHAR_DATA *rch;
      int count = 0;

      for( rch = ch->in_room->first_person; rch; rch = rch->next_in_room )
      {
         if( ch != rch
             && !IS_NPC( rch )
             && can_see( ch, rch )
             && rch->master == ch
             && !ch->master
             && !ch->leader
             && abs( ch->level - rch->level ) < 9 && !is_same_group( rch, ch ) && IS_PKILL( ch ) == IS_PKILL( rch ) )
         {
            rch->leader = ch;
            count++;
         }
      }

      if( count == 0 )
         send_to_char( "You have no eligible group members.\r\n", ch );
      else
      {
         act( AT_ACTION, "$n groups $s followers.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You group your followers.\r\n", ch );
      }
      return;
   }

   if( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ch->master || ( ch->leader && ch->leader != ch ) )
   {
      send_to_char( "But you are following someone else!\r\n", ch );
      return;
   }

   if( victim->master != ch && ch != victim && victim->mount != ch )
   {
      act( AT_PLAIN, "$N isn't following you.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim == ch )
   {
      act( AT_PLAIN, "You can't group yourself.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( is_same_group( victim, ch ) && ch != victim )
   {
      victim->leader = NULL;
      act( AT_ACTION, "$n removes $N from $s group.", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "$n removes you from $s group.", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "You remove $N from your group.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( ch->level - victim->level < -8 || ch->level - victim->level > 8 || ( IS_PKILL( ch ) != IS_PKILL( victim ) ) )
   {
      act( AT_PLAIN, "$N cannot join $n's group.", ch, NULL, victim, TO_NOTVICT );
      act( AT_PLAIN, "You cannot join $n's group.", ch, NULL, victim, TO_VICT );
      act( AT_PLAIN, "$N cannot join your group.", ch, NULL, victim, TO_CHAR );
      return;
   }

   victim->leader = ch;
   act( AT_ACTION, "$N joins $n's group.", ch, NULL, victim, TO_NOTVICT );
   act( AT_ACTION, "You join $n's group.", ch, NULL, victim, TO_VICT );
   act( AT_ACTION, "$N joins your group.", ch, NULL, victim, TO_CHAR );
   return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split( CHAR_DATA* ch, const char* argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *gch;
   int members;
   int amount;
   int share;
   int extra;

   one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      send_to_char( "Split how much?\r\n", ch );
      return;
   }

   amount = atoi( arg );

   if( amount < 0 )
   {
      send_to_char( "Your group wouldn't like that.\r\n", ch );
      return;
   }

   if( amount == 0 )
   {
      send_to_char( "You hand out zero coins, but no one notices.\r\n", ch );
      return;
   }

   if( ch->gold < amount )
   {
      send_to_char( "You don't have that much gold.\r\n", ch );
      return;
   }

   members = 0;
   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( is_same_group( gch, ch ) )
         members++;
   }


   if( xIS_SET( ch->act, PLR_AUTOGOLD ) && members < 2 )
      return;

   if( members < 2 )
   {
      send_to_char( "Just keep it all.\r\n", ch );
      return;
   }

   share = amount / members;
   extra = amount % members;

   if( share == 0 )
   {
      send_to_char( "Don't even bother, cheapskate.\r\n", ch );
      return;
   }

   ch->gold -= amount;
   ch->gold += share + extra;

   set_char_color( AT_GOLD, ch );
   ch_printf( ch, "You split %d gold coins.  Your share is %d gold coins.\r\n", amount, share + extra );

   snprintf( buf, MAX_STRING_LENGTH, "$n splits %d gold coins.  Your share is %d gold coins.", amount, share );

   for( gch = ch->in_room->first_person; gch; gch = gch->next_in_room )
   {
      if( gch != ch && is_same_group( gch, ch ) )
      {
         act( AT_GOLD, buf, ch, NULL, gch, TO_VICT );
         gch->gold += share;
      }
   }
   return;
}

void do_gtell( CHAR_DATA* ch, const char* argument)
{
   CHAR_DATA *gch;
#ifndef SCRAMBLE
   int speaking = -1, lang;

   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( ch->speaking & lang_array[lang] )
      {
         speaking = lang;
         break;
      }
#endif

   if( argument[0] == '\0' )
   {
      send_to_char( "Tell your group what?\r\n", ch );
      return;
   }

   if( xIS_SET( ch->act, PLR_NO_TELL ) )
   {
      send_to_char( "Your message didn't get through!\r\n", ch );
      return;
   }

   /*
    * Note use of send_to_char, so gtell works on sleepers.
    */
   for( gch = first_char; gch; gch = gch->next )
   {
      if( is_same_group( gch, ch ) )
      {
         set_char_color( AT_GTELL, gch );
         /*
          * Groups unscrambled regardless of clan language.  Other languages
          * still garble though. -- Altrag
          */
#ifndef SCRAMBLE
         if( speaking != -1 && ( !IS_NPC( ch ) || ch->speaking ) )
         {
            int speakswell = UMIN( knows_language( gch, ch->speaking, ch ),
                                   knows_language( ch, ch->speaking, gch ) );

            if( speakswell < 85 )
               ch_printf( gch, "%s tells the group '%s'.\r\n", ch->name,
                          translate( speakswell, argument, lang_names[speaking] ) );
            else
               ch_printf( gch, "%s tells the group '%s'.\r\n", ch->name, argument );
         }
         else
            ch_printf( gch, "%s tells the group '%s'.\r\n", ch->name, argument );
#else
         if( knows_language( gch, ch->speaking, gch ) || ( IS_NPC( ch ) && !ch->speaking ) )
            ch_printf( gch, "%s tells the group '%s'.\r\n", ch->name, argument );
         else
            ch_printf( gch, "%s tells the group '%s'.\r\n", ch->name, scramble( argument, ch->speaking ) );
#endif
      }
   }

   return;
}


/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( CHAR_DATA * ach, CHAR_DATA * bch )
{
   if( ach->leader )
      ach = ach->leader;
   if( bch->leader )
      bch = bch->leader;
   return ach == bch;
}

/*
 * this function sends raw argument over the AUCTION: channel
 * I am not too sure if this method is right..
 */

void talk_auction( char *argument )
{
   DESCRIPTOR_DATA *d;
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *original;

   snprintf( buf, MAX_STRING_LENGTH, "Auction: %s", argument );   /* last %s to reset color */

   for( d = first_descriptor; d; d = d->next )
   {
      original = d->original ? d->original : d->character;  /* if switched */
      if( ( d->connected == CON_PLAYING ) && !IS_SET( original->deaf, CHANNEL_AUCTION )
          && !xIS_SET( original->in_room->room_flags, ROOM_SILENCE ) && !NOT_AUTHED( original ) )
         act( AT_GOSSIP, buf, original, NULL, NULL, TO_CHAR );
   }
}

/*
 * Language support functions. -- Altrag
 * 07/01/96
 *
 * Modified to return how well the language is known 04/04/98 - Thoric
 * Currently returns 100% for known languages... but should really return
 * a number based on player's wisdom (maybe 50+((25-wisdom)*2) ?)
 */
int knows_language( CHAR_DATA * ch, int language, CHAR_DATA * cch )
{
   short sn;

   if( !IS_NPC( ch ) && IS_IMMORTAL( ch ) )
      return 100;
   if( IS_NPC( ch ) && !ch->speaks )   /* No langs = knows all for npcs */
      return 100;

   if( !IS_NPC( ch ) )
   {
      int lang;

      /*
       * Racial languages for PCs
       */
      if( IS_SET( race_table[ch->race]->language, language ) )
         return 100;

      for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
         if( IS_SET( language, lang_array[lang] ) && IS_SET( ch->speaks, lang_array[lang] ) )
         {
            if( ( sn = skill_lookup( lang_names[lang] ) ) != -1 )
               return ch->pcdata->learned[sn];
         }
   }
   return 0;
}

bool can_learn_lang( CHAR_DATA * ch, int language )
{
   if( IS_NPC( ch ) || IS_IMMORTAL( ch ) )
      return FALSE;
   if( race_table[ch->race]->language & language )
      return FALSE;
   if( ch->speaks & language )
   {
      int lang;

      for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
         if( language & lang_array[lang] )
         {
            int sn;

            if( !( VALID_LANGS & lang_array[lang] ) )
               return FALSE;
            if( ( sn = skill_lookup( lang_names[lang] ) ) < 0 )
            {
               bug( "Can_learn_lang: valid language without sn: %d", lang );
               continue;
            }
            if( ch->pcdata->learned[sn] >= 99 )
               return FALSE;
         }
   }
   if( VALID_LANGS & language )
      return TRUE;
   return FALSE;
}

int const lang_array[] = {
	LANG_COLISTANI, LANG_CAOLIDAN, LANG_DORISH, LANG_DARDI,
    LANG_LAVANDAI, LANG_VALAO, LANG_FTAGHN, LANG_FAHN, LANG_VAOLUMIN,
    LANG_CHYA, LANG_FENRADYA, LANG_SIRU, LANG_SOLESE, LANG_RUSSA,
    LANG_WOLOF, LANG_CORUSTENYA, LANG_SIMGH, LANG_ASKRAVAARDI, LANG_GREGA,
    LANG_WILDTONGUE, LANG_YRANIB, LANG_MIJ, LANG_SAULOON, LANG_VEDRIC,
    LANG_UNKNOWN
};

const char *const lang_names[] = {
   "colistani", "caolidan", "dorish", "dardi",
   "lavandai", "valao", "ftaghn", "fahn", "vaolumin",
   "chya", "fenradya", "siru", "solese", "russa",
   "wolof", "corustenya", "simgh", "askravaardi", "grega",
   "wildtongue", "yranib", "mij", "sauloon", "vedric",
   "", "", "", "", "", "", "", "", ""  /* pad to 32 for compat with flag_string */
};

/* Note: does not count racial language.  This is intentional (for now). */
int countlangs( int languages )
{
   int numlangs = 0;
   int looper;

   for( looper = 0; lang_array[looper] != LANG_UNKNOWN; looper++ )
   {
      if( languages & lang_array[looper] )
         numlangs++;
   }
   return numlangs;
}

void do_speak( CHAR_DATA* ch, const char* argument)
{
   int langs;
   char arg[MAX_INPUT_LENGTH];

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' )
   {
      for( langs = 0; lang_array[langs] != LANG_UNKNOWN; langs++ )
         if( lang_array[langs] & ch->speaking )
         {
            set_char_color( AT_SAY, ch );
            ch_printf( ch, "You are currently speaking %s.\r\n", lang_names[langs] );
         }
      return;
   }

   if( !str_cmp( arg, "all" ) && IS_IMMORTAL( ch ) )
   {
      set_char_color( AT_SAY, ch );
      send_to_char( "Now speaking all languages.\r\n", ch );
      return;
   }

   for( langs = 0; lang_array[langs] != LANG_UNKNOWN; langs++ )
      if( !str_prefix( arg, lang_names[langs] ) )
         if( knows_language( ch, lang_array[langs], ch ) )
         {
            ch->speaking = lang_array[langs];
            set_char_color( AT_SAY, ch );
            ch_printf( ch, "You now speak %s.\r\n", lang_names[langs] );
            return;
         }
   set_char_color( AT_SAY, ch );
   send_to_char( "You do not know that language.\r\n", ch );
}

/*  Removing for now, since we have our own languages command -Danny
void do_languages( CHAR_DATA* ch, const char* argument)
{
   char arg[MAX_INPUT_LENGTH];
   int lang;

   argument = one_argument( argument, arg );
   if( arg[0] != '\0' && !str_prefix( arg, "learn" ) && !IS_IMMORTAL( ch ) && !IS_NPC( ch ) )
   {
      CHAR_DATA *sch;
      char arg2[MAX_INPUT_LENGTH];
      int sn;
      int prct;
      int prac;

      argument = one_argument( argument, arg2 );
      if( arg2[0] == '\0' )
      {
         send_to_char( "Learn which language?\r\n", ch );
         return;
      }
      for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      {
         if( !str_prefix( arg2, lang_names[lang] ) )
            break;
      }
      if( lang_array[lang] == LANG_UNKNOWN )
      {
         send_to_char( "That is not a language.\r\n", ch );
         return;
      }
      if( !( VALID_LANGS & lang_array[lang] ) )
      {
         send_to_char( "You may not learn that language.\r\n", ch );
         return;
      }
      if( ( sn = skill_lookup( lang_names[lang] ) ) < 0 )
      {
         send_to_char( "That is not a language.\r\n", ch );
         return;
      }
      if( race_table[ch->race]->language & lang_array[lang] || ch->pcdata->learned[sn] >= 99 )
      {
         act( AT_PLAIN, "You are already fluent in $t.", ch, lang_names[lang], NULL, TO_CHAR );
         return;
      }
      for( sch = ch->in_room->first_person; sch; sch = sch->next_in_room )
         if( IS_NPC( sch ) && xIS_SET( sch->act, ACT_SCHOLAR )
             && knows_language( sch, ch->speaking, ch )
             && knows_language( sch, lang_array[lang], sch )
             && ( !sch->speaking || knows_language( ch, sch->speaking, sch ) ) )
            break;
      if( !sch )
      {
         send_to_char( "There is no one who can teach that language here.\r\n", ch );
         return;
      }
      if( countlangs( ch->speaks ) >= ( ch->level / 10 ) && ch->pcdata->learned[sn] <= 0 )
      {
         act( AT_TELL, "$n tells you 'You may not learn a new language yet.'", sch, NULL, ch, TO_VICT );
         return;
      }
       *
       * 0..16 cha = 2 pracs, 17..25 = 1 prac. -- Altrag
       *
      prac = 2 - ( get_curr_cha( ch ) / 17 );
      if( ch->practice < prac )
      {
         act( AT_TELL, "$n tells you 'You do not have enough practices.'", sch, NULL, ch, TO_VICT );
         return;
      }
      ch->practice -= prac;
       *
       * Max 12% (5 + 4 + 3) at 24+ int and 21+ wis. -- Altrag
       *
      prct = 5 + ( get_curr_int( ch ) / 6 ) + ( get_curr_wis( ch ) / 7 );
      ch->pcdata->learned[sn] += prct;
      ch->pcdata->learned[sn] = UMIN( ch->pcdata->learned[sn], 99 );
      SET_BIT( ch->speaks, lang_array[lang] );
      if( ch->pcdata->learned[sn] == prct )
         act( AT_PLAIN, "You begin lessons in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else if( ch->pcdata->learned[sn] < 60 )
         act( AT_PLAIN, "You continue lessons in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else if( ch->pcdata->learned[sn] < 60 + prct )
         act( AT_PLAIN, "You feel you can start communicating in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else if( ch->pcdata->learned[sn] < 99 )
         act( AT_PLAIN, "You become more fluent in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else
         act( AT_PLAIN, "You now speak perfect $t.", ch, lang_names[lang], NULL, TO_CHAR );
      return;
   }
   for( lang = 0; lang_array[lang] != LANG_UNKNOWN; lang++ )
      if( knows_language( ch, lang_array[lang], ch ) )
      {
         if( ch->speaking & lang_array[lang] || ( IS_NPC( ch ) && !ch->speaking ) )
            set_char_color( AT_SAY, ch );
         else
            set_char_color( AT_PLAIN, ch );
         send_to_char( lang_names[lang], ch );
         send_to_char( "\r\n", ch );
      }
   send_to_char( "\r\n", ch );
   return;
}
*/

void do_pvp( CHAR_DATA* ch, const char* argument)
{
   if( NOT_AUTHED( ch ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }
   talk_channel( ch, argument, CHANNEL_PVP, "PVP" );
   return;
}


