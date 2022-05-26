/***************************************************************************

  inptport.c

  Input ports handling

TODO:	remove the 1 analog device per port limitation
		support for inputports producing interrupts

                 Controller-Specific Input Port Mappings
                 ---------------------------------------

The main purpose of the controller-specific configuration is to remap
inputs.  This is handled via two mechanisms: key re-mapping and
sequence re-mapping.

Key re-mapping occurs at the most basic level.  The specified keycode is
replaced where ever it occurs (in all sequences) with the specified
replacement.  Only single keycodes can be used as a replacement (that is, a
single key cannot be replaced with a key sequence).

Sequence mapping occurs at a higher level.  In this case, the entire
key sequence for the given input is replaced with the specified key
sequence.

Keycodes are specified as a single keyword.  Key sequences are specified
as a series of keycodes separated.  Two special keywords are available
for defining key sequences, CODE_OR and CODE_NOT.

When two keycodes are specified together (separated by only whitespace),
the default action is a logical AND, such that both keys must be pressed
at the same time for the action to occur.  Often it desired that either
key can be pressed for the action to occur (for example LEFT CTRL and
Joystick Button 0), in which case the two keycodes need to be separated
by a CODE_OR keyword.  Finally, certain combinations may be
undesirable and warrant no action by MAME.  For these, the keywords should
be specified by a CODE_NOT (for example, ALT-TAB on a windows machine).


***************************************************************************/

#include <math.h>
#include "driver.h"
#include "config.h"

#define CMD_PLUS
#define UI_COLOR_DISPLAY
#define AUTO_FIRE
#define NEOGEO_HACKS

/***************************************************************************

	Extern declarations

***************************************************************************/
#ifdef UI_COLOR_DISPLAY
extern void ConvertCommandMacro(char *buf);
#endif /* UI_COLOR_DISPLAY */

extern void *record;
extern void *playback;

extern unsigned int dispensed_tickets;
extern unsigned int coins[COIN_COUNTERS];
extern unsigned int lastcoin[COIN_COUNTERS];
extern unsigned int coinlockedout[COIN_COUNTERS];

extern bool save_protection;

static unsigned short input_port_value[MAX_INPUT_PORTS];
static unsigned short input_vblank[MAX_INPUT_PORTS];


/***************************************************************************

	Local variables

***************************************************************************/

/* Assuming a maxium of one analog input device per port BW 101297 */
static struct InputPort *input_analog[MAX_INPUT_PORTS];
static int input_analog_current_value[MAX_INPUT_PORTS], input_analog_previous_value[MAX_INPUT_PORTS];
static int          input_analog_init[MAX_INPUT_PORTS];
static int         input_analog_scale[MAX_INPUT_PORTS];

/* [player#][mame axis#] array */
static InputCode      analogjoy_input[MAX_PLAYER_COUNT][MAX_ANALOG_AXES];
	
static int           mouse_delta_axis[MAX_PLAYER_COUNT][MAX_ANALOG_AXES];
static int        lightgun_delta_axis[MAX_PLAYER_COUNT][MAX_ANALOG_AXES];
static int        analog_current_axis[MAX_PLAYER_COUNT][MAX_ANALOG_AXES];
static int       analog_previous_axis[MAX_PLAYER_COUNT][MAX_ANALOG_AXES];


/***************************************************************************

  Configuration load/save

***************************************************************************/

/* this must match the enum in inptport.h */
const char ipdn_defaultstrings[][MAX_DEFSTR_LEN] =
{
	"Off",
	"On",
	"No",
	"Yes",
	"생명수",		//"Lives",
	"보너스 생명",		//"Bonus Life",
	"난이도",		//"Difficulty",
	"데모 사운드",		//"Demo Sounds",
	"코인설정",		//"Coinage",
	"코인 A",		//"Coin A",
	"코인 B",		//"Coin B",
	"9 코인/1 크레딧",		//"9 Coins/1 Credit",
	"8 코인/1 크레딧",		//"8 Coins/1 Credit",
	"7 코인/1 크레딧",		//"7 Coins/1 Credit",
	"6 코인/1 크레딧",		//"6 Coins/1 Credit",
	"5 코인/1 크레딧",		//"5 Coins/1 Credit",
	"4 코인/1 크레딧",		//"4 Coins/1 Credit",
	"3 코인/1 크레딧",		//"3 Coins/1 Credit",
	"8 코인/3 크레딧",		//"8 Coins/3 Credits",
	"4 코인/2 크레딧",		//"4 Coins/2 Credits",
	"2 코인/1 크레딧",		//"2 Coins/1 Credit",
	"5 코인/3 크레딧",		//"5 Coins/3 Credits",
	"3 코인/2 크레딧",		//"3 Coins/2 Credits",
	"4 코인/3 크레딧",		//"4 Coins/3 Credits",
	"4 코인/4 크레딧",		//"4 Coins/4 Credits",
	"3 코인/3 크레딧",		//"3 Coins/3 Credits",
	"2 코인/2 크레딧",		//"2 Coins/2 Credits",
	"1 코인/1 크레딧",		//"1 Coin/1 Credit",
	"4 코인/5 크레딧",		//"4 Coins/5 Credits",
	"3 코인/4 크레딧",		//"3 Coins/4 Credits",
	"2 코인/3 크레딧",		//"2 Coins/3 Credits",
	"4 코인/7 크레딧",		//"4 Coins/7 Credits",
	"2 코인/4 크레딧",		//"2 Coins/4 Credits",
	"1 코인/2 크레딧",		//"1 Coin/2 Credits",
	"2 코인/5 크레딧",		//"2 Coins/5 Credits",
	"2 코인/6 크레딧",		//"2 Coins/6 Credits",
	"1 코인/3 크레딧",		//"1 Coin/3 Credits",
	"2 코인/7 크레딧",		//"2 Coins/7 Credits",
	"2 코인/8 크레딧",		//"2 Coins/8 Credits",
	"1 코인/4 크레딧",		//"1 Coin/4 Credits",
	"1 코인/5 크레딧",		//"1 Coin/5 Credits",
	"1 코인/6 크레딧",		//"1 Coin/6 Credits",
	"1 코인/7 크레딧",		//"1 Coin/7 Credits",
	"1 코인/8 크레딧",		//"1 Coin/8 Credits",
	"1 코인/9 크레딧",		//"1 Coin/9 Credits",
	"자유 플레이",		//"Free Play",
	"Cabinet",		//"Cabinet",
	"Upright",		//"Upright",
	"칵테일",		//"Cocktail",
	"화면 플립",		//"Flip Screen",
	"서비스 모드",		//"Service Mode",
	"Unused",
	"Unknown"
};


struct ipd inputport_defaults[] =
{
  { IPT_UI_CONFIGURE,         "설정 메뉴",    SEQ_DEF_3(KEYCODE_TAB,   CODE_OR, JOYCODE_1_BUTTON8) },
  { IPT_UI_SHOW_GFX,          "Show Gfx",       SEQ_DEF_1(KEYCODE_NONE) },
  { IPT_UI_TOGGLE_CHEAT,      "토글 치트",   SEQ_DEF_1(KEYCODE_NONE) },
  { IPT_UI_RESET_MACHINE,     "게임 리셋",			SEQ_DEF_1(KEYCODE_F3) },  //reset game
  { IPT_UI_UP,                "UI 위",          SEQ_DEF_5(KEYCODE_UP,    CODE_OR, JOYCODE_1_UP,         CODE_OR,   JOYCODE_1_LEFT_UP)     },
  { IPT_UI_DOWN,              "UI 아래",        SEQ_DEF_5(KEYCODE_DOWN,  CODE_OR, JOYCODE_1_DOWN,       CODE_OR,   JOYCODE_1_LEFT_DOWN)   },
  { IPT_UI_LEFT,              "UI 왼쪽",        SEQ_DEF_5(KEYCODE_LEFT,  CODE_OR, JOYCODE_1_LEFT,       CODE_OR,   JOYCODE_1_LEFT_LEFT)   },
  { IPT_UI_RIGHT,             "UI 오른쪽",       SEQ_DEF_5(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT,      CODE_OR,   JOYCODE_1_LEFT_RIGHT)  },
  { IPT_UI_SELECT,            "UI 선택",      SEQ_DEF_3(KEYCODE_ENTER, CODE_OR, JOYCODE_1_BUTTON2) },
  { IPT_UI_CANCEL,            "UI 취소",      SEQ_DEF_3(KEYCODE_ESC,   CODE_OR, JOYCODE_1_BUTTON1) },
  { IPT_UI_ADD_CHEAT,         "치트 추가",      SEQ_DEF_1(KEYCODE_NONE) },
  { IPT_UI_DELETE_CHEAT,      "치트 삭제",   SEQ_DEF_1(KEYCODE_NONE) },
  { IPT_UI_SAVE_CHEAT,        "치트 저장",     SEQ_DEF_1(KEYCODE_NONE) },
  { IPT_UI_WATCH_VALUE,       "값 보기",    SEQ_DEF_1(KEYCODE_NONE) },
#ifdef CMD_PLUS
  {	IPT_UI_COMMAND,           "공략 보기",   SEQ_DEF_3(KEYCODE_END,   CODE_OR, JOYCODE_2_BUTTON8) },
#endif
#ifdef AUTO_FIRE
  { IPT_UI_TOGGLE_AUTOFIRE,   "토글 자동연사",SEQ_DEF_3(KEYCODE_INSERT,   CODE_OR, JOYCODE_2_BUTTON7)},
#endif
  { IPT_UI_CHEAT,			  "치트 목록 보기",	SEQ_DEF_3(KEYCODE_PGUP,   CODE_OR, JOYCODE_1_BUTTON7) },// Show_CHEATLIST_START
  { IPT_UI_EDIT_CHEAT,        "치트 수정",     SEQ_DEF_1(KEYCODE_NONE) },

  { IPT_START1,   "P1 시작",      SEQ_DEF_5(KEYCODE_1, CODE_OR, JOYCODE_1_START, CODE_OR, JOYCODE_GUN_1_START) },
  { IPT_START2,   "P2 시작",      SEQ_DEF_5(KEYCODE_2, CODE_OR, JOYCODE_2_START, CODE_OR, JOYCODE_GUN_2_START) },
  { IPT_START3,   "P3 시작",      SEQ_DEF_5(KEYCODE_3, CODE_OR, JOYCODE_3_START, CODE_OR, JOYCODE_GUN_3_START) },
  { IPT_START4,   "P4 시작",      SEQ_DEF_5(KEYCODE_4, CODE_OR, JOYCODE_4_START, CODE_OR, JOYCODE_GUN_4_START) },
  { IPT_START5,   "P5 시작",      SEQ_DEF_1(JOYCODE_5_START) },
  { IPT_START6,   "P6 시작",      SEQ_DEF_1(JOYCODE_6_START) },
  { IPT_START7,   "P7 시작",      SEQ_DEF_0 },
  { IPT_START8,   "P8 시작",      SEQ_DEF_0 },
  { IPT_COIN1,    "P1 동전",       SEQ_DEF_5(KEYCODE_5, CODE_OR, JOYCODE_1_SELECT, CODE_OR, JOYCODE_GUN_1_SELECT) },
  { IPT_COIN2,    "P2 동전",       SEQ_DEF_5(KEYCODE_6, CODE_OR, JOYCODE_2_SELECT, CODE_OR, JOYCODE_GUN_1_SELECT) },
  { IPT_COIN3,    "P3 동전",       SEQ_DEF_5(KEYCODE_7, CODE_OR, JOYCODE_3_SELECT, CODE_OR, JOYCODE_GUN_1_SELECT) },
  { IPT_COIN4,    "P4 동전",       SEQ_DEF_5(KEYCODE_8, CODE_OR, JOYCODE_4_SELECT, CODE_OR, JOYCODE_GUN_1_SELECT) },
  { IPT_COIN5,    "P5 동전",       SEQ_DEF_1(JOYCODE_5_SELECT) },
  { IPT_COIN6,    "P6 동전",       SEQ_DEF_1(JOYCODE_6_SELECT) },
  { IPT_COIN7,    "P7 동전",       SEQ_DEF_0 },
  { IPT_COIN8,    "P8 동전",       SEQ_DEF_0 },

  { IPT_SERVICE1, "Service 1",     SEQ_DEF_1(KEYCODE_9)      },
  { IPT_SERVICE2, "Service 2",     SEQ_DEF_1(KEYCODE_0)      },
  { IPT_SERVICE3, "Service 3",     SEQ_DEF_1(KEYCODE_MINUS)  },
  { IPT_SERVICE4, "Service 4",     SEQ_DEF_1(KEYCODE_EQUALS) },
  { IPT_TILT,     "Tilt",          SEQ_DEF_1(KEYCODE_T)      },

