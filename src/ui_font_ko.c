/*****************************************************************************

  ui_font.c

  Korean Build Version by EKMAME
  Korean font by Mbox32
*****************************************************************************/

//#include <windows.h>
#include "driver.h"
#include "artwork.h"
#include "ui_pal.h"

#define UI_COLOR_DISPLAY
#define CMD_PLUS
// MBOX32_FIX: 공략폰트의 색 지정 및 전체 폰트수(17,631자)
#define MAX_FONT_DATA	(17631)

static  UINT16 _uifonttable[65536];		
static UINT32  colortable[MAX_COLORTABLE];

static int uirawcharwidth, uirawcharheight;
int uirotcharwidth, uirotcharheight;

/*-------------------------------------------------
	ui_rot2raw_rect - convert a rect from rotated
	coordinates to raw coordinates
-------------------------------------------------*/

static void ui_rot2raw_rect(struct rectangle *rect)
{
	int temp, w, h;

	/* get the effective screen size, including artwork */
	artwork_get_screensize(&w, &h);

	/* apply X/Y swap first */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		temp = rect->min_x; rect->min_x = rect->min_y; rect->min_y = temp;
		temp = rect->max_x; rect->max_x = rect->max_y; rect->max_y = temp;
	}

	/* apply X flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		temp = w - rect->min_x - 1;
		rect->min_x = w - rect->max_x - 1;
		rect->max_x = temp;
	}

	/* apply Y flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		temp = h - rect->min_y - 1;
		rect->min_y = h - rect->max_y - 1;
		rect->max_y = temp;
	}
}

/*-------------------------------------------------
	ui_markdirty - mark a raw rectangle dirty
-------------------------------------------------*/

INLINE void ui_markdirty(const struct rectangle *rect)
{
	artwork_mark_ui_dirty(rect->min_x, rect->min_y, rect->max_x, rect->max_y);
	ui_dirty = 5;
}

