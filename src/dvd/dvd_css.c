//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - libdvdcss hardware CSS replacement
 *                              (parts of code from libdvdcss)
 *  \file       dvd/dvd_css.c
 *  \author     bombur
 *  \version    0.1
 *  \date       7.02.2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 */
//////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "config.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning(disable: 4201)
#include <winioctl.h>
#pragma warning(default: 4201)
#else
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#endif
#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif
#include <fcntl.h>

#ifdef HAVE_LIMITS_H
#   include <limits.h>
#endif

// these are from libdvdcss:
#include "dvdcss/dvdcss.h"
#include "common.h"
#include "css.h"
#ifdef WIN32
#pragma warning(disable: 4200)
#pragma warning(disable: 4201)
#pragma warning(disable: 4214)
#endif
#include "ioctl.h"
#ifdef WIN32
#pragma warning(default: 4200)
#pragma warning(default: 4201)
#pragma warning(default: 4214)
#endif
#include "device.h"
#include "libdvdcss.h"

#include <libsp/sp_misc.h>
#include <libsp/sp_khwl.h>

/////////////////////////////////////////////
#ifdef DVDCSS_USE_EXTERNAL_CSS

/////////////////////////
//#define OUR_DEBUG_OUTPUT
/////////////////////////

#ifdef OUR_DEBUG_OUTPUT
void msg(char *text, ...);
#define debug_error(e) msg("dvdcss err: %s\n", e)
#define debug_msg(m) msg("dvdcss: %s\n", m)
#else
#define debug_error(e) 
//_dvdcss_error( dvdcss, e)
#define debug_msg(m) 
//_dvdcss_debug( dvdcss, m)
#endif


extern int mv_cgms_flags;

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int  GetBusKey       ( dvdcss_t );
static int  GetASF          ( dvdcss_t );
static int ReadTitleKey(int i_fd, int *pi_agid, int i_pos, uint8_t *p_key, int *cgms_flags);


/*****************************************************************************
 * _dvdcss_test: check if the disc is encrypted or not
 *****************************************************************************/
int _dvdcss_test( dvdcss_t dvdcss )
{
    int i_ret, i_copyright;
	
    i_ret = ioctl_ReadCopyright( dvdcss->i_fd, 0 /* i_layer */, &i_copyright );

#ifdef WIN32
    if( i_ret < 0 )
    {
        /* Maybe we didn't have enough priviledges to read the copyright
         * (see ioctl_ReadCopyright comments).
         * Apparently, on unencrypted DVDs _dvdcss_disckey() always fails, so
         * we can check this as a work-around. */
        i_ret = 0;
        if( _dvdcss_disckey( dvdcss ) < 0 )
            i_copyright = 0;
        else
            i_copyright = 1;
    }
#endif

    if( i_ret < 0 )
    {
        /* Since it's the first ioctl we try to issue, we add a notice */
        debug_error("css error: ioctl_ReadCopyright failed, "
                       "make sure there is a DVD in the drive, and that "
                       "you have used the correct device node." );

        return i_ret;
    }

    return i_copyright;
}

/*****************************************************************************
 * GetBusKey : Go through the CSS Authentication process
 *****************************************************************************
 * It simulates the mutual authentication between logical unit and host,
 * and stops when a session key (called bus key) has been established.
 * Always do the full auth sequence. Some drives seem to lie and always
 * respond with ASF=1.  For instance the old DVD roms on Compaq Armada says
 * that ASF=1 from the start and then later fail with a 'read of scrambled
 * block without authentication' error.
 *****************************************************************************/
