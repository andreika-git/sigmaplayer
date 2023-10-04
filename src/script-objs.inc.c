//////////////////////////////////////////////////////
// SigmaPlayer Project.
// File auto-generated from MMSL language reference.
//////////////////////////////////////////////////////

static const struct SCRIPT_OBJECT
{
    MmslObjectCallback Create, Delete;
    int ID;
} script_objects[] = 
{
    { on_image_create, on_image_delete, SCRIPT_OBJECT_IMAGE },
    { on_text_create, on_text_delete, SCRIPT_OBJECT_TEXT },
    { on_rect_create, on_rect_delete, SCRIPT_OBJECT_RECT },
    { NULL, NULL, -1 }
};

static const struct SCRIPT_OBJECT_VAR
{
    MmslVariableCallback Get, Set;
    int obj_ID;
    int ID;
} script_object_vars[] = 
{
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_TYPE },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_GROUP },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_SRC },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_X },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_Y },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_VISIBLE },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_WIDTH },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_HEIGHT },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_HFLIP },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_VFLIP },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_HALIGN },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_VALIGN },
    {  on_image_get, on_image_set,	 SCRIPT_OBJECT_IMAGE,		SCRIPT_OBJECT_VAR_IMAGE_TIMER },

    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_TYPE },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_GROUP },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_X },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_Y },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_VISIBLE },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_WIDTH },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_HEIGHT },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_HALIGN },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_VALIGN },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_COLOR },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_BACKCOLOR },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_VALUE },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_COUNT },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_DELETE },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_FONT },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_TEXTALIGN },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_STYLE },
    {  on_text_get, on_text_set,	 SCRIPT_OBJECT_TEXT,		SCRIPT_OBJECT_VAR_TEXT_TIMER },

    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_TYPE },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_GROUP },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_X },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_Y },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_VISIBLE },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_WIDTH },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_HEIGHT },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_LINEWIDTH },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_COLOR },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_BACKCOLOR },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_HALIGN },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_VALIGN },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_ROUND },
    {  on_rect_get, on_rect_set,	 SCRIPT_OBJECT_RECT,		SCRIPT_OBJECT_VAR_RECT_TIMER },
    { NULL, NULL, -1, -1, }
};

