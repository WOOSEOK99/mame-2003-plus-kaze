/*********************************************************************

  ui_text.c

  Functions used to retrieve text used by MAME, to aid in
  translation.

*********************************************************************/

#include "driver.h"
#include "ui_text.h"


struct lang_struct lang;

/* All entries in this table must match the enum ordering in "ui_text.h" */
static const char *mame_default_text[] =
{
	"MAME",

	/* copyright stuff */
	"소유하지 않는 ROM을 에물레이터로 사용하는 것은, 저작권법에 따라 금지하고 있습니다.",

	/* misc stuff */
	"메인메뉴로 돌아가기",
	"이전 메뉴로 돌아가기",
	"아무 키나 누르세요",
	"On",
	"Off",
	"없음",
	"OK",
	"무효",
	"(없음)",
	"CPU",
	"주소",
	"값",
	"사운드 코어",
	"사운드용",
	"스테레오",
	"벡터 게임",
	"화면 해상도",
	"텍스트",
	"음량",
	"상태",
	"모든 채널",
	"밝기",
	"감마",
	"벡터 플릭커",
	"벡터 명도",
	"오버클럭",
	"모든 CPU",
	" 정보가 없습니다",

	/* special characters */
	"\x11",
	"\x10",
	"\x18",
	"\x19",
	"\x1a",
	"\x1b",

	/* known problems */
	"이 게임은 다음과 같은 문제가 있습니다:",
	"색의 재현이 불완전합니다.",
	"색이 재현이 완전히 잘못됐습니다.",
	"비디오 에뮬레이션이 불완전합니다.",
	"사운드 에뮬레이션이 불완전합니다.",
	"소리가 나오지 않습니다.",
	"칵테일 모드의 화면 반전을 지원하지 못합니다.",
	"※이 게임은 전혀 동작하지 않습니다",
	"이 게임은 보호장치 때문에 에뮬레이션이 불완전합니다.",
	"missing serialization",
	"There are working clones of this game:",

	/* main menu */
	"입력 설정 (전체)",
	"딥 스위치",
	"아날로그 제어",
	"조이스틱 조정",
	"부가 정보",
	"입력 설정 (이 게임)",
	"Flush Current CFG",
	"Flush All CFGs",
	"게임 정보",
	"게임 역사",
	"게임 리셋(초기화)",
	"XML DAT 만들기",
	"게임으로 돌아가기",
	"치트",
	"메모리 카드",

	/* input */
	"Key/Joy Speed",
	"Reverse",
	"Sensitivity",

	/* stats */
	"티켓 분배수",
	"코인",
	"(고정)",

	/* memory card */
	"메모리 카드 읽기",
	"메모리 카드 꺼내기",
	"메모리 카드 만들기",
	"메모리 카드 읽기 실패!",
	"읽기 성공!",
	"메모리 카드 꺼냄!",
	"메모리 카드 만들기 성공!",
	"메모리 카드 만들기 실패!",
	"(메모리 카드가 있습니다?)",
	"내부 에러 발생!",

	/* cheats */
	"치트 사용/해제",
	"치트 추가/편집",
	"새로운 치트 검색 시작",
	"치트 검색 계속",
	"마지막 결과 표시",
	"이전 결과 복구",
	"조사 위치의 설정",
	"치트 도움말",
	"치트 옵션",
	"치트 파일 다시 읽기",
	"조사 위치",
	"무효",
	"치트",
	"조사 위치",
	"보충 설명",
	"보충 설명:",
	"이름",
	"설명",
	"활성화 키",
	"코드",
	"최대",
	"사용",
	"같은 치트 찾음: 무효로 하기",
	"치트 도움말이 없습니다",

	/* watchpoints */
	"Number of bytes",
	"Display Type",
	"Label Type",
	"Label",
	"X Position",
	"Y Position",
	"Watch",
	"Hex",
	"Decimal",
	"Binary",

	/* searching */
	"Lives (or another value)",
	"Timers (+/- some value)",
	"Energy (greater or less)",
	"Status (bits or flags)",
	"Slow But Sure (changed or not)",
	"Default Search Speed",
	"Fast",
	"Medium",
	"Slow",
	"Very Slow",
	"All Memory",
	"Select Memory Areas",
	"Matches found",
	"Search not initialized",
	"No previous values saved",
	"Previous values already restored",
	"Restoration successful",
	"Select a value",
	"All values saved",
	"One match found - added to list",

	"공략 보기",				// CMD_PLUS 공략관련
	"공략 파일이 없습니다.",	// CMD_PLUS 공략관련
	"자동 연사설정",			// AUTO_FIRE
	/* autofire */
	"사용안함",
	"사용함",
	"토글",
	"딜레이",
 	/* main menu */
	"메인메뉴",
	"Select Search",
	NULL
};



