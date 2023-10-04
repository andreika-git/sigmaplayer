//////////////////////////////////////////////////////
// SigmaPlayer Project.
// File auto-generated from MMSL language reference.
//////////////////////////////////////////////////////

void on_kernel_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_kernel_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);

void on_screen_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_screen_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);

void on_pad_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_pad_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);

void on_drive_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_drive_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);

void on_explorer_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_explorer_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);

void on_player_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_player_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);

void on_settings_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_settings_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);

void on_flash_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_flash_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);


enum SCRIPT_VARS
{
    SCRIPT_VAR_KERNEL_POWER,
    SCRIPT_VAR_KERNEL_FREQUENCY,
    SCRIPT_VAR_KERNEL_CHIP,
    SCRIPT_VAR_KERNEL_FREE_MEMORY,
    SCRIPT_VAR_KERNEL_FLASH_MEMORY,
    SCRIPT_VAR_KERNEL_PRINT,
    SCRIPT_VAR_KERNEL_FIRMWARE_VERSION,
    SCRIPT_VAR_KERNEL_MMSL_VERSION,
    SCRIPT_VAR_KERNEL_MMSL_ERRORS,
    SCRIPT_VAR_KERNEL_RUN,
    SCRIPT_VAR_KERNEL_RANDOM,

    SCRIPT_VAR_SCREEN_SWITCH,
    SCRIPT_VAR_SCREEN_UPDATE,
    SCRIPT_VAR_SCREEN_TVSTANDARD,
    SCRIPT_VAR_SCREEN_TVOUT,
    SCRIPT_VAR_SCREEN_LEFT,
    SCRIPT_VAR_SCREEN_TOP,
    SCRIPT_VAR_SCREEN_RIGHT,
    SCRIPT_VAR_SCREEN_BOTTOM,
    SCRIPT_VAR_SCREEN_PALETTE,
    SCRIPT_VAR_SCREEN_PALIDX,
    SCRIPT_VAR_SCREEN_PALALPHA,
    SCRIPT_VAR_SCREEN_PALCOLOR,
    SCRIPT_VAR_SCREEN_FONT,
    SCRIPT_VAR_SCREEN_COLOR,
    SCRIPT_VAR_SCREEN_BACKCOLOR,
    SCRIPT_VAR_SCREEN_TRCOLOR,
    SCRIPT_VAR_SCREEN_HALIGN,
    SCRIPT_VAR_SCREEN_VALIGN,
    SCRIPT_VAR_SCREEN_BACK,
    SCRIPT_VAR_SCREEN_BACK_LEFT,
    SCRIPT_VAR_SCREEN_BACK_TOP,
    SCRIPT_VAR_SCREEN_BACK_RIGHT,
    SCRIPT_VAR_SCREEN_BACK_BOTTOM,
    SCRIPT_VAR_SCREEN_PRELOAD,
    SCRIPT_VAR_SCREEN_HZOOM,
    SCRIPT_VAR_SCREEN_VZOOM,
    SCRIPT_VAR_SCREEN_HSCROLL,
    SCRIPT_VAR_SCREEN_VSCROLL,
    SCRIPT_VAR_SCREEN_ROTATE,
    SCRIPT_VAR_SCREEN_BRIGHTNESS,
    SCRIPT_VAR_SCREEN_CONTRAST,
    SCRIPT_VAR_SCREEN_SATURATION,
    SCRIPT_VAR_SCREEN_FULLSCREEN,

    SCRIPT_VAR_PAD_KEY,
    SCRIPT_VAR_PAD_DISPLAY,
    SCRIPT_VAR_PAD_SET,
    SCRIPT_VAR_PAD_CLEAR,

    SCRIPT_VAR_DRIVE_MEDIATYPE,
    SCRIPT_VAR_DRIVE_TRAY,

    SCRIPT_VAR_EXPLORER_CHARSET,
    SCRIPT_VAR_EXPLORER_FOLDER,
    SCRIPT_VAR_EXPLORER_TARGET,
    SCRIPT_VAR_EXPLORER_FILTER,
    SCRIPT_VAR_EXPLORER_MASK1,
    SCRIPT_VAR_EXPLORER_MASK2,
    SCRIPT_VAR_EXPLORER_MASK3,
    SCRIPT_VAR_EXPLORER_MASK4,
    SCRIPT_VAR_EXPLORER_MASK5,
    SCRIPT_VAR_EXPLORER_PATH,
    SCRIPT_VAR_EXPLORER_FILENAME,
    SCRIPT_VAR_EXPLORER_EXTENSION,
    SCRIPT_VAR_EXPLORER_DRIVE_LETTER,
    SCRIPT_VAR_EXPLORER_TYPE,
    SCRIPT_VAR_EXPLORER_MASKINDEX,
    SCRIPT_VAR_EXPLORER_FILESIZE,
    SCRIPT_VAR_EXPLORER_FILETIME,
    SCRIPT_VAR_EXPLORER_COUNT,
    SCRIPT_VAR_EXPLORER_SORT,
    SCRIPT_VAR_EXPLORER_POSITION,
    SCRIPT_VAR_EXPLORER_COMMAND,
    SCRIPT_VAR_EXPLORER_FIND,
    SCRIPT_VAR_EXPLORER_COPIED,

