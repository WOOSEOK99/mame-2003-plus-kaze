/****************************************************************************
 *      datafile_cmd.c
 *      command database engine
 *  
 *	Based on MAMEYA 5.4
 *
 * command.dat 를 처리할 전용 루틴만 분리한 것으로 datafile.c의 끝부분에
 * #include 처리해서 컴파일하면 되도록 작성함.
 *
 * 2005/03/03 - 중요함수 완전히 재작성. 공략데이터의 로딩전략이 완전히 수정됨 [DarkCoder]
 
 * 2005/03/02 - caname_wrap관련 부분 삭제 및 몇가지 def와 변수/함수명 조절
 *              command_sub_menu() 함수 튜닝 [DarkCoder]
 *
 * 2005/02/12 - 제9차 공략관련 및 한글폰트에 대한 총정리 소스 적용 (ALzard K. Ceio)
 *
 ****************************************************************************/
#include "ui_font.h"
#include "ui_text.h"

#define MAX_PATH          260
#define CMD_PLUS
#define TRUE 1
#define FALSE 0
#define UI_TITLE_MENU

#ifdef CMD_PLUS
enum
{
	COMMANDTYPE_GAME,
	COMMANDTYPE_DRVGAME,
	COMMANDTYPE_ALLGAME,
	COMMANDTYPE_SOURCE,
	COMMANDTYPE_DATABASE,
	MAX_COMMANDTYPE,
	COMMANDTYPE_INVALID=-1
};

struct tMenuIndex
{
	long offset;
	char *menuitem;
	char *cmd_data;
};

/****************************************************************************
 *      token parsing constants
 ****************************************************************************/
const char *DATAFILE_TAG_COMMAND = "$cmd";
const char *DATAFILE_TAG_END	 = "$end";

const char *DEFAULT_COMMAND_FILENAME = "command.dat";
const char *COMMAND_FOLDER = "command" ;

static struct tDatafileIndex 	*cmnd_idx = 0;
static struct tMenuIndex 	 	*menu_idx = 0;
static const struct GameDriver  *cmnd_last_drv = 0;

/**************************************************************************
 **************************************************************************
 *
 *              Parsing functions
 *
 **************************************************************************
 **************************************************************************/

/****************************************************************************
 *	ParseOpenCommand - Open up file for reading
 ****************************************************************************/
static UINT8 ParseOpenCommand(int cmdtype, const struct GameDriver *drv)
{
	/* Open file up in binary mode */
	const struct GameDriver *gdrv = drv;
	char tmpbuf[MAX_PATH];
	char szFileName[MAX_PATH];
  char sourcename[80];

	sprintf(sourcename, "%s", gdrv->source_file+12);
	sourcename[strlen(sourcename) - 2] = 0;
	  
 if (cmdtype == COMMANDTYPE_SOURCE && (gdrv && gdrv->name[0]))
		{
			gdrv = gdrv->clone_of;
			if (gdrv && gdrv->name[0])
				cmdtype = COMMANDTYPE_GAME;
		}
		
	// 공략데이터의 타입에 따른 파일명 설정
	switch(cmdtype)
	{
		case COMMANDTYPE_GAME:
			sprintf(tmpbuf, "%s.dat", drv->name);
			break;

		case COMMANDTYPE_DRVGAME:
			sprintf(tmpbuf, "%s\\%s.dat", sourcename, gdrv->name);
			break;

		case COMMANDTYPE_ALLGAME:
			sprintf(tmpbuf, "Allgames\\%s.dat", gdrv->name);
			break;
			
		case COMMANDTYPE_SOURCE:
			sprintf(tmpbuf, "drivers\\%s.dat", sourcename);
			break;
						
		case COMMANDTYPE_DATABASE:
			sprintf(tmpbuf, "%s", DEFAULT_COMMAND_FILENAME );
			break;

		default:
			return(FALSE);
			break;
	}

	// 공략데이터의 풀패스 설정
	sprintf(szFileName, "%s\\%s", COMMAND_FOLDER, tmpbuf);


	// 공략데이터 파일 오픈
	fp = mame_fopen (NULL, szFileName, FILETYPE_HISTORY, 0);

	/* If this is NULL, return FALSE. We can't open it */
	if (NULL == fp)
	{
		return (FALSE);
	}

	/* Otherwise, prepare! */
	dwFilePos = 0;
	
	return (TRUE);
}

/****************************************************************************
 *      cmd_GetNextToken - Pointer to the token string pointer
 *                                 Pointer to position within file
 *
 *      Returns token, or TOKEN_INVALID if at end of file
 ****************************************************************************/