/*
// 완성형(KSC5601) 코드 영역 : 8366자		c1: (0xa1-0xfd)  c2: (0xa1-0xfe)
		
		a1a1 ~ a1fe : 94
		...
		fda1 ~ fdfe
		-----------
		93          = 93 * 94 = 8742
		
		(특수문자영역과 한글폰트영역사이의 376개의 공간이 비어있음. 따라서 8742 - 376 = 8366)
		376개의 빈 공간은 공략폰트용으로 사용

		
// 확장영역A: 5696자	c1: (0x81-0xa0)  c2: (0x41-0x5a,0x61-0x7a,0x81-0xfe)

		8141 ~ 815a, 8161 ~ 817a, 818a ~ 81fe : 26 + 26 + 126 = 178
		8241 ~ 82fe
		...
		a041 ~ a0fe
		-----------
		32			= 32 * 178 = 5696


// 확장영역B: 3192자	c1: (0xa1-0xc6)  c2: (0x41-0x5a,0x61-0x7a,0x81-0xa0)
		
		a141 ~ a15a, a161 ~ a17a, a181 ~ a1a0 : 26 + 26 + 32 = 84
		...
		c641...c6a0
		-----------
		38          = 38 * 84 = 3192
		
*/
static UINT16 uifont_buildtable(UINT16 code)
{
	UINT8 c1, c2;
	
	c1 = (code >> 8) & 0xff;
	c2 = code & 0xff;
	
	/*
	   KSC-5601 완성형 코드
	   --------------------------------------------------------------------------------------------
	 	부호: hi:0xa1 - 0xac	lo:0xa1 - 0xfe (http://cs.sungshin.ac.kr/~shim/demo/ksc5601-a.htm)
	 	한글: hi:0xb0 - 0xc8	lo:0xa1 - 0xfe (http://cs.sungshin.ac.kr/~shim/demo/ksc5601-b.htm)
	 	한자: hi:0xca - 0xfd	lo:0xa1 - 0xfe (http://cs.sungshin.ac.kr/~shim/demo/ksc5601-c.htm)
	*/
	// 설명 : ** 문자코드 처리부분 **
	// 설명 : 완성형(KSC5601) 코드 영역 : c1: (0xa1-0xfd)  c2: (0xa1-0xfe)
	if ((0xa1 <= c1 && c1 <= 0xfd) && (c2 >= 0xa1))
	{
		if(0xa1 <= c1 && c1 <= 0xac)		// 특수문자 영역
			code = (c1 - 0xa1 + 4) * 94 + (c2 - 0xa1);
		else if(0xad <= c1 && c1 <= 0xaf)	// 공략폰트 영역: 3블럭
			code = (c1 - 0xad) * 94 + (c2 - 0xa1);
		else if(0xb0 <= c1 && c1 <= 0xc8)	// 한글 2,350자 영역
			code = (c1 - 0xa1 + 1) * 94 + (c2 - 0xa1);
		else if(c1 == 0xc9)					// 공략폰트 영역: 1블럭
			code = (c1 - 0xc9 + 3) * 94 + (c2 - 0xa1);
		else if(0xca <= c1 && c1 <= 0xfe )	// 한자 4,888자 영역
			code = (c1 - 0xa1) * 94 + (c2 - 0xa1);
	}
	
	// 설명 : 확장영역A : c1: (0x81-0xa0)  c2: (0x41-0x5a,0x61-0x7a,0x81-0xfe)
	else if (0x81 <= c1 && c1 <= 0xa0)
	{
		if(0x41 <= c2 && c2 <= 0x5a)
			code = (c1 - 0x81) * 178 + (c2 - 0x41) + 8742;
		else if( 0x61 <= c2 && c2 <= 0x7a )
			code = (c1 - 0x81) * 178 + (c2 - 0x61 + 0x1a) + 8742;
		else if( 0x81 <= c2 && c2 <= 0xfe )
			code = (c1 - 0x81) * 178 + (c2 - 0x81 + 0x34) + 8742;
	}
	
	// 설명 : 확장영역B : c1: (0xa1-0xc6)  c2: (0x41-0x5a,0x61-0x7a,0x81-0xa0)
	else if ((0xa1 <= c1 && c1 <= 0xc6) && (c2 <= 0xa0))
	{
		if( 0x41 <= c2 && c2 <= 0x5a )
			code = (c1 - 0xa1) * 84 + (c2 - 0x41) + 14438;
		else if( 0x61 <= c2 && c2 <= 0x7a )
			code = (c1 - 0xa1) * 84 + (c2 - 0x61 + 0x1a) + 14438;
		else if( 0x81 <= c2 && c2 <= 0xa0 )
			code = (c1 - 0xa1) * 84 + (c2 - 0x81 + 0x34) + 14438;
	}
	else
		return 0x60;
	
	return code;
}

void uifont_freefont(void)
{
	if (Machine->uifont)
	{
		freegfx(Machine->uifont);
		Machine->uifont = NULL;
	}

	if (Machine->uirotfont)
		freegfx(Machine->uirotfont);

	if (Machine->uirotfont2)
		freegfx(Machine->uirotfont2);
	
}

/*-------------------------------------------------
    builduifont - build the user interface fonts
-------------------------------------------------*/

