/*********************************************************************

  ui_font.h

*********************************************************************/

#ifndef UI_FONT_H
#define UI_FONT_H

#define UI_COLOR_DISPLAY

int uifont_decodechar(unsigned char *s, unsigned short *code);
int uifont_drawfont(struct mame_bitmap *bitmap, const char *s, int sx,
					int sy, int color);

#ifdef UI_COLOR_DISPLAY
void ConvertCommandMacro(char *buf);
#endif /* UI_COLOR_DISPLAY */

#endif
