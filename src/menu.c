/*
 * GNUjump
 * =======
 *
 * Copyright (C) 2005-2006, Juan Pedro Bolivar Puente
 *
 * GNUjump is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#include <stdarg.h> 
 
#include "gnujump.h"
#include "menu.h"
#include "game.h"
#include "setup.h"
#include "tools.h"
#include "replay.h"

extern SDL_Surface * screen;
extern L_gblOptions gblOps;

void initMouse(data_t* gfx, mouse_t* mouse)
{
	int i;
	
	for (i = 0; i < M_STATES; i++) {
		initializeSpriteCtl(&(mouse->sprite[i]), gfx->mouse[i]);
	}
	mouse->id = M_IDLE;
	mouse->clicked = FALSE;
}

void printMouse(data_t* gfx, mouse_t* mouse)
{
	SDL_Rect dest;
	dest.x = mouse->x - gfx->mouseX;
	dest.y = mouse->y - gfx->mouseY;
	printSprite(&(mouse->sprite[mouse->id]), NULL, &dest, 0);
}

void unprintMouse(data_t* gfx, mouse_t* mouse)
{
	SDL_Rect dest;
	SDL_Rect src;
	src.x = dest.x = mouse->x - gfx->mouseX;
	src.y = dest.y = mouse->y - gfx->mouseY;
	src.w = mouse->sprite[mouse->id].sdata->pic[0]->w;
	src.h = mouse->sprite[mouse->id].sdata->pic[0]->h;
	JPB_PrintSurface(gfx->menuBg, &src, &dest);
}

void setMouseState(mouse_t* mouse, int id)
{
	if (mouse->id != id) {
		mouse->sprite[id].elpTime = mouse->sprite[id].frame = 0;
		mouse->id = id;
	}
}

void checkMouse(data_t* gfx, mouse_t* mouse, int* sel, int nops, int off)
{
	int rx = 0, ry = 0;
	int no = MIN(gfx->mMaxOps,nops);
	
	mouse->clicked = SDL_GetMouseState(&(mouse->x), &(mouse->y)); 
	
	if ( mouse->x >= gfx->menuX
		&& mouse->x < gfx->menuX + gfx->menuW
		&& mouse->y >= gfx->menuY
		&& mouse->y < gfx->menuY+SFont_TextHeight(gfx->menufont)*no) {
		
		if (((mouse->clicked = SDL_GetRelativeMouseState(&rx, &ry)) || rx != 0 || ry != 0))
			*sel = (mouse->y - gfx->menuY)/SFont_TextHeight(gfx->menufont)+off;
		
		if (!mouse->clicked) setMouseState(mouse, M_OVER);
		else setMouseState(mouse, M_DOWN);
	} else {
		if (!mouse->clicked) setMouseState(mouse, M_IDLE);
		else setMouseState(mouse, M_DOWN);
	}
}

/*
 * NEW MENU SYSTEM =============================================================
 */

void initMenuT(menu_t* menu)
{
	menu->nops = 0;
	menu->opt = NULL;
}

/*
	Arguments are:
	- menu: pointer to the menu where the option should be included.
	- caption/tip: caption and tip of the option (these won't be freed)
	- flags: some flags defining what kind of option this is
	- data: depends on our option flags.
		+ (*int) for MB_CHOOSE
		+ (**char) for MB_INPUT
		+ (*SDLKey) for MB_KEYDEF
	- nops: NONE if no flags are chosen; 0, if flags != MB_CHOOSE, etc..
	- ...: char* to the name of the different options (if nops>0 :-))
*/
void addMenuTOption(menu_t* menu, char* caption, char* tip, int flags,  
					void* data, int nops, ...)
{
	va_list p;
	opt_t* opt = NULL;
	int i = 0;
		
	menu->nops++;
	menu->opt = realloc(menu->opt, sizeof(opt_t) * menu->nops);
	
	opt = &menu->opt[menu->nops-1];
	opt->caption = opt->tip =  NULL;
	opt->caption = caption;
	opt->tip = tip;
	
	opt->flags = flags;
	opt->nops = nops;
	opt->data = data;
	opt->opcap = NULL;
	
	if (nops > 0) {
		va_start(p, nops);
		opt->opcap = malloc(sizeof(char*) * nops);
		for (i=0; i<nops; i++) {
      		opt->opcap[i] = va_arg(p, char*);
		}
    	va_end(p);
    } else {
    	opt->flags |= MB_RETURN;
    }
	
}

void freeMenuTOption(opt_t* opt)
{
	free(opt->opcap);
}

void freeMenuT(menu_t* menu)
{
	int i;
	for (i = 0; i < menu->nops; i++) {
		freeMenuTOption(&menu->opt[i]);
	}
	free(menu->opt);
}