#ifdef CMD_PLUS
int uifont_buildfont(void)
{
	static unsigned char fontdata6x12[] =
	{
#include "font/asc6x12.dat"
	};
	
	static unsigned char fontdata12x12[] =
	{
#include "font/raw/cmd12x12.dat"		// 공략폰트:  376
#include "font/raw/ksc_sp12x12.dat"		// KSC5601:  1128(특수문자)
#include "font/raw/ksc_han12x12.dat"	// KSC5601:  2350(한글)
#include "font/raw/ksc_hj12x12.dat"		// KSC5601:  4888(한자)
#include "font/raw/kse_han12x12.dat"	// 확장한글: 8888 = 5696(A영역) + 3192(B영역)
#include "font/raw/end12x12.dat"		// EndofData:   1
									// ----------------
									//		    17631
	};
	
	static struct GfxLayout fontlayout6x12 =
	{
		6,12,	/* 6*12 characters */
		256,    /* 256 characters */
		1,		/* 1 bit per pixel */
		{ 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* straightforward layout */
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8 },
		8*12	/* every char takes 8 consecutive bytes */
	};
	
	static struct GfxLayout fontlayout12x12 =
	{
		12,12,
		0,		// total number set later
		1,
		{ 0 },
		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 },
		{ 0*16,1*16,2*16,3*16,4*16,5*16,6*16,7*16,8*16,9*16,10*16,11*16 },
		16*12
	};
	
	struct GfxLayout layout1;
	struct GfxLayout layout2;
	unsigned char *fontdata_sb;
	unsigned char *fontdata_db;
	struct GfxElement *font;
	UINT32 tempoffset[MAX_GFX_SIZE];
	int temp, i, gamescale;

	uifont_freefont();
	
	fontdata_sb = fontdata6x12;
	layout1 = fontlayout6x12;

	fontdata_db = fontdata12x12;
	layout2 = fontlayout12x12;
	//layout2.total = 22428;
	layout2.total = MAX_FONT_DATA;

#ifdef EXT_GAMEUI_FONT
	{
		extern void DecodeFontName( char *src, char *dst );
		extern void EncodeFontName( char *src, char *dst );

		FILE *fp;
		int  isdefault;
		unsigned long dwSig, dwWidth;
		char szFontName[256], szFontFile[256];


/*
		fp = fopen("font/new_def.mfd", "wb" );
		dwSig = 0x1234;
		fwrite( &dwSig, sizeof(unsigned long), 1, fp );
		dwSig = (DWORD)fontlayout6x12.width;
		fwrite( &dwSig, sizeof(unsigned long), 1, fp );
		fwrite( fontdata6x12, sizeof(fontdata6x12), 1, fp );
		dwSig = 0x5678;
		fwrite( &dwSig, sizeof(unsigned long), 1, fp );
		fwrite( fontdata12x12, sizeof(fontdata12x12), 1, fp );
		fclose(fp);
*/
		
		// Setup Default font data
		sprintf( szFontFile, "font/Default.mfd" );
		
		fp = fopen(szFontFile, "rb" );
		if( fp != NULL ) {
			fread( &dwSig, sizeof(unsigned long), 1, fp );
			if(dwSig == 0x1234) {
				fread( &dwWidth, sizeof(unsigned long), 1, fp );
				fread( fontdata6x12, sizeof(fontdata6x12), 1, fp );	// 3,060 bytes
				fontlayout6x12.width = (UINT16)6;
			}
			else {
				fread( &dwWidth, sizeof(unsigned long), 1, fp );
				fseek( fp, sizeof(fontdata6x12), SEEK_CUR );
			}
			
			fread( &dwSig, sizeof(unsigned long), 1, fp );
			if(dwSig == 0x5678) 
				fread( fontdata12x12, sizeof(fontdata12x12), 1, fp );	// 423,144 bytes
				
			fclose(fp);	
		}

		// 개부기본폰트를 이용할것인지 아니면 로드해야 하는지 판단		
		if( (options.gmui_font != NULL) && stricmp(options.gmui_font, "none") ) {
			// Decode font name
			DecodeFontName(options.gmui_font, szFontName );
			isdefault = 0;
		}
		else {
			sprintf( szFontName, "Default" );
			isdefault = 1;
		}

		if(!isdefault) {
			sprintf( szFontFile, "font/%s.mfd", szFontName );
	
			//logmsg( "Try loading font file:'%s'...\n", szFontFile );
			fp = fopen(szFontFile, "rb" );
			if( fp != NULL ) {
				fread( &dwSig, sizeof(unsigned long), 1, fp );
				if(dwSig == 0x1234) {
					fread( &dwWidth, sizeof(unsigned long), 1, fp );
					fread( fontdata6x12, sizeof(fontdata6x12), 1, fp );	// 3,060 bytes
					fontlayout6x12.width = (UINT16)dwWidth;
				}
				else {
					fread( &dwWidth, sizeof(unsigned long), 1, fp );
					fseek( fp, sizeof(fontdata6x12), SEEK_CUR );
				}
				
				fread( &dwSig, sizeof(unsigned long), 1, fp );
				if(dwSig == 0x5678) 
					fread( fontdata12x12, sizeof(fontdata12x12), 1, fp );	// 423,144 bytes
					
				fclose(fp);
			}
		}
	}