  { IPT_JOYSTICK_UP         | IPF_PLAYER1, "P1 위",          SEQ_DEF_7(KEYCODE_UP,       CODE_OR, JOYCODE_1_UP,          CODE_OR, JOYCODE_1_LEFT_UP, 		  CODE_OR, JOYCODE_GUN_1_DPAD_UP)       },
  { IPT_JOYSTICK_DOWN       | IPF_PLAYER1, "P1 아래",        SEQ_DEF_7(KEYCODE_DOWN,     CODE_OR, JOYCODE_1_DOWN,        CODE_OR, JOYCODE_1_LEFT_DOWN,        CODE_OR, JOYCODE_GUN_1_DPAD_DOWN)     },
  { IPT_JOYSTICK_LEFT       | IPF_PLAYER1, "P1 왼쪽",        SEQ_DEF_7(KEYCODE_LEFT,     CODE_OR, JOYCODE_1_LEFT,        CODE_OR, JOYCODE_1_LEFT_LEFT,        CODE_OR, JOYCODE_GUN_1_DPAD_LEFT)     },
  { IPT_JOYSTICK_RIGHT      | IPF_PLAYER1, "P1 오른쪽",      SEQ_DEF_7(KEYCODE_RIGHT,    CODE_OR, JOYCODE_1_RIGHT,       CODE_OR, JOYCODE_1_LEFT_RIGHT,       CODE_OR, JOYCODE_GUN_1_DPAD_RIGHT)    },
  { IPT_BUTTON1             | IPF_PLAYER1, "P1 버튼 1",      SEQ_DEF_7(KEYCODE_LCONTROL, CODE_OR, JOYCODE_1_BUTTON1,     CODE_OR, JOYCODE_MOUSE_1_BUTTON1,  CODE_OR, JOYCODE_GUN_1_BUTTON1) },
  { IPT_BUTTON2             | IPF_PLAYER1, "P1 버튼 2",      SEQ_DEF_7(KEYCODE_LALT,     CODE_OR, JOYCODE_1_BUTTON2,     CODE_OR, JOYCODE_MOUSE_1_BUTTON2,  CODE_OR, JOYCODE_GUN_1_BUTTON2) },
  { IPT_BUTTON3             | IPF_PLAYER1, "P1 버튼 3",      SEQ_DEF_7(KEYCODE_SPACE,    CODE_OR, JOYCODE_1_BUTTON3,     CODE_OR, JOYCODE_MOUSE_1_BUTTON3,  CODE_OR, JOYCODE_GUN_1_BUTTON3) },
  { IPT_BUTTON4             | IPF_PLAYER1, "P1 버튼 4",      SEQ_DEF_7(KEYCODE_LSHIFT,   CODE_OR, JOYCODE_1_BUTTON4,     CODE_OR, JOYCODE_MOUSE_1_BUTTON4,  CODE_OR, JOYCODE_GUN_1_BUTTON4)  },
  { IPT_BUTTON5             | IPF_PLAYER1, "P1 버튼 5",      SEQ_DEF_3(KEYCODE_Z,        CODE_OR, JOYCODE_1_BUTTON5)  },
  { IPT_BUTTON6             | IPF_PLAYER1, "P1 버튼 6",      SEQ_DEF_3(KEYCODE_X,        CODE_OR, JOYCODE_1_BUTTON6)  },
  { IPT_BUTTON7             | IPF_PLAYER1, "P1 버튼 7",      SEQ_DEF_3(KEYCODE_C,        CODE_OR, JOYCODE_1_BUTTON7)  },
  { IPT_BUTTON8             | IPF_PLAYER1, "P1 버튼 8",      SEQ_DEF_3(KEYCODE_V,        CODE_OR, JOYCODE_1_BUTTON8)  },
  { IPT_BUTTON9             | IPF_PLAYER1, "P1 버튼 9",      SEQ_DEF_3(KEYCODE_B,        CODE_OR, JOYCODE_1_BUTTON9)  },
  { IPT_BUTTON10            | IPF_PLAYER1, "P1 버튼 10",     SEQ_DEF_3(KEYCODE_N,        CODE_OR, JOYCODE_1_BUTTON10) },
  { IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER1, "P1 오른쪽/위",    SEQ_DEF_5(KEYCODE_I,        CODE_OR, JOYCODE_1_RIGHT_UP,    CODE_OR, JOYCODE_2_UP)    },
  { IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER1, "P1 오른쪽/아래",  SEQ_DEF_5(KEYCODE_K,        CODE_OR, JOYCODE_1_RIGHT_DOWN,  CODE_OR, JOYCODE_2_DOWN)  },
  { IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER1, "P1 오른쪽/왼쪽",  SEQ_DEF_5(KEYCODE_J,        CODE_OR, JOYCODE_1_RIGHT_LEFT,  CODE_OR, JOYCODE_2_LEFT)  },
  { IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER1, "P1 오른쪽/오른쪽", SEQ_DEF_5(KEYCODE_L,        CODE_OR, JOYCODE_1_RIGHT_RIGHT, CODE_OR, JOYCODE_2_RIGHT) },
  { IPT_JOYSTICKLEFT_UP     | IPF_PLAYER1, "P1 왼쪽/위",     SEQ_DEF_5(KEYCODE_E,        CODE_OR, JOYCODE_1_LEFT_UP,     CODE_OR, JOYCODE_1_UP)    },
  { IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER1, "P1 왼쪽/아래",   SEQ_DEF_5(KEYCODE_D,        CODE_OR, JOYCODE_1_LEFT_DOWN,   CODE_OR, JOYCODE_1_DOWN)  },
  { IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER1, "P1 왼쪽/왼쪽",   SEQ_DEF_5(KEYCODE_S,        CODE_OR, JOYCODE_1_LEFT_LEFT,   CODE_OR, JOYCODE_1_LEFT)  },
  { IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER1, "P1 왼쪽/오른쪽",  SEQ_DEF_5(KEYCODE_F,        CODE_OR, JOYCODE_1_LEFT_RIGHT,  CODE_OR, JOYCODE_1_RIGHT) },

  { IPT_JOYSTICK_UP         | IPF_PLAYER2, "P2 위",           SEQ_DEF_7(KEYCODE_R,             CODE_OR, JOYCODE_2_UP,         CODE_OR, JOYCODE_2_LEFT_UP,         CODE_OR, JOYCODE_GUN_2_DPAD_UP)       },
  { IPT_JOYSTICK_DOWN       | IPF_PLAYER2, "P2 아래",         SEQ_DEF_7(KEYCODE_F,             CODE_OR, JOYCODE_2_DOWN,       CODE_OR, JOYCODE_2_LEFT_DOWN,       CODE_OR, JOYCODE_GUN_2_DPAD_DOWN)     },
  { IPT_JOYSTICK_LEFT       | IPF_PLAYER2, "P2 왼쪽",         SEQ_DEF_7(KEYCODE_D,             CODE_OR, JOYCODE_2_LEFT,       CODE_OR, JOYCODE_2_LEFT_LEFT,       CODE_OR, JOYCODE_GUN_2_DPAD_LEFT)     },
  { IPT_JOYSTICK_RIGHT      | IPF_PLAYER2, "P2 오른쪽",       SEQ_DEF_7(KEYCODE_G,             CODE_OR, JOYCODE_2_RIGHT,      CODE_OR, JOYCODE_2_LEFT_RIGHT,      CODE_OR, JOYCODE_GUN_2_DPAD_RIGHT)    },
  { IPT_BUTTON1             | IPF_PLAYER2, "P2 버튼 1",       SEQ_DEF_7(KEYCODE_A,             CODE_OR, JOYCODE_2_BUTTON1,    CODE_OR, JOYCODE_MOUSE_2_BUTTON1,   CODE_OR, JOYCODE_GUN_2_BUTTON1) },
  { IPT_BUTTON2             | IPF_PLAYER2, "P2 버튼 2",       SEQ_DEF_7(KEYCODE_S,             CODE_OR, JOYCODE_2_BUTTON2,    CODE_OR, JOYCODE_MOUSE_2_BUTTON2,   CODE_OR, JOYCODE_GUN_2_BUTTON2) },
  { IPT_BUTTON3             | IPF_PLAYER2, "P2 버튼 3",       SEQ_DEF_7(KEYCODE_Q,             CODE_OR, JOYCODE_2_BUTTON3,    CODE_OR, JOYCODE_MOUSE_2_BUTTON3,   CODE_OR, JOYCODE_GUN_2_BUTTON3) },
  { IPT_BUTTON4             | IPF_PLAYER2, "P2 버튼 4",       SEQ_DEF_7(KEYCODE_W,             CODE_OR, JOYCODE_2_BUTTON4,    CODE_OR, JOYCODE_MOUSE_2_BUTTON4,   CODE_OR, JOYCODE_GUN_2_BUTTON4) },
  { IPT_BUTTON5             | IPF_PLAYER2, "P2 버튼 5",       SEQ_DEF_1(JOYCODE_2_BUTTON5)  },
  { IPT_BUTTON6             | IPF_PLAYER2, "P2 버튼 6",       SEQ_DEF_1(JOYCODE_2_BUTTON6)  },
  { IPT_BUTTON7             | IPF_PLAYER2, "P2 버튼 7",       SEQ_DEF_1(JOYCODE_2_BUTTON7)  },
  { IPT_BUTTON8             | IPF_PLAYER2, "P2 버튼 8",       SEQ_DEF_1(JOYCODE_2_BUTTON8)  },
  { IPT_BUTTON9             | IPF_PLAYER2, "P2 버튼 9",       SEQ_DEF_1(JOYCODE_2_BUTTON9)  },
  { IPT_BUTTON10            | IPF_PLAYER2, "P2 버튼 10",      SEQ_DEF_1(JOYCODE_2_BUTTON10) },
  { IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER2, "P2 오른쪽/위",    SEQ_DEF_3(JOYCODE_2_RIGHT_UP,    CODE_OR, JOYCODE_4_UP)    },
  { IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER2, "P2 오른쪽/아래",  SEQ_DEF_3(JOYCODE_2_RIGHT_DOWN,  CODE_OR, JOYCODE_4_DOWN)  },
  { IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER2, "P2 오른쪽/왼쪽",  SEQ_DEF_3(JOYCODE_2_RIGHT_LEFT,  CODE_OR, JOYCODE_4_LEFT)  },
  { IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER2, "P2 오른쪽/오른쪽", SEQ_DEF_3(JOYCODE_2_RIGHT_RIGHT, CODE_OR, JOYCODE_4_RIGHT) },
  { IPT_JOYSTICKLEFT_UP     | IPF_PLAYER2, "P2 왼쪽/위",      SEQ_DEF_3(JOYCODE_2_LEFT_UP,     CODE_OR, JOYCODE_3_UP)    },
  { IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER2, "P2 왼쪽/아래",    SEQ_DEF_3(JOYCODE_2_LEFT_DOWN,   CODE_OR, JOYCODE_3_DOWN)  },
  { IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER2, "P2 왼쪽/왼쪽",    SEQ_DEF_3(JOYCODE_2_LEFT_LEFT,   CODE_OR, JOYCODE_3_LEFT)  },
  { IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER2, "P2 왼쪽/오른쪽",  SEQ_DEF_3(JOYCODE_2_LEFT_RIGHT,  CODE_OR, JOYCODE_3_RIGHT) },