static int GetBusKey( dvdcss_t dvdcss )
{
    uint8_t   p_challenge[2*KEY_SIZE];
    uint8_t   p_challenge2[2*KEY_SIZE];
    dvd_key_t p_key1;
    dvd_key_t p_key2;
    char      psz_warning[80];
    int       i_ret = -1;
    int       i;

    debug_msg("requesting AGID" );
    i_ret = ioctl_ReportAgid( dvdcss->i_fd, &dvdcss->css.i_agid );

    /* We might have to reset hung authentication processes in the drive
       by invalidating the corresponding AGID'.  As long as we haven't got
       an AGID, invalidate one (in sequence) and try again. */
    for( i = 0; i_ret == -1 && i < 4 ; ++i )
    {
        sprintf( psz_warning,
                 "ioctl ReportAgid failed, invalidating AGID %d", i );
        debug_msg(psz_warning );

        /* This is really _not good_, should be handled by the OS.
           Invalidating an AGID could make another process fail some
           where in it's authentication process. */
        dvdcss->css.i_agid = i;
        ioctl_InvalidateAgid( dvdcss->i_fd, &dvdcss->css.i_agid );

        debug_msg("requesting AGID (2)" );
        i_ret = ioctl_ReportAgid( dvdcss->i_fd, &dvdcss->css.i_agid );
    }

    /* Unable to authenticate without AGID */
    if( i_ret == -1 )
    {
        debug_error("ioctl ReportAgid failed, fatal" );
        return -1;
    }

    /* Get challenge from host */
    khwl_getproperty(KHWL_DECODER_SET, edecCSSChlg, sizeof(p_challenge), p_challenge);

    /* Send challenge to LU */
    if( ioctl_SendChallenge( dvdcss->i_fd,
                             &dvdcss->css.i_agid, p_challenge) < 0 )
    {
        debug_error("ioctl SendChallenge failed" );
        ioctl_InvalidateAgid( dvdcss->i_fd, &dvdcss->css.i_agid );
        return -1;
    }

    /* Get key1 from LU */
    if( ioctl_ReportKey1( dvdcss->i_fd, &dvdcss->css.i_agid, p_key1) < 0)
    {
        debug_error("ioctl ReportKey1 failed" );
        ioctl_InvalidateAgid( dvdcss->i_fd, &dvdcss->css.i_agid );
        return -1;
    }

    /* Send key1 to host */
    khwl_setproperty(KHWL_DECODER_SET, edecCSSKey1, KEY_SIZE, p_key1);

    /* Get challenge from LU */
    if( ioctl_ReportChallenge( dvdcss->i_fd,
                               &dvdcss->css.i_agid, p_challenge2) < 0 )
    {
        debug_error("ioctl ReportKeyChallenge failed" );
        ioctl_InvalidateAgid( dvdcss->i_fd, &dvdcss->css.i_agid );
        return -1;
    }

    /* Send challenge to host */
    khwl_setproperty(KHWL_DECODER_SET, edecCSSChlg2, sizeof(p_challenge2), p_challenge2);

	/* Get key2 from host */
	khwl_getproperty(KHWL_DECODER_SET, edecCSSKey2, sizeof(p_key2), p_key2);

    /* Send key2 to LU */
    if( ioctl_SendKey2( dvdcss->i_fd, &dvdcss->css.i_agid, p_key2) < 0 )
    {
        debug_error("AUTH: ioctl SendKey2 failed" );
        ioctl_InvalidateAgid( dvdcss->i_fd, &dvdcss->css.i_agid );
        return -1;
    }

    /* The drive has accepted us as authentic. */
    debug_msg("AUTH established" );

    return 0;
}

/*****************************************************************************
 * _dvdcss_title: crack or decrypt the current title key if needed
 *****************************************************************************
 * This function should only be called by dvdcss->pf_seek and should eventually
 * not be external if possible.
 *****************************************************************************/
int _dvdcss_title ( dvdcss_t dvdcss, int i_block )
{
    dvd_key_t    p_title_key;
    int          i_ret = -1;

//	debug_msg("_dvdcss_title() start" );

    if( ! dvdcss->b_scrambled )
    {
        debug_msg("_dvdcss_title() end (not scrambled)" );
		return 0;
    }

//	debug_msg("_dvdcss_title() 2" );

    /* Crack or decrypt CSS title key for current VTS */
    i_ret = _dvdcss_titlekey( dvdcss, i_block, p_title_key );

    if( i_ret < 0 )
    {
        debug_error("fatal error in vts css key" );
        return i_ret;
    }

	memcpy( dvdcss->css.p_title_key, p_title_key, KEY_SIZE );

	debug_msg("_dvdcss_title() end" );

    return 0;
}