#endif /* EXT_GAMEUI_FONT */

	/* decode a straight on version for games */
	Machine->uifont = font = decodegfx(fontdata_sb, &layout1);
	Machine->uifontwidth  = layout1.width;
	Machine->uifontheight = layout1.height;
	
	gamescale = options.artwork_res;

	if (gamescale == 2)
	{
		/* pixel double horizontally */
		if (Machine->uiwidth >= 420)
		{
			memcpy(tempoffset, layout1.yoffset, sizeof(tempoffset));
			for (i = 0; i < layout1.width; i++)
				layout1.xoffset[i*2+0] = layout1.xoffset[i*2+1] = tempoffset[i];
			layout1.width *= 2;

			memcpy(tempoffset, layout2.yoffset, sizeof(tempoffset));
			for (i = 0; i < layout2.width; i++)
				layout2.xoffset[i*2+0] = layout2.xoffset[i*2+1] = tempoffset[i];
			layout2.width *= 2;
		}
			
		/* pixel double vertically */
		if (Machine->uiheight >= 420)
		{
			memcpy(tempoffset, layout1.xoffset, sizeof(tempoffset));
			for (i = 0; i < layout1.height; i++)
				layout1.yoffset[i*2+0] = layout1.yoffset[i*2+1] = tempoffset[i];
			layout1.height *= 2;

			memcpy(tempoffset, layout2.xoffset, sizeof(tempoffset));
			for (i = 0; i < layout2.height; i++)
				layout2.yoffset[i*2+0] = layout2.yoffset[i*2+1] = tempoffset[i];
			layout2.height *= 2;
		}
	}
	
	/* apply swappage */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		memcpy(tempoffset, layout1.xoffset, sizeof(tempoffset));
		memcpy(layout1.xoffset, layout1.yoffset, sizeof(layout1.xoffset));
		memcpy(layout1.yoffset, tempoffset, sizeof(layout1.yoffset));
		
		temp = layout1.width;
		layout1.width = layout1.height;
		layout1.height = temp;
		
		memcpy(tempoffset, layout2.xoffset, sizeof(tempoffset));
		memcpy(layout2.xoffset, layout2.yoffset, sizeof(layout2.xoffset));
		memcpy(layout2.yoffset, tempoffset, sizeof(layout2.yoffset));
		
		temp = layout2.width;
		layout2.width = layout2.height;
		layout2.height = temp;
	}
	
	/* apply xflip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		memcpy(tempoffset, layout1.xoffset, sizeof(tempoffset));
		for (i = 0; i < layout1.width; i++)
			layout1.xoffset[i] = tempoffset[layout1.width - 1 - i];
		
		memcpy(tempoffset, layout2.xoffset, sizeof(tempoffset));
		for (i = 0; i < layout2.width; i++)
			layout2.xoffset[i] = tempoffset[layout2.width - 1 - i];
	}
	
	/* apply yflip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		memcpy(tempoffset, layout1.yoffset, sizeof(tempoffset));
		for (i = 0; i < layout1.height; i++)
			layout1.yoffset[i] = tempoffset[layout1.height - 1 - i];
		
		memcpy(tempoffset, layout2.yoffset, sizeof(tempoffset));
		for (i = 0; i < layout2.height; i++)
			layout2.yoffset[i] = tempoffset[layout2.height - 1 - i];
	}

	/* decode rotated font */
	Machine->uirotfont  = decodegfx(fontdata_sb, &layout1);
	Machine->uirotfont2 = decodegfx(fontdata_db, &layout2);

	/* set the raw and rotated character width/height */
	uirawcharwidth  = layout1.width;
	uirawcharheight = layout1.height;
	uirotcharwidth  = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? layout1.height : layout1.width;
	uirotcharheight = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? layout1.width : layout1.height;

	/* set up the bogus colortable */
	if (font)
	{
		/* colortable will be set at run time */
		font->colortable = colortable;
		font->total_colors = MAX_COLORTABLE;

		Machine->uirotfont->colortable = colortable;
		Machine->uirotfont->total_colors = MAX_COLORTABLE;

		Machine->uirotfont2->colortable   = colortable;
		Machine->uirotfont2->total_colors = MAX_COLORTABLE;
	}

	
	for (i = 0x4081; i < 0xfffe; i++)
		_uifonttable[i] = uifont_buildtable(i) + 256;
	
	return 1;
}