    SCRIPT_VAR_PLAYER_SOURCE,
    SCRIPT_VAR_PLAYER_PLAYING,
    SCRIPT_VAR_PLAYER_SAVED,
    SCRIPT_VAR_PLAYER_COMMAND,
    SCRIPT_VAR_PLAYER_SELECT,
    SCRIPT_VAR_PLAYER_REPEAT,
    SCRIPT_VAR_PLAYER_SPEED,
    SCRIPT_VAR_PLAYER_MENU,
    SCRIPT_VAR_PLAYER_LANGUAGE_MENU,
    SCRIPT_VAR_PLAYER_LANGUAGE_AUDIO,
    SCRIPT_VAR_PLAYER_LANGUAGE_SUBTITLE,
    SCRIPT_VAR_PLAYER_SUBTITLE_CHARSET,
    SCRIPT_VAR_PLAYER_SUBTITLE_WRAP,
    SCRIPT_VAR_PLAYER_SUBTITLE,
    SCRIPT_VAR_PLAYER_ANGLE,
    SCRIPT_VAR_PLAYER_VOLUME,
    SCRIPT_VAR_PLAYER_BALANCE,
    SCRIPT_VAR_PLAYER_AUDIO_OFFSET,
    SCRIPT_VAR_PLAYER_TIME,
    SCRIPT_VAR_PLAYER_TITLE,
    SCRIPT_VAR_PLAYER_CHAPTER,
    SCRIPT_VAR_PLAYER_NAME,
    SCRIPT_VAR_PLAYER_ARTIST,
    SCRIPT_VAR_PLAYER_AUDIO_INFO,
    SCRIPT_VAR_PLAYER_VIDEO_INFO,
    SCRIPT_VAR_PLAYER_AUDIO_STREAM,
    SCRIPT_VAR_PLAYER_SUBTITLE_STREAM,
    SCRIPT_VAR_PLAYER_LENGTH,
    SCRIPT_VAR_PLAYER_NUM_TITLES,
    SCRIPT_VAR_PLAYER_NUM_CHAPTERS,
    SCRIPT_VAR_PLAYER_WIDTH,
    SCRIPT_VAR_PLAYER_HEIGHT,
    SCRIPT_VAR_PLAYER_FRAME_RATE,
    SCRIPT_VAR_PLAYER_COLOR_SPACE,
    SCRIPT_VAR_PLAYER_DEBUG,
    SCRIPT_VAR_PLAYER_ERROR,

    SCRIPT_VAR_SETTINGS_COMMAND,
    SCRIPT_VAR_SETTINGS_AUDIOOUT,
    SCRIPT_VAR_SETTINGS_TVTYPE,
    SCRIPT_VAR_SETTINGS_TVSTANDARD,
    SCRIPT_VAR_SETTINGS_TVOUT,
    SCRIPT_VAR_SETTINGS_DVI,
    SCRIPT_VAR_SETTINGS_HQ_JPEG,
    SCRIPT_VAR_SETTINGS_HDD_SPEED,
    SCRIPT_VAR_SETTINGS_DVD_PARENTAL,
    SCRIPT_VAR_SETTINGS_DVD_MV,
    SCRIPT_VAR_SETTINGS_DVD_LANG_MENU,
    SCRIPT_VAR_SETTINGS_DVD_LANG_AUDIO,
    SCRIPT_VAR_SETTINGS_DVD_LANG_SPU,
    SCRIPT_VAR_SETTINGS_VOLUME,
    SCRIPT_VAR_SETTINGS_BALANCE,
    SCRIPT_VAR_SETTINGS_BRIGHTNESS,
    SCRIPT_VAR_SETTINGS_CONTRAST,
    SCRIPT_VAR_SETTINGS_SATURATION,
    SCRIPT_VAR_SETTINGS_USER1,
    SCRIPT_VAR_SETTINGS_USER2,
    SCRIPT_VAR_SETTINGS_USER3,
    SCRIPT_VAR_SETTINGS_USER4,
    SCRIPT_VAR_SETTINGS_USER5,
    SCRIPT_VAR_SETTINGS_USER6,
    SCRIPT_VAR_SETTINGS_USER7,
    SCRIPT_VAR_SETTINGS_USER8,
    SCRIPT_VAR_SETTINGS_USER9,
    SCRIPT_VAR_SETTINGS_USER10,
    SCRIPT_VAR_SETTINGS_USER11,
    SCRIPT_VAR_SETTINGS_USER12,
    SCRIPT_VAR_SETTINGS_USER13,
    SCRIPT_VAR_SETTINGS_USER14,
    SCRIPT_VAR_SETTINGS_USER15,
    SCRIPT_VAR_SETTINGS_USER16,

    SCRIPT_VAR_FLASH_FILE,
    SCRIPT_VAR_FLASH_ADDRESS,
    SCRIPT_VAR_FLASH_PROGRESS,
};