static UINT32 cmd_GetNextToken(UINT8 **ppszTokenText, long *pdwPosition)
{
	UINT32 dwLength;			/* Length of symbol */
	UINT8 *pbTokenPtr = bToken; /* Point to the beginning */
	UINT8 bData;                /* Temporary data byte */
	UINT8 space, character;

	space     = '\t';
	character = ' '-1;

    while (1)
    {
		bData = mame_fgetc(fp);                                  /* Get next character */

        /* If we're at the end of the file, bail out */
        if (mame_feof(fp))
                return(TOKEN_INVALID);

        /* If it's not whitespace, then let's start eating characters */
		if (space != bData && '\t' != bData)
        {
            /* Store away our file position (if given on input) */
            if (pdwPosition)
                    *pdwPosition = dwFilePos;

            /* If it's a separator, special case it */
            if (',' == bData || '=' == bData)
            {
                *pbTokenPtr++ = bData;
                *pbTokenPtr = '\0';
                ++dwFilePos;

                if (',' == bData)
                    return(TOKEN_COMMA);
                else
                    return(TOKEN_EQUALS);
            }

            /* Otherwise, let's try for a symbol */
			if (bData > character)
            {
                dwLength = 0;                   /* Assume we're 0 length to start with */

                /* Loop until we've hit something we don't understand */
                while (bData != ',' &&
                       bData != '=' &&
				       bData != space &&
                       bData != '\t' &&
                       bData != '\n' &&
                       bData != '\r' &&
                       mame_feof(fp) == 0)
                {
                    ++dwFilePos;
                    *pbTokenPtr++ = bData;  /* Store our byte */
                    ++dwLength;
                    assert(dwLength < MAX_TOKEN_LENGTH);
                    bData = mame_fgetc(fp);
                }

                /* If it's not the end of the file, put the last received byte */
                /* back. We don't want to touch the file position, though if */
                /* we're past the end of the file. Otherwise, adjust it. */

                if (0 == mame_feof(fp))
                {
                    mame_ungetc(bData, fp);
                }

                /* Null terminate the token */
                *pbTokenPtr = '\0';

                /* Connect up the */
                if (ppszTokenText)
                        *ppszTokenText = bToken;

                return(TOKEN_SYMBOL);
            }

            /* Not a symbol. Let's see if it's a cr/cr, lf/lf, or cr/lf/cr/lf */
            /* sequence */

            if (LF == bData)
            {
                /* Unix style perhaps? */
                bData = mame_fgetc(fp);          /* Peek ahead */
                mame_ungetc(bData, fp);          /* Force a retrigger if subsequent LF's */

                if (LF == bData)                /* Two LF's in a row - it's a UNIX hard CR */
                {
                    ++dwFilePos;
                    *pbTokenPtr++ = bData;  /* A real linefeed */
                    *pbTokenPtr = '\0';
                    return(TOKEN_LINEBREAK);
                }

            /* Otherwise, fall through and keep parsing. */
            }
            else
            if (CR == bData)                /* Carriage return? */
            {
                /* Figure out if it's Mac or MSDOS format */
                ++dwFilePos;
                bData = mame_fgetc(fp);          /* Peek ahead */

                /* We don't need to bother with EOF checking. It will be 0xff if */
                /* it's the end of the file and will be caught by the outer loop. */
                if (CR == bData)                /* Mac style hard return! */
                {
                    /* Do not advance the file pointer in case there are successive */
                    /* CR/CR sequences */

                    /* Stuff our character back upstream for successive CR's */
                    mame_ungetc(bData, fp);

                    *pbTokenPtr++ = bData;  /* A real carriage return (hard) */
                    *pbTokenPtr = '\0';
                    return(TOKEN_LINEBREAK);
                }
                else
                if (LF == bData)        /* MSDOS format! */
                {
                	// 한번의 CR/LF검사 - GetNextToken를 안쓰고 cmd_GetNextToken를 쓰는 유일한 이유
					mame_ungetc(bData, fp);

					*pbTokenPtr++ = bData;	/* A real carriage return (hard) */
					*pbTokenPtr = '\0';

					return(TOKEN_LINEBREAK);
				}
                else
                {
                    --dwFilePos;
                    mame_ungetc(bData, fp);  /* Put the character back. No good */
                }

                /* Otherwise, fall through and keep parsing */
            }
		}

        ++dwFilePos;
    }
}