int playMenuT(data_t* gfx, menu_t* menu)
{
	int done = FALSE;
	int prevselect = 0, select = 0;
	fader_t* mfaders;
	fader_t afaders[ARROWS];
	char* tmpstr = NULL;
	SDL_Event event;
	L_timer timer;
	mouse_t mouse;
	int offset = 0, maxops = MIN(menu->nops,gfx->mMaxOps), maxoffset = menu->nops-gfx->mMaxOps;
	int i;
	
	if (menu->nops == 0) {
		Mix_PlayChannel(-1, gfx->mback, 0);
		return NONE;
	}
	
	initMouse(gfx, &mouse);
	initTimer(&timer, getFps());
	mfaders = malloc( sizeof(fader_t) * menu->nops );
	setFader( &mfaders[select], 0, gfx->hlalpha, 1, FALSE);
	for (i=select+1; i< menu->nops; i++)
		setFader(&mfaders[i],0,0,1, FALSE);
	for (i=0; i< ARROWS; i++)
		setFader(&afaders[i],0,0,1, FALSE);
    drawMenuT(gfx, menu, offset);
	if (menu->opt[select].tip != NULL)	drawTip(gfx, menu->opt[select].tip);
	FlipScreen();
	
    while (!done) {
		updateTimer(&timer);
		for (i=0; i< menu->nops; i++) {
			updateFader(&mfaders[i], timer.ms);
		}
		for (i=0; i< ARROWS; i++) {
			updateFader(&afaders[i], timer.ms);
		}
		unprintMouse(gfx, &mouse);
		undrawTip(gfx);
		checkMouse(gfx, &mouse, &select, menu->nops, offset);
        switch(checkMenuKeys(&mouse)) {
			case KMENTER:
				if (mouse.id != M_OVER) break;
			case KENTER:
				if (menu->opt[select].nops > 0){
					(*((int*)menu->opt[select].data))++;
					if (*((int*)menu->opt[select].data) >= menu->opt[select].nops) {
						*((int*)menu->opt[select].data) = 0;
					}
				}
				if ((menu->opt[select].flags & MB_INPUT) == MB_INPUT) {
					tmpstr = inputMenu(gfx, menu->opt[select].tip, *((char**)menu->opt[select].data), MAX_CHAR-1);
					if (tmpstr != NULL) {
						free(*((char**)menu->opt[select].data));
						*((char**)menu->opt[select].data) = tmpstr;
					}
				}
				if ((menu->opt[select].flags & MB_KEYDEF) == MB_KEYDEF) {
					for (;;) {
						SDL_WaitEvent(NULL);
						if( SDL_PollEvent( &event ) && event.type == SDL_KEYDOWN) {
							*((SDLKey*)menu->opt[select].data) = event.key.keysym.sym;
							break;
						}
					}
				}
				if ((menu->opt[select].flags & MB_VOLSET) == MB_VOLSET)
					resetVolumes();
				if ((menu->opt[select].flags & MB_RETURN) == MB_RETURN)
					done = TRUE;
					
				Mix_PlayChannel(-1, gfx->mclick, 0);
					
				break;
				
			case KUP:
				select--;
				if (select < 0) {
					select = menu->nops-1;
				}
				break;
				
			case KDOWN:
				select++;
				if (select >= menu->nops) select = 0;
				break;
				
			case KBACK:
				select = NONE;
				done = TRUE;
				Mix_PlayChannel(-1, gfx->mback, 0);
				continue;
				
			case KLEFT:
				if ((menu->opt[select].flags & MB_CHOOSE) == MB_CHOOSE
					&& menu->opt[select].nops > 0) {
					(*((int*)menu->opt[select].data))--;
					if (*((int*)menu->opt[select].data) < 0)
						*((int*)menu->opt[select].data) = menu->opt[select].nops-1;
				}
				if ((menu->opt[select].flags & MB_VOLSET) == MB_VOLSET)
					resetVolumes();
				if ((menu->opt[select].flags & MB_RETURN) == MB_RETURN)
					done = TRUE;
				
				Mix_PlayChannel(-1, gfx->mclick, 0);
				
				break;
				
			case KRIGHT:
				if ((menu->opt[select].flags & MB_CHOOSE) == MB_CHOOSE
					&& menu->opt[select].nops > 0) {
					(*((int*)menu->opt[select].data))++;
					if (*((int*)menu->opt[select].data) >= menu->opt[select].nops)
						*((int*)menu->opt[select].data) = 0;
				}
				if ((menu->opt[select].flags & MB_VOLSET) == MB_VOLSET)
					resetVolumes();
				if ((menu->opt[select].flags & MB_RETURN) == MB_RETURN)
					done = TRUE;
				
				Mix_PlayChannel(-1, gfx->mclick, 0);
				
				break;
			case KMUP:
				if (mouse.id == M_OVER && offset < maxoffset) offset++;
				break;
			case KMDOWN:
				if (mouse.id == M_OVER && offset > 0) offset--;
				break;
			default:
				break;
        }
        if (select != prevselect) {
        	setFader(&mfaders[prevselect], mfaders[prevselect].value, 0, MENUFADE, FALSE);
			setFader(&mfaders[select], gfx->hlalpha, gfx->hlalpha, MENUFADE, FALSE);
			prevselect = select;
			if (select >= maxops+offset) {
				offset = select-maxops+1;
			} else if (select < offset) {
				offset = select;
			}
        }
 		if (offset < maxoffset) setFader(&afaders[A_DOWN], afaders[A_DOWN].value, SDL_ALPHA_OPAQUE, ABLINKTIME*gblOps.useGL+1, FALSE);
		else setFader(&afaders[A_DOWN], afaders[A_DOWN].value, SDL_ALPHA_TRANSPARENT, ABLINKTIME*gblOps.useGL+1, FALSE);
		if (offset > 0) setFader(&afaders[A_UP], afaders[A_UP].value, SDL_ALPHA_OPAQUE, ABLINKTIME*gblOps.useGL+1, FALSE);
		else setFader(&afaders[A_UP], afaders[A_UP].value, SDL_ALPHA_TRANSPARENT, ABLINKTIME*gblOps.useGL+1, FALSE);

		for (i=offset; i < offset+maxops; i++) {
			updateFader(&mfaders[i],timer.ms);
			drawMenuTOption(gfx,  i, offset, &menu->opt[i], mfaders[i].value);
		}
		if (menu->opt[select].tip != NULL) drawTip(gfx, menu->opt[select].tip);
		drawMenuTArrows(gfx, afaders[A_UP].value, afaders[A_DOWN].value);
		animateSprite(&(mouse.sprite[mouse.id]), timer.ms);
		printMouse(gfx, &mouse);

		FlipScreen();
    }
	
	free(mfaders);
    return select;
}

