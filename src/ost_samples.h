/*********************************************************************

	OST sample support

*********************************************************************/

<<<<<<< HEAD
enum
{
  OST_SUPPORT_DISABLED = 0,
  OST_SUPPORT_DDRAGON,
  OST_SUPPORT_FFIGHT,
  OST_SUPPORT_MK,
  OST_SUPPORT_MK_T,
  OST_SUPPORT_MOONWALKER,
  OST_SUPPORT_NBA_JAM,
  OST_SUPPORT_OUTRUN,
  OST_SUPPORT_SF1,
  OST_SUPPORT_SF2
};


extern bool ost_support_enabled (int ost);
extern void init_ost_settings (int ost);
=======

extern bool     ddragon_playing;
extern int      ddragon_current_music;
extern int      ddragon_stage;
extern int      d_title_counter;

extern bool     ff_playing_final_fight;
extern bool     ff_alternate_song_1;
extern bool     ff_alternate_song_2;

extern bool     mk_playing_mortal_kombat;
extern bool     mk_playing_mortal_kombat_t;

extern bool     moonwalker_playing;
extern bool     moon_diddy;
extern int      mj_current_music;

extern bool     nba_jam_playing;
extern bool     nba_jam_title_screen;
extern bool     nba_jam_select_screen;
extern bool     nba_jam_intermission;
extern bool     nba_jam_in_game;
extern bool     nba_jam_boot_up;
extern bool     nba_jam_playing_title_music;
extern int      m_nba_last_offset;
extern int      m_nba_start_counter;

extern bool     outrun_playing;
extern bool     outrun_start;
extern bool     outrun_diddy;
extern bool     outrun_title_diddy;
extern bool     outrun_title;
extern bool     outrun_lastwave;
extern int      outrun_start_counter;

extern bool     sf2_playing_street_fighter;
extern bool     fadingMusic;
>>>>>>> 7268b4800bc1d7a47ba44483043167f3f45d77b5


extern struct Samplesinterface ost_ddragon;
extern struct Samplesinterface ost_ffight;
extern struct Samplesinterface ost_mk;
extern struct Samplesinterface ost_moonwalker;
extern struct Samplesinterface ost_nba_jam;
extern struct Samplesinterface ost_outrun;
<<<<<<< HEAD
extern struct Samplesinterface ost_sf1;
=======
>>>>>>> 7268b4800bc1d7a47ba44483043167f3f45d77b5
extern struct Samplesinterface ost_sf2;


extern bool generate_ost_sound_ddragon    (int data);
extern bool generate_ost_sound_ffight     (int data);
extern bool generate_ost_sound_mk         (int data);
extern bool generate_ost_sound_mk_tunit   (int data);
extern bool generate_ost_sound_moonwalker (int data);
extern bool generate_ost_sound_nba_jam    (int data);
extern bool generate_ost_sound_outrun     (int data);
<<<<<<< HEAD
extern bool generate_ost_sound_sf1        (int data);
extern bool generate_ost_sound_sf2        (int data);


=======
extern bool generate_ost_sound_sf2        (int data);

>>>>>>> 7268b4800bc1d7a47ba44483043167f3f45d77b5
extern void ost_fade_volume (void);