struct GfxElement *builduifont(void)
{
	uifont_buildfont();

	return Machine->uifont;
}

#else /* CMD_PLUS */

struct GfxElement *builduifont(void)
{
	struct GfxLayout layout = uifontlayout;
	UINT32 tempoffset[MAX_GFX_SIZE];
	struct GfxElement *font;
	int temp, i;

	/* free any existing fonts */
	if (Machine->uifont)
		freegfx(Machine->uifont);
	if (Machine->uirotfont)
		freegfx(Machine->uirotfont);

	/* first decode a straight on version for games */
	Machine->uifont = font = decodegfx(uifontdata, &layout);
	Machine->uifontwidth = layout.width;
	Machine->uifontheight = layout.height;

	/* pixel double horizontally */
	if (uirotwidth >= 420)
	{
		memcpy(tempoffset, layout.xoffset, sizeof(tempoffset));
		for (i = 0; i < layout.width; i++)
			layout.xoffset[i*2+0] = layout.xoffset[i*2+1] = tempoffset[i];
		layout.width *= 2;
	}

	/* pixel double vertically */
	if (uirotheight >= 420)
	{
		memcpy(tempoffset, layout.yoffset, sizeof(tempoffset));
		for (i = 0; i < layout.height; i++)
			layout.yoffset[i*2+0] = layout.yoffset[i*2+1] = tempoffset[i];
		layout.height *= 2;
	}

	/* apply swappage */
	if (Machine->ui_orientation & ORIENTATION_SWAP_XY)
	{
		memcpy(tempoffset, layout.xoffset, sizeof(tempoffset));
		memcpy(layout.xoffset, layout.yoffset, sizeof(layout.xoffset));
		memcpy(layout.yoffset, tempoffset, sizeof(layout.yoffset));

		temp = layout.width;
		layout.width = layout.height;
		layout.height = temp;
	}

	/* apply xflip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
	{
		memcpy(tempoffset, layout.xoffset, sizeof(tempoffset));
		for (i = 0; i < layout.width; i++)
			layout.xoffset[i] = tempoffset[layout.width - 1 - i];
	}

	/* apply yflip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
	{
		memcpy(tempoffset, layout.yoffset, sizeof(tempoffset));
		for (i = 0; i < layout.height; i++)
			layout.yoffset[i] = tempoffset[layout.height - 1 - i];
	}

	/* decode rotated font */
	Machine->uirotfont = decodegfx(uifontdata, &layout);

	/* set the raw and rotated character width/height */
	uirawcharwidth = layout.width;
	uirawcharheight = layout.height;
	uirotcharwidth = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? layout.height : layout.width;
	uirotcharheight = (Machine->ui_orientation & ORIENTATION_SWAP_XY) ? layout.width : layout.height;