  { IPT_JOYSTICK_UP         | IPF_PLAYER3, "P3 위",           SEQ_DEF_5(KEYCODE_I,             CODE_OR, JOYCODE_3_UP,      CODE_OR, JOYCODE_3_LEFT_UP)       },
  { IPT_JOYSTICK_DOWN       | IPF_PLAYER3, "P3 아래",         SEQ_DEF_5(KEYCODE_K,             CODE_OR, JOYCODE_3_DOWN,    CODE_OR, JOYCODE_3_LEFT_DOWN)     },
  { IPT_JOYSTICK_LEFT       | IPF_PLAYER3, "P3 왼쪽",         SEQ_DEF_5(KEYCODE_J,             CODE_OR, JOYCODE_3_LEFT,    CODE_OR, JOYCODE_3_LEFT_LEFT)     },
  { IPT_JOYSTICK_RIGHT      | IPF_PLAYER3, "P3 오른쪽",       SEQ_DEF_5(KEYCODE_L,             CODE_OR, JOYCODE_3_RIGHT,   CODE_OR, JOYCODE_3_LEFT_RIGHT)    },
  { IPT_BUTTON1             | IPF_PLAYER3, "P3 버튼 1",       SEQ_DEF_5(KEYCODE_RCONTROL,      CODE_OR, JOYCODE_3_BUTTON1, CODE_OR, JOYCODE_MOUSE_3_BUTTON1) },
  { IPT_BUTTON2             | IPF_PLAYER3, "P3 버튼 2",       SEQ_DEF_5(KEYCODE_RSHIFT,        CODE_OR, JOYCODE_3_BUTTON2, CODE_OR, JOYCODE_MOUSE_3_BUTTON2) },
  { IPT_BUTTON3             | IPF_PLAYER3, "P3 버튼 3",       SEQ_DEF_5(KEYCODE_ENTER,         CODE_OR, JOYCODE_3_BUTTON3, CODE_OR, JOYCODE_MOUSE_3_BUTTON3) },
  { IPT_BUTTON4             | IPF_PLAYER3, "P3 Button 4",    SEQ_DEF_5(JOYCODE_3_BUTTON4, CODE_OR, JOYCODE_MOUSE_3_BUTTON4, CODE_OR, JOYCODE_GUN_3_BUTTON4 ) },
  { IPT_BUTTON5             | IPF_PLAYER3, "P3 Button 5",    SEQ_DEF_1(JOYCODE_3_BUTTON5)  },
  { IPT_BUTTON6             | IPF_PLAYER3, "P3 Button 6",    SEQ_DEF_1(JOYCODE_3_BUTTON6)  },
  { IPT_BUTTON7             | IPF_PLAYER3, "P3 Button 7",    SEQ_DEF_1(JOYCODE_3_BUTTON7)  },
  { IPT_BUTTON8             | IPF_PLAYER3, "P3 Button 8",    SEQ_DEF_1(JOYCODE_3_BUTTON8)  },
  { IPT_BUTTON9             | IPF_PLAYER3, "P3 Button 9",    SEQ_DEF_1(JOYCODE_3_BUTTON9)  },
  { IPT_BUTTON10            | IPF_PLAYER3, "P3 Button 10",   SEQ_DEF_1(JOYCODE_3_BUTTON10) },
  { IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER3, "P3 Right/Up",    SEQ_DEF_0 },
  { IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER3, "P3 Right/Down",  SEQ_DEF_0 },
  { IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER3, "P3 Right/Left",  SEQ_DEF_0 },
  { IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER3, "P3 Right/Right", SEQ_DEF_0 },
  { IPT_JOYSTICKLEFT_UP     | IPF_PLAYER3, "P3 Left/Up",     SEQ_DEF_0 },
  { IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER3, "P3 Left/Down",   SEQ_DEF_0 },
  { IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER3, "P3 Left/Left",   SEQ_DEF_0 },
  { IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER3, "P3 Left/Right",  SEQ_DEF_0 },

/* Players 4-8 share this basic mapping, without any keyboard or lightgun binds */
#define BASIC_DIGITAL_MAP(PLAYER_NUM) \
  { IPT_JOYSTICK_UP         | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Up",          SEQ_DEF_3(JOYCODE_##PLAYER_NUM##_UP,          CODE_OR, JOYCODE_##PLAYER_NUM##_LEFT_UP   )  }, \
  { IPT_JOYSTICK_DOWN       | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Down",        SEQ_DEF_3(JOYCODE_##PLAYER_NUM##_DOWN,        CODE_OR, JOYCODE_##PLAYER_NUM##_LEFT_DOWN )  }, \
  { IPT_JOYSTICK_LEFT       | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Left",        SEQ_DEF_3(JOYCODE_##PLAYER_NUM##_LEFT,        CODE_OR, JOYCODE_##PLAYER_NUM##_LEFT_LEFT )  }, \
  { IPT_JOYSTICK_RIGHT      | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Right",       SEQ_DEF_3(JOYCODE_##PLAYER_NUM##_RIGHT,       CODE_OR, JOYCODE_##PLAYER_NUM##_LEFT_RIGHT ) }, \
  { IPT_BUTTON1             | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Button 1",    SEQ_DEF_3(JOYCODE_##PLAYER_NUM##_BUTTON1,     CODE_OR, JOYCODE_MOUSE_##PLAYER_NUM##_BUTTON1 ) }, \
  { IPT_BUTTON2             | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Button 2",    SEQ_DEF_3(JOYCODE_##PLAYER_NUM##_BUTTON2,     CODE_OR, JOYCODE_MOUSE_##PLAYER_NUM##_BUTTON2 ) }, \
  { IPT_BUTTON3             | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Button 3",    SEQ_DEF_3(JOYCODE_##PLAYER_NUM##_BUTTON3,     CODE_OR, JOYCODE_MOUSE_##PLAYER_NUM##_BUTTON3 ) }, \
  { IPT_BUTTON4             | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Button 4",    SEQ_DEF_3(JOYCODE_##PLAYER_NUM##_BUTTON4,     CODE_OR, JOYCODE_MOUSE_##PLAYER_NUM##_BUTTON4 ) }, \
  { IPT_BUTTON5             | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Button 5",    SEQ_DEF_1(JOYCODE_##PLAYER_NUM##_BUTTON5)  }, \
  { IPT_BUTTON6             | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Button 6",    SEQ_DEF_1(JOYCODE_##PLAYER_NUM##_BUTTON6)  }, \
  { IPT_BUTTON7             | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Button 7",    SEQ_DEF_1(JOYCODE_##PLAYER_NUM##_BUTTON7)  }, \
  { IPT_BUTTON8             | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Button 8",    SEQ_DEF_1(JOYCODE_##PLAYER_NUM##_BUTTON8)  }, \
  { IPT_BUTTON9             | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Button 9",    SEQ_DEF_1(JOYCODE_##PLAYER_NUM##_BUTTON9)  }, \
  { IPT_BUTTON10            | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Button 10",   SEQ_DEF_1(JOYCODE_##PLAYER_NUM##_BUTTON10) }, \
  { IPT_JOYSTICKRIGHT_UP    | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Right/Up",    SEQ_DEF_0 }, \
  { IPT_JOYSTICKRIGHT_DOWN  | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Right/Down",  SEQ_DEF_0 }, \
  { IPT_JOYSTICKRIGHT_LEFT  | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Right/Left",  SEQ_DEF_0 }, \
  { IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Right/Right", SEQ_DEF_0 }, \
  { IPT_JOYSTICKLEFT_UP     | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Left/Up",     SEQ_DEF_0 }, \
  { IPT_JOYSTICKLEFT_DOWN   | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Left/Down",   SEQ_DEF_0 }, \
  { IPT_JOYSTICKLEFT_LEFT   | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Left/Left",   SEQ_DEF_0 }, \
  { IPT_JOYSTICKLEFT_RIGHT  | IPF_PLAYER##PLAYER_NUM, "P"  #PLAYER_NUM " Left/Right",  SEQ_DEF_0 },

  BASIC_DIGITAL_MAP(4)
  BASIC_DIGITAL_MAP(5)
  BASIC_DIGITAL_MAP(6)
  BASIC_DIGITAL_MAP(7)
  BASIC_DIGITAL_MAP(8)

  { IPT_PEDAL                 | IPF_PLAYER1, "P1 Pedal 1",     SEQ_DEF_3(KEYCODE_LCONTROL, CODE_OR, JOYCODE_1_BUTTON6) },
  { (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER1, "P1 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL                 | IPF_PLAYER2, "P2 Pedal 1",     SEQ_DEF_3(KEYCODE_A, CODE_OR, JOYCODE_2_BUTTON6) },
  { (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER2, "P2 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL                 | IPF_PLAYER3, "P3 Pedal 1",     SEQ_DEF_3(KEYCODE_RCONTROL, CODE_OR, JOYCODE_3_BUTTON6) },
  { (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER3, "P3 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL                 | IPF_PLAYER4, "P4 Pedal 1",     SEQ_DEF_1(JOYCODE_4_BUTTON6) },
  { (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER4, "P4 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL                 | IPF_PLAYER5, "P5 Pedal 1",     SEQ_DEF_1(JOYCODE_5_BUTTON6) },
  { (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER5, "P5 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL                 | IPF_PLAYER6, "P6 Pedal 1",     SEQ_DEF_1(JOYCODE_6_BUTTON6) },
  { (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER6, "P6 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL                 | IPF_PLAYER7, "P7 Pedal 1",     SEQ_DEF_1(JOYCODE_7_BUTTON6) },
  { (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER7, "P7 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL                 | IPF_PLAYER8, "P8 Pedal 1",     SEQ_DEF_1(JOYCODE_8_BUTTON6) },
  { (IPT_PEDAL+IPT_EXTENSION) | IPF_PLAYER8, "P8 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },

  { IPT_PEDAL2                 | IPF_PLAYER1, "P1 Pedal 2",     SEQ_DEF_1(JOYCODE_1_BUTTON5) },
  { (IPT_PEDAL2+IPT_EXTENSION) | IPF_PLAYER1, "P1 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL2                 | IPF_PLAYER2, "P2 Pedal 2",     SEQ_DEF_1(JOYCODE_2_BUTTON5) },
  { (IPT_PEDAL2+IPT_EXTENSION) | IPF_PLAYER2, "P2 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL2                 | IPF_PLAYER3, "P3 Pedal 2",     SEQ_DEF_1(JOYCODE_3_BUTTON5) },
  { (IPT_PEDAL2+IPT_EXTENSION) | IPF_PLAYER3, "P3 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL2                 | IPF_PLAYER4, "P4 Pedal 2",     SEQ_DEF_1(JOYCODE_4_BUTTON5) },
  { (IPT_PEDAL2+IPT_EXTENSION) | IPF_PLAYER4, "P4 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL2                 | IPF_PLAYER5, "P5 Pedal 2",     SEQ_DEF_1(JOYCODE_5_BUTTON5) },
  { (IPT_PEDAL2+IPT_EXTENSION) | IPF_PLAYER5, "P5 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL2                 | IPF_PLAYER6, "P6 Pedal 2",     SEQ_DEF_1(JOYCODE_6_BUTTON5) },
  { (IPT_PEDAL2+IPT_EXTENSION) | IPF_PLAYER6, "P6 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL2                 | IPF_PLAYER7, "P7 Pedal 2",     SEQ_DEF_1(JOYCODE_7_BUTTON5) },
  { (IPT_PEDAL2+IPT_EXTENSION) | IPF_PLAYER7, "P7 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },
  { IPT_PEDAL2                 | IPF_PLAYER8, "P8 Pedal 2",     SEQ_DEF_1(JOYCODE_8_BUTTON5) },
  { (IPT_PEDAL2+IPT_EXTENSION) | IPF_PLAYER8, "P8 Auto Release <Y/N>", SEQ_DEF_1(KEYCODE_Y) },

  { IPT_PADDLE | IPF_PLAYER1,  "Paddle",        SEQ_DEF_5(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT, CODE_OR, JOYCODE_1_LEFT_LEFT) },
  { (IPT_PADDLE | IPF_PLAYER1)+IPT_EXTENSION,             "Paddle",        SEQ_DEF_5(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT, CODE_OR, JOYCODE_1_LEFT_RIGHT) },
  { IPT_PADDLE | IPF_PLAYER2,  "Paddle 2",      SEQ_DEF_5(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT, CODE_OR, JOYCODE_2_LEFT_LEFT) },
  { (IPT_PADDLE | IPF_PLAYER2)+IPT_EXTENSION,             "Paddle 2",      SEQ_DEF_5(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT, CODE_OR, JOYCODE_2_LEFT_RIGHT) },
  { IPT_PADDLE | IPF_PLAYER3,  "Paddle 3",      SEQ_DEF_5(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT, CODE_OR, JOYCODE_3_LEFT_LEFT) },
  { (IPT_PADDLE | IPF_PLAYER3)+IPT_EXTENSION,             "Paddle 3",      SEQ_DEF_5(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT, CODE_OR, JOYCODE_3_LEFT_RIGHT) },
  { IPT_PADDLE | IPF_PLAYER4,  "Paddle 4",      SEQ_DEF_3(JOYCODE_4_LEFT, CODE_OR, JOYCODE_4_LEFT_LEFT) },
  { (IPT_PADDLE | IPF_PLAYER4)+IPT_EXTENSION,             "Paddle 4",      SEQ_DEF_3(JOYCODE_4_RIGHT, CODE_OR, JOYCODE_4_LEFT_RIGHT) },
  { IPT_PADDLE | IPF_PLAYER5,  "Paddle 5",      SEQ_DEF_3(JOYCODE_5_LEFT, CODE_OR, JOYCODE_5_LEFT_LEFT) },
  { (IPT_PADDLE | IPF_PLAYER5)+IPT_EXTENSION,             "Paddle 5",      SEQ_DEF_3(JOYCODE_5_RIGHT, CODE_OR, JOYCODE_5_LEFT_RIGHT) },
  { IPT_PADDLE | IPF_PLAYER6,  "Paddle 6",      SEQ_DEF_3(JOYCODE_6_LEFT, CODE_OR, JOYCODE_6_LEFT_LEFT) },
  { (IPT_PADDLE | IPF_PLAYER6)+IPT_EXTENSION,             "Paddle 6",      SEQ_DEF_3(JOYCODE_6_RIGHT, CODE_OR, JOYCODE_6_LEFT_RIGHT) },
  { IPT_PADDLE | IPF_PLAYER7,  "Paddle 7",      SEQ_DEF_3(JOYCODE_7_LEFT, CODE_OR, JOYCODE_7_LEFT_LEFT) },
  { (IPT_PADDLE | IPF_PLAYER7)+IPT_EXTENSION,             "Paddle 7",      SEQ_DEF_3(JOYCODE_7_RIGHT, CODE_OR, JOYCODE_7_LEFT_RIGHT) },
  { IPT_PADDLE | IPF_PLAYER8,  "Paddle 8",      SEQ_DEF_3(JOYCODE_8_LEFT, CODE_OR, JOYCODE_8_LEFT_LEFT) },
  { (IPT_PADDLE | IPF_PLAYER8)+IPT_EXTENSION,             "Paddle 8",      SEQ_DEF_3(JOYCODE_8_RIGHT, CODE_OR, JOYCODE_8_LEFT_RIGHT) },

  { IPT_PADDLE_V | IPF_PLAYER1,  "Paddle V",          SEQ_DEF_5(KEYCODE_UP, CODE_OR, JOYCODE_1_UP, CODE_OR, JOYCODE_1_LEFT_UP) },
  { (IPT_PADDLE_V | IPF_PLAYER1)+IPT_EXTENSION,             "Paddle V",        SEQ_DEF_5(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN, CODE_OR, JOYCODE_1_LEFT_DOWN) },
  { IPT_PADDLE_V | IPF_PLAYER2,  "Paddle V 2",        SEQ_DEF_5(KEYCODE_R, CODE_OR, JOYCODE_2_UP, CODE_OR, JOYCODE_2_LEFT_UP) },
  { (IPT_PADDLE_V | IPF_PLAYER2)+IPT_EXTENSION,             "Paddle V 2",      SEQ_DEF_5(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN, CODE_OR, JOYCODE_2_LEFT_DOWN) },
  { IPT_PADDLE_V | IPF_PLAYER3,  "Paddle V 3",        SEQ_DEF_5(KEYCODE_I, CODE_OR, JOYCODE_3_UP, CODE_OR, JOYCODE_3_LEFT_UP) },
  { (IPT_PADDLE_V | IPF_PLAYER3)+IPT_EXTENSION,             "Paddle V 3",      SEQ_DEF_5(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN, CODE_OR, JOYCODE_3_LEFT_DOWN) },
  { IPT_PADDLE_V | IPF_PLAYER4,  "Paddle V 4",        SEQ_DEF_3(JOYCODE_4_UP, CODE_OR, JOYCODE_4_LEFT_UP) },
  { (IPT_PADDLE_V | IPF_PLAYER4)+IPT_EXTENSION,             "Paddle V 4",      SEQ_DEF_3(JOYCODE_4_DOWN, CODE_OR, JOYCODE_4_LEFT_DOWN) },
  { IPT_PADDLE_V | IPF_PLAYER5,  "Paddle V 5",        SEQ_DEF_3(JOYCODE_5_UP, CODE_OR, JOYCODE_5_LEFT_UP) },
  { (IPT_PADDLE_V | IPF_PLAYER5)+IPT_EXTENSION,             "Paddle V 5",      SEQ_DEF_3(JOYCODE_5_DOWN, CODE_OR, JOYCODE_5_LEFT_DOWN) },
  { IPT_PADDLE_V | IPF_PLAYER6,  "Paddle V 6",        SEQ_DEF_3(JOYCODE_6_UP, CODE_OR, JOYCODE_6_LEFT_UP) },
  { (IPT_PADDLE_V | IPF_PLAYER6)+IPT_EXTENSION,             "Paddle V 6",      SEQ_DEF_3(JOYCODE_6_DOWN, CODE_OR, JOYCODE_6_LEFT_DOWN) },
  { IPT_PADDLE_V | IPF_PLAYER7,  "Paddle V 7",        SEQ_DEF_3(JOYCODE_7_UP, CODE_OR, JOYCODE_7_LEFT_UP) },
  { (IPT_PADDLE_V | IPF_PLAYER7)+IPT_EXTENSION,             "Paddle V 7",      SEQ_DEF_3(JOYCODE_7_DOWN, CODE_OR, JOYCODE_7_LEFT_DOWN) },
  { IPT_PADDLE_V | IPF_PLAYER8,  "Paddle V 8",        SEQ_DEF_3(JOYCODE_8_UP, CODE_OR, JOYCODE_8_LEFT_UP) },
  { (IPT_PADDLE_V | IPF_PLAYER8)+IPT_EXTENSION,             "Paddle V 8",      SEQ_DEF_3(JOYCODE_8_DOWN, CODE_OR, JOYCODE_8_LEFT_DOWN) },

  { IPT_DIAL | IPF_PLAYER1,    "Dial",          SEQ_DEF_5(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT, CODE_OR, JOYCODE_1_LEFT_LEFT) },
  { (IPT_DIAL | IPF_PLAYER1)+IPT_EXTENSION,               "Dial",        SEQ_DEF_5(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT, CODE_OR, JOYCODE_1_LEFT_RIGHT) },
  { IPT_DIAL | IPF_PLAYER2,    "Dial 2",        SEQ_DEF_5(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT, CODE_OR, JOYCODE_2_LEFT_LEFT) },
  { (IPT_DIAL | IPF_PLAYER2)+IPT_EXTENSION,               "Dial 2",      SEQ_DEF_5(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT, CODE_OR, JOYCODE_2_LEFT_RIGHT) },
  { IPT_DIAL | IPF_PLAYER3,    "Dial 3",        SEQ_DEF_5(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT, CODE_OR, JOYCODE_3_LEFT_LEFT) },
  { (IPT_DIAL | IPF_PLAYER3)+IPT_EXTENSION,               "Dial 3",      SEQ_DEF_5(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT, CODE_OR, JOYCODE_3_LEFT_RIGHT) },
  { IPT_DIAL | IPF_PLAYER4,    "Dial 4",        SEQ_DEF_3(JOYCODE_4_LEFT, CODE_OR, JOYCODE_4_LEFT_LEFT) },
  { (IPT_DIAL | IPF_PLAYER4)+IPT_EXTENSION,               "Dial 4",      SEQ_DEF_3(JOYCODE_4_RIGHT, CODE_OR, JOYCODE_4_LEFT_RIGHT) },
  { IPT_DIAL | IPF_PLAYER5,    "Dial 5",        SEQ_DEF_3(JOYCODE_5_LEFT, CODE_OR, JOYCODE_5_LEFT_LEFT) },
  { (IPT_DIAL | IPF_PLAYER5)+IPT_EXTENSION,               "Dial 5",      SEQ_DEF_3(JOYCODE_5_RIGHT, CODE_OR, JOYCODE_5_LEFT_RIGHT) },
  { IPT_DIAL | IPF_PLAYER6,    "Dial 6",        SEQ_DEF_3(JOYCODE_6_LEFT, CODE_OR, JOYCODE_6_LEFT_LEFT) },
  { (IPT_DIAL | IPF_PLAYER6)+IPT_EXTENSION,               "Dial 6",      SEQ_DEF_3(JOYCODE_6_RIGHT, CODE_OR, JOYCODE_6_LEFT_RIGHT) },
  { IPT_DIAL | IPF_PLAYER7,    "Dial 7",        SEQ_DEF_3(JOYCODE_7_LEFT, CODE_OR, JOYCODE_7_LEFT_LEFT) },
  { (IPT_DIAL | IPF_PLAYER7)+IPT_EXTENSION,               "Dial 7",      SEQ_DEF_3(JOYCODE_7_RIGHT, CODE_OR, JOYCODE_7_LEFT_RIGHT) },
  { IPT_DIAL | IPF_PLAYER8,    "Dial 8",        SEQ_DEF_3(JOYCODE_8_LEFT, CODE_OR, JOYCODE_8_LEFT_LEFT) },
  { (IPT_DIAL | IPF_PLAYER8)+IPT_EXTENSION,               "Dial 8",      SEQ_DEF_3(JOYCODE_8_RIGHT, CODE_OR, JOYCODE_8_LEFT_RIGHT) },

  { IPT_DIAL_V | IPF_PLAYER1,  "Dial V",          SEQ_DEF_5(KEYCODE_UP, CODE_OR, JOYCODE_1_UP, CODE_OR, JOYCODE_1_LEFT_UP) },
  { (IPT_DIAL_V | IPF_PLAYER1)+IPT_EXTENSION,             "Dial V",        SEQ_DEF_5(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN, CODE_OR, JOYCODE_1_LEFT_DOWN) },
  { IPT_DIAL_V | IPF_PLAYER2,  "Dial V 2",        SEQ_DEF_5(KEYCODE_R, CODE_OR, JOYCODE_2_UP, CODE_OR, JOYCODE_2_LEFT_UP) },
  { (IPT_DIAL_V | IPF_PLAYER2)+IPT_EXTENSION,             "Dial V 2",      SEQ_DEF_5(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN, CODE_OR, JOYCODE_2_LEFT_DOWN) },
  { IPT_DIAL_V | IPF_PLAYER3,  "Dial V 3",        SEQ_DEF_5(KEYCODE_I, CODE_OR, JOYCODE_3_UP, CODE_OR, JOYCODE_3_LEFT_UP) },
  { (IPT_DIAL_V | IPF_PLAYER3)+IPT_EXTENSION,             "Dial V 3",      SEQ_DEF_5(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN, CODE_OR, JOYCODE_3_LEFT_DOWN) },
  { IPT_DIAL_V | IPF_PLAYER4,  "Dial V 4",        SEQ_DEF_3(JOYCODE_4_UP, CODE_OR, JOYCODE_4_LEFT_UP) },
  { (IPT_DIAL_V | IPF_PLAYER4)+IPT_EXTENSION,             "Dial V 4",      SEQ_DEF_3(JOYCODE_4_DOWN, CODE_OR, JOYCODE_4_LEFT_DOWN) },
  { IPT_DIAL_V | IPF_PLAYER5,  "Dial V 5",        SEQ_DEF_3(JOYCODE_5_UP, CODE_OR, JOYCODE_5_LEFT_UP) },
  { (IPT_DIAL_V | IPF_PLAYER5)+IPT_EXTENSION,             "Dial V 5",      SEQ_DEF_3(JOYCODE_5_DOWN, CODE_OR, JOYCODE_5_LEFT_DOWN) },
  { IPT_DIAL_V | IPF_PLAYER6,  "Dial V 6",        SEQ_DEF_3(JOYCODE_6_UP, CODE_OR, JOYCODE_6_LEFT_UP) },
  { (IPT_DIAL_V | IPF_PLAYER6)+IPT_EXTENSION,             "Dial V 6",      SEQ_DEF_3(JOYCODE_6_DOWN, CODE_OR, JOYCODE_6_LEFT_DOWN) },
  { IPT_DIAL_V | IPF_PLAYER7,  "Dial V 7",        SEQ_DEF_3(JOYCODE_7_UP, CODE_OR, JOYCODE_7_LEFT_UP) },
  { (IPT_DIAL_V | IPF_PLAYER7)+IPT_EXTENSION,             "Dial V 7",      SEQ_DEF_3(JOYCODE_7_DOWN, CODE_OR, JOYCODE_7_LEFT_DOWN) },
  { IPT_DIAL_V | IPF_PLAYER8,  "Dial V 8",        SEQ_DEF_3(JOYCODE_8_UP, CODE_OR, JOYCODE_8_LEFT_UP) },
  { (IPT_DIAL_V | IPF_PLAYER8)+IPT_EXTENSION,             "Dial V 8",      SEQ_DEF_3(JOYCODE_8_DOWN, CODE_OR, JOYCODE_8_LEFT_DOWN) },

  { IPT_TRACKBALL_X | IPF_PLAYER1, "Track X",   SEQ_DEF_5(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT, CODE_OR, JOYCODE_1_LEFT_LEFT) },
  { (IPT_TRACKBALL_X | IPF_PLAYER1)+IPT_EXTENSION,                 "Track X",   SEQ_DEF_5(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT, CODE_OR, JOYCODE_1_LEFT_RIGHT) },
  { IPT_TRACKBALL_X | IPF_PLAYER2, "Track X 2", SEQ_DEF_5(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT, CODE_OR, JOYCODE_2_LEFT_LEFT) },
  { (IPT_TRACKBALL_X | IPF_PLAYER2)+IPT_EXTENSION,                 "Track X 2", SEQ_DEF_5(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT, CODE_OR, JOYCODE_2_LEFT_RIGHT) },
  { IPT_TRACKBALL_X | IPF_PLAYER3, "Track X 3", SEQ_DEF_5(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT, CODE_OR, JOYCODE_3_LEFT_LEFT) },
  { (IPT_TRACKBALL_X | IPF_PLAYER3)+IPT_EXTENSION,                 "Track X 3", SEQ_DEF_5(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT, CODE_OR, JOYCODE_3_LEFT_RIGHT) },
  { IPT_TRACKBALL_X | IPF_PLAYER4, "Track X 4", SEQ_DEF_3(JOYCODE_4_LEFT, CODE_OR, JOYCODE_4_LEFT_LEFT) },
  { (IPT_TRACKBALL_X | IPF_PLAYER4)+IPT_EXTENSION,                 "Track X 4", SEQ_DEF_3(JOYCODE_4_RIGHT, CODE_OR, JOYCODE_4_LEFT_RIGHT) },
  { IPT_TRACKBALL_X | IPF_PLAYER5, "Track X 5", SEQ_DEF_3(JOYCODE_5_LEFT, CODE_OR, JOYCODE_5_LEFT_LEFT) },
  { (IPT_TRACKBALL_X | IPF_PLAYER5)+IPT_EXTENSION,                 "Track X 5", SEQ_DEF_3(JOYCODE_5_RIGHT, CODE_OR, JOYCODE_5_LEFT_RIGHT) },
  { IPT_TRACKBALL_X | IPF_PLAYER6, "Track X 6", SEQ_DEF_3(JOYCODE_6_LEFT, CODE_OR, JOYCODE_6_LEFT_LEFT) },
  { (IPT_TRACKBALL_X | IPF_PLAYER6)+IPT_EXTENSION,                 "Track X 6", SEQ_DEF_3(JOYCODE_6_RIGHT, CODE_OR, JOYCODE_6_LEFT_RIGHT) },
  { IPT_TRACKBALL_X | IPF_PLAYER7, "Track X 7", SEQ_DEF_3(JOYCODE_7_LEFT, CODE_OR, JOYCODE_7_LEFT_LEFT) },
  { (IPT_TRACKBALL_X | IPF_PLAYER7)+IPT_EXTENSION,                 "Track X 7", SEQ_DEF_3(JOYCODE_7_RIGHT, CODE_OR, JOYCODE_7_LEFT_RIGHT) },
  { IPT_TRACKBALL_X | IPF_PLAYER8, "Track X 8", SEQ_DEF_3(JOYCODE_8_LEFT, CODE_OR, JOYCODE_8_LEFT_LEFT) },
  { (IPT_TRACKBALL_X | IPF_PLAYER8)+IPT_EXTENSION,                 "Track X 8", SEQ_DEF_3(JOYCODE_8_RIGHT, CODE_OR, JOYCODE_8_LEFT_RIGHT) },

  { IPT_TRACKBALL_Y | IPF_PLAYER1, "Track Y",   SEQ_DEF_5(KEYCODE_UP, CODE_OR, JOYCODE_1_UP, CODE_OR, JOYCODE_1_LEFT_UP) },
  { (IPT_TRACKBALL_Y | IPF_PLAYER1)+IPT_EXTENSION,                 "Track Y",   SEQ_DEF_5(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN, CODE_OR, JOYCODE_1_LEFT_DOWN) },
  { IPT_TRACKBALL_Y | IPF_PLAYER2, "Track Y 2", SEQ_DEF_5(KEYCODE_R, CODE_OR, JOYCODE_2_UP, CODE_OR, JOYCODE_2_LEFT_UP) },
  { (IPT_TRACKBALL_Y | IPF_PLAYER2)+IPT_EXTENSION,                 "Track Y 2", SEQ_DEF_5(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN, CODE_OR, JOYCODE_2_LEFT_DOWN) },
  { IPT_TRACKBALL_Y | IPF_PLAYER3, "Track Y 3", SEQ_DEF_5(KEYCODE_I, CODE_OR, JOYCODE_3_UP, CODE_OR, JOYCODE_3_LEFT_UP) },
  { (IPT_TRACKBALL_Y | IPF_PLAYER3)+IPT_EXTENSION,                 "Track Y 3", SEQ_DEF_5(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN, CODE_OR, JOYCODE_3_LEFT_DOWN) },
  { IPT_TRACKBALL_Y | IPF_PLAYER4, "Track Y 4", SEQ_DEF_3(JOYCODE_4_UP, CODE_OR, JOYCODE_4_LEFT_UP) },
  { (IPT_TRACKBALL_Y | IPF_PLAYER4)+IPT_EXTENSION,                 "Track Y 4", SEQ_DEF_3(JOYCODE_4_DOWN, CODE_OR, JOYCODE_4_LEFT_DOWN) },
  { IPT_TRACKBALL_Y | IPF_PLAYER5, "Track Y 5", SEQ_DEF_3(JOYCODE_5_UP, CODE_OR, JOYCODE_5_LEFT_UP) },
  { (IPT_TRACKBALL_Y | IPF_PLAYER5)+IPT_EXTENSION,                 "Track Y 5", SEQ_DEF_3(JOYCODE_5_DOWN, CODE_OR, JOYCODE_5_LEFT_DOWN) },
  { IPT_TRACKBALL_Y | IPF_PLAYER6, "Track Y 6", SEQ_DEF_3(JOYCODE_6_UP, CODE_OR, JOYCODE_6_LEFT_UP) },
  { (IPT_TRACKBALL_Y | IPF_PLAYER6)+IPT_EXTENSION,                 "Track Y 6", SEQ_DEF_3(JOYCODE_6_DOWN, CODE_OR, JOYCODE_6_LEFT_DOWN) },
  { IPT_TRACKBALL_Y | IPF_PLAYER7, "Track Y 7", SEQ_DEF_3(JOYCODE_7_UP, CODE_OR, JOYCODE_7_LEFT_UP) },
  { (IPT_TRACKBALL_Y | IPF_PLAYER7)+IPT_EXTENSION,                 "Track Y 7", SEQ_DEF_3(JOYCODE_7_DOWN, CODE_OR, JOYCODE_7_LEFT_DOWN) },
  { IPT_TRACKBALL_Y | IPF_PLAYER8, "Track Y 8", SEQ_DEF_3(JOYCODE_8_UP, CODE_OR, JOYCODE_8_LEFT_UP) },
  { (IPT_TRACKBALL_Y | IPF_PLAYER8)+IPT_EXTENSION,                 "Track Y 8", SEQ_DEF_3(JOYCODE_8_DOWN, CODE_OR, JOYCODE_8_LEFT_DOWN) },

  { IPT_AD_STICK_X | IPF_PLAYER1, "AD Stick X",   SEQ_DEF_5(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT, CODE_OR, JOYCODE_1_LEFT_LEFT) },
  { (IPT_AD_STICK_X | IPF_PLAYER1)+IPT_EXTENSION,                "AD Stick X",   SEQ_DEF_5(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT, CODE_OR, JOYCODE_1_LEFT_RIGHT) },
  { IPT_AD_STICK_X | IPF_PLAYER2, "AD Stick X 2", SEQ_DEF_5(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT, CODE_OR, JOYCODE_2_LEFT_LEFT) },
  { (IPT_AD_STICK_X | IPF_PLAYER2)+IPT_EXTENSION,                "AD Stick X 2", SEQ_DEF_5(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT, CODE_OR, JOYCODE_2_LEFT_RIGHT) },
  { IPT_AD_STICK_X | IPF_PLAYER3, "AD Stick X 3", SEQ_DEF_5(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT, CODE_OR, JOYCODE_3_LEFT_LEFT) },
  { (IPT_AD_STICK_X | IPF_PLAYER3)+IPT_EXTENSION,                "AD Stick X 3", SEQ_DEF_5(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT, CODE_OR, JOYCODE_3_LEFT_RIGHT) },
  { IPT_AD_STICK_X | IPF_PLAYER4, "AD Stick X 4", SEQ_DEF_3(JOYCODE_4_LEFT, CODE_OR, JOYCODE_4_LEFT_LEFT) },
  { (IPT_AD_STICK_X | IPF_PLAYER4)+IPT_EXTENSION,                "AD Stick X 4", SEQ_DEF_3(JOYCODE_4_RIGHT, CODE_OR, JOYCODE_4_LEFT_RIGHT) },
  { IPT_AD_STICK_X | IPF_PLAYER5, "AD Stick X 5", SEQ_DEF_3(JOYCODE_5_LEFT, CODE_OR, JOYCODE_5_LEFT_LEFT) },
  { (IPT_AD_STICK_X | IPF_PLAYER5)+IPT_EXTENSION,                "AD Stick X 5", SEQ_DEF_3(JOYCODE_5_RIGHT, CODE_OR, JOYCODE_5_LEFT_RIGHT) },
  { IPT_AD_STICK_X | IPF_PLAYER6, "AD Stick X 6", SEQ_DEF_3(JOYCODE_6_LEFT, CODE_OR, JOYCODE_6_LEFT_LEFT) },
  { (IPT_AD_STICK_X | IPF_PLAYER6)+IPT_EXTENSION,                "AD Stick X 6", SEQ_DEF_3(JOYCODE_6_RIGHT, CODE_OR, JOYCODE_6_LEFT_RIGHT) },
  { IPT_AD_STICK_X | IPF_PLAYER7, "AD Stick X 7", SEQ_DEF_3(JOYCODE_7_LEFT, CODE_OR, JOYCODE_7_LEFT_LEFT) },
  { (IPT_AD_STICK_X | IPF_PLAYER7)+IPT_EXTENSION,                "AD Stick X 7", SEQ_DEF_3(JOYCODE_7_RIGHT, CODE_OR, JOYCODE_7_LEFT_RIGHT) },
  { IPT_AD_STICK_X | IPF_PLAYER8, "AD Stick X 8", SEQ_DEF_3(JOYCODE_8_LEFT, CODE_OR, JOYCODE_8_LEFT_LEFT) },
  { (IPT_AD_STICK_X | IPF_PLAYER8)+IPT_EXTENSION,                "AD Stick X 8", SEQ_DEF_3(JOYCODE_8_RIGHT, CODE_OR, JOYCODE_8_LEFT_RIGHT) },

  { IPT_AD_STICK_Y | IPF_PLAYER1, "AD Stick Y",   SEQ_DEF_5(KEYCODE_UP, CODE_OR, JOYCODE_1_UP, CODE_OR, JOYCODE_1_LEFT_UP) },
  { (IPT_AD_STICK_Y | IPF_PLAYER1)+IPT_EXTENSION,                "AD Stick Y",   SEQ_DEF_5(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN, CODE_OR, JOYCODE_1_LEFT_DOWN) },
  { IPT_AD_STICK_Y | IPF_PLAYER2, "AD Stick Y 2", SEQ_DEF_5(KEYCODE_R, CODE_OR, JOYCODE_2_UP, CODE_OR, JOYCODE_2_LEFT_UP) },
  { (IPT_AD_STICK_Y | IPF_PLAYER2)+IPT_EXTENSION,                "AD Stick Y 2", SEQ_DEF_5(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN, CODE_OR, JOYCODE_2_LEFT_DOWN) },
  { IPT_AD_STICK_Y | IPF_PLAYER3, "AD Stick Y 3", SEQ_DEF_5(KEYCODE_I, CODE_OR, JOYCODE_3_UP, CODE_OR, JOYCODE_3_LEFT_UP) },
  { (IPT_AD_STICK_Y | IPF_PLAYER3)+IPT_EXTENSION,                "AD Stick Y 3", SEQ_DEF_5(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN, CODE_OR, JOYCODE_3_LEFT_DOWN) },
  { IPT_AD_STICK_Y | IPF_PLAYER4, "AD Stick Y 4", SEQ_DEF_3(JOYCODE_4_UP, CODE_OR, JOYCODE_4_LEFT_UP) },
  { (IPT_AD_STICK_Y | IPF_PLAYER4)+IPT_EXTENSION,                "AD Stick Y 4", SEQ_DEF_3(JOYCODE_4_DOWN, CODE_OR, JOYCODE_4_LEFT_DOWN) },
  { IPT_AD_STICK_Y | IPF_PLAYER5, "AD Stick Y 5", SEQ_DEF_3(JOYCODE_5_UP, CODE_OR, JOYCODE_5_LEFT_UP) },
  { (IPT_AD_STICK_Y | IPF_PLAYER5)+IPT_EXTENSION,                "AD Stick Y 5", SEQ_DEF_3(JOYCODE_5_DOWN, CODE_OR, JOYCODE_5_LEFT_DOWN) },
  { IPT_AD_STICK_Y | IPF_PLAYER6, "AD Stick Y 6", SEQ_DEF_3(JOYCODE_6_UP, CODE_OR, JOYCODE_6_LEFT_UP) },
  { (IPT_AD_STICK_Y | IPF_PLAYER6)+IPT_EXTENSION,                "AD Stick Y 6", SEQ_DEF_3(JOYCODE_6_DOWN, CODE_OR, JOYCODE_6_LEFT_DOWN) },
  { IPT_AD_STICK_Y | IPF_PLAYER7, "AD Stick Y 7", SEQ_DEF_3(JOYCODE_7_UP, CODE_OR, JOYCODE_7_LEFT_UP) },
  { (IPT_AD_STICK_Y | IPF_PLAYER7)+IPT_EXTENSION,                "AD Stick Y 7", SEQ_DEF_3(JOYCODE_7_DOWN, CODE_OR, JOYCODE_7_LEFT_DOWN) },
  { IPT_AD_STICK_Y | IPF_PLAYER8, "AD Stick Y 8", SEQ_DEF_3(JOYCODE_8_UP, CODE_OR, JOYCODE_8_LEFT_UP) },
  { (IPT_AD_STICK_Y | IPF_PLAYER8)+IPT_EXTENSION,                "AD Stick Y 8", SEQ_DEF_3(JOYCODE_8_DOWN, CODE_OR, JOYCODE_8_LEFT_DOWN) },

  { IPT_AD_STICK_Z | IPF_PLAYER1, "AD Stick Z",   SEQ_DEF_0 },
  { (IPT_AD_STICK_Z | IPF_PLAYER1)+IPT_EXTENSION,                "AD Stick Z",   SEQ_DEF_0 },
  { IPT_AD_STICK_Z | IPF_PLAYER2, "AD Stick Z 2", SEQ_DEF_0 },
  { (IPT_AD_STICK_Z | IPF_PLAYER2)+IPT_EXTENSION,                "AD Stick Z 2", SEQ_DEF_0 },
  { IPT_AD_STICK_Z | IPF_PLAYER3, "AD Stick Z 3", SEQ_DEF_0 },
  { (IPT_AD_STICK_Z | IPF_PLAYER3)+IPT_EXTENSION,                "AD Stick Z 3", SEQ_DEF_0 },
  { IPT_AD_STICK_Z | IPF_PLAYER4, "AD Stick Z 4", SEQ_DEF_0 },
  { (IPT_AD_STICK_Z | IPF_PLAYER4)+IPT_EXTENSION,                "AD Stick Z 4", SEQ_DEF_0 },
  { IPT_AD_STICK_Z | IPF_PLAYER5, "AD Stick Z 5", SEQ_DEF_0 },
  { (IPT_AD_STICK_Z | IPF_PLAYER5)+IPT_EXTENSION,                "AD Stick Z 5", SEQ_DEF_0 },
  { IPT_AD_STICK_Z | IPF_PLAYER6, "AD Stick Z 6", SEQ_DEF_0 },
  { (IPT_AD_STICK_Z | IPF_PLAYER6)+IPT_EXTENSION,                "AD Stick Z 6", SEQ_DEF_0 },
  { IPT_AD_STICK_Z | IPF_PLAYER7, "AD Stick Z 7", SEQ_DEF_0 },
  { (IPT_AD_STICK_Z | IPF_PLAYER7)+IPT_EXTENSION,                "AD Stick Z 7", SEQ_DEF_0 },
  { IPT_AD_STICK_Z | IPF_PLAYER8, "AD Stick Z 8", SEQ_DEF_0 },
  { (IPT_AD_STICK_Z | IPF_PLAYER8)+IPT_EXTENSION,                "AD Stick Z 8", SEQ_DEF_0 },

  { IPT_LIGHTGUN_X | IPF_PLAYER1, "Lightgun X",   SEQ_DEF_5(KEYCODE_LEFT, CODE_OR, JOYCODE_1_LEFT, CODE_OR, JOYCODE_1_LEFT_LEFT) },
  { (IPT_LIGHTGUN_X | IPF_PLAYER1)+IPT_EXTENSION,                "Lightgun X",   SEQ_DEF_5(KEYCODE_RIGHT, CODE_OR, JOYCODE_1_RIGHT, CODE_OR, JOYCODE_1_LEFT_RIGHT) },
  { IPT_LIGHTGUN_X | IPF_PLAYER2, "Lightgun X 2", SEQ_DEF_5(KEYCODE_D, CODE_OR, JOYCODE_2_LEFT, CODE_OR, JOYCODE_2_LEFT_LEFT) },
  { (IPT_LIGHTGUN_X | IPF_PLAYER2)+IPT_EXTENSION,                "Lightgun X 2", SEQ_DEF_5(KEYCODE_G, CODE_OR, JOYCODE_2_RIGHT, CODE_OR, JOYCODE_2_LEFT_RIGHT) },
  { IPT_LIGHTGUN_X | IPF_PLAYER3, "Lightgun X 3", SEQ_DEF_5(KEYCODE_J, CODE_OR, JOYCODE_3_LEFT, CODE_OR, JOYCODE_3_LEFT_LEFT) },
  { (IPT_LIGHTGUN_X | IPF_PLAYER3)+IPT_EXTENSION,                "Lightgun X 3", SEQ_DEF_5(KEYCODE_L, CODE_OR, JOYCODE_3_RIGHT, CODE_OR, JOYCODE_3_LEFT_RIGHT) },
  { IPT_LIGHTGUN_X | IPF_PLAYER4, "Lightgun X 4", SEQ_DEF_3(JOYCODE_4_LEFT, CODE_OR, JOYCODE_4_LEFT_LEFT) },
  { (IPT_LIGHTGUN_X | IPF_PLAYER4)+IPT_EXTENSION,                "Lightgun X 4", SEQ_DEF_3(JOYCODE_4_RIGHT, CODE_OR, JOYCODE_4_LEFT_RIGHT) },
  { IPT_LIGHTGUN_X | IPF_PLAYER5, "Lightgun X 5", SEQ_DEF_3(JOYCODE_5_LEFT, CODE_OR, JOYCODE_5_LEFT_LEFT) },
  { (IPT_LIGHTGUN_X | IPF_PLAYER5)+IPT_EXTENSION,                "Lightgun X 5", SEQ_DEF_3(JOYCODE_5_RIGHT, CODE_OR, JOYCODE_5_LEFT_RIGHT) },
  { IPT_LIGHTGUN_X | IPF_PLAYER6, "Lightgun X 6", SEQ_DEF_3(JOYCODE_6_LEFT, CODE_OR, JOYCODE_6_LEFT_LEFT) },
  { (IPT_LIGHTGUN_X | IPF_PLAYER6)+IPT_EXTENSION,                "Lightgun X 6", SEQ_DEF_3(JOYCODE_6_RIGHT, CODE_OR, JOYCODE_6_LEFT_RIGHT) },
  { IPT_LIGHTGUN_X | IPF_PLAYER7, "Lightgun X 7", SEQ_DEF_3(JOYCODE_7_LEFT, CODE_OR, JOYCODE_7_LEFT_LEFT) },
  { (IPT_LIGHTGUN_X | IPF_PLAYER7)+IPT_EXTENSION,                "Lightgun X 7", SEQ_DEF_3(JOYCODE_7_RIGHT, CODE_OR, JOYCODE_7_LEFT_RIGHT) },
  { IPT_LIGHTGUN_X | IPF_PLAYER8, "Lightgun X 8", SEQ_DEF_3(JOYCODE_8_LEFT, CODE_OR, JOYCODE_8_LEFT_LEFT) },
  { (IPT_LIGHTGUN_X | IPF_PLAYER8)+IPT_EXTENSION,                "Lightgun X 8", SEQ_DEF_3(JOYCODE_8_RIGHT, CODE_OR, JOYCODE_8_LEFT_RIGHT) },

  { IPT_LIGHTGUN_Y | IPF_PLAYER1, "Lightgun Y",   SEQ_DEF_5(KEYCODE_UP, CODE_OR, JOYCODE_1_UP, CODE_OR, JOYCODE_1_LEFT_UP) },
  { (IPT_LIGHTGUN_Y | IPF_PLAYER1)+IPT_EXTENSION,                "Lightgun Y",   SEQ_DEF_5(KEYCODE_DOWN, CODE_OR, JOYCODE_1_DOWN, CODE_OR, JOYCODE_1_LEFT_DOWN) },
  { IPT_LIGHTGUN_Y | IPF_PLAYER2, "Lightgun Y 2", SEQ_DEF_5(KEYCODE_R, CODE_OR, JOYCODE_2_UP, CODE_OR, JOYCODE_2_LEFT_UP) },
  { (IPT_LIGHTGUN_Y | IPF_PLAYER2)+IPT_EXTENSION,                "Lightgun Y 2", SEQ_DEF_5(KEYCODE_F, CODE_OR, JOYCODE_2_DOWN, CODE_OR, JOYCODE_2_LEFT_DOWN) },
  { IPT_LIGHTGUN_Y | IPF_PLAYER3, "Lightgun Y 3", SEQ_DEF_5(KEYCODE_I, CODE_OR, JOYCODE_3_UP, CODE_OR, JOYCODE_3_LEFT_UP) },
  { (IPT_LIGHTGUN_Y | IPF_PLAYER3)+IPT_EXTENSION,                "Lightgun Y 3", SEQ_DEF_5(KEYCODE_K, CODE_OR, JOYCODE_3_DOWN, CODE_OR, JOYCODE_3_LEFT_DOWN) },
  { IPT_LIGHTGUN_Y | IPF_PLAYER4, "Lightgun Y 4", SEQ_DEF_3(JOYCODE_4_UP, CODE_OR, JOYCODE_4_LEFT_UP) },
  { (IPT_LIGHTGUN_Y | IPF_PLAYER4)+IPT_EXTENSION,                "Lightgun Y 4", SEQ_DEF_3(JOYCODE_4_DOWN, CODE_OR, JOYCODE_4_LEFT_DOWN) },
  { IPT_LIGHTGUN_Y | IPF_PLAYER5, "Lightgun Y 5", SEQ_DEF_3(JOYCODE_5_UP, CODE_OR, JOYCODE_5_LEFT_UP) },
  { (IPT_LIGHTGUN_Y | IPF_PLAYER5)+IPT_EXTENSION,                "Lightgun Y 5", SEQ_DEF_3(JOYCODE_5_DOWN, CODE_OR, JOYCODE_5_LEFT_DOWN) },
  { IPT_LIGHTGUN_Y | IPF_PLAYER6, "Lightgun Y 6", SEQ_DEF_3(JOYCODE_6_UP, CODE_OR, JOYCODE_6_LEFT_UP) },
  { (IPT_LIGHTGUN_Y | IPF_PLAYER6)+IPT_EXTENSION,                "Lightgun Y 6", SEQ_DEF_3(JOYCODE_6_DOWN, CODE_OR, JOYCODE_6_LEFT_DOWN) },
  { IPT_LIGHTGUN_Y | IPF_PLAYER7, "Lightgun Y 7", SEQ_DEF_3(JOYCODE_7_UP, CODE_OR, JOYCODE_7_LEFT_UP) },
  { (IPT_LIGHTGUN_Y | IPF_PLAYER7)+IPT_EXTENSION,                "Lightgun Y 7", SEQ_DEF_3(JOYCODE_7_DOWN, CODE_OR, JOYCODE_7_LEFT_DOWN) },
  { IPT_LIGHTGUN_Y | IPF_PLAYER8, "Lightgun Y 8", SEQ_DEF_3(JOYCODE_8_UP, CODE_OR, JOYCODE_8_LEFT_UP) },
  { (IPT_LIGHTGUN_Y | IPF_PLAYER8)+IPT_EXTENSION,                "Lightgun Y 8", SEQ_DEF_3(JOYCODE_8_DOWN, CODE_OR, JOYCODE_8_LEFT_DOWN) },

  { IPT_UNKNOWN,             "UNKNOWN",         SEQ_DEF_0 },
  { IPT_OSD_DESCRIPTION,     "",                SEQ_DEF_0 },
  { IPT_OSD_DESCRIPTION,     "",                SEQ_DEF_0 },
  { IPT_OSD_DESCRIPTION,     "",                SEQ_DEF_0 },
  { IPT_OSD_DESCRIPTION,     "",                SEQ_DEF_0 },
  { IPT_END,                 0,                 SEQ_DEF_0 }  /* returned when there is no match */
};

struct ipd inputport_defaults_backup[sizeof(inputport_defaults)/sizeof(struct ipd)];
#ifdef AUTO_FIRE
#define MAX_INPUT_BITS  1024
static int auto_pressed(InputSeq *seq, UINT32 type, int bit);
static UINT8  autopressed[MAX_INPUT_BITS];
static UINT32 autofire_enable;
#endif 


/***************************************************************************/


const char *generic_ctrl_label(int input)
{
  unsigned int i = 0;

  for( ; inputport_defaults[i].type != IPT_END; ++i)
  {
    struct ipd *entry = &inputport_defaults[i];
    if(entry->type == input)
    {
      if(input >= IPT_SERVICE1 && input <= IPT_UI_EDIT_CHEAT)
        return entry->name; /* these strings are not player-specific */
      else /* start with the third character, trimming the initial 'P1', 'P2', etc which we don't need for libretro */
        return &entry->name[3];
    }
  }
  return NULL;
}


/***************************************************************************/
/* Generic IO */

static int readint(mame_file *f,UINT32 *num)
{
	unsigned i;

	*num = 0;
	for (i = 0;i < sizeof(UINT32);i++)
	{
		unsigned char c;


		*num <<= 8;
		if (mame_fread(f,&c,1) != 1)
			return -1;
		*num |= c;
	}

	return 0;
}

static void writeint(mame_file *f,UINT32 num)
{
	unsigned i;

	for (i = 0;i < sizeof(UINT32);i++)
	{
		unsigned char c;


		c = (num >> 8 * (sizeof(UINT32)-1)) & 0xff;
		mame_fwrite(f,&c,1);
		num <<= 8;
	}
}

static int readword(mame_file *f,UINT16 *num)
{
	unsigned i;
	int res;

	res = 0;
	for (i = 0;i < sizeof(UINT16);i++)
	{
		unsigned char c;


		res <<= 8;
		if (mame_fread(f,&c,1) != 1)
			return -1;
		res |= c;
	}

	*num = res;
	return 0;
}

static void writeword(mame_file *f,UINT16 num)
{
	unsigned i;

	for (i = 0;i < sizeof(UINT16);i++)
	{
		unsigned char c;


		c = (num >> 8 * (sizeof(UINT16)-1)) & 0xff;
		mame_fwrite(f,&c,1);
		num <<= 8;
	}
}



/***************************************************************************/
/* Load */

static void load_default_keys(void)
{
	config_file *cfg;

	memcpy(inputport_defaults_backup,inputport_defaults,sizeof(inputport_defaults));

	cfg = config_open(NULL);
	if (cfg)
	{
		config_read_default_ports(cfg, inputport_defaults);
		config_close(cfg);
	}

	osd_customize_inputport_defaults(inputport_defaults);
}

static void save_default_keys(void)
{
	config_file *cfg;

	cfg = config_create(NULL);
	if (cfg)
	{
		config_write_default_ports(cfg, inputport_defaults_backup, inputport_defaults);
		config_close(cfg);
	}

	memcpy(inputport_defaults,inputport_defaults_backup,sizeof(inputport_defaults_backup));
}


int load_input_port_settings(void)
{
	config_file *cfg;
	int err;
	struct mixer_config mixercfg;
	char buf[20];

	if(options.mame_remapping)
		sprintf(buf,"%s",Machine->gamedrv->name);
	else
		sprintf(buf,"ra_%s",Machine->gamedrv->name);

#ifdef AUTO_FIRE
	memset(autopressed, 0, MAX_INPUT_BITS);

	load_default_keys();

	autofire_enable = 0;
	options.autofiredelay = 1;
#else
	load_default_keys();
#endif

	cfg = config_open(buf);
	if (cfg)
		{
		err = config_read_ports(cfg, Machine->input_ports_default, Machine->input_ports);
		if (err)
				goto getout;

		err = config_read_coin_and_ticket_counters(cfg, coins, lastcoin, coinlockedout, &dispensed_tickets);
		if (err)
				goto getout;

		err = config_read_mixer_config(cfg, &mixercfg);
		if (err)
			goto getout;

		mixer_load_config(&mixercfg);

getout:
		config_close(cfg);
	}

#ifdef AUTO_FIRE
	if (options.autofiredelay > 99)
		options.autofiredelay = 99;
#endif
	/* All analog ports need initialization */
	{
		int i;
		for (i = 0; i < MAX_INPUT_PORTS; i++)
			input_analog_init[i] = 1;
	}

	init_analog_seq();

	update_input_ports();

	/* if we didn't find a saved config, return 0 so the main core knows that it */
	/* is the first time the game is run and it should diplay the disclaimer. */
	return cfg ? 1 : 0;
}

/***************************************************************************/
/* Save */

void save_input_port_settings(void)
{
	if (save_protection == options.mame_remapping)
	{
		config_file *cfg;
		struct mixer_config mixercfg;
		char buf[20];
		if(options.mame_remapping)
			sprintf(buf,"%s",Machine->gamedrv->name);
		else
			sprintf(buf,"ra_%s",Machine->gamedrv->name);

		save_default_keys();

		cfg = config_create(buf);
		if (cfg)
		{
			mixer_save_config(&mixercfg);

			config_write_ports(cfg, Machine->input_ports_default, Machine->input_ports);
			config_write_coin_and_ticket_counters(cfg, coins, lastcoin, coinlockedout, dispensed_tickets);
			config_write_mixer_config(cfg, &mixercfg);
			config_close(cfg);
		}
	}
}


/* Note that the following 3 routines have slightly different meanings with analog ports */
const char *input_port_name(const struct InputPort *in)
{
	int i;
	unsigned type;

	if (in->name != IP_NAME_DEFAULT) return in->name;

	i = 0;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
		type = (in-1)->type & (~IPF_MASK | IPF_PLAYERMASK);
	else
		type = in->type & (~IPF_MASK | IPF_PLAYERMASK);

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
		return inputport_defaults[i+1].name;
	else
		return inputport_defaults[i].name;
}

InputSeq* input_port_type_seq(int type)
{
	unsigned i;

	i = 0;

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	return &inputport_defaults[i].seq;
}

InputSeq* input_port_seq(const struct InputPort *in)
{
	int i,type;

	static InputSeq ip_none = SEQ_DEF_1(CODE_NONE);

	while (seq_get_1((InputSeq*)&in->seq) == CODE_PREVIOUS) in--;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
	{
		type = (in-1)->type & (~IPF_MASK | IPF_PLAYERMASK);
		/* if port is disabled, or cheat with cheats disabled, return no key */
		if (((in-1)->type & IPF_UNUSED) || (!options.cheat_input_ports && ((in-1)->type & IPF_CHEAT)))
			return &ip_none;
	}
	else
	{
		type = in->type & (~IPF_MASK | IPF_PLAYERMASK);
		/* if port is disabled, or cheat with cheats disabled, return no key */
		if ((in->type & IPF_UNUSED) || (!options.cheat_input_ports && (in->type & IPF_CHEAT)))
			return &ip_none;
	}

	if (seq_get_1((InputSeq*)&in->seq) != CODE_DEFAULT)
		return (InputSeq*)&in->seq;

	i = 0;

	while (inputport_defaults[i].type != IPT_END &&
			inputport_defaults[i].type != type)
		i++;

	if ((in->type & ~IPF_MASK) == IPT_EXTENSION)
		return &inputport_defaults[i+1].seq;
	else
		return &inputport_defaults[i].seq;
}

void update_analog_port(int port)
{
	struct InputPort *in;
	int current, delta, type, sensitivity, min, max, default_value;
	int axis, is_stick, is_gun, check_bounds;
	InputSeq* incseq;
	InputSeq* decseq;
	int keydelta;
	int player;

	/* get input definition */
	in = input_analog[port];

	/* if we're not cheating and this is a cheat-only port, bail */
	if (!options.cheat_input_ports && (in->type & IPF_CHEAT)) return;
	type=(in->type & ~IPF_MASK);

	decseq = input_port_seq(in);
	incseq = input_port_seq(in+1);

	keydelta = IP_GET_DELTA(in);

	switch (type)
	{
		case IPT_PADDLE:
			axis = X_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_PADDLE_V:
			axis = Y_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_DIAL:
			axis = X_AXIS; is_stick = 0; is_gun=0; check_bounds = 0; break;
		case IPT_DIAL_V:
			axis = Y_AXIS; is_stick = 0; is_gun=0; check_bounds = 0; break;
		case IPT_TRACKBALL_X:
			axis = X_AXIS; is_stick = 0; is_gun=0; check_bounds = 0; break;
		case IPT_TRACKBALL_Y:
			axis = Y_AXIS; is_stick = 0; is_gun=0; check_bounds = 0; break;
		case IPT_AD_STICK_X:
			axis = X_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_AD_STICK_Y:
			axis = Y_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_AD_STICK_Z:
			axis = Z_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_LIGHTGUN_X:
			axis = X_AXIS; is_stick = 1; is_gun=1; check_bounds = 1; break;
		case IPT_LIGHTGUN_Y:
			axis = Y_AXIS; is_stick = 1; is_gun=1; check_bounds = 1; break;
		case IPT_PEDAL:
			axis = PEDAL_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		case IPT_PEDAL2:
			axis = Z_AXIS; is_stick = 1; is_gun=0; check_bounds = 1; break;
		default:
			/* Use some defaults to prevent crash */
			axis = X_AXIS; is_stick = 0; is_gun=0; check_bounds = 0;
			log_cb(RETRO_LOG_ERROR, LOGPRE "Oops, polling non analog device in update_analog_port()????\n");
	}


	sensitivity = IP_GET_SENSITIVITY(in);
	min = IP_GET_MIN(in);
	max = IP_GET_MAX(in);
	default_value = in->default_value * 100 / sensitivity;
	/* extremes can be either signed or unsigned */
	if (min > max)
	{
		if (in->mask > 0xff) min = min - 0x10000;
		else min = min - 0x100;
	}

	input_analog_previous_value[port] = input_analog_current_value[port];

	/* if IPF_CENTER go back to the default position */
	/* sticks are handled later... */
	if ((in->type & IPF_CENTER) && (!is_stick))
		input_analog_current_value[port] = in->default_value * 100 / sensitivity;

	current = input_analog_current_value[port];

	delta = 0;

	player = IP_GET_PLAYER(in);

    /* if second player on a dial, and dial sharing turned on, use Y axis from player 1 */
    if (options.dial_share_xy && type == IPT_DIAL && player == 1)
    {
        axis = Y_AXIS;
        player = 0;
    }

	delta = mouse_delta_axis[player][axis];

	if (seq_pressed(decseq)) delta -= keydelta;

	if (type != IPT_PEDAL && type != IPT_PEDAL2)
	{
		if (seq_pressed(incseq)) delta += keydelta;
	}
	else
	{
		/* is this cheesy or what? */
		if (!delta && seq_get_1(incseq) == KEYCODE_Y) delta += keydelta;
		delta = -delta;
	}

	if (in->type & IPF_REVERSE) delta = -delta;

	if (is_gun)
	{
		/* The OSD lightgun call should return the delta from the middle of the screen
		when the gun is fired (not the absolute pixel value), and 0 when the gun is
		inactive.  We take advantage of this to provide support for other controllers
		in place of a physical lightgun.  When the OSD lightgun returns 0, then control
		passes through to the analog joystick, and mouse, in that order.  When the OSD
		lightgun returns a value it overrides both mouse & analog joystick.

		The value returned by the OSD layer should be -128 to 128, same as analog
		joysticks.

		There is an ugly hack to stop scaling of lightgun returned values.  It really
		needs rewritten...
		*/
		if (axis == X_AXIS) {
			if (lightgun_delta_axis[player][X_AXIS] || lightgun_delta_axis[player][Y_AXIS]) {
				analog_previous_axis[player][X_AXIS]=0;
				analog_current_axis[player][X_AXIS]=lightgun_delta_axis[player][X_AXIS];
				input_analog_scale[port]=0;
				sensitivity=100;
			}
		}
		else
		{
			if (lightgun_delta_axis[player][X_AXIS] || lightgun_delta_axis[player][Y_AXIS]) {
				analog_previous_axis[player][Y_AXIS]=0;
				analog_current_axis[player][Y_AXIS]=lightgun_delta_axis[player][Y_AXIS];
				input_analog_scale[port]=0;
				sensitivity=100;
			}
		}
	}

	if (is_stick)
	{
		int new, prev;

		/* center stick - core option to center digital joysticks */
		if (delta == 0 && options.digital_joy_centering && (type == IPT_AD_STICK_X || type == IPT_AD_STICK_Y))
			current = default_value;

		else if ((delta == 0) && (in->type & IPF_CENTER))
		{
			if (current > default_value)
			delta = -100 / sensitivity;
			if (current < default_value)
			delta = 100 / sensitivity;
		}

		/* An analog joystick which is not at zero position (or has just */
		/* moved there) takes precedence over all other computations */
		/* analog_x/y holds values from -128 to 128 (yes, 128, not 127) */

		new  = analog_current_axis[player][axis];
		prev = analog_previous_axis[player][axis];

		if ((new != 0) || (new-prev != 0))
		{
			delta=0;

			/* for pedals, need to change to possitive number */
			/* and, if needed, reverse pedal input */
			if (type == IPT_PEDAL || type == IPT_PEDAL2)
			{
				new  = -new;
				prev = -prev;
				if (in->type & IPF_REVERSE)		/* a reversed pedal is diff than normal reverse */
				{								/* 128 = no gas, 0 = all gas */
					new  = 128-new;				/* the default "new=-new" doesn't handle this */
					prev = 128-prev;
				}
			}
			else if (in->type & IPF_REVERSE)
			{
				new  = -new;
				prev = -prev;
			}

			/* apply sensitivity using a logarithmic scale */
			if (in->mask > 0xff)
			{
				if (new > 0)
				{
					current = (pow(new / 32768.0, 100.0 / sensitivity) * (max-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
				else
				{
					current = (pow(-new / 32768.0, 100.0 / sensitivity) * (min-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
			}
			else
			{
				if (new > 0)
				{
					current = (pow(new / 128.0, 100.0 / sensitivity) * (max-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
				else
				{
					current = (pow(-new / 128.0, 100.0 / sensitivity) * (min-in->default_value)
							+ in->default_value) * 100 / sensitivity;
				}
			}
		}
	}

	current += delta;

	if (check_bounds)
	{
		int temp;

		if (current >= 0)
			temp = (current * sensitivity + 50) / 100;
		else
			temp = (-current * sensitivity + 50) / -100;

		if (temp < min)
		{
			if (min >= 0)
				current = (min * 100 + sensitivity/2) / sensitivity;
			else
				current = (-min * 100 + sensitivity/2) / -sensitivity;
		}
		if (temp > max)
		{
			if (max >= 0)
				current = (max * 100 + sensitivity/2) / sensitivity;
			else
				current = (-max * 100 + sensitivity/2) / -sensitivity;
		}
	}

	input_analog_current_value[port] = current;
}

static void scale_analog_port(int port)
{
	struct InputPort *in;
	int delta,current,sensitivity;

profiler_mark(PROFILER_INPUT);
	in = input_analog[port];
	sensitivity = IP_GET_SENSITIVITY(in);

	/* apply scaling fairly in both positive and negative directions */
	delta = input_analog_current_value[port] - input_analog_previous_value[port];
	if (delta >= 0)
		delta = cpu_scalebyfcount(delta);
	else
		delta = -cpu_scalebyfcount(-delta);

	current = input_analog_previous_value[port] + delta;

	/* An ugly hack to remove scaling on lightgun ports */
	if (input_analog_scale[port]) {
		/* apply scaling fairly in both positive and negative directions */
		if (current >= 0)
			current = (current * sensitivity + 50) / 100;
		else
			current = (-current * sensitivity + 50) / -100;
	}

	input_port_value[port] &= ~in->mask;
	input_port_value[port] |= current & in->mask;

	if (playback)
		readword(playback,&input_port_value[port]);
	if (record)
		writeword(record,input_port_value[port]);
	profiler_mark(PROFILER_END);
}

#define MAX_JOYSTICKS 3
#define MAX_PLAYERS 8
static int mJoyCurrent[MAX_JOYSTICKS*MAX_PLAYERS];
static int mJoyPrevious[MAX_JOYSTICKS*MAX_PLAYERS];
static int mJoy4Way[MAX_JOYSTICKS*MAX_PLAYERS];
/*
The above "Joy" states contain packed bits:
	0001	up
	0010	down
	0100	left
	1000	right
*/

static void
ScanJoysticks( struct InputPort *in )
{
	int i;
	int port = 0;

	/* Save old Joystick state. */
	memcpy( mJoyPrevious, mJoyCurrent, sizeof(mJoyPrevious) );

	/* Initialize bits of mJoyCurrent to zero. */
	memset( mJoyCurrent, 0, sizeof(mJoyCurrent) );

	/* Now iterate over the input port structure to populate mJoyCurrent. */
	while( in->type != IPT_END && port < MAX_INPUT_PORTS )
	{
		while (in->type != IPT_END && in->type != IPT_PORT)
		{
			if ((in->type & ~IPF_MASK) >= IPT_JOYSTICK_UP &&
				(in->type & ~IPF_MASK) <= IPT_JOYSTICKLEFT_RIGHT)
			{
				InputSeq* seq;
				seq = input_port_seq(in);
				if( seq_pressed(seq) )
				{
					int joynum,joydir,player;
					player = IP_GET_PLAYER(in);

					joynum = player * MAX_JOYSTICKS +
							 ((in->type & ~IPF_MASK) - IPT_JOYSTICK_UP) / 4;
					joydir = ((in->type & ~IPF_MASK) - IPT_JOYSTICK_UP) % 4;

					mJoyCurrent[joynum] |= 1<<joydir;
				}
			}
			in++;
		}
		port++;
		if (in->type == IPT_PORT) in++;
	}

	/* Process the joystick states, to filter out illegal combinations of switches. */
	for( i=0; i<MAX_JOYSTICKS*MAX_PLAYERS; i++ )
	{
		if( (mJoyCurrent[i]&0x3)==0x3 ) /* both up and down are pressed */
		{
			mJoyCurrent[i]&=0xc; /* clear up and down */
		}
		if( (mJoyCurrent[i]&0xc)==0xc ) /* both left and right are pressed */
		{
			mJoyCurrent[i]&=0x3; /* clear left and right */
		}

		/* Only update mJoy4Way if the joystick has moved. */
		if( mJoyCurrent[i]!=mJoyPrevious[i] && !options.restrict_4_way)
		{
			mJoy4Way[i] = mJoyCurrent[i];

			  if( (mJoy4Way[i] & 0x3) && (mJoy4Way[i] & 0xc) )
			  {
			  	  /* If joystick is pointing at a diagonal, acknowledge that the player moved
				   * the joystick by favoring a direction change.  This minimizes frustration
				   * when using a keyboard for input, and maximizes responsiveness.
				   *
				   * For example, if you are holding "left" then switch to "up" (where both left
				   * and up are briefly pressed at the same time), we'll transition immediately
				   * to "up."
				   *
				   * Under the old "sticky" key implentation, "up" wouldn't be triggered until
				   * left was released.
				   *
				   * Zero any switches that didn't change from the previous to current state.
				   */
				  mJoy4Way[i] ^= (mJoy4Way[i] & mJoyPrevious[i]);
			  }

			  if( (mJoy4Way[i] & 0x3) && (mJoy4Way[i] & 0xc) )
			  {
			  	  /* If we are still pointing at a diagonal, we are in an indeterminant state.
				   *
				   * This could happen if the player moved the joystick from the idle position directly
				   * to a diagonal, or from one diagonal directly to an extreme diagonal.
				   *
				   * The chances of this happening with a keyboard are slim, but we still need to
				   * constrain this case.
				   *
				   * For now, just resolve randomly.
				   */
				  if( rand()&1 )
				  {
				  	  mJoy4Way[i] &= 0x3; /* eliminate horizontal component */
				  }
				  else
				  {
				 	  mJoy4Way[i] &= 0xc; /* eliminate vertical component */
				  }
			  }

		}
    else if (options.restrict_4_way) //start use alternative code
    {
      if(options.content_flags[CONTENT_ROTATE_JOY_45])
      {
        if  ( (mJoyCurrent[i]) && (mJoyCurrent[i] !=1) &&
              (mJoyCurrent[i] !=2) && (mJoyCurrent[i] !=4) &&
              (mJoyCurrent[i] !=8) )
        {
          if      (mJoyCurrent[i] == 9)  mJoy4Way[i]=1;
          else if (mJoyCurrent[i] == 6)  mJoy4Way[i]=2;
          else if (mJoyCurrent[i] == 5)  mJoy4Way[i]=4;
          else if (mJoyCurrent[i] == 10) mJoy4Way[i]=8;
        }
        else if (mJoy4Way[i])
          mJoy4Way[i]=0;
      }
      else // just a regular 4-way - last press no code needed just ignore diagonals and no movement
      {
        if  ( (mJoyCurrent[i]) && (mJoyCurrent[i] !=5) && (mJoyCurrent[i] !=6)
          &&  (mJoyCurrent[i] !=9) && (mJoyCurrent[i] !=10) )
        {
          mJoy4Way[i] = mJoyCurrent[i];
        }
      }
		}
	}
} /* ScanJoysticks */

void update_input_ports(void)
{
	int port,ib;
	struct InputPort *in;

#define MAX_INPUT_BITS 1024
	static int impulsecount[MAX_INPUT_BITS];
	static int waspressed[MAX_INPUT_BITS];
	static int pbwaspressed[MAX_INPUT_BITS];

profiler_mark(PROFILER_INPUT);

	/* clear all the values before proceeding */
	for (port = 0;port < MAX_INPUT_PORTS;port++)
	{
		input_port_value[port] = 0;
		input_vblank[port] = 0;
		input_analog[port] = 0;
	}

	in = Machine->input_ports;
	if (in->type == IPT_END) return; 	/* nothing to do */

	/* make sure the InputPort definition is correct */
	if (in->type != IPT_PORT)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in InputPort definition: expecting PORT_START\n");
		return;
	}
	else
	{
		in++;
	}

	ScanJoysticks( in ); /* populates mJoyCurrent[] */

	/* scan all the input ports */
	port = 0;
	ib = 0;
	while (in->type != IPT_END && port < MAX_INPUT_PORTS)
	{
		struct InputPort *start;
		/* first of all, scan the whole input port definition and build the */
		/* default value. I must do it before checking for input because otherwise */
		/* multiple keys associated with the same input bit wouldn't work (the bit */
		/* would be reset to its default value by the second entry, regardless if */
		/* the key associated with the first entry was pressed) */
		start = in;
		while (in->type != IPT_END && in->type != IPT_PORT)
		{
			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING &&	/* skip dipswitch definitions */
				(in->type & ~IPF_MASK) != IPT_EXTENSION)			/* skip analog extension fields */
			{
				input_port_value[port] =
						(input_port_value[port] & ~in->mask) | (in->default_value & in->mask);
			}

			in++;
		}

		/* now get back to the beginning of the input port and check the input bits. */
		for (in = start;
			 in->type != IPT_END && in->type != IPT_PORT;
			 in++, ib++)
		{
			if ((in->type & ~IPF_MASK) != IPT_DIPSWITCH_SETTING &&	/* skip dipswitch definitions */
					(in->type & ~IPF_MASK) != IPT_EXTENSION)		/* skip analog extension fields */
			{
				if ((in->type & ~IPF_MASK) == IPT_VBLANK)
				{
					input_vblank[port] ^= in->mask;
					input_port_value[port] ^= in->mask;
					if (Machine->drv->vblank_duration == 0)
						log_cb(RETRO_LOG_ERROR, LOGPRE "Warning: you are using IPT_VBLANK with vblank_duration = 0. You need to increase vblank_duration for IPT_VBLANK to work.\n");
				}
				/* If it's an analog control, handle it appropriately */
				else if (((in->type & ~IPF_MASK) > IPT_ANALOG_START)
					  && ((in->type & ~IPF_MASK) < IPT_ANALOG_END  )) /* LBO 120897 */
				{
					input_analog[port]=in;
					/* reset the analog port on first access */
					if (input_analog_init[port])
					{
						input_analog_init[port] = 0;
						input_analog_scale[port] = 1;
						input_analog_current_value[port] = input_analog_previous_value[port]
							= in->default_value * 100 / IP_GET_SENSITIVITY(in);
					}
				}
				else
				{
					InputSeq* seq;
					seq = input_port_seq(in);
#ifdef AUTO_FIRE // AUTO_FIRE					
					if (auto_pressed(seq, in->type, ib))  
#else					
					if (seq_pressed(seq))
#endif
					{
						/* skip if coin input and it's locked out */
						if ((in->type & ~IPF_MASK) >= IPT_COIN1 &&
							(in->type & ~IPF_MASK) <= IPT_COIN4 &&
							coinlockedout[(in->type & ~IPF_MASK) - IPT_COIN1])
						{
							continue;
						}
						if ((in->type & ~IPF_MASK) >= IPT_COIN5 &&
							(in->type & ~IPF_MASK) <= IPT_COIN8 &&
							coinlockedout[(in->type & ~IPF_MASK) - IPT_COIN5 + 4])
						{
							continue;
						}

						/* if IPF_RESET set, reset the first CPU */
						if ((in->type & IPF_RESETCPU) && waspressed[ib] == 0 && !playback)
						{
							cpu_set_reset_line(0,PULSE_LINE);
						}

						if (in->type & IPF_IMPULSE)
						{
							if (IP_GET_IMPULSE(in) == 0)
								log_cb(RETRO_LOG_ERROR, LOGPRE "error in input port definition: IPF_IMPULSE with length = 0\n");
							if (waspressed[ib] == 0)
								impulsecount[ib] = IP_GET_IMPULSE(in);
								/* the input bit will be toggled later */
						}
						else if (in->type & IPF_TOGGLE)
						{
							if (waspressed[ib] == 0)
							{
								in->default_value ^= in->mask;
								input_port_value[port] ^= in->mask;
							}
						}
						else if ((in->type & ~IPF_MASK) >= IPT_JOYSTICK_UP &&
								(in->type & ~IPF_MASK) <= IPT_JOYSTICKLEFT_RIGHT)
						{
							int joynum,joydir,mask,player;

							player = IP_GET_PLAYER(in);
							joynum = player * MAX_JOYSTICKS +
									((in->type & ~IPF_MASK) - IPT_JOYSTICK_UP) / 4;

							joydir = ((in->type & ~IPF_MASK) - IPT_JOYSTICK_UP) % 4;

							mask = in->mask;

							if( in->type & IPF_4WAY )
							{
								/* apply 4-way joystick constraint */
								if( ((mJoy4Way[joynum]>>joydir)&1) == 0 )
								{
									mask = 0;
								}
							}
							else
							{
								/* filter up+down and left+right */
								if( ((mJoyCurrent[joynum]>>joydir)&1) == 0 )
								{
									mask = 0;
								}
							}

							input_port_value[port] ^= mask;
						} /* joystick */
						else
						{
							input_port_value[port] ^= in->mask;
						}
						waspressed[ib] = 1;
					}
					else
						waspressed[ib] = 0;

					if ((in->type & IPF_IMPULSE) && impulsecount[ib] > 0)
					{
						impulsecount[ib]--;
						waspressed[ib] = 1;
						input_port_value[port] ^= in->mask;
					}
				}
			}
		}

		port++;
		if (in->type == IPT_PORT) in++;
	}

	if (playback)
	{
		int i;

		ib=0;
		in = Machine->input_ports;
		in++;
		for (i = 0; i < MAX_INPUT_PORTS; i ++)
		{
			readword(playback,&input_port_value[i]);

			/* check if the input port includes an IPF_RESETCPU bit
			   and reset the CPU on first "press", no need to check
			   the impulse count as this was done during recording */
			for (; in->type != IPT_END && in->type != IPT_PORT; in++, ib++)
			{
				if (in->type & IPF_RESETCPU)
				{
					if((input_port_value[i] ^ in->default_value) & in->mask)
					{
						if (pbwaspressed[ib] == 0)
							cpu_set_reset_line(0,PULSE_LINE);
						pbwaspressed[ib] = 1;
					}
					else
						pbwaspressed[ib] = 0;
				}
			}
			if (in->type == IPT_PORT) in++;
		}
	}

	if (record)
	{
		int i;

		for (i = 0; i < MAX_INPUT_PORTS; i ++)
			writeword(record,input_port_value[i]);
	}

	profiler_mark(PROFILER_END);
}



/* used the the CPU interface to notify that VBlank has ended, so we can update */
/* IPT_VBLANK input ports. */
void inputport_vblank_end(void)
{
	int port;
	int i;


profiler_mark(PROFILER_INPUT);
	for (port = 0;port < MAX_INPUT_PORTS;port++)
	{
		if (input_vblank[port])
		{
			input_port_value[port] ^= input_vblank[port];
			input_vblank[port] = 0;
		}
	}

	/* update the analog devices */
	for (i = 0;i < MAX_PLAYER_COUNT;i++)
	{
		/* update the analog joystick position */
		int a;
		for (a=0; a<MAX_ANALOG_AXES ; a++)
		{
			analog_previous_axis[i][a] = analog_current_axis[i][a];
		}
		osd_analogjoy_read (i, analog_current_axis[i], analogjoy_input[i]);

		/* update mouse/trackball position */
		osd_xy_device_read (i, &(mouse_delta_axis[i])[X_AXIS], &(mouse_delta_axis[i])[Y_AXIS], "relative");

		/* update lightgun position, if any */
		osd_xy_device_read (i, &(lightgun_delta_axis[i])[X_AXIS], &(lightgun_delta_axis[i])[Y_AXIS], "absolute");
	}

	for (i = 0;i < MAX_INPUT_PORTS;i++)
	{
		struct InputPort *in;

		in=input_analog[i];
		if (in)
		{
			update_analog_port(i);
		}
	}
profiler_mark(PROFILER_END);
}



int readinputport(int port)
{
	struct InputPort *in;

	/* Update analog ports on demand */
	in=input_analog[port];
	if (in)
	{
		scale_analog_port(port);
	}

	return input_port_value[port];
}

READ_HANDLER( input_port_0_r ) { return readinputport(0); }
READ_HANDLER( input_port_1_r ) { return readinputport(1); }
READ_HANDLER( input_port_2_r ) { return readinputport(2); }
READ_HANDLER( input_port_3_r ) { return readinputport(3); }
READ_HANDLER( input_port_4_r ) { return readinputport(4); }
READ_HANDLER( input_port_5_r ) { return readinputport(5); }
READ_HANDLER( input_port_6_r ) { return readinputport(6); }
READ_HANDLER( input_port_7_r ) { return readinputport(7); }
READ_HANDLER( input_port_8_r ) { return readinputport(8); }
READ_HANDLER( input_port_9_r ) { return readinputport(9); }
READ_HANDLER( input_port_10_r ) { return readinputport(10); }
READ_HANDLER( input_port_11_r ) { return readinputport(11); }
READ_HANDLER( input_port_12_r ) { return readinputport(12); }
READ_HANDLER( input_port_13_r ) { return readinputport(13); }
READ_HANDLER( input_port_14_r ) { return readinputport(14); }
READ_HANDLER( input_port_15_r ) { return readinputport(15); }
READ_HANDLER( input_port_16_r ) { return readinputport(16); }
READ_HANDLER( input_port_17_r ) { return readinputport(17); }
READ_HANDLER( input_port_18_r ) { return readinputport(18); }
READ_HANDLER( input_port_19_r ) { return readinputport(19); }
READ_HANDLER( input_port_20_r ) { return readinputport(20); }
READ_HANDLER( input_port_21_r ) { return readinputport(21); }
READ_HANDLER( input_port_22_r ) { return readinputport(22); }
READ_HANDLER( input_port_23_r ) { return readinputport(23); }
READ_HANDLER( input_port_24_r ) { return readinputport(24); }
READ_HANDLER( input_port_25_r ) { return readinputport(25); }
READ_HANDLER( input_port_26_r ) { return readinputport(26); }
READ_HANDLER( input_port_27_r ) { return readinputport(27); }
READ_HANDLER( input_port_28_r ) { return readinputport(28); }
READ_HANDLER( input_port_29_r ) { return readinputport(29); }

READ16_HANDLER( input_port_0_word_r ) { return readinputport(0); }
READ16_HANDLER( input_port_1_word_r ) { return readinputport(1); }
READ16_HANDLER( input_port_2_word_r ) { return readinputport(2); }
READ16_HANDLER( input_port_3_word_r ) { return readinputport(3); }
READ16_HANDLER( input_port_4_word_r ) { return readinputport(4); }
READ16_HANDLER( input_port_5_word_r ) { return readinputport(5); }
READ16_HANDLER( input_port_6_word_r ) { return readinputport(6); }
READ16_HANDLER( input_port_7_word_r ) { return readinputport(7); }
READ16_HANDLER( input_port_8_word_r ) { return readinputport(8); }
READ16_HANDLER( input_port_9_word_r ) { return readinputport(9); }
READ16_HANDLER( input_port_10_word_r ) { return readinputport(10); }
READ16_HANDLER( input_port_11_word_r ) { return readinputport(11); }
READ16_HANDLER( input_port_12_word_r ) { return readinputport(12); }
READ16_HANDLER( input_port_13_word_r ) { return readinputport(13); }
READ16_HANDLER( input_port_14_word_r ) { return readinputport(14); }
READ16_HANDLER( input_port_15_word_r ) { return readinputport(15); }
READ16_HANDLER( input_port_16_word_r ) { return readinputport(16); }
READ16_HANDLER( input_port_17_word_r ) { return readinputport(17); }
READ16_HANDLER( input_port_18_word_r ) { return readinputport(18); }
READ16_HANDLER( input_port_19_word_r ) { return readinputport(19); }
READ16_HANDLER( input_port_20_word_r ) { return readinputport(20); }
READ16_HANDLER( input_port_21_word_r ) { return readinputport(21); }
READ16_HANDLER( input_port_22_word_r ) { return readinputport(22); }
READ16_HANDLER( input_port_23_word_r ) { return readinputport(23); }
READ16_HANDLER( input_port_24_word_r ) { return readinputport(24); }
READ16_HANDLER( input_port_25_word_r ) { return readinputport(25); }
READ16_HANDLER( input_port_26_word_r ) { return readinputport(26); }
READ16_HANDLER( input_port_27_word_r ) { return readinputport(27); }
READ16_HANDLER( input_port_28_word_r ) { return readinputport(28); }
READ16_HANDLER( input_port_29_word_r ) { return readinputport(29); }


/***************************************************************************/
/* InputPort conversion */

static unsigned input_port_count(const struct InputPortTiny *src)
{
	unsigned total;

	total = 0;
	while (src->type != IPT_END)
	{
		int type = src->type & ~IPF_MASK;
		if (type > IPT_ANALOG_START && type < IPT_ANALOG_END)
			total += 2;
		else if (type != IPT_EXTENSION)
			++total;
		++src;
	}

	++total; /* for IPT_END */

	return total;
}

struct InputPort* input_port_allocate(const struct InputPortTiny *src)
{
	struct InputPort* dst;
	struct InputPort* base;
	unsigned total;

#ifdef NEOGEO_HACKS
	int remove_neogeo_territory = 0;
	int remove_neogeo_arcade = 0;

	if (!strcasecmp(Machine->gamedrv->source_file+12, "neogeo.c"))
	{
		int system_bios = determine_bios_rom(Machine->gamedrv->bios);

		/* first mark all items to disable */
		remove_neogeo_territory = 1;
		remove_neogeo_arcade = 1;

		switch (system_bios)
		{
		/* Europe, 1 Slot: enable arcade/console and territory */
		case 0:
			remove_neogeo_arcade = 0;
			remove_neogeo_territory = 0;
			break;

		/* Debug: enable territory */
		case 11:
			remove_neogeo_territory = 0;
		}

		/* Special BIOS: enable arcade/console and territory */
		if (!strcmp(Machine->gamedrv->name,"irrmaze"))
		{
			remove_neogeo_territory = 0;
			remove_neogeo_arcade = 0;
		}
	}
#endif /* USE_NEOGEO_HACKS */

	total = input_port_count(src);

	base = (struct InputPort*)malloc(total * sizeof(struct InputPort));
	dst = base;

	while (src->type != IPT_END)
	{
		int type = src->type & ~IPF_MASK;
		const struct InputPortTiny *ext;
		const struct InputPortTiny *src_end;
		InputCode seq_default;

#ifdef NEOGEO_HACKS // NEOGEO_HACKS
		while (type == IPT_DIPSWITCH_NAME)
		{
			if ((remove_neogeo_territory && !strcmp(src->name, "Territory")) ||
			    (remove_neogeo_arcade && (!strcmp(src->name, "Machine Mode") ||
			                              !strcmp(src->name, "Game Slots"))))
			{
				do {
					type = (++src)->type & ~IPF_MASK;
				} while (type == IPT_DIPSWITCH_SETTING);
			}
			else
				break;
		}
#endif /* USE_NEOGEO_HACKS */

		if (type > IPT_ANALOG_START && type < IPT_ANALOG_END)
			src_end = src + 2;
		else
			src_end = src + 1;

		switch (type)
		{
			case IPT_END :
			case IPT_PORT :
			case IPT_DIPSWITCH_NAME :
			case IPT_DIPSWITCH_SETTING :
				seq_default = CODE_NONE;
			break;
			default:
				seq_default = CODE_DEFAULT;
				break;
		}

		ext = src_end;
		while (src != src_end)
		{
			dst->type = src->type;
			dst->mask = src->mask;
			dst->default_value = src->default_value;
			dst->name = src->name;
			
			/* PORT_BITX declarations that specify JOYCODE_a_BUTTONb for their default code */
			/* will also get JOYCODE_MOUSE_a_BUTTONb or'd in. */
  			if (ext->type == IPT_EXTENSION)
  			{
				InputCode or1 =	IP_GET_CODE_OR1(ext);
				InputCode or2 =	IP_GET_CODE_OR2(ext);
				InputCode or3;

				#define MATCH_ANALOG_JOYCODE(PLAYER_NUM) \
				case JOYCODE_##PLAYER_NUM##_BUTTON1:    or3 = JOYCODE_MOUSE_##PLAYER_NUM##_BUTTON1;      break; \
				case JOYCODE_##PLAYER_NUM##_BUTTON2:    or3 = JOYCODE_MOUSE_##PLAYER_NUM##_BUTTON2;      break; \
				case JOYCODE_##PLAYER_NUM##_BUTTON3:    or3 = JOYCODE_MOUSE_##PLAYER_NUM##_BUTTON3;      break;

				switch(or2)
				{
					MATCH_ANALOG_JOYCODE(1)
					MATCH_ANALOG_JOYCODE(2)
					MATCH_ANALOG_JOYCODE(3)
					MATCH_ANALOG_JOYCODE(4)
					MATCH_ANALOG_JOYCODE(5)
					MATCH_ANALOG_JOYCODE(6)
					MATCH_ANALOG_JOYCODE(7)
					MATCH_ANALOG_JOYCODE(8)
					default:
						or3 = CODE_NONE;
						break;
				}

				if (or1 < __code_max)
				{
					if (or3 < __code_max)
						seq_set_5(&dst->seq, or1, CODE_OR, or2, CODE_OR, or3);
					else if (or2 < __code_max)
						seq_set_3(&dst->seq, or1, CODE_OR, or2);
					else
						seq_set_1(&dst->seq, or1);
				} else {
					if (or1 == CODE_NONE)
						seq_set_1(&dst->seq, or2);
					else
						seq_set_1(&dst->seq, or1);
				}

  				++ext;
  			} else {
				seq_set_1(&dst->seq,seq_default);
  			}

			++src;
			++dst;
		}

		src = ext;
	}

	dst->type = IPT_END;

	return base;
}

void input_port_free(struct InputPort* dst)
{
	free(dst);
}

void init_analog_seq()
{
	struct InputPort *in;
	int player, axis;

/* init analogjoy_input array */
	for (player=0; player<MAX_PLAYER_COUNT; player++)
	{
		for (axis=0; axis<MAX_ANALOG_AXES; axis++)
		{
			analogjoy_input[player][axis] = CODE_NONE;
		}
	}

	in = Machine->input_ports;
	if (in->type == IPT_END) return; 	/* nothing to do */

	/* make sure the InputPort definition is correct */
	if (in->type != IPT_PORT)
	{
		log_cb(RETRO_LOG_ERROR, LOGPRE "Error in InputPort definition: expecting PORT_START\n");
		return;
	}
	else
	{
		in++;
	}

	while (in->type != IPT_END)
	{
		if (in->type != IPT_PORT && ((in->type & ~IPF_MASK) > IPT_ANALOG_START)
			&& ((in->type & ~IPF_MASK) < IPT_ANALOG_END))
		{
			int j, invert;
			InputSeq *seq;
			InputCode analog_seq;

			seq = input_port_seq(in);
			invert = 0;
			analog_seq = CODE_NONE;

			for(j=0; j<SEQ_MAX && analog_seq == CODE_NONE; ++j)
			{
				switch ((*seq)[j])
				{
					case CODE_NONE :
						continue;
					case CODE_NOT :
						invert = !invert;
						break;
					case CODE_OR :
						invert = 0;
						break;
					default:
						if (!invert && is_joystick_axis_code((*seq)[j]) )
						{
							analog_seq = return_os_joycode((*seq)[j]);
						}
						invert = 0;
						break;
				}
			}
			if (analog_seq != CODE_NONE)
			{
				player = IP_GET_PLAYER(in);

				switch (in->type & ~IPF_MASK)
				{
					case IPT_DIAL:
					case IPT_PADDLE:
					case IPT_TRACKBALL_X:
					case IPT_LIGHTGUN_X:
					case IPT_AD_STICK_X:
						axis = X_AXIS;
						break;
					case IPT_DIAL_V:
					case IPT_PADDLE_V:
					case IPT_TRACKBALL_Y:
					case IPT_LIGHTGUN_Y:
					case IPT_AD_STICK_Y:
						axis = Y_AXIS;
						break;
					case IPT_AD_STICK_Z:
					case IPT_PEDAL2:
						axis = Z_AXIS;
						break;
					case IPT_PEDAL:
						axis = PEDAL_AXIS;
						break;
					default:
						axis = 0;
						break;
				}

				analogjoy_input[player][axis] = analog_seq;
			}
		}

		in++;
	}

	return;
}

#ifdef AUTO_FIRE // AUTO_FIRE
UINT32 toggle_autofire(void)
{
	autofire_enable = autofire_enable ? 0 : 1;

	return autofire_enable;
}

static int auto_pressed(InputSeq *seq, UINT32 type, int bit)
{
	int pressed = seq_pressed(seq);

	if (pressed)
	{
		if ((type & IPF_AUTOFIRE_ON) || ((type & IPF_AUTOFIRE_TOGGLE) && autofire_enable))
		{
			if (autopressed[bit] >= options.autofiredelay)
			{
				pressed = 0;
				autopressed[bit] = 0;
			}
			else
				autopressed[bit]++;
		}
	}

	return pressed;
}
#endif