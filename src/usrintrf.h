/*********************************************************************

  usrintrf.h

  Functions used to handle MAME's crude user interface.

*********************************************************************/

#ifndef USRINTRF_H
#define USRINTRF_H
#define UI_TITLE_MENU

struct DisplayText
{
	const char *text;	/* 0 marks the end of the array */
	int color;	/* see #defines below */
	int x;
	int y;
};

#define UI_COLOR_NORMAL 0	/* white on black text */
#define UI_COLOR_INVERSE 1	/* black on white text */

#ifdef UI_TITLE_MENU
#define UI_COLOR_BOLD  2	/* white bold text */
#define UI_COLOR_BLACK 3	/* black bold text */
#endif /* UI_TITLE_MENU */

#define SEL_BITS 12		/* main menu selection mask */
#define SEL_BITS2 4		/* submenu selection masks */
#define SEL_MASK ((1<<SEL_BITS)-1)
#define SEL_MASK2 ((1<<SEL_BITS2)-1)

extern UINT8 ui_dirty;

struct GfxElement *builduifont(void);
void pick_uifont_colors(void);
void displaytext(struct mame_bitmap *bitmap,const struct DisplayText *dt);

void ui_drawchar(struct mame_bitmap *dest, int ch, int color, int sx, int sy);
void ui_text(struct mame_bitmap *bitmap,const char *buf,int x,int y);
void ui_drawbox(struct mame_bitmap *bitmap,int leftx,int topy,int width,int height);
void ui_copyright_and_warnings(void);
void set_ui_visarea (int xmin, int ymin, int xmax, int ymax);

#ifdef UI_TITLE_MENU
void ui_displaymessagewindow(struct mame_bitmap *bitmap,const char *text,const char *title);
void ui_displaymenu(struct mame_bitmap *bitmap,const char **items,const char **subitems,char *flag,const char *title,int selected,int arrowize_subitem);
#else
void ui_displaymessagewindow(struct mame_bitmap *bitmap,const char *text);
void ui_displaymenu(struct mame_bitmap *bitmap,const char **items,const char **subitems,char *flag,int selected,int arrowize_subitem);
#endif /* UI_TITLE_MENU */

void init_user_interface(void);
int handle_user_interface(struct mame_bitmap *bitmap);
void setup_menu_init(void);

void generate_xml_dat(void);

int setup_active(void);

#if defined(__sgi)
int is_game_paused(void);
#endif

#ifdef __GNUC__
void CLIB_DECL usrintf_showmessage(const char *text,...)
      __attribute__ ((format (printf, 1, 2)));

void CLIB_DECL usrintf_showmessage_secs(int seconds, const char *text,...)
      __attribute__ ((format (printf, 2, 3)));
#else
void CLIB_DECL usrintf_showmessage(const char *text,...);
void CLIB_DECL usrintf_showmessage_secs(int seconds, const char *text,...);
#endif

#endif