/**************************************************************************
 *	index_menuidx
 *	
 *
 *	Returns 0 on error, or the number of index entries created.
 **************************************************************************/
static int index_menuidx (const struct GameDriver *drv, struct tDatafileIndex *d_idx, struct tMenuIndex **_index)
{
	struct tMenuIndex *m_idx;
	const struct GameDriver *gdrv;
	struct tDatafileIndex *gd_idx;
	int m_count = 0;
	UINT32 token = TOKEN_SYMBOL;

	long tell;
	long cmdtag_offset = 0;
	char *s;

	gdrv = drv;
	do
	{
		gd_idx = d_idx;

		/* find driver in datafile index */
		while (gd_idx->driver)
		{
			if (gd_idx->driver == gdrv) break;
			gd_idx++;
		}

		gdrv = gdrv->clone_of;
	} while (!gd_idx->driver && gdrv);

	if (gdrv == 0) return 0;	/* driver not found in Data_file_index */

	/* seek to correct point in datafile */
	if (ParseSeek (gd_idx->offset, SEEK_SET)) return 0;

	/* allocate index */
	m_idx = *_index = (struct tMenuIndex *)malloc(MAX_MENUIDX_ENTRIES * sizeof (struct tMenuIndex));
	if (NULL == m_idx) return 0;

	memset( m_idx, 0, MAX_MENUIDX_ENTRIES * sizeof (struct tMenuIndex));
	
	/* loop through between $cmd= */
	token = cmd_GetNextToken ((UINT8 **)&s, &tell);
	while ((m_count < (MAX_MENUIDX_ENTRIES - 1)) && TOKEN_INVALID != token)
	{
		if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen(DATAFILE_TAG_KEY)))
			break;

		/* DATAFILE_TAG_COMMAND identifies the driver */
		if (!ci_strncmp (DATAFILE_TAG_COMMAND, s, strlen(DATAFILE_TAG_COMMAND)))
		{
			cmdtag_offset = tell;
			token = cmd_GetNextToken ((UINT8 **)&s, &tell);

			if (TOKEN_EQUALS == token)
			{
				token = cmd_GetNextToken ((UINT8 **)&s, &tell);
				m_idx->menuitem = (char*)malloc(strlen(s)+1);
				strcpy(m_idx->menuitem, s);

				m_idx->offset = cmdtag_offset;

				m_idx++;
				m_count++;
			}
			else
			{
				while (TOKEN_SYMBOL != token)
					token = cmd_GetNextToken ((UINT8 **)&s, &tell);

				m_idx->menuitem = malloc( strlen( s ) + 1 );
				strcpy( m_idx->menuitem, s );

				m_idx->offset = cmdtag_offset;

				m_idx++;
				m_count++;
			}
		}
		token = cmd_GetNextToken ((UINT8 **)&s, &tell);
	}

	/* mark end of index */
	m_idx->offset = 0L;
	m_idx->menuitem = NULL;

	return m_count;
}


/**************************************************************************
 *	cmd_load_datafile_text
 *
 *	Loads text field for a driver into the buffer specified. Specify the
 *	driver, a pointer to the buffer, the buffer size, the index created by
 *	index_menuidx(), and the desired text field (e.g., DATAFILE_TAG_BIO).
 *
 *	Returns 0 if successful.
 **************************************************************************/
static int cmd_load_datafile_text (const struct GameDriver *drv, char *buffer, int bufsize,
								const char *tag, struct tMenuIndex *m_idx, const int menu_sel)
{
	int offset = 0;
	int found = 0;
	UINT32	token = TOKEN_SYMBOL;
	UINT32 	prev_token = TOKEN_SYMBOL;
	int first = 1;

	*buffer = '\0';

	/* seek to correct point in datafile */
	if (ParseSeek ((m_idx + menu_sel)->offset, SEEK_SET)) return 1;

	/* read text until buffer is full or end of entry is encountered */
	while (TOKEN_INVALID != token)
	{
		char *s;
		int len;
		long tell;

		token = cmd_GetNextToken ((UINT8 **)&s, &tell);
		if (TOKEN_INVALID == token) continue;

		if (found)
		{
			/* end entry when a tag is encountered */
			if (TOKEN_SYMBOL == token && !ci_strncmp (DATAFILE_TAG_END, s, strlen (DATAFILE_TAG_END))) break;

			prev_token = token;

			/* translate platform-specific linebreaks to '\n' */
			if (TOKEN_LINEBREAK == token)
				strcpy (s, "\n");

			/* append a space to words */
			if (TOKEN_LINEBREAK != token)
				strcat (s, " ");

			/* remove extraneous space before commas */
			if (TOKEN_COMMA == token)
			{
				--buffer;
				--offset;
				*buffer = '\0';
			}

			/* add this word to the buffer */
			len = strlen (s);
			if ((len + offset) >= bufsize) break;
			strcpy (buffer, s);
			buffer += len;
			offset += len;
		}
		else
		{
			if (TOKEN_SYMBOL == token)
			{
				/* looking for requested tag */
				if (!ci_strncmp (tag, s, strlen (tag)))
				{
					found = 1;
					token = cmd_GetNextToken ((UINT8 **)&s, &tell);
					if ( TOKEN_EQUALS == token )
					{
						token = cmd_GetNextToken ((UINT8 **)&s, &tell);
						token = cmd_GetNextToken ((UINT8 **)&s, &tell);
					}

					if (first) first = 0;
				}
				else if (!ci_strncmp (DATAFILE_TAG_KEY, s, strlen (DATAFILE_TAG_KEY)))
					break; /* error: tag missing */
			}
		}
	}
	return (!found);
}