void undrawTip(data_t* gfx)
{
	SDL_Rect dest;
	dest.x = gfx->tipX;
	dest.y = gfx->tipY;
	dest.w = gfx->tipW;
	dest.h = gfx->tipH;
	JPB_PrintSurface(gfx->menuBg, &dest, &dest);
}

void drawMenuTArrows(data_t* gfx, int alphaUp, int alphaDown)
{
	SDL_Rect dest;
	gfx->upArrow->alpha = alphaUp;
	gfx->dwArrow->alpha = alphaDown;
	dest.x = gfx->mUpArrowX;
	dest.y = gfx->mUpArrowY;
	dest.w = gfx->upArrow->w;
	dest.h = gfx->upArrow->h;
	JPB_PrintSurface(gfx->menuBg, &dest, &dest);
	dest.x = gfx->mDwArrowX;
	dest.y = gfx->mDwArrowY;
	dest.w = gfx->dwArrow->w;
	dest.h = gfx->dwArrow->h;
	JPB_PrintSurface(gfx->menuBg, &dest, &dest);
	gfx->upArrow->alpha = alphaUp;
	gfx->dwArrow->alpha = alphaDown;
	dest.x = gfx->mUpArrowX;
	dest.y = gfx->mUpArrowY;
	dest.w = gfx->upArrow->w;
	dest.h = gfx->upArrow->h;
	JPB_PrintSurface(gfx->upArrow, NULL, &dest);
	dest.x = gfx->mDwArrowX;
	dest.y = gfx->mDwArrowY;
	dest.w = gfx->dwArrow->w;
	dest.h = gfx->dwArrow->h;
	JPB_PrintSurface(gfx->dwArrow, NULL, &dest);
}

void drawMenuT(data_t* gfx, menu_t* menu, int offset)
{
    int i;
    
    JPB_PrintSurface(gfx->menuBg,NULL,NULL);

    for (i=offset; i < offset+MIN(menu->nops,gfx->mMaxOps); i++) {
        drawMenuTOption(gfx, i, offset, &menu->opt[i], 0);
    }
}

void drawMenuTOption(data_t* gfx,  int opt, int offset, opt_t* option, int alpha)
{
	SDL_Rect rect;
	int capwidth;
	
	rect.x = gfx->menuX;
	rect.y = gfx->menuY + (opt-offset) * SFont_TextHeight(gfx->menufont);
	rect.w = gfx->menuW;
	rect.h = SFont_TextHeight(gfx->menufont);
	
	JPB_PrintSurface(gfx->menuBg, &rect, &rect);
    JPB_drawSquare(gfx->hlcolor, alpha, rect.x,rect.y,rect.w,rect.h);
    
    if (option->flags == MB_RETURN)
		SFont_WriteMaxWidth (gfx->menufont, rect.x + gfx->mMargin, rect.y, rect.w-gfx->mMargin, gfx->mAlign, "...", option->caption);	
	else {
		SFont_WriteMaxWidth (gfx->menufont, rect.x + gfx->mMargin, rect.y, rect.w-gfx->mMargin, ALEFT, "...", option->caption);
		capwidth = SFont_TextWidth(gfx->menufont, option->caption) + gfx->mMargin;
		if ((option->flags & MB_INPUT) == MB_INPUT)
			SFont_WriteMaxWidth (gfx->menufont, rect.x + capwidth, rect.y, rect.w -gfx->mMargin-capwidth, ARIGHT,"...", *((char**)(option->data)));
		else if ((option->flags & MB_KEYDEF) == MB_KEYDEF)
			SFont_WriteMaxWidth (gfx->menufont, rect.x + capwidth, rect.y, rect.w -gfx->mMargin-capwidth, ARIGHT,"...", SDL_GetKeyName(*((SDLKey*)option->data)));
		else if ((option->flags & MB_CHOOSE) == MB_CHOOSE)
			SFont_WriteMaxWidth (gfx->menufont, rect.x + capwidth, rect.y, rect.w -gfx->mMargin-capwidth, ARIGHT,"...", option->opcap[*((int*)option->data)]);
	}
}

void drawTip (data_t* gfx, char* tip)
{
	SFont_WriteAligned(gfx->tipfont, gfx->tipX, gfx->tipY, gfx->tipW,0, gfx->tAlign, tip);
}

