// Follow Defined Game Command Font Converting Conditions
#define COMMAND_DEFAULT_TEXT	'_'

#define COMMAND_LOGO_MARK		'#'
#define COMMAND_EXPAND_TEXT		'^'
#define COMMAND_CONVERT_TEXT	'@'

// Follow Varialbe Defined Arraies for Game Command Tag
struct fix_command_t
{
	const unsigned char *org_name;
	const int            fix_name;
};

#ifdef COMMAND_CONVERT_TEXT
struct fix_strings_t
{
	const unsigned char *org_name;
	const unsigned char *fix_name;
};
#endif /* COMMAND_CONVERT_TEXT */

#ifdef COMMAND_LOGO_MARK
static struct fix_command_t logo[] =
{
	{ "#엠박스#", 188 },
	{ "#공략집#", 192 },
	{ "#마메32#", 196 },
	{ "#커맨드#", 200 },
	{ 0, 0 }
};

static struct fix_command_t deco[] =
{
	{ "#mbox#", 204 },
	{ "#zzir#", 207 },
	{ 0, 0 }
};
#endif /* COMMAND_LOGO_MARK */

static struct fix_command_t default_text[] =
{
	// NeoGeo Buttons A~B
	{ "_A", 1 },	// BTN_A
	{ "_B", 2 },	// BTN_B
	{ "_C", 3 },	// BTN_C
	{ "_D", 4 },	// BTN_D
	// Capcom Buttons 1~10
	{ "_a", 27 },	// BTN_1
	{ "_b", 28 },	// BTN_2
	{ "_c", 29 },	// BTN_3
	{ "_d", 30 },	// BTN_4
	{ "_e", 31 },	// BTN_5
	{ "_f", 32 },	// BTN_6
	{ "_g", 33 },	// BTN_7
	{ "_h", 34 },	// BTN_8
	{ "_i", 35 },	// BTN_9
	{ "_j", 36 },	// BTN_10
	// Directions of Arrow
	{ "_+", 39 },	// BTN_+
	{ "_.", 40 },	// DIR_...
	{ "_1", 41 },	// DIR_1
	{ "_2", 42 },	// DIR_2
	{ "_3", 43 },	// DIR_3
	{ "_4", 44 },	// DIR_4
	{ "_5", 45 },	// Joystick Ball
	{ "_6", 46 },	// DIR_6
	{ "_7", 47 },	// DIR_7
	{ "_8", 48 },	// DIR_8
	{ "_9", 49 },	// DIR_9
	{ "_N", 50 },	// DIR_N
	{ "_>", 51 },	// Continue Arrow
	// Special Buttons
	{ "_S", 52 },	// BTN_START
	{ "_T", 53 },	// BTN_SELETE
	{ "_P", 54 },	// BTN_Punch
	{ "_K", 55 },	// BTN_Kick
	{ "_G", 56 },	// BTN_Guard
	{ 0, 0 }
};
static struct fix_command_t expand_text[] =
{
	// Etcs of Command Moves
	{ "^N", 14 },	// BTN_N
	{ "^Z", 26 },	// BTN_Z
	{ "^0", 82 },	// Non Player Lever
	// Multiple Punches & Kicks
	{ "^E", 59 },	// Light_Punch
	{ "^F", 60 },	// Middle_Punch
	{ "^G", 61 },	// Strong_Punch
	{ "^H", 62 },	// Light_Kick
	{ "^I", 63 },	// Middle_Kick
	{ "^J", 64 },	// Strong_Kick
	{ "^T", 65 },	// 3 Kick
	{ "^U", 66 },	// 3 Punch
	{ "^V", 67 },	// 2 Kick
	{ "^W", 68 },	// 2 Pick
	{ "^X", 69 },	// Tap
	// Composition of Arrow Directions
	{ "^k",  94 },	// Half Circle Back
	{ "^l",  95 },	// Half Circle Front Up
	{ "^m",  96 },	// Half Circle Front
	{ "^n",  97 },	// Half Circle Back Up
	{ "^o",  98 },	// 1/4 Cir For 2 Down
	{ "^p",  99 },	// 1/4 Cir Down 2 Back
	{ "^q", 100 },	// 1/4 Cir Back 2 Up
	{ "^r", 101 },	// 1/4 Cir Up 2 For
	{ "^s", 102 },	// 1/4 Cir Back 2 Down
	{ "^t", 103 },	// 1/4 Cir Down 2 For
	{ "^u", 104 },	// 1/4 Cir For 2 Up
	{ "^v", 105 },	// 1/4 Cir Up 2 Back
	{ "^w", 106 },	// Full Clock Forward
	{ "^x", 107 },	// Full Clock Back
	{ "^y", 108 },	// Full Count Forward
	{ "^z", 109 },	// Full Count Back
	{ "^L", 110 },	// 2x Forward
	{ "^M", 111 },	// 2x Back
	{ "^Q", 115 },	// Dragon Screw Forward
	{ "^R", 116 },	// Dragon Screw Back
	// Condition of Positions
	{ "^j", 112 },	// Jump
	{ "^h", 113 },	// Hold
	{ "^=", 117 },	// Air
	{ "^-", 118 },	// Squatting
	{ "^]", 119 },	// Close
	{ "^[", 120 },	// Away
	{ "^~", 121 },	// Charge
	{ "^*", 122 },	// Serious Tap
	{ "^?", 123 },	// Any Button
	{ 0, 0 }
};

static struct fix_strings_t convert_text[] =
{
	{ "@hcb", "^k" },	// Half Circle Back
	{ "@huf", "^l" },	// Half Circle Front Up
	{ "@hcf", "^m" },	// Half Circle Front
	{ "@hub", "^n" },	// Half Circle Back Up
	{ "@qfd", "^o" },	// 1/4 Cir For 2 Down
	{ "@qdb", "^p" },	// 1/4 Cir Down 2 Back
	{ "@qbu", "^q" },	// 1/4 Cir Back 2 Up
	{ "@quf", "^r" },	// 1/4 Cir Up 2 For
	{ "@qbd", "^s" },	// 1/4 Cir Back 2 Down
	{ "@qdf", "^t" },	// 1/4 Cir Down 2 For
	{ "@qfu", "^u" },	// 1/4 Cir For 2 Up
	{ "@qub", "^v" },	// 1/4 Cir Up 2 Back
	{ "@fdf", "^w" },	// Full Clock Forward
	{ "@fub", "^x" },	// Full Clock Back
	{ "@fuf", "^y" },	// Full Count Forward
	{ "@fdb", "^z" },	// Full Count Back
	{ "@xff", "^L" },	// 2x Forward
	{ "@xbb", "^M" },	// 2x Back
	{ "@dsf", "^Q" },	// Dragon Punch Forward
	{ "@dsb", "^R" },	// Dragon Punch Back
	{ "@air", "^=" },	// Air
	{ "@sit", "^-" },	// Squatting, Sit Down
	{ "@hot", "^]" },	// Close, getting hot
	{ "@far", "^[" },	// Away, far away
	{ "@any", "^?" },	// Any Button
	{ "@dir", "^0" },	// Non Player Lever, Any Directions
	{ 0, 0 }
};
