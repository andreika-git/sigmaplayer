//////////////////////////////////////////////////////
// SigmaPlayer Project.
// File auto-generated from MMSL language reference.
//////////////////////////////////////////////////////

static const struct SCRIPT_OBJECT
{
    MmslObjectCallback Create, Delete;
    int ID;
    const char *name;
} script_objects[] = 
{
    { on_image_create, on_image_delete, SCRIPT_OBJECT_IMAGE,		"image" },
    { on_text_create, on_text_delete, SCRIPT_OBJECT_TEXT,		"text" },
    { on_rect_create, on_rect_delete, SCRIPT_OBJECT_RECT,		"rect" },
    { NULL, NULL, -1, NULL }
};

static const struct SCRIPT_OBJECT_VAR
{
    MmslVariableCallback Get, Set;
    int obj_ID;
    int ID;
    const char *name;
} script_object_vars[] = 
{
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_TYPE,		".type" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_GROUP,		".group" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_SRC,		".src" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_X,		".x" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_Y,		".y" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_VISIBLE,		".visible" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_WIDTH,		".width" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_HEIGHT,		".height" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_HFLIP,		".hflip" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_VFLIP,		".vflip" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_HALIGN,		".halign" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_VALIGN,		".valign" },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_TIMER,		".timer" },

    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_TYPE,		".type" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_GROUP,		".group" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_X,		".x" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_Y,		".y" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_VISIBLE,		".visible" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_WIDTH,		".width" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_HEIGHT,		".height" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_HALIGN,		".halign" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_VALIGN,		".valign" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_COLOR,		".color" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_BACKCOLOR,		".backcolor" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_VALUE,		".value" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_COUNT,		".count" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_DELETE,		".delete" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_FONT,		".font" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_TEXTALIGN,		".textalign" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_STYLE,		".style" },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_TIMER,		".timer" },

    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_TYPE,		".type" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_GROUP,		".group" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_X,		".x" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_Y,		".y" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_VISIBLE,		".visible" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_WIDTH,		".width" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_HEIGHT,		".height" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_LINEWIDTH,		".linewidth" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_COLOR,		".color" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_BACKCOLOR,		".backcolor" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_HALIGN,		".halign" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_VALIGN,		".valign" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_ROUND,		".round" },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_TIMER,		".timer" },
    { NULL, NULL, -1, -1, NULL }
};