char* inputMenu (data_t* gfx, char* tip, char* inittext, int maxWidth)
{
    SDL_Event event;
	SDL_Rect rect;
    char ch = '\0';
    char text[MAX_CHAR];
    char* retText = NULL;
    int len;
    int prevUnic;
    
    drawMenu(gfx, 0, NULL);
    
    if (tip != NULL) 
		drawTip(gfx,tip);
            
    sprintf(text,"%s",inittext);
    len = strlen(text);
    text[len] = '|';
    text[len+1] = '\0';
    
	rect.x = gfx->menuX;
	rect.y = gfx->menuY;
	rect.w = gfx->menuW;
	rect.h = SFont_AlignedHeight(gfx->menufont, gfx->menuW,0, text);
	
    //JPB_drawSquare(gfx->hlcolor, gfx->hlalpha, rect.x, rect.y, rect.w,rect.h);
    SFont_WriteAligned(gfx->menufont, rect.x + gfx->mMargin, rect.y, gfx->menuW -gfx->mMargin ,0, ACENTER, text);
    FlipScreen();   
    
    prevUnic = SDL_EnableUNICODE(TRUE);
    while (ch != SDLK_RETURN) {
        while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
				ch=event.key.keysym.unicode;
				
				rect.h = SFont_AlignedHeight(gfx->menufont, gfx->menuW-gfx->mMargin, 0, text);
				JPB_PrintSurface(gfx->menuBg, &rect, &rect);
				
				if ( (ch>31) || (ch=='\b')) {
					if ((ch=='\b')&&(strlen(text)>0)) {
						len = strlen(text);
						text[strlen(text)-2]='|';
						text[strlen(text)-1]='\0';
					} else {
						len = strlen(text);
						text[len-1] = ch;
						text[len] = '|';
						text[len+1] = '\0';
					}
				}
				if (strlen(text)>maxWidth) 
					text[maxWidth]='\0';
					
				//JPB_drawSquare(gfx->hlcolor, gfx->hlalpha, rect.x, rect.y, rect.w,rect.h);
				SFont_WriteAligned(gfx->menufont, rect.x + gfx->mMargin, rect.y, gfx->menuW - gfx->mMargin, 0, ACENTER, text);
				FlipScreen();
			}
        }
        SDL_WaitEvent(NULL);
    }
    SDL_EnableUNICODE(prevUnic);
    text[strlen(text)-1]='\0';
    if ((retText = malloc(sizeof(char)*(strlen(text)+1))) == NULL)
        return NULL;
    strcpy(retText, text);
    
    return retText;
}

/*
 * OLD MENU SYSTEM. (Still usefull ) ===========================================
 */

/* These four functions are still used by the new menu system */

void drawMenu(data_t* gfx, int nops, char** ops)
{
    int i;
    
    JPB_PrintSurface(gfx->menuBg,NULL,NULL);
 
    for (i=0; i<nops; i++) {
        drawOption(gfx, i,ops[i],0);
    }
}

void drawOption(data_t* gfx,  int opt, char* option, int alpha)
{
	SDL_Rect rect;

	rect.x = gfx->menuX;
	rect.y = gfx->menuY + opt * SFont_TextHeight(gfx->menufont);
	rect.w = gfx->menuW;
	rect.h = SFont_TextHeight(gfx->menufont);
	
	JPB_PrintSurface(gfx->menuBg, &rect, &rect);
    JPB_drawSquare(gfx->hlcolor, alpha, rect.x, rect.y, rect.w, rect.h);
    
    //SFont_Write(gfx->menufont, x, y, option);
	SFont_WriteMaxWidth (gfx->menufont, rect.x + gfx->mMargin, rect.y, rect.w-gfx->mMargin, gfx->mAlign, "...", option);

}

int checkMenuKeys(mouse_t* mouse)
{
    SDL_Event event;
    int ret = KIDLE;
	
    while( SDL_PollEvent( &event ) ){
        switch( event.type ){
            /* A key is pressed */
            case SDL_KEYDOWN:
            	switch (event.key.keysym.sym) {
            		case SDLK_RETURN: ret = KENTER; break;
            		case SDLK_ESCAPE: ret = KBACK; break;
            		case SDLK_UP: ret = KUP; break;
            		case SDLK_DOWN: ret = KDOWN; break;
            		case SDLK_LEFT: ret = KLEFT; break;
            		case SDLK_RIGHT: ret = KRIGHT; break;
            		default: break;
            	}
                break;
            /* A key UP. */
            case SDL_MOUSEBUTTONDOWN:
				switch (event.button.button) {
					case SDL_BUTTON_LEFT: ret = KMENTER; break;
					case SDL_BUTTON_RIGHT: ret = KBACK; break;
					case SDL_BUTTON_WHEELDOWN: ret = KMUP; break;
					case SDL_BUTTON_WHEELUP: ret = KMDOWN; break;
            		default: break;
				}
				break;
			case SDL_MOUSEBUTTONUP: mouse->clicked--; break;
            case SDL_KEYUP:
                break;
            /* Quit: */
            case SDL_QUIT:
                break;
            /* Default */
            default:
                break;
        }
    }

    return ret;
}


/*
 * This has been converted to a wrapper to the new menu system so I don't have to
 * debug and add features to two pieces of almost identical code.
 */

/*
 * Usage:
 *  The first value must be the number of options passed. If, additionally, you
 *  want an explanatory text appearing on top of the menu, turn this value to
 *  negative.
 *      Examples: playMenu (3, "option1", "option2", "option3");
 *                playMenu (-2, "option1", "explanation1", "option2", "explanation2");
 */

int playMenu(data_t* gfx, int nops, ...)
{
    va_list p; 
    char **options = NULL;
	char **tips = NULL;
    int i;
	int select = 0;
 
    va_start(p, nops);
   
    options = malloc(sizeof(char*)*abs(nops));
    if (nops < 0)
        tips = malloc(sizeof(char*)*abs(nops));
	
    for (i=0; i<abs(nops); i++) {
        options[i] = va_arg(p, char*);
		if (nops < 0)
			tips[i] = va_arg(p, char*);
	}
    va_end(p);
    
    select = playMenuTab(gfx, nops, options, tips);
	free(options);
	free(tips);
	return select;
}

