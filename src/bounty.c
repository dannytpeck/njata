/***************************************************************************
*                           STAR WARS REALITY 1.0                          *
*--------------------------------------------------------------------------*
* Star Wars Reality Code Additions and changes from the Smaug Code         *
* copyright (c) 1997 by Sean Cooper                                        *
* -------------------------------------------------------------------------*
* Starwars and Starwars Names copyright(c) Lucas Film Ltd.                 *
*--------------------------------------------------------------------------*
* SMAUG 1.0 (C) 1994, 1995, 1996 by Derek Snider                           *
* SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,                    *
* Scryn, Rennard, Swordbearer, Gorog, Grishnakh and Tricops                *
* ------------------------------------------------------------------------ *
* Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
* Chastain, Michael Quan, and Mitchell Tse.                                *
* Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
* Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
* ------------------------------------------------------------------------ *
*		            Bounty Hunter Module    			   *
*                    (  and area capturing as well  )                      *
****************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "mud.h"

BOUNTY_DATA * first_bounty;
BOUNTY_DATA * last_bounty;
BOUNTY_DATA * first_disintigration;
BOUNTY_DATA * last_disintigration;


void disintigration args ( ( CHAR_DATA *ch, CHAR_DATA *victim, long amount ) );
void nodisintigration args ( ( CHAR_DATA *ch, CHAR_DATA *victim, long amount ) );
int  xp_compute( CHAR_DATA *ch, CHAR_DATA *victim);
void show_char_to_char( CHAR_DATA *list, CHAR_DATA *ch );


void save_disintigrations( )
{
	BOUNTY_DATA *tbounty;
    FILE *fpout;
    char filename[256];

    sprintf( filename, "%s%s", SYSTEM_DIR, BOUNTY_LIST );
    fpout = fopen( filename, "w" );
    if ( !fpout )
    {
      	bug( "%s", "save_disintigrations: can't open bounty.lst." );
		return;
    }
    for ( tbounty = first_disintigration; tbounty; tbounty = tbounty->next )
    {
        fprintf( fpout, "%s\n", tbounty->target );
        fprintf( fpout, "%ld\n", tbounty->amount );
    }
    fprintf( fpout, "$\n" );
    fclose( fpout );

}


bool is_disintigration( CHAR_DATA *victim )
{
    BOUNTY_DATA *bounty;

    for ( bounty = first_disintigration; bounty; bounty = bounty->next )
    if ( !str_cmp( victim->name , bounty->target ) )
  		return TRUE;
    return FALSE;
}

BOUNTY_DATA *get_disintigration( const char *target )
{
    BOUNTY_DATA *bounty;

    for ( bounty = first_disintigration; bounty; bounty = bounty->next )
		if ( !str_cmp( target, bounty->target ) )
			return bounty;
    return NULL;
}

void load_bounties( )
{
    FILE *fpList;
    const char *target;
    char bountylist[256];
    BOUNTY_DATA *bounty;
    long int amount;

    first_disintigration = NULL;
    last_disintigration	= NULL;

    sprintf( bountylist, "%s%s", SYSTEM_DIR, BOUNTY_LIST );
    if ( ( fpList = fopen( bountylist, "r" ) ) == NULL )
    {
		perror( bountylist );
		exit( 1 );
    }

    for ( ; ; )
    {
        target = feof( fpList ) ? "$" : fread_word( fpList );
        if ( target[0] == '$' )
        	break;
		CREATE( bounty, BOUNTY_DATA, 1 );
		LINK( bounty, first_disintigration, last_disintigration, next, prev );
		bounty->target = STRALLOC(target);
		amount = fread_number( fpList );
		bounty->amount = amount;
    }
    fclose( fpList );
    log_string("Done." );

    return;
}

void do_bounties( CHAR_DATA *ch, const char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    bool found;
    BOUNTY_DATA *bounty;
    int count = 0;
    long amount;

    one_argument( argument, arg );

    set_char_color( AT_WHITE, ch );
    send_to_char( "\n\rBounty                     Amount\n\r", ch );
    if ( arg[0] == '\0' )
   	{
    	for ( bounty = first_disintigration; bounty; bounty = bounty->next )
    	{
			amount = bounty->amount;
        	set_char_color( AT_RED, ch );
			ch_printf( ch, "%-26s %-14s\n\r", bounty->target, num_punct(bounty->amount));
        	count++;
    	}

    	if ( !count )
    	{
        	set_char_color( AT_GREY, ch );
        	send_to_char( "There are no bounties set at this time.\n\r", ch );
		return;
    	}
   	}
    else
   	{
    	found = FALSE;
        for ( d = first_descriptor; d; d = d->next )
  			if ( (d->connected == CON_PLAYING || d->connected == CON_EDITING )
             && ( victim = d->character ) != NULL
             &&   !IS_NPC(victim)
	         &&  !xIS_SET( victim->act, PLR_WIZINVIS)
             &&  (bounty = get_disintigration( victim->name )) > 0)
            {
    			found = TRUE;
				set_char_color( AT_RED, ch );
				amount = bounty->amount;
	       		ch_printf( ch, "%-26s %-14s\n\r", victim->name, num_punct(amount) );
            }
        if ( !found )
		{
           	send_to_char( "None\n\r", ch );
	    	return;
		}
    }
}

void disintigration ( CHAR_DATA *ch, CHAR_DATA *victim, long amount )
{
    BOUNTY_DATA *bounty;
    bool found;

    found = FALSE;

    for ( bounty = first_disintigration; bounty; bounty = bounty->next )
    {
    	if ( !str_cmp( bounty->target, victim->name ))
    	{
    		found = TRUE;
    		break;
    	}
    }

    if (! found)
    {
        CREATE( bounty, BOUNTY_DATA, 1 );
        LINK( bounty, first_disintigration, last_disintigration, next, prev );

        bounty->target = STRALLOC( victim->name );
        bounty->amount = 0;
    }

    bounty->amount      = bounty->amount + amount;
    save_disintigrations();
}

void do_addbounty( CHAR_DATA *ch, const char *argument )
{
    char arg[MAX_STRING_LENGTH];
    long int amount;
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];
    BOUNTY_DATA *bounty;

    argument = one_argument(argument, arg);

    if( !argument || argument[0] == '\0' )
    {
    	send_to_char( "Usage: Addbounty <target> <amount>\n\r", ch );
    	return;
    }

   	if( argument[0] == '\0' )
   	    amount = 0;
   	else
   		amount = atoi (argument);

   	if( amount < 100 )
   	{
   		send_to_char( "A bounty should be at least 100 gold.\n\r", ch );
   		return;
   	}

	if( amount > 2000000000)
	{
		send_to_char( "You can't put that large of a bounty on someone!\r\n", ch);
		return;
	}

   	if( !(victim = get_char_world( ch, arg )) )
   	{
   	    send_to_char( "They don't appear to be here, wait until they log in.\n\r", ch );
   	    return;
   	}

   	if( ch == victim && !IS_IMMORTAL(ch))
   	{
		send_to_char("You can't place bounties on yourself!\n\r", ch);
		return;
   	}

	bounty = get_disintigration( arg );

   	if( IS_NPC(victim) )
   	{
   		send_to_char( "You can only set bounties on other players .. not mobs!\n\r", ch );
		return;
   	}

   	if( amount <= 0)
   	{
   	    send_to_char( "Nice try! How about 100 or more gold instead.\n\r", ch );
   	    return;
   	}

   	if( ch->gold < amount)
   	{
   		send_to_char( "You don't have that much gold!\n\r", ch );
   		return;
   	}

	if( bounty && ( bounty->amount + amount ) > 2000000000)
	{
		send_to_char( "A bounty cannot be over 2 billion gold!\r\n", ch);
		return;
	}

   	ch->gold = ch->gold - amount;
   	send_to_char( "Bounty has been added.\n\r", ch );
   	sprintf( buf, "%s has added %s gold to the bounty on %s.", ch->name, num_punct(amount) , victim->name );
    echo_to_all ( AT_RED , buf, 0 );
   	disintigration( ch, victim, amount);

}

void do_rembounty( CHAR_DATA *ch, const char *argument )
{
  	BOUNTY_DATA *bounty = get_disintigration( argument );

  	if( bounty != NULL )
  	{
    	remove_disintigration(bounty);
		send_to_char( "Bounty has been removed.\n\r", ch );
  	}
  	return;
}

void remove_disintigration( BOUNTY_DATA *bounty )
{
	UNLINK( bounty, first_disintigration, last_disintigration, next, prev );
	STRFREE( bounty->target );
	DISPOSE( bounty );

	save_disintigrations();
}

void claim_disintigration( CHAR_DATA *ch, CHAR_DATA *victim )
{
	BOUNTY_DATA *bounty;
	char buf[MAX_STRING_LENGTH];

    if( IS_NPC(victim) )
    	return;

	bounty = get_disintigration( victim->name );

    if( ch == victim )
    {
    	if( bounty != NULL )
        remove_disintigration(bounty);
        return;
	}

    sprintf( buf, "%s has been captured!", victim->name);
	echo_to_all ( AT_RED , buf, 0 );
	if( ch->gold + bounty->amount < 2000000000)
	{
		ch->gold += bounty->amount;
		ch_printf( ch, "You receive %ld gold from the bounty on %s\n\r", bounty->amount, bounty->target );
	}
	else
		send_to_char( "You're carrying too much gold to receive your bounty!\n\r", ch);

	set_char_color( AT_BLOOD, ch );
	sprintf( buf, "The bounty on %s has been claimed!", victim->name);
	echo_to_all ( AT_RED , buf, 0 );
	sprintf( buf, "%s is rushed away to the dungeons.", victim->name );
	echo_to_all ( AT_RED , buf, 0 );

	if( xIS_SET(victim->act, PLR_THIEF) && !IS_NPC(ch) && !IS_NPC(victim) )
    {
		xREMOVE_BIT( victim->act, PLR_THIEF);
        sprintf( buf, "The wretched thief %s has been killed!", victim->name);
        echo_to_all ( AT_RED , buf, 0 );
	}

	remove_disintigration( bounty );
	return;

}