	/* set up the bogus colortable */
	if (font)
	{
		static pen_t colortable[2*2];

		/* colortable will be set at run time */
		font->colortable = colortable;
		font->total_colors = 2;
		Machine->uirotfont->colortable = colortable;
		Machine->uirotfont->total_colors = 2;
	}

	return font;
}

#endif /* CMD_PLUS */

int uifont_decodechar(unsigned char *s, unsigned short *code)
{
	unsigned char c1 = *(unsigned char *)s;
	unsigned char c2 = *(unsigned char *)(s + 1);
	int isDBC;
	
	// Remove to Null Space Character
	#define NULL_SPACE1	'\b'
	if (c1 == NULL_SPACE1)
		return 3;
		
	// 설명 : 확장완성형 코드영역 범위
	isDBC = ((c1 >= 0x81 && c1 <= 0xfe) && ((c2 >= 0x41 && c2 <= 0x5a) || (c2 >= 0x61 && c2 <= 0x7a) || (c2 >= 0x81 && c2 <= 0xfe)));
	
	if (isDBC)
	{
		*code = _uifonttable[(c1 << 8) | c2];
		
		if (*code == 0)
			*code = _uifonttable[0x817f];
		
		return 2;
	}
	
	*code = c1;
	return 1;
}

int uifont_drawfont(struct mame_bitmap *bitmap, const char *s, int sx,
					int sy, int color)
{
	struct rectangle bounds;
	int x, y, wrapped, next_sy, increment, uifontwidth;
#ifdef UI_COLOR_DISPLAY
	int white = 0;
	int xinc, yinc;
#endif
	unsigned short code;

	/* 공략폰트:로고 폰트 위치 지정 */
	int btn_line = 9 * 94;
	int btn_logo = 188;
	int btn_deco = btn_logo + 16;

	/* 공략폰트:로고 폰트 색 지정 */
	#define MBOX32_LOGO_COLOR	BUTTON_COLOR_BLUE
	#define SECOND_LOGO_COLOR	BUTTON_COLOR_BLUE
	#define DECO_COLOR			BUTTON_COLOR_YELLOW

	unsigned char *c;
	struct GfxElement *uifont;
	
	c       = (unsigned char *)s;
	x       = sx;
	y       = sy;
	code    = 0;
	next_sy = 0;
	
#ifdef UI_COLOR_DISPLAY
	if (color == UI_COLOR_INVERSE)
	{
		white = Machine->uifont->colortable[FONT_COLOR_NORMAL];
		Machine->uifont->colortable[FONT_COLOR_NORMAL] = Machine->uifont->colortable[FONT_COLOR_SPECIAL];
	}

	/* 글자를 또렷하게 하기위해 검은색글자를 출력하기 위한 오프셋 */
	xinc = 1;
	yinc = 1;
	
	/* apply X flip */				
	if (Machine->ui_orientation & ORIENTATION_FLIP_X)
		xinc = -1;

	/* apply Y flip */
	if (Machine->ui_orientation & ORIENTATION_FLIP_Y)
		yinc = -1;
#endif
	
	while (*c)
	{
		wrapped = 0;
		
		increment = uifont_decodechar(c, &code);
		if (increment == 3)
		{
			c++;
			continue;
		}
		
		if (*c == '\n')
		{
			x = sx;
			y += Machine->uifontheight + 1;
			next_sy += Machine->uifontheight;
			c++;
			continue;
		}
		else
		{
			if ((code > 0x00ff ) && (Machine->uirotfont2 != NULL))
			{
				uifont      = Machine->uirotfont2;
				uifontwidth = Machine->uifontwidth * 2;
			}
			else
			{
				uifont      = Machine->uirotfont;
				uifontwidth = Machine->uifontwidth;
			}
		}
		
		if (y >= Machine->uiheight)
			return next_sy;
		
		if (!wrapped)
		{
#ifdef UI_COLOR_DISPLAY
			pen_t col = 0;
#endif
			
			if (code > 0x00ff)
			{
			  #include "lang/cmd_table_ek.c"
				code -= 256;
				
#ifdef UI_COLOR_DISPLAY
				if (code > 0 && code < COLOR_BUTTONS)
					col = cmd_colortable[code];

				else if (code >= btn_line && code <= btn_line + 68)			// 선문자
					col = BUTTON_COLOR_BLUE;
				else if (code >= (btn_logo) && code <= (btn_logo + 7))		// 엠박스32 로고 문자
					col = MBOX32_LOGO_COLOR;
				else if (code >= (btn_logo + 8) && code <= (btn_logo + 15))	// 2차 게임 로고 문자
					col = SECOND_LOGO_COLOR;
				else if (code >= btn_deco && code <= btn_deco + 2)			// 개발자 문자
					col = DECO_COLOR;
				
				if (col)
				{
					white = Machine->uifont->colortable[FONT_COLOR_NORMAL];
					Machine->uifont->colortable[FONT_COLOR_NORMAL] = Machine->uifont->colortable[col];
				}
#endif
			}
			
			bounds.min_x = x + Machine->uixmin;
			bounds.min_y = y + Machine->uiymin;
			bounds.max_x = x + Machine->uixmin + uifontwidth - 1;
			bounds.max_y = y + Machine->uiymin + Machine->uifontheight - 1;
			ui_rot2raw_rect(&bounds);
			
#ifdef UI_COLOR_DISPLAY
			if( color == UI_COLOR_INVERSE ) {
				drawgfx(bitmap, uifont, code, color, 0, 0,
					bounds.min_x, bounds.min_y,	0, TRANSPARENCY_PENS, 0);
			}
			else {
				// bottom black text
				drawgfx(bitmap, uifont, code, UI_COLOR_INVERSE, 0, 0,
					bounds.min_x+xinc, bounds.min_y+yinc,	0, TRANSPARENCY_PEN, 0);
				// upper white text
				drawgfx(bitmap, uifont, code, color, 0, 0,
					bounds.min_x, bounds.min_y,	0, TRANSPARENCY_PEN, 0);
			}

			bounds.max_x += xinc;
			bounds.max_y += yinc;
#else
			drawgfx(bitmap, uifont,	code, color, 0, 0,
				bounds.min_x, bounds.min_y,	0, TRANSPARENCY_NONE, 0);
#endif			
			
			ui_markdirty(&bounds);
			
			x += uifontwidth;
			
			while (increment)
			{
				c++;
				increment--;
			}
			
#ifdef UI_COLOR_DISPLAY
			if (col)
				Machine->uifont->colortable[FONT_COLOR_NORMAL] = white;
#endif
		}
	}
	
#ifdef UI_COLOR_DISPLAY
	if (color == UI_COLOR_INVERSE)
		Machine->uifont->colortable[FONT_COLOR_NORMAL] = white;
#endif
	
	return next_sy;
}

