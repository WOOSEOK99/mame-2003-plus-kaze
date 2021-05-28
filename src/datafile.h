#ifndef DATAFILE_H
#define DATAFILE_H

#define CMD_PLUS
#define UI_COLOR_DISPLAY
#define UI_TITLE_MENU

struct tDatafileIndex
{
	long offset;
	const struct GameDriver *driver;
};

extern int load_driver_history (const struct GameDriver *drv, char *buffer, int bufsize);

#ifdef CMD_PLUS

#define NULL_SPACE1			'\b'
#define MAX_MENUIDX_ENTRIES 64

#ifdef UI_COLOR_DISPLAY
extern pen_t cmd_colortable[];
#endif

char *command_GetDataPtr(int idx);

UINT8 command_BuildMenuItem(const char *menu_item[]);
UINT8 command_CreateData(const struct GameDriver *drv, const char *menu_item[]);
void  command_DestroyData(void);

// #ifdef UI_TITLE_MENU
// extern int load_driver_command (const struct GameDriver *drv, char *buffer, int bufsize, const int menu_sel, const char *menu_item[]);
// #endif
// extern int load_driver_command (const struct GameDriver *drv, char *buffer, int bufsize, const int menu_sel);

#endif /* CMD_PLUS */

#endif