int playMenuTab(data_t* gfx, int nops, char **options, char **tips)
{
	menu_t menu;
	int ret;
	int i;
	
	initMenuT(&menu);
	
	for (i = 0; i < abs(nops); i++) {
		if (nops < 0) addMenuTOption(&menu, options[i], tips[i], 0, NULL, NONE);
		else addMenuTOption(&menu, options[i], NULL, 0, NULL, NONE);
    }
    
    ret = playMenuT(gfx, &menu);
    
    freeMenuT(&menu);
    return ret;
}


/*
 * ACTUAL MENUS ================================================================
 */

void saveReplayMenu(data_t* gfx, replay_t* rep)
{
	char *fname;
	char *comment;
    int done = FALSE;
    menu_t menu;

	fname = malloc(sizeof(char)*(strlen("default")+1));
	sprintf(fname, "default");
	comment = malloc(sizeof(char)*(strlen("no comment")+1));
	sprintf(comment, "no comment");
	
	initMenuT(&menu);
	addMenuTOption(&menu, _("Play"), _("Watch this replay."), MB_RETURN, NULL, 0);
	addMenuTOption(&menu, _("Filename"), _("Give a nice filename to your replay."), MB_INPUT, &(fname), 0);
	addMenuTOption(&menu, _("Comment"), _("Attach some annotations or explanations to the replay"), MB_INPUT, &(comment), 0);
	addMenuTOption(&menu, _("Save"), _("Store the replay to the disk with the given parameters."), MB_RETURN, NULL, 0);
    addMenuTOption(&menu, _("Cancel"),  _("I do not want to store this replay."),  MB_RETURN, NULL, 0);
    
    while (!done) {
    	switch(playMenuT(gfx,&menu)) {
			case 0:
				while (playReplay(gfx, rep));
				break;
			case 3:
				saveReplay(rep, fname, comment);
				done = TRUE;
				break;
			case 4:
			case NONE:
				done = TRUE;
				break;
			default:
				break;
    	}
    }
    
    free(fname);
    free(comment);
    
    freeMenuT(&menu);
}

void mainMenu(data_t* gfx)
{
    int opt;
    int done = FALSE;
    
    Mix_PlayMusic(gfx->musmenu, -1);

    while (!done) {
        opt = playMenu(gfx,-6,
            _("New Game"), _("Play a new game."),
            _("Options"), _("Configure some settings."),
			_("Highscores"), _("Have a look at the local Hall of Fame."),
			_("Replays"), _("Watch previously played games."),
            _("Credits"), _("See who made this program."),
            _("Quit"), _("See you some other day :)")
			);
      
        switch(opt) {
			case 0:
				newGameMenu(gfx);
				break;
			case 1:
				optionsMenu(gfx);
				break;
			case 2:
				JPB_PrintSurface(gfx->gameBg, NULL, NULL);
				drawRecords(gfx,gblOps.records, -1);
				pressAnyKey();
				break;
			case 3:
				viewReplayMenu(gfx);
				break;
			case 4:
				JPB_PrintSurface(gfx->gameBg, NULL, NULL);
				drawCredits(gfx);
				pressAnyKey();
				break;
			case 5:
			case NONE:
				done = TRUE;
				break;
			default:
					break;  
        }
    }
}

void newGameMenu(data_t* gfx)
{
    int opt;
    int done = FALSE;
    menu_t menu;
    
    initMenuT(&menu);
    
    addMenuTOption(&menu, _("Start Game"), _("Start playing this match"), 0, NULL, NONE);
    addMenuTOption(&menu, _("Players:"), _("Set the number of players for this match."), MB_CHOOSE,
    			   &gblOps.nplayers, 4, "1", "2", "3", "4");
    addMenuTOption(&menu, _("MP Lives:"), _("Set the number of tries each player will have in multiplayer games."), MB_CHOOSE,
    			   &gblOps.mpLives, 5, "1", "2", "3", "4", "5");
    addMenuTOption(&menu, _("Replay:"), _("Activate this option if you want the match to be recorder while you play."), MB_CHOOSE,
    			   &gblOps.recReplay, 2, _("Off"), _("On"));
    addMenuTOption(&menu, _("Configure Players"), _("Select some options for each player, such as keys, name, etc."), 0, NULL, NONE);
    addMenuTOption(&menu, _("Back"), _("Leave this menu"), 0, NULL, NONE);
	
	
	while (!done) {
		gblOps.mpLives -= 1; // Start at 0
		opt = playMenuT(gfx,&menu);
		gblOps.mpLives += 1; // restore mplives
		if      (opt == 0) {
			while (playGame(gfx, gblOps.nplayers+1)) SDL_Delay(1);
			Mix_PlayMusic(gfx->musmenu, -1);
		}
		else if (opt == 4)
			configurePlayersMenu(gfx);
		else if (opt == 5 || opt == NONE)
			done = TRUE;
	}
	
	freeMenuT(&menu);
}

void configurePlayersMenu(data_t* gfx)
{
	int opt;
	int player = 0;
    int done = FALSE;
    menu_t menu;
    
    while (!done) {
    	initMenuT(&menu);
    	addMenuTOption(&menu, _("Player:"), _("Choose which player do you want to configure."), MB_RETURN|MB_CHOOSE,
    			   &player, 4, "1", "2", "3", "4");
		addMenuTOption(&menu, _("Name:"), _("Choose a name for this player."), MB_INPUT,
				   &(gblOps.pname[player]), 0);
		addMenuTOption(&menu, _("Left Key:"), _("Press enter and then press the key you want to use."), MB_KEYDEF,
				   &(gblOps.keys[player][LEFTK]), 0);
		addMenuTOption(&menu, _("Right Key:"), _("Press enter and then press the key you want to use."), MB_KEYDEF,
				   &(gblOps.keys[player][RIGHTK]), 0);
		addMenuTOption(&menu, _("Jump Key:"), _("Press enter and then press the key you want to use."), MB_KEYDEF,
				   &(gblOps.keys[player][JUMPK]), 0);
    	
   		addMenuTOption(&menu, _("Back"), _("Leave this menu."), 0, NULL, NONE);
    	
    	opt = playMenuT(gfx,&menu);
    	if (opt == NONE || opt == 5)
    		done = TRUE;
    		
    	freeMenuT(&menu);
    }
}