static const char **default_text[] =
{
	mame_default_text,
	NULL
};



static const char **trans_text;


int uistring_init (mame_file *langfile)
{
	/*
		TODO: This routine needs to do several things:
			- load an external font if needed
			- determine the number of characters in the font
			- deal with multibyte languages

	*/

	int i, j, str;
	char curline[255];
	char section[255] = "\0";
	char *ptr, *trans_ptr;
	int string_count;

	/* count the total amount of strings */
	string_count = 0;
	for (i = 0; default_text[i]; i++)
	{
		for (j = 0; default_text[i][j]; j++)
			string_count++;
	}

	/* allocate the translated text array, and set defaults */
	trans_text = auto_malloc(sizeof(const char *) * string_count);
	if (!trans_text)
		return 1;

	/* copy in references to all of the strings */
	str = 0;
	for (i = 0; default_text[i]; i++)
	{
		for (j = 0; default_text[i][j]; j++)
			trans_text[str++] = default_text[i][j];
	}

	memset(&lang, 0, sizeof(lang));

	/* if no language file, exit */
	if (!langfile)
		return 0;

	while (mame_fgets (curline, sizeof(curline) / sizeof(curline[0]), langfile) != NULL)
	{
		/* Ignore commented and blank lines */
		if (curline[0] == ';') continue;
		if (curline[0] == '\n') continue;
		if (curline[0] == '\r') continue;

		if (curline[0] == '[')
		{
			ptr = strtok (&curline[1], "]");
			/* Found a section, indicate as such */
			strcpy (section, ptr);

			/* Skip to the next line */
			continue;
		}

		/* Parse the LangInfo section */
		if (strcmp (section, "LangInfo") == 0)
		{
			ptr = strtok (curline, "=");
			if (strcmp (ptr, "Version") == 0)
			{
				ptr = strtok (NULL, "\n\r");
				sscanf (ptr, "%d", &lang.version);
			}
			else if (strcmp (ptr, "Language") == 0)
			{
				ptr = strtok (NULL, "\n\r");
				strcpy (lang.langname, ptr);
			}
			else if (strcmp (ptr, "Author") == 0)
			{
				ptr = strtok (NULL, "\n\r");
				strcpy (lang.author, ptr);
			}
			else if (strcmp (ptr, "Font") == 0)
			{
				ptr = strtok (NULL, "\n\r");
				strcpy (lang.fontname, ptr);
			}
		}

		/* Parse the Strings section */
		if (strcmp (section, "Strings") == 0)
		{
			char transline[255];
			
			/* Get all text up to the first line ending */
			ptr = strtok (curline, "\n\r");

			// 아래의 루프에서 검색하지 못하는 경우에 오동작하지 않도록 하기 위해
			// 다음라인을 미리 읽어서 마련한다. [DarkCoder]
			/* read next line as the translation */
			mame_fgets (transline, 255, langfile);
			/* Get all text up to the first line ending */
			trans_ptr = strtok (transline, "\n\r");
			
			/* Find a matching default string */
			str = 0;
			for (i = 0; default_text[i]; i++)
			{
				for (j = 0; default_text[i][j]; j++)
				{
					// 대/소문자를 구분하지 않게 함 [DarkCoder]
					if (stricmp (curline, default_text[i][j]) == 0)
					{
						/* Allocate storage and copy the string */
						trans_text[str] = auto_strdup(transline);
						if (!trans_text[str]) return 1;
					}
					str++;
				}
			}
		}
	}

	/* indicate success */
	return 0;
}



const char * ui_getstring (int string_num)
{
		return trans_text[string_num];
}