/**************************************************************************
 *	load_driver_command
 **************************************************************************/
#ifdef UI_TITLE_MENU
int load_driver_command (const struct GameDriver *drv, char *buffer, int bufsize, const int menu_sel, const char *menu_item[])
#else
int load_driver_command (const struct GameDriver *drv, char *buffer, int bufsize, const int menu_sel)
#endif /* UI_TITLE_MENU */
{
	int err = 0;
	int cmdtype = COMMANDTYPE_GAME;
	int isLoaded = 0;

	*buffer = 0;

	fp = mame_fopen (NULL, DEFAULT_COMMAND_FILENAME, FILETYPE_HISTORY, 0);

	if(fp != NULL)
	{
			/* create index if necessary */
			if (cmnd_idx)
				isLoaded = 1;
			else
				isLoaded = (index_datafile (&cmnd_idx) != 0);

			/* create menu_index if necessary */
			if (menu_idx)
				isLoaded = 1;
			else
				isLoaded = (index_menuidx (drv, cmnd_idx, &menu_idx) != 0);

			/* load informational text (append) */
			if (cmnd_idx)
			{
				const struct GameDriver *gdrv;

				gdrv = drv;
				do
				{
					err = cmd_load_datafile_text ( gdrv, 
												buffer, bufsize,
						  						DATAFILE_TAG_COMMAND, 
						  						menu_idx, menu_sel);
					gdrv = gdrv->clone_of;
				} while (err && gdrv);

				if (err) isLoaded = 0;
			}
			ParseClose ();
	
			if (isLoaded) return 1; // success
	}
#if 0
	while (cmdtype < MAX_COMMANDTYPE)
	{
		/* try to open command datafile of Machine Driver */ 
		if (ParseOpenCommand (cmdtype, drv))
		{
			/* create index if necessary */
			if (cmnd_idx)
				isLoaded = 1;
			else
				isLoaded = (index_datafile (&cmnd_idx) != 0);

			/* create menu_index if necessary */
			if (menu_idx)
				isLoaded = 1;
			else
				isLoaded = (index_menuidx (drv, cmnd_idx, &menu_idx) != 0);

			/* load informational text (append) */
			if (cmnd_idx)
			{
				const struct GameDriver *gdrv;

				gdrv = drv;
				do
				{
					err = cmd_load_datafile_text ( gdrv, 
												buffer, bufsize,
						  						DATAFILE_TAG_COMMAND, 
						  						menu_idx, menu_sel);
					gdrv = gdrv->clone_of;
				} while (err && gdrv);

				if (err) isLoaded = 0;
			}
			ParseClose ();
		}

		if (isLoaded) return 1; // success
			
		cmdtype++;
	}
#endif

#ifdef UI_TITLE_MENU
	if(menu_item != NULL) 
	{
		UINT8 total = 0;
	
		if(menu_idx && isLoaded == 1)
		{
			struct tMenuIndex *m_idx = menu_idx;
			
			while(m_idx->menuitem != NULL)
			{
				menu_item[total++] = m_idx->menuitem;
				m_idx++;
			}
		}
	}
#endif /* UI_TITLE_MENU */

	return isLoaded;
}

/**************************************************************************
 *	command_GetDataPtr
 **************************************************************************/
char *command_GetDataPtr(int idx)
{
	if(!menu_idx || (idx<0) || (idx>=MAX_MENUIDX_ENTRIES)) return 0;
	
	return menu_idx[idx].cmd_data;
}