void optionsMenu(data_t* gfx)
{
	int opt;
    int done = FALSE;
    menu_t menu;
    
	initMenuT(&menu);
	addMenuTOption(&menu, _("Choose Theme"), _("Change the appearance and sounds of the game."), 0, NULL, NONE);
	/* addMenuTOption(&menu, _("Choose Language"), _("Select a language."), 0, NULL, NONE); */
	addMenuTOption(&menu, _("Game Options"), _("Change the gameplay."), 0, NULL, NONE);
	addMenuTOption(&menu, _("Graphic Options"), _("Modify how the game is displayed in the screen."), 0, NULL, NONE);
	addMenuTOption(&menu, _("Sound Options"), _("Modify options related to sound effects and music."), 0, NULL, NONE);
	addMenuTOption(&menu, _("Manage Folders"), _("Change the folders where themes and replays are."), 0, NULL, NONE);
	addMenuTOption(&menu, _("Back"), _("Leave this menu."), 0, NULL, NONE);
    	
    while (!done) {
        opt = playMenuT(gfx, &menu);
            
        switch(opt) {
			case 0:
				chooseThemeMenu(gfx);
				break;
			/*case 1:
				if (chooseLangMenu(gfx)) done = TRUE;
				break;*/
            case 1:
                gameOptionsMenu(gfx);
                break;
            case 2:
                gfxOptionsMenu(gfx);
                break;
            case 3:
				soundOptionsMenu(gfx);
				break;
            case 4:
                folderOptionsMenu(gfx);
                break;
            case 5:
            case NONE:
                done = TRUE;
                break;
            default:
                break;  
        }
    }
    
    freeMenuT(&menu);
}

void soundOptionsMenu(data_t* gfx)
{
	int opt;
    int done = FALSE;
    menu_t menu;
    
	initMenuT(&menu);
	addMenuTOption(&menu, _("Sound Volume:"), _("Set how low you want to hear the sounds. 0 means off. 9 is the max value."),
		MB_CHOOSE|MB_VOLSET, &(gblOps.sndvolume), 10, "0","1","2","3","4","5","6","7","8","9");
	addMenuTOption(&menu, _("Music Volume:"), _("Set how low you want to hear the music. 0 means off. 9 is the max value."),
		MB_CHOOSE|MB_VOLSET, &(gblOps.musvolume), 10, "0","1","2","3","4","5","6","7","8","9");
	addMenuTOption(&menu, _("Back"), _("Leave this menu."), 0, NULL, NONE);
    	
    while (!done) {
        opt = playMenuT(gfx, &menu);
            
        switch(opt) {
            case 2:
            case NONE:
                done = TRUE;
                break;
            default:
                break;  
        }
    }
    
    freeMenuT(&menu);
}

void folderOptionsMenu(data_t* gfx)
{
	int opt;
    int done = FALSE;
    menu_t menu;
    
	initMenuT(&menu);
	addMenuTOption(&menu, _("Replay save folder:") , _("Choose where new recorded replays should be stored."),MB_INPUT, &(gblOps.repDir), NONE);
	addMenuTOption(&menu, _("Replay Folders"), _("Manage the folders where GNUjump should look for replays."), 0, NULL, NONE);
	addMenuTOption(&menu, _("Theme Folders"), _("Manage the folders where GNUjump should look for themes."), 0, NULL, NONE);
	addMenuTOption(&menu, _("Back"), _("Leave this menu."), 0, NULL, NONE);
    	
    while (!done) {
        opt = playMenuT(gfx, &menu);
            
        switch(opt) {
            case 1:
                gblOps.nrfolders = manageDirsMenu(gfx, &gblOps.repDirs, gblOps.nrfolders);
                break;
            case 2:
                gblOps.ntfolders = manageDirsMenu(gfx, &gblOps.themeDirs, gblOps.ntfolders);
                break;
            case 3:
                done = TRUE;
                break;
            case NONE:
                done = TRUE;
                break;
            default:
                break;  
        }
    }
    
    freeMenuT(&menu);	
}

