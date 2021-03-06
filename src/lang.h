/*
 * GNUjump
 * =======
 *
 * Copyright (C) 2005-2006, Juan Pedro Bolivar Puente
 *
 * GNUjump is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNUjump is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNUjump; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LANG_H_
#define _LANG_H_

/*
 * The order of the different phrases is the order in which they are loaded.
 * loadLang() in setup.c might solve any doubts :)
 */

enum {
	txt_name,
	txt_floor, 
	txt_mode,
	txt_time,
	txt_date,
	txt_hscnote,
	txt_newhsc,
	txt_gameover,
	txt_askquit,
	txt_askreplay,
	txt_pause,
	txt_askquitrep,
	txt_askrepagain,
	txt_codeauthor,
	txt_gfxauthor,
	txt_sndauthor,
	txt_langauthor,
	TXT_COUNT
};

enum {
	msg_newgame,
	msg_options,
	msg_highscores,
	msg_replays,
	msg_credits,
	msg_quit,
	
	msg_back,
	
	msg_startgame,
	msg_players,
	msg_mplives,
	msg_recreplay,
	msg_configplayers,
	
	msg_player,
	msg_name,
	msg_leftkey,
	msg_rightkey,
	msg_jumpkey,
	
	msg_addfolder,
	msg_deletefolder,
	msg_editfolder,
	
	msg_themes,
	msg_lang,
	msg_gameoptions,
	msg_graphicoptions,
	msg_soundoptions,
	msg_folders,
	
	msg_themefolders,
	msg_langfolders,
	msg_repfolders,
	msg_repsavefolder,
	
	msg_fpslimit,
	msg_jumpingrot,
	msg_scrollmode,
	msg_trail,
	msg_blur,
	
	msg_opengl,
	msg_bpp,
	msg_fullscreen,
	msg_antialiasing,
	
	msg_sndvolume,
	msg_musvolume,
	
	msg_repname,
	msg_repcomment,
	msg_repplay,
	msg_repsave,
	
	msg_cancel,
	MSG_COUNT
};

enum {
	tip_newgame,
	tip_options,
	tip_highscores,
	tip_replays,
	tip_credits,
	tip_quit,
	
	tip_back,
	
	tip_startgame,
	tip_players,
	tip_mplives,
	tip_recreplay,
	tip_configplayers,
	
	tip_player,
	tip_name,
	tip_leftkey,
	tip_rightkey,
	tip_jumpkey,
	
	tip_addfolder,
	tip_folder,
	tip_deletefolder,
	tip_editfolder,
	tip_writefolder,
	
	tip_themes,
	tip_lang,
	tip_gameoptions,
	tip_graphicoptions,
	tip_soundoptions,
	tip_folders,
	
	tip_themefolders,
	tip_langfolders,
	tip_repfolders,
	tip_repsavefolder,
	
	tip_fpslimit,
	tip_jumpingrot,
	tip_scrollmode,
	tip_trail,
	tip_blur,
	
	tip_opengl,
	tip_bpp,
	tip_fullscreen,
	tip_antialiasing,
	
	tip_sndvolume,
	tip_musvolume,
	
	tip_repname,
	tip_repcomment,
	tip_repplay,
	tip_repsave,
	tip_cancel,
	TIP_COUNT
};

enum {
	opt_40fps,
	opt_100fps,
	opt_300fps,
	opt_nolimit,
	
	opt_norot,
	opt_orginalrot,
	opt_fullrot,
	
	opt_softscroll,
	opt_hardscroll,
	
	opt_notrail,
	opt_thintrail,
	opt_normaltrail,
	opt_strongtrail,
	
	opt_8bpp,
	opt_16bpp,
	opt_24bpp,
	opt_32bpp,
	opt_autobpp,	
	
	opt_on,
	opt_off,
	OPT_COUNT
};

#endif /* _LANG_H_ */

