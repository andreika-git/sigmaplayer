//////////////////////////////////////////////////////
// SigmaPlayer Project.
// File auto-generated from MMSL language reference.
//////////////////////////////////////////////////////

// callbacks:
void on_image_create(int obj_id, MMSL_OBJECT *obj, void *param);
void on_image_delete(int obj_id, MMSL_OBJECT *obj, void *param);

void on_text_create(int obj_id, MMSL_OBJECT *obj, void *param);
void on_text_delete(int obj_id, MMSL_OBJECT *obj, void *param);

void on_rect_create(int obj_id, MMSL_OBJECT *obj, void *param);
void on_rect_delete(int obj_id, MMSL_OBJECT *obj, void *param);


enum
{
    SCRIPT_OBJECT_IMAGE,
    SCRIPT_OBJECT_TEXT,
    SCRIPT_OBJECT_RECT,
};

void on_image_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_image_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);

void on_text_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_text_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);

void on_rect_get(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);
void on_rect_set(int var_id, MmslVariable *var, void *param, int obj_id = -1, MMSL_OBJECT *obj = NULL);


enum SCRIPT_OBJECT_VARS
{
    SCRIPT_OBJECT_VAR_IMAGE_TYPE,
    SCRIPT_OBJECT_VAR_IMAGE_GROUP,
    SCRIPT_OBJECT_VAR_IMAGE_SRC,
    SCRIPT_OBJECT_VAR_IMAGE_X,
    SCRIPT_OBJECT_VAR_IMAGE_Y,
    SCRIPT_OBJECT_VAR_IMAGE_VISIBLE,
    SCRIPT_OBJECT_VAR_IMAGE_WIDTH,
    SCRIPT_OBJECT_VAR_IMAGE_HEIGHT,
    SCRIPT_OBJECT_VAR_IMAGE_HFLIP,
    SCRIPT_OBJECT_VAR_IMAGE_VFLIP,
    SCRIPT_OBJECT_VAR_IMAGE_HALIGN,
    SCRIPT_OBJECT_VAR_IMAGE_VALIGN,
    SCRIPT_OBJECT_VAR_IMAGE_TIMER,

    SCRIPT_OBJECT_VAR_TEXT_TYPE,
    SCRIPT_OBJECT_VAR_TEXT_GROUP,
    SCRIPT_OBJECT_VAR_TEXT_X,
    SCRIPT_OBJECT_VAR_TEXT_Y,
    SCRIPT_OBJECT_VAR_TEXT_VISIBLE,
    SCRIPT_OBJECT_VAR_TEXT_WIDTH,
    SCRIPT_OBJECT_VAR_TEXT_HEIGHT,
    SCRIPT_OBJECT_VAR_TEXT_HALIGN,
    SCRIPT_OBJECT_VAR_TEXT_VALIGN,
    SCRIPT_OBJECT_VAR_TEXT_COLOR,
    SCRIPT_OBJECT_VAR_TEXT_BACKCOLOR,
    SCRIPT_OBJECT_VAR_TEXT_VALUE,
    SCRIPT_OBJECT_VAR_TEXT_COUNT,
    SCRIPT_OBJECT_VAR_TEXT_DELETE,
    SCRIPT_OBJECT_VAR_TEXT_FONT,
    SCRIPT_OBJECT_VAR_TEXT_TEXTALIGN,
    SCRIPT_OBJECT_VAR_TEXT_STYLE,
    SCRIPT_OBJECT_VAR_TEXT_TIMER,

    SCRIPT_OBJECT_VAR_RECT_TYPE,
    SCRIPT_OBJECT_VAR_RECT_GROUP,
    SCRIPT_OBJECT_VAR_RECT_X,
    SCRIPT_OBJECT_VAR_RECT_Y,
    SCRIPT_OBJECT_VAR_RECT_VISIBLE,
    SCRIPT_OBJECT_VAR_RECT_WIDTH,
    SCRIPT_OBJECT_VAR_RECT_HEIGHT,
    SCRIPT_OBJECT_VAR_RECT_LINEWIDTH,
    SCRIPT_OBJECT_VAR_RECT_COLOR,
    SCRIPT_OBJECT_VAR_RECT_BACKCOLOR,
    SCRIPT_OBJECT_VAR_RECT_HALIGN,
    SCRIPT_OBJECT_VAR_RECT_VALIGN,
    SCRIPT_OBJECT_VAR_RECT_ROUND,
    SCRIPT_OBJECT_VAR_RECT_TIMER,
};