void gameOptionsMenu(data_t* gfx)
{
    menu_t menu;
    
	initMenuT(&menu);
	
    addMenuTOption(&menu, _("FPS Limit:"), _("The original Xjump worked at 40 fps. Higher FPS imply more fluid gameplay, but limiting it is good if you don't want to use all your CPU."), MB_CHOOSE,
    			   &gblOps.fps, 4,
    			   "Xjump",
    			   "100 FPS",
    			   "300 FPS",
    			   _("No limit."));
    
    addMenuTOption(&menu, _("Rotation:"),_("Change how the player rotates when jumping."), MB_CHOOSE,
    			   &gblOps.rotMode, 3,
    			   _("None"),
    			   _("Xjump"),
    			   _("Full"));
    			   
	addMenuTOption(&menu, _("Scrolling"), _("How do you want the tower to fall."), MB_CHOOSE,
    			   &gblOps.scrollMode, 2, 
    			   _("Xjump"),
    			   _("Soft"));
	
	addMenuTOption(&menu, _("Scroll BG"), _("Do you want the background an walls to scroll?"), MB_CHOOSE,
    			   &gblOps.scrollMode, 2, 
    			   _("No"),
    			   _("Yes"));
    			   
	addMenuTOption(&menu, _("Trail:"), _("Set this if you want the player to leave a trail behind him."), MB_CHOOSE,
    			   &gblOps.trailMode, 4, 
    			   _("None"),
    			   _("Thin"),
    			   _("Normal"),
    			   _("Strong"));

	addMenuTOption(&menu, _("Blur:"), _("Nice blur effect. Set a level from 0 to 9. Only availible in OpenGL mode."), MB_CHOOSE,
    			   &gblOps.blur, 10, "0","1","2","3","4","5","6","7","8","9");

    addMenuTOption(&menu, _("Back"), _("Leave this menu."), 0, NULL, NONE);
    			   
    playMenuT(gfx, &menu);
    
    freeMenuT(&menu);
}

void gfxOptionsMenu(data_t* gfx)
{
    menu_t menu;
    int done = 0;
    int ogl = gblOps.useGL;
    
    initMenuT(&menu);
    
	addMenuTOption(&menu, _("Fullscreen:"), _("Choose wether you want to play in a window or feel the overwhelming sensation of falling in all your screen!"), MB_CHOOSE|MB_RETURN,
    			   &gblOps.fullsc, 2, 
    			   _("Off"),
    			   _("On"));
    			   
    addMenuTOption(&menu, _("OpenGL:"), _("Use OpenGL if you've got a 3d card. In most cases it will improve the performance. Some fading effects are only availible in this mode."), MB_CHOOSE|MB_RETURN,
    			   &ogl, 2,
    			   _("Off"),
    			   _("On")
    			   );
    
    addMenuTOption(&menu, _("Color depth:"), _("How many colors do you want to see in the screen. Usually auto is the best option, but when playing in fullscreen lower values might improve performance."), MB_CHOOSE|MB_RETURN,
    			   &gblOps.bpp, 4,
    			   "32 bpp",
    			   "16 bpp",
    			   "8 bpp",
    			   _("Auto"));
    
    addMenuTOption(&menu, _("Antialiasing:"), _("Antialiasing will improve the look of the character when full rotation is set."), MB_CHOOSE|MB_RETURN,
    			   &gblOps.aa, 2,
    			   _("Off"),
    			   _("On"));
    		   
    addMenuTOption(&menu, _("Back"), _("Leave this menu."), 0, NULL, NONE);

    while(!done) {
		switch (playMenuT(gfx, &menu)) {
			case 0:
				setWindow();
				break;
			case 1:
			case 2:
			case 3:
				gblOps.useGL = ogl;
				freeGraphics(gfx);
				loadGraphics(gfx, gblOps.dataDir);
				Mix_PlayMusic(gfx->musmenu, -1);
				break;
			case 4:
			case NONE:
			default:
				done = TRUE;
				break;
		}
    }
    
    freeMenuT(&menu);
    
}

void viewReplayMenu(data_t* gfx)
{
	char **options = NULL;
	char **tips = NULL;
	char **dirs = NULL;
	char **buf = NULL;
	char **fullpaths = NULL;
	int *index = NULL;

	char* str = NULL;
	int nf = 0, n = 0, no = 0;
	int i = 0;
	int r;
	int done = FALSE;
	
	for (i=0; i < gblOps.nrfolders; i++) {
		n = getFileList(gblOps.repDirs[i], &buf);
		sumStringTabs(&dirs, nf, buf, n);
		nf = sumStringTabs_Cat(&fullpaths, nf, buf, n, gblOps.repDirs[i]);
		free(buf);
		buf = NULL;
	}
	
	no = 0;
	for (i = 0; i < nf; i++) {
		if (checkExtension(fullpaths[i], REPEXT) || checkExtension(fullpaths[i], REPEXTOLD))
			str = getReplayComment(fullpaths[i]);
		else str = NULL;
		
		if (str != NULL) {
			no++;
			
			index = realloc(index, sizeof(int)*no);
			tips = realloc(tips, sizeof(char*)*no);
			options = realloc(options, sizeof(char*)*no);
			
			tips[no-1] = str;
			options[no-1] = dirs[i];
			index[no-1] = i;
		}
	}

	while (!done) {
		r = playMenuTab(gfx,-no, options, tips);
		if (r >= 0 && r < no) {
			loadReplay(gfx, fullpaths[index[r]]);
			Mix_PlayMusic(gfx->musmenu, -1);
		} else if (r == NONE) {
			done = TRUE;
		}
	}
	
	for (i = 0; i < nf; i++) {
		free(dirs[i]);
		free(fullpaths[i]);
	}
	for (i = 0; i< no; i++) {
		free(tips[i]);
	}
	
	free(tips);
	free(options);
	free(index);
	free(dirs);
	free(fullpaths);
}