/*****************************************************************************
 * _dvdcss_disckey: get disc key.
 *****************************************************************************
 * This function should only be called if DVD ioctls are present.
 * It will set dvdcss->i_method = DVDCSS_METHOD_TITLE if it fails to find
 * a valid disc key.
 * Two decryption methods are offered:
 *  -disc key hash crack,
 *  -decryption with player keys if they are available.
 *****************************************************************************/
int _dvdcss_disckey( dvdcss_t dvdcss )
{
    unsigned char p_buffer[ DVD_DISCKEY_SIZE ];

	debug_msg("disckey start" );

    if( GetBusKey( dvdcss ) < 0 )
    {
        return -1;
    }

    /* Get encrypted disc key */
    if( ioctl_ReadDiscKey( dvdcss->i_fd, &dvdcss->css.i_agid, p_buffer) < 0 )
    {
        debug_error("ioctl ReadDiscKey failed" );
        return -1;
    }

    /* This should have invalidated the AGID and got us ASF=1. */
    if( GetASF( dvdcss ) != 1 )
    {
        /* Region mismatch (or region not set) is the most likely source. */
        _dvdcss_error( dvdcss,
                       "ASF not 1 after reading disc key (region mismatch?)" );
        ioctl_InvalidateAgid( dvdcss->i_fd, &dvdcss->css.i_agid );
        return -1;
    }

    /* Set disc key to the host */
    khwl_setproperty(KHWL_DECODER_SET, edecCSSDiscKey, sizeof(p_buffer), p_buffer);

	debug_msg("disckey success" );

    return 0;
}


/****************************************************************************
 * ReadTitleKey()
 * This is an extended version of ioctl_ReadTitleKey() with CGMS support.
 ****************************************************************************/
int ReadTitleKey(int i_fd, int *pi_agid, int i_pos, uint8_t *p_key, int *cgms_flags)
{   
	int i_ret = 0;
	int cpm = 0, cp_sec = 0, cgms = 0;

#ifdef WIN32
	if( WIN2K ) /* NT/2k/XP */
    {
        DWORD tmp;
        uint8_t buffer[DVD_TITLE_KEY_LENGTH];
        PDVD_COPY_PROTECT_KEY key = (PDVD_COPY_PROTECT_KEY) &buffer;

        memset( &buffer, 0, sizeof( buffer ) );

        key->KeyLength  = DVD_TITLE_KEY_LENGTH;
        key->SessionId  = *pi_agid;
        key->KeyType    = DvdTitleKey;
        key->KeyFlags   = 0;
        key->Parameters.TitleOffset.QuadPart = (LONGLONG) i_pos * 2048;

        i_ret = DeviceIoControl( (HANDLE)i_fd, IOCTL_DVD_READ_KEY, key,
                key->KeyLength, key, key->KeyLength, &tmp, NULL ) ? 0 : -1;

		memcpy( p_key, key->KeyData, DVD_KEY_SIZE );

		cpm = (key->KeyFlags >> 7) & 1;
		cp_sec = (key->KeyFlags >> 6) & 1;
		cgms = (key->KeyFlags >> 4) & 3;
    } else
	{
		debug_msg("Cannot get Macrovision flags for this Windows version.");
	}
#else
	dvd_authinfo auth_info;

    memset( &auth_info, 0, sizeof( auth_info ) );
    auth_info.type = DVD_LU_SEND_TITLE_KEY;
    auth_info.lstk.agid = *pi_agid;
    auth_info.lstk.lba = i_pos;

    i_ret = ioctl(i_fd, DVD_AUTH, &auth_info );

    memcpy( p_key, auth_info.lstk.title_key, DVD_KEY_SIZE );

    cpm = (auth_info.lstk.cpm & 1);
	cp_sec = (auth_info.lstk.cp_sec & 1);
	cgms = (auth_info.lstk.cgms & 3);
#endif

#ifdef OUR_DEBUG_OUTPUT
	msg("dvdcss: MV/CGMS Flags = (%d %d %d)\n", cpm, cp_sec, cgms);
#endif

	*cgms_flags = cgms;

	return i_ret;
}