// Follow Defined Address & Block for Game Command Font
#define FONT_ADDRESS1 (0xad)
#define FONT_ADDRESS2 (0xa1)
#define FONT_BLOCK    (94)

#include "lang/cmd_plus_ek.c"

INLINE int isHanCode(unsigned char c1, unsigned char c2)
{
	if( (c1 >= 0x81 && c1 <= 0xfe) && 
	   ((c2 >= 0x41 && c2 <= 0x5a) || (c2 >= 0x61 && c2 <= 0x7a) || (c2 >= 0x81 && c2 <= 0xfe )))
  	{
		return 1;
    }
  	  
	return 0;
}

void ConvertCommandMacro(char *buf)
{
	unsigned char *c = (unsigned char *)buf;

	int i = 0;

	// ** Font Code Address Process Part **
	int c1 = FONT_ADDRESS1;	// Game Command Font 1st Byte Address
	int c2 = FONT_ADDRESS2;	// Game Command Font 2nd Byte Address
	int s1 = FONT_BLOCK;	// Game Command Font Block

	while (*c)
	{
		if ( isHanCode(*c, *(c+1)) )
		{
			c+=2;
		}
		else
		{
			if (*c == COMMAND_CONVERT_TEXT)
			{
				i = 0;
				while (convert_text[i].org_name)
				{
					if (*(c+1) == convert_text[i].org_name[1] &&
						*(c+2) == convert_text[i].org_name[2] &&
						*(c+3) == convert_text[i].org_name[3] )
					{
						*c     = convert_text[i].fix_name[0];
						*(c+1) = convert_text[i].fix_name[1];
						*(c+2) = NULL_SPACE1;
						*(c+3) = NULL_SPACE1;
						break;
					}
					i++;
				}
			}

			if (*c == COMMAND_DEFAULT_TEXT)
			{
				i = 0;
				while (default_text[i].org_name)
				{
					if (*(c+1) == default_text[i].org_name[1])
					{
						*c     = c1 + (default_text[i].fix_name / s1);
						*(c+1) = c2 + (default_text[i].fix_name % s1);
						c++;
						break;
					}
					i++;
				}
				c++;
			}

			else if (*c == COMMAND_EXPAND_TEXT)
			{
				i = 0;
				while (expand_text[i].org_name)
				{
					if (*(c+1) == expand_text[i].org_name[1])
					{
						*c     = c1 + (expand_text[i].fix_name / s1);
						*(c+1) = c2 + (expand_text[i].fix_name % s1);
						c++;
						break;
					}
					i++;
				}
				c++;
			}

			else if (*c == COMMAND_LOGO_MARK)
			{
				i = 0;
				while (logo[i].org_name)
				{
					if (*(c+1) == logo[i].org_name[1] &&
						*(c+2) == logo[i].org_name[2] &&
						*(c+3) == logo[i].org_name[3] &&
						*(c+4) == logo[i].org_name[4] &&
						*(c+5) == logo[i].org_name[5] &&
						*(c+6) == logo[i].org_name[6] &&
						*(c+7) == logo[i].org_name[7] )
						{
							*c     = c1 + (logo[i].fix_name / s1);
							*(c+1) = c2 + (logo[i].fix_name % s1);
							*(c+2) = c1 + ((logo[i].fix_name + 1) / s1);
							*(c+3) = c2 + ((logo[i].fix_name + 1) % s1);
							*(c+4) = c1 + ((logo[i].fix_name + 2) / s1);
							*(c+5) = c2 + ((logo[i].fix_name + 2) % s1);
							*(c+6) = c1 + ((logo[i].fix_name + 3) / s1);
							*(c+7) = c2 + ((logo[i].fix_name + 3) % s1);
							c+=7;
							break;
						}
					i++;
				}

				i = 0;
				while (deco[i].org_name)
				{
					if (*(c+1) == deco[i].org_name[1] &&
						*(c+2) == deco[i].org_name[2] &&
						*(c+3) == deco[i].org_name[3] &&
						*(c+4) == deco[i].org_name[4] &&
						*(c+5) == deco[i].org_name[5] )
						{
							*c     = c1 + (deco[i].fix_name / s1);
							*(c+1) = c2 + (deco[i].fix_name % s1);
							*(c+2) = c1 + ((deco[i].fix_name + 1) / s1);
							*(c+3) = c2 + ((deco[i].fix_name + 1) % s1);
							*(c+4) = c1 + ((deco[i].fix_name + 2) / s1);
							*(c+5) = c2 + ((deco[i].fix_name + 2) % s1);
							c+=5;
							break;
						}
					i++;
				}
				c++;
			}

			else if (*c == '\\' && *(c+1) == 'n')
			{
				*c     = ' ';
				*(c+1) = '\n';
				c+=2;
			}
			else
				c++;
		}
	}
}