int chooseLangMenu(data_t* gfx)
{
/*	char **options = NULL;
	char **tips = NULL;
	char **dirs = NULL;
	char **buf = NULL;
	char **fullpaths = NULL;
	int *index = NULL;

	char* str = NULL;
	int nf = 0, n = 0, no = 0;
	int i = 0;
	int r;
	int ret = FALSE;
	
	for (i=0; i < gblOps.nlfolders; i++) {
		n = getFileList(gblOps.langDirs[i], &buf);
		sumStringTabs(&dirs, nf, buf, n);
		nf = sumStringTabs_Cat(&fullpaths, nf, buf, n, gblOps.langDirs[i]);
		free(buf);
		buf = NULL;
	}
	
	no = 0;
	for (i = 0; i < nf; i++) {
		if (checkExtension(fullpaths[i], LANGEXT))
			str = getLangComment(fullpaths[i]);
		else str = NULL;
		
		if (str != NULL) {
			no++;
			
			index = realloc(index, sizeof(int)*no);
			tips = realloc(tips, sizeof(char*)*no);
			options = realloc(options, sizeof(char*)*no);
			
			tips[no-1] = str;
			options[no-1] = dirs[i];
			index[no-1] = i;
		}
	}

	r = playMenuTab(gfx,-no, options, tips);
	if (r >= 0 && r < no) {
		gblOps.langFile = realloc(gblOps.langFile, sizeof(char)* (strlen( fullpaths[index[r]] )+1) );
		strcpy(gblOps.langFile, fullpaths[index[r]]);
		freeLanguage(gfx);
		loadLanguage(gfx, gblOps.langFile);
		ret = TRUE;
	}
	
	for (i = 0; i < nf; i++) {
		free(dirs[i]);
		free(fullpaths[i]);
	}
	for (i = 0; i< no; i++) {
		free(tips[i]);
	}
	
	free(tips);
	free(options);
	free(index);
	free(dirs);
	free(fullpaths);
	
	return ret;*/ return 0;
}

void chooseThemeMenu(data_t* gfx)
{
	char **options = NULL;
	char **tips = NULL;
	char **dirs = NULL;
	char **buf = NULL;
	char **fullpaths = NULL;
	int *index = NULL;

	char* str = NULL;
	int nf = 0, n = 0, no = 0;
	int i = 0;
	int r;
	
	for (i=0; i < gblOps.ntfolders; i++) {
		n = getDirList(gblOps.themeDirs[i], &buf);
		sumStringTabs(&dirs, nf, buf, n);
		nf = sumStringTabs_Cat(&fullpaths, nf, buf, n, gblOps.themeDirs[i]);
		free(buf);
		buf = NULL;
	}
	
	no = 0;
	for (i = 0; i < nf; i++) {
		str = getThemeComment(fullpaths[i]);
		if (str != NULL) {
			no++;
			
			index = realloc(index, sizeof(int)*no);
			tips = realloc(tips, sizeof(char*)*no);
			options = realloc(options, sizeof(char*)*no);
			
			tips[no-1] = str;
			options[no-1] = dirs[i];
			index[no-1] = i;
		}
	}

	r = playMenuTab(gfx,-no, options, tips);
	if (r >= 0 && r < no) {
		gblOps.dataDir = realloc(gblOps.dataDir, 
			sizeof(char)* (strlen( fullpaths[index[r]] )+1) );
		strcpy(gblOps.dataDir, fullpaths[index[r]]);
		freeGraphics(gfx);
		loadGraphics(gfx, gblOps.dataDir);
		Mix_PlayMusic(gfx->musmenu, -1);
	}
	
	for (i = 0; i < nf; i++) {
		free(dirs[i]);
		free(fullpaths[i]);
	}
	for (i = 0; i< no; i++) {
		free(tips[i]);
	}
	
	free(tips);
	free(options);
	free(index);
	free(dirs);
	free(fullpaths);
}


int manageDirsMenu(data_t* gfx, char*** folders, int nfolders)
{
	char **options = NULL;
	char **tips = NULL;
	char* buf = NULL;
	int i;
	int done = FALSE;
	int opt;
	
	while (!done) {
		options = malloc(sizeof(char*)* (nfolders+2));
		tips    = malloc(sizeof(char*)* (nfolders+2));
		
		options[0] 			= _("Add");
		options[nfolders+1] = _("Back");
		tips[0] 			= _("Add a folder to the list.");
		tips[nfolders+1]    = _("Leave this menu.");
		
		for (i=0; i< nfolders; i++) {
			options[i+1] = (*folders)[i];
			tips[i+1]    = _("View more options about this folder.");
		}
	
		opt = playMenuTab(gfx, -(nfolders+2), options, tips);
		
		if (opt == 0) { /* Add a new item */
			nfolders++;
			*folders = realloc(*folders, sizeof(char*)* nfolders); 
			
			buf = getcwd(NULL,0);
			(*folders)[nfolders-1] = inputMenu(gfx, _("Write a full or relative path."), buf, gfx->menuW);
			
			free(buf); buf = NULL;
		} else if (opt > 0 && opt < nfolders+1) {
			/* Display the menu to edit an entry */
			switch ( playMenu(gfx, -3,
				     _("Modify"), _(""),
				     _("Delete"), _(""),
				     _("Back"), _("Leave this menu.")) 
				   ){
			case 0: /* edit */
				buf = (*folders)[opt-1];
				(*folders)[opt-1] = inputMenu(gfx,_("Write a full or relative path."), buf, 256);
				free(buf);
				break;
			case 1: /* delete */
				buf = (*folders)[opt-1];
				for (i = opt; i < nfolders; i++) {
					(*folders)[i-1] = (*folders)[i];
				}
				free(buf); buf = NULL;
				nfolders--;
				*folders = realloc((*folders), sizeof(char*)*nfolders);
				break;
			default:
				break;
			}
			
		} else {
			done = TRUE;
		}

		free(options);
		free(tips);
	}
	
	return nfolders;
}