/*****************************************************************************
 * _dvdcss_titlekey: get title key.
 *****************************************************************************/
int _dvdcss_titlekey( dvdcss_t dvdcss, int i_pos, dvd_key_t p_title_key )
{
    static uint8_t p_garbage[ DVDCSS_BLOCK_SIZE ];  /* we never read it back */
    uint8_t p_key[ KEY_SIZE ];
    int i_ret = 0;

	debug_msg("titlekey start" );

    if (dvdcss->b_ioctls)
    {
		/* We need to authenticate again every time to get a new session key */
        if( GetBusKey( dvdcss ) < 0 )
        {
            return -1;
        }

        /* Get encrypted title key */
        i_ret = ReadTitleKey( dvdcss->i_fd, &dvdcss->css.i_agid, i_pos+1, p_key, &mv_cgms_flags);
        if (i_ret < 0)
        {
            /* We need to authenticate again every time to get a new session key */
            if( GetBusKey( dvdcss ) < 0 )
            {
                return -1;
            }

            /* Get encrypted title key */
            i_ret = ReadTitleKey( dvdcss->i_fd, &dvdcss->css.i_agid, i_pos, p_key, &mv_cgms_flags);
            if (i_ret < 0 )
            {
                debug_error("ioctl ReadTitleKey failed (region mismatch?)" );
            }
        }

        if (i_ret >= 0)
        {
			khwl_setproperty(KHWL_DECODER_SET, edecCSSTitleKey, sizeof(p_key), p_key);
			memcpy( p_title_key, p_key, KEY_SIZE );
			debug_msg("Titlekey set OK!" );
        } else
        {
        	debug_msg("Titlekey not found!");
        }

        /* Test ASF, it will be reset to 0 if we got a Region error */
        switch( GetASF( dvdcss ) )
        {
            case -1:
                /* An error getting the ASF status, something must be wrong. */
                debug_msg("lost ASF requesting title key" );
                ioctl_InvalidateAgid( dvdcss->i_fd, &dvdcss->css.i_agid );
                i_ret = -1;
                break;

            case 0:
                /* This might either be a title that has no key,
                 * or we encountered a region error. */
                debug_msg("lost ASF requesting title key" );
                break;

            case 1:
                /* Drive status is ok. */
                /* If the title key request failed, but we did not loose ASF,
                 * we might stil have the AGID.  Other code assume that we
                 * will not after this so invalidate it(?). */
                if( i_ret < 0 )
                {
	                debug_msg("Invalidating Agid (in title key)" );
                    ioctl_InvalidateAgid( dvdcss->i_fd, &dvdcss->css.i_agid );
                }
                break;
        }

        /* The title key request failed */
        if (i_ret < 0)
        {
            debug_msg("titlekey error - resetting drive " );

            /* Read an unscrambled sector and reset the drive */
            dvdcss->pf_seek( dvdcss, 0 );
            dvdcss->pf_read( dvdcss, p_garbage, 1 );
            dvdcss->pf_seek( dvdcss, 0 );
            
    		// we don't have a cracking mechanism anyway ????
    		_dvdcss_disckey( dvdcss );

            /* Fallback */
        }
    }

    return i_ret;
}

// we do nothing! All work is done in hardware!
int _dvdcss_unscramble( dvd_key_t p_key, uint8_t *p_sec )
{
    return 0;
}

/* Following functions are local */

/*****************************************************************************
 * GetASF : Get Authentication success flag
 *****************************************************************************
 * Returns :
 *  -1 on ioctl error,
 *  0 if the device needs to be authenticated,
 *  1 either.
 *****************************************************************************/
static int GetASF( dvdcss_t dvdcss )
{
    int i_asf = 0;

    if( ioctl_ReportASF( dvdcss->i_fd, NULL, &i_asf ) != 0 )
    {
        /* The ioctl process has failed */
        debug_error("GetASF fatal error" );
        return -1;
    }

    if( i_asf )
    {
        debug_msg("GetASF authenticated, ASF=1" );
    }
    else
    {
        debug_msg("GetASF not authenticated, ASF=0" );
    }

    return i_asf;
}

#endif // DVDCSS_USE_EXTERNAL_CSS