/**************************************************************************
 *	command_BuildMenuItem
 **************************************************************************/
UINT8 command_BuildMenuItem(const char *menu_item[])
{
	UINT8 idx = 0;
	
	if(menu_idx && menu_item) 
	{
		while(menu_idx[idx].menuitem != NULL)
		{
			menu_item[idx] = menu_idx[idx].menuitem;
			idx++;
		}
	}
	
	return idx;
}

/**************************************************************************
 *	command_CreateData
 **************************************************************************/
UINT8 command_CreateData(const struct GameDriver *drv, const char *menu_item[])
{
	int cmndtype = COMMANDTYPE_GAME;
	int isLoaded = 0;
  
	if (menu_idx)
	{	
		command_DestroyData();
	}

	/*
	Step 1/2 - Open command file & create datafile index and menu index data
	*/
#if 0
	while (cmndtype < MAX_COMMANDTYPE)
	{
		/* try to open command datafile of Machine Driver */
		if (ParseOpenCommand (cmndtype, drv))
		{
			/* create index if necessary */
			if (cmnd_idx)
				isLoaded = 1;
			else
				isLoaded = (index_datafile (&cmnd_idx) != 0);
			
			if (menu_idx)
				isLoaded = 1;
			else
				isLoaded = (index_menuidx (drv, cmnd_idx, &menu_idx) != 0);

			ParseClose ();
		}

		if(menu_idx && isLoaded) break;

		cmndtype++;
	}
#endif

	fp = mame_fopen (NULL, DEFAULT_COMMAND_FILENAME, FILETYPE_HISTORY, 0);

	if(fp != NULL)
	{
			/* create index if necessary */
			if (cmnd_idx)
				isLoaded = 1;
			else
				isLoaded = (index_datafile (&cmnd_idx) != 0);

			/* create menu_index if necessary */
			if (menu_idx)
				isLoaded = 1;
			else
				isLoaded = (index_menuidx (drv, cmnd_idx, &menu_idx) != 0);

			ParseClose ();			
	}
	/*
	Step 2/2 - Load command data from file per menu
	*/
	if(menu_idx && isLoaded)
	{
		char *tmpBuf;
		int   iflag, datasize, idx;

		/* allocate temporary buffer to load command data per menu */		
		tmpBuf = (char*)malloc(65536);
		if(!tmpBuf) {
			command_DestroyData();
			return 0;
		}
		
		idx = 0;
		while(menu_idx[idx].menuitem != NULL)
		{
			memset( tmpBuf, 0, 65536 );
#ifdef UI_TITLE_MENU
			iflag = load_driver_command (drv, tmpBuf, 65536, idx, NULL );
#else
			iflag = load_driver_command (drv, tmpBuf, 65536, idx);
#endif /* UI_TITLE_MENU */
			/* iflag is nonzero if successfully loaded */
			if(iflag) {
				/* convert command specialized macro */
				ConvertCommandMacro(tmpBuf);

				strcat(tmpBuf,	"\n\t");
				strcat(tmpBuf,	ui_getstring(UI_lefthilight));
				strcat(tmpBuf,	" ");
				strcat(tmpBuf,	ui_getstring(UI_returntoprior));
				strcat(tmpBuf,	" ");
				strcat(tmpBuf,	ui_getstring(UI_righthilight));
				strcat(tmpBuf,	"\n");
				
				datasize = strlen(tmpBuf) + 2;
				
				/* allocate & store command data */				
				menu_idx[idx].cmd_data = (char*)malloc(datasize);
				if(menu_idx[idx].cmd_data)
					memcpy(menu_idx[idx].cmd_data, tmpBuf, datasize);
			}
			else {
				/* there's no command data for this menu */
				menu_idx[idx].cmd_data = 0;
			}
			
			idx++;
		}
		cmnd_last_drv = drv;

		/* clean up temporary buffer */
		free(tmpBuf);
		
		command_BuildMenuItem( menu_item );
		
		return idx;
	}

	return 0;
}

/**************************************************************************
 *	command_DestroyData
 **************************************************************************/
void command_DestroyData(void)
{
	int idx;
	
	if(menu_idx == 0) return;

	idx = 0;
	while(menu_idx[idx].menuitem != NULL)
	{
		if(menu_idx[idx].cmd_data) 
			free(menu_idx[idx].cmd_data);
			
		idx++;
	}
	
	free(menu_idx);
	menu_idx = 0;
}

#endif /* CMD_PLUS */
