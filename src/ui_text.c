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
	"�������� �ʴ� ROM�� ���������ͷ� ����ϴ� ����, ���۱ǹ��� ���� �����ϰ� �ֽ��ϴ�.",

	/* misc stuff */
	"���θ޴��� ���ư���",
	"���� �޴��� ���ư���",
	"�ƹ� Ű�� ��������",
	"On",
	"Off",
	"����",
	"OK",
	"��ȿ",
	"(����)",
	"CPU",
	"�ּ�",
	"��",
	"���� �ھ�",
	"�����",
	"���׷���",
	"���� ����",
	"ȭ�� �ػ�",
	"�ؽ�Ʈ",
	"����",
	"����",
	"��� ä��",
	"���",
	"����",
	"���� �ø�Ŀ",
	"���� ��",
	"����Ŭ��",
	"��� CPU",
	" ������ �����ϴ�",

	/* special characters */
	"\x11",
	"\x10",
	"\x18",
	"\x19",
	"\x1a",
	"\x1b",

	/* known problems */
	"�� ������ ������ ���� ������ �ֽ��ϴ�:",
	"���� ������ �ҿ����մϴ�.",
	"���� ������ ������ �߸��ƽ��ϴ�.",
	"���� ���ķ��̼��� �ҿ����մϴ�.",
	"���� ���ķ��̼��� �ҿ����մϴ�.",
	"�Ҹ��� ������ �ʽ��ϴ�.",
	"Ĭ���� ����� ȭ�� ������ �������� ���մϴ�.",
	"���� ������ ���� �������� �ʽ��ϴ�",
	"�� ������ ��ȣ��ġ ������ ���ķ��̼��� �ҿ����մϴ�.",
	"missing serialization",
	"There are working clones of this game:",

	/* main menu */
	"�Է� ���� (��ü)",
	"�� ����ġ",
	"�Ƴ��α� ����",
	"���̽�ƽ ����",
	"�ΰ� ����",
	"�Է� ���� (�� ����)",
	"Flush Current CFG",
	"Flush All CFGs",
	"���� ����",
	"���� ����",
	"���� ����(�ʱ�ȭ)",
	"XML DAT �����",
	"�������� ���ư���",
	"ġƮ",
	"�޸� ī��",

	/* input */
	"Key/Joy Speed",
	"Reverse",
	"Sensitivity",

	/* stats */
	"Ƽ�� �й��",
	"����",
	"(����)",

	/* memory card */
	"�޸� ī�� �б�",
	"�޸� ī�� ������",
	"�޸� ī�� �����",
	"�޸� ī�� �б� ����!",
	"�б� ����!",
	"�޸� ī�� ����!",
	"�޸� ī�� ����� ����!",
	"�޸� ī�� ����� ����!",
	"(�޸� ī�尡 �ֽ��ϴ�?)",
	"���� ���� �߻�!",

	/* cheats */
	"ġƮ ���/����",
	"ġƮ �߰�/����",
	"���ο� ġƮ �˻� ����",
	"ġƮ �˻� ���",
	"������ ��� ǥ��",
	"���� ��� ����",
	"���� ��ġ�� ����",
	"ġƮ ����",
	"ġƮ �ɼ�",
	"ġƮ ���� �ٽ� �б�",
	"���� ��ġ",
	"��ȿ",
	"ġƮ",
	"���� ��ġ",
	"���� ����",
	"���� ����:",
	"�̸�",
	"����",
	"Ȱ��ȭ Ű",
	"�ڵ�",
	"�ִ�",
	"���",
	"���� ġƮ ã��: ��ȿ�� �ϱ�",
	"ġƮ ������ �����ϴ�",

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

	"���� ����",				// CMD_PLUS ��������
	"���� ������ �����ϴ�.",	// CMD_PLUS ��������
	"�ڵ� ���缳��",			// AUTO_FIRE
	/* autofire */
	"������",
	"�����",
	"���",
	"������",
 	/* main menu */
	"���θ޴�",
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

			// �Ʒ��� �������� �˻����� ���ϴ� ��쿡 ���������� �ʵ��� �ϱ� ����
			// ���������� �̸� �о �����Ѵ�. [DarkCoder]
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
					// ��/�ҹ��ڸ� �������� �ʰ� �� [DarkCoder]
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
