//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - script objects wrapper impl.
 *  \file       script-objects.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       12.10.2006
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 */
//////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <dirent.h>

#include <script-internal.h>

void on_image_create(int obj_id, MMSL_OBJECT *obj, void *)
{
	if (obj_id == SCRIPT_OBJECT_IMAGE)
	{
		Image *img = new Image();
		img->timerobj = NULL;
		gui.AddWindow(img);
		*obj = (MMSL_OBJECT)img;
	}
}

void on_image_delete(int obj_id, MMSL_OBJECT *obj, void *)
{
	if (obj_id == SCRIPT_OBJECT_IMAGE)
	{
		Image *img = (Image *)*obj;
		if (img->timerobj != NULL)
		{
			img->timerobj->obj = NULL;
			params->timed_objs.Delete(img->timerobj);
			img->timerobj = NULL;
		}
		if (img->updateobj != NULL)
		{
			img->updateobj->obj = NULL;
			params->timed_objs.Delete(img->updateobj);
			img->updateobj = NULL;
		}
		gui.RemoveWindow(img);
		*obj = NULL;
	}
}

////////////////////////////////////////////////////////////////////////

void on_text_create(int obj_id, MMSL_OBJECT *obj, void *)
{
	if (obj_id == SCRIPT_OBJECT_TEXT)
	{
		Text *txt = new Text();
		txt->SetFont(params->font);
		gui.AddWindow(txt);
		*obj = (MMSL_OBJECT)txt;
	}
}

void on_text_delete(int obj_id, MMSL_OBJECT *obj, void *)
{
	if (obj_id == SCRIPT_OBJECT_TEXT)
	{
		Text *txt = (Text *)*obj;
		if (txt->timerobj != NULL)
		{
			txt->timerobj->obj = NULL;
			if (params != NULL)
				params->timed_objs.Delete(txt->timerobj);
			txt->timerobj = NULL;
		}
		gui.RemoveWindow(txt);
		*obj = NULL;
	}
}

////////////////////////////////////////////////////////////////////////

void on_rect_create(int obj_id, MMSL_OBJECT *obj, void *)
{
	if (obj_id == SCRIPT_OBJECT_RECT)
	{
		Rectangle *rect = new Rectangle();
		gui.AddWindow(rect);
		*obj = (MMSL_OBJECT)rect;
	}
}

void on_rect_delete(int obj_id, MMSL_OBJECT *obj, void *)
{
	if (obj_id == SCRIPT_OBJECT_RECT)
	{
		Rectangle *rect = (Rectangle *)*obj;
		if (rect->timerobj != NULL)
		{
			rect->timerobj->obj = NULL;
			params->timed_objs.Delete(rect->timerobj);
			rect->timerobj = NULL;
		}
		gui.RemoveWindow(rect);
		*obj = NULL;
	}
}

////////////////////////////////////////////////////////////////////////
/// common vars for all dynamic objects
void on_common_get(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *obj)
{
	if (obj == NULL)
		return;
	Window *win = (Window *)*obj;
	if (win == NULL || var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_OBJECT_VAR_IMAGE_TYPE:
		var->Set("image");
		break;
	case SCRIPT_OBJECT_VAR_TEXT_TYPE:
		var->Set("text");
		break;
	case SCRIPT_OBJECT_VAR_RECT_TYPE:
		var->Set("rect");
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_GROUP:
	case SCRIPT_OBJECT_VAR_TEXT_GROUP:
	case SCRIPT_OBJECT_VAR_RECT_GROUP:
		var->Set(win->group != NULL ? *win->group : SPString());
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_X:
	case SCRIPT_OBJECT_VAR_TEXT_X:
	case SCRIPT_OBJECT_VAR_RECT_X:
		var->Set(win->GetX());
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_Y:
	case SCRIPT_OBJECT_VAR_TEXT_Y:
	case SCRIPT_OBJECT_VAR_RECT_Y:
		var->Set(win->GetY());
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_VISIBLE:
	case SCRIPT_OBJECT_VAR_TEXT_VISIBLE:
	case SCRIPT_OBJECT_VAR_RECT_VISIBLE:
		var->Set(win->visible);
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_WIDTH:
	case SCRIPT_OBJECT_VAR_TEXT_WIDTH:
	case SCRIPT_OBJECT_VAR_RECT_WIDTH:
		var->Set(win->GetWidth());
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_HEIGHT:
	case SCRIPT_OBJECT_VAR_TEXT_HEIGHT:
	case SCRIPT_OBJECT_VAR_RECT_HEIGHT:
		var->Set(win->GetHeight());
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_HALIGN:
	case SCRIPT_OBJECT_VAR_TEXT_HALIGN:
	case SCRIPT_OBJECT_VAR_RECT_HALIGN:
		{
			const char *ha[] = { "left", "center", "right" };
			var->Set(ha[win->halign]);
		}
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_VALIGN:
	case SCRIPT_OBJECT_VAR_TEXT_VALIGN:
	case SCRIPT_OBJECT_VAR_RECT_VALIGN:
		{
			const char *va[] = { "top", "center", "bottom" };
			var->Set(va[win->valign]);
		}
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_TIMER:
	case SCRIPT_OBJECT_VAR_TEXT_TIMER:
	case SCRIPT_OBJECT_VAR_RECT_TIMER:
		var->Set(win->timer > 0 ?  win->timer - params->curtime : win->timer);
		break;
	}
}

void on_common_set(int var_id, MmslVariable *var, void *, int , MMSL_OBJECT *obj)
{
	if (obj == NULL)
		return;
	Window *win = (Window *)*obj;
	if (win == NULL || var == NULL)
		return;
	switch (var_id)
	{
	case SCRIPT_OBJECT_VAR_IMAGE_GROUP:
	case SCRIPT_OBJECT_VAR_TEXT_GROUP:
	case SCRIPT_OBJECT_VAR_RECT_GROUP:
		SPSafeDelete(win->group);
		win->group = new SPString(var->GetString());
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_X:
	case SCRIPT_OBJECT_VAR_TEXT_X:
	case SCRIPT_OBJECT_VAR_RECT_X:
		win->SetX(var->GetInteger());
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_Y:
	case SCRIPT_OBJECT_VAR_TEXT_Y:
	case SCRIPT_OBJECT_VAR_RECT_Y:
		win->SetY(var->GetInteger());
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_VISIBLE:
	case SCRIPT_OBJECT_VAR_TEXT_VISIBLE:
	case SCRIPT_OBJECT_VAR_RECT_VISIBLE:
		win->SetVisible(var->GetInteger() != 0);
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_WIDTH:
	case SCRIPT_OBJECT_VAR_TEXT_WIDTH:
	case SCRIPT_OBJECT_VAR_RECT_WIDTH:
		win->SetWidth(var->GetInteger());
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_HEIGHT:
	case SCRIPT_OBJECT_VAR_TEXT_HEIGHT:
	case SCRIPT_OBJECT_VAR_RECT_HEIGHT:
		win->SetHeight(var->GetInteger());
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_HALIGN:
	case SCRIPT_OBJECT_VAR_TEXT_HALIGN:
	case SCRIPT_OBJECT_VAR_RECT_HALIGN:
		{
			SPString str = var->GetString();
			if (str.CompareNoCase("center") == 0)
				win->SetHAlign(WINDOW_ALIGN_CENTER);
			else if (str.CompareNoCase("right") == 0)
				win->SetHAlign(WINDOW_ALIGN_RIGHT);
			else 
				win->SetHAlign(WINDOW_ALIGN_LEFT);
		}
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_VALIGN:
	case SCRIPT_OBJECT_VAR_TEXT_VALIGN:
	case SCRIPT_OBJECT_VAR_RECT_VALIGN:
		{
			SPString str = var->GetString();
			if (str.CompareNoCase("center") == 0)
				win->SetVAlign(WINDOW_ALIGN_CENTER);
			else if (str.CompareNoCase("bottom") == 0)
				win->SetVAlign(WINDOW_ALIGN_BOTTOM);
			else 
				win->SetVAlign(WINDOW_ALIGN_TOP);
		}
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_TIMER:
	case SCRIPT_OBJECT_VAR_TEXT_TIMER:
	case SCRIPT_OBJECT_VAR_RECT_TIMER:
		{
			int delta = var->GetInteger();
			if (delta > 0)
			{
				ScriptTimerObject *to = new ScriptTimerObject();
				to->type = SCRIPT_OBJECT_TIMER;
				to->obj = win;
				to->var_ID = var_id;
				win->timer = params->curtime + delta;
				if (win->timerobj != NULL)
					params->timed_objs.Delete(win->timerobj);
				win->timerobj = to;
				params->timed_objs.Add(to);
			} else
				win->timer = delta;
		}
		break;
	}
}

////////////////////////////////////////////////////////////////////////

void on_image_get(int var_id, MmslVariable *var, void *param, int obj_id, MMSL_OBJECT *obj)
{
	if (var == NULL || obj_id != SCRIPT_OBJECT_IMAGE || obj == NULL)
		return;
	
	Image *img = (Image *)*obj;
	if (img == NULL)
		return;

	on_common_get(var_id, var, param, obj_id, obj);

	switch (var_id)
	{
	case SCRIPT_OBJECT_VAR_IMAGE_SRC:
		var->Set(img->img != NULL ? img->img->name : 0);
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_HFLIP:
		var->Set(img->flipx);
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_VFLIP:
		var->Set(img->flipy);
		break;
	}
}

void on_image_set(int var_id, MmslVariable *var, void *param, int obj_id, MMSL_OBJECT *obj)
{
	if (var == NULL || obj_id != SCRIPT_OBJECT_IMAGE || obj == NULL)
		return;
	
	Image *img = (Image *)*obj;
	if (img == NULL)
		return;

	on_common_set(var_id, var, param, obj_id, obj);

	switch (var_id)
	{
	case SCRIPT_OBJECT_VAR_IMAGE_SRC:
		{
			img->SetSource(var->GetString());
			// add to update queue
			ScriptTimerObject *to = img->updateobj;
			if (img->img != NULL && img->img->num_frames > 1)
			{
				if (to == NULL)
				{
					to = new ScriptTimerObject();
					to->obj = img;
					to->var_ID = var_id;
					to->type = SCRIPT_OBJECT_UPDATE;
					img->updateobj = to;
					params->timed_objs.Add(to);
				}
			} 
			else if (to != NULL)
			{
				params->timed_objs.Delete(to);
				img->updateobj = NULL;
			}
			mmsl->UpdateObjectVariable(img, SCRIPT_OBJECT_VAR_IMAGE_WIDTH);
			mmsl->UpdateObjectVariable(img, SCRIPT_OBJECT_VAR_IMAGE_HEIGHT);
		}
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_HFLIP:
		img->SetFlipX(var->GetInteger() != 0);
		break;
	case SCRIPT_OBJECT_VAR_IMAGE_VFLIP:
		img->SetFlipY(var->GetInteger() != 0);
		break;
	}
}

////////////////////////////////////////////////////////////////////////

void on_text_get(int var_id, MmslVariable *var, void *param, int obj_id, MMSL_OBJECT *obj)
{
	if (var == NULL || obj_id != SCRIPT_OBJECT_TEXT || obj == NULL)
		return;
	
	Text *text = (Text *)*obj;
	if (text == NULL)
		return;

	on_common_get(var_id, var, param, obj_id, obj);

	switch (var_id)
	{
	case SCRIPT_OBJECT_VAR_TEXT_COLOR:
		var->Set(text->color);
		break;
	case SCRIPT_OBJECT_VAR_TEXT_BACKCOLOR:
		var->Set(text->color);
		break;
	case SCRIPT_OBJECT_VAR_TEXT_FONT:
		var->Set(text->font != NULL ? text->font->name : "");
		break;
	case SCRIPT_OBJECT_VAR_TEXT_VALUE:
		var->Set(text->text);
		break;
	case SCRIPT_OBJECT_VAR_TEXT_COUNT:
		var->Set(text->text.GetLength());
		break;
	case SCRIPT_OBJECT_VAR_TEXT_TEXTALIGN:
		{
			const char *ta[] = { "left", "center", "right" };
			var->Set(ta[text->text_align]);
		}
		break;
	case SCRIPT_OBJECT_VAR_TEXT_DELETE:
		var->Set(0);
		break;
	case SCRIPT_OBJECT_VAR_TEXT_STYLE:
		{
			const char *sty[] = { "", "underline", "outline" };
			var->Set(sty[text->style]);
		}
		break;
	}
}

void on_text_set(int var_id, MmslVariable *var, void *param, int obj_id, MMSL_OBJECT *obj)
{
	if (var == NULL || obj_id != SCRIPT_OBJECT_TEXT || obj == NULL)
		return;
	
	Text *text = (Text *)*obj;
	if (text == NULL)
		return;

	on_common_set(var_id, var, param, obj_id, obj);

	switch (var_id)
	{
	case SCRIPT_OBJECT_VAR_TEXT_COLOR:
		text->SetColor(var->GetInteger());
		break;
	case SCRIPT_OBJECT_VAR_TEXT_BACKCOLOR:
		text->SetBkColor(var->GetInteger());
		break;
	case SCRIPT_OBJECT_VAR_TEXT_FONT:
		text->SetFont(var->GetString());
		mmsl->UpdateObjectVariable(text, SCRIPT_OBJECT_VAR_TEXT_WIDTH);
		mmsl->UpdateObjectVariable(text, SCRIPT_OBJECT_VAR_TEXT_HEIGHT);
		break;
	case SCRIPT_OBJECT_VAR_TEXT_VALUE:
		text->SetText(var->GetString());
		mmsl->UpdateObjectVariable(text, SCRIPT_OBJECT_VAR_TEXT_WIDTH);
		mmsl->UpdateObjectVariable(text, SCRIPT_OBJECT_VAR_TEXT_HEIGHT);
		break;
	case SCRIPT_OBJECT_VAR_TEXT_COUNT:
		text->SetText(text->text.Left(var->GetInteger()));
		mmsl->UpdateObjectVariable(text, SCRIPT_OBJECT_VAR_TEXT_WIDTH);
		mmsl->UpdateObjectVariable(text, SCRIPT_OBJECT_VAR_TEXT_HEIGHT);
		break;
	case SCRIPT_OBJECT_VAR_TEXT_DELETE:
		text->SetText(text->text.Mid(var->GetInteger()));
		mmsl->UpdateObjectVariable(text, SCRIPT_OBJECT_VAR_TEXT_WIDTH);
		mmsl->UpdateObjectVariable(text, SCRIPT_OBJECT_VAR_TEXT_HEIGHT);
		break;
	case SCRIPT_OBJECT_VAR_TEXT_STYLE:
		{
			SPString str = var->GetString();
			if (str.CompareNoCase("underline") == 0)
				text->SetStyle(TEXT_STYLE_UNDERLINE);
			else if (str.CompareNoCase("outline") == 0)
				text->SetStyle(TEXT_STYLE_OUTLINE);
			else
				text->SetStyle(TEXT_STYLE_NORMAL);
		}
		mmsl->UpdateObjectVariable(text, SCRIPT_OBJECT_VAR_TEXT_WIDTH);
		mmsl->UpdateObjectVariable(text, SCRIPT_OBJECT_VAR_TEXT_HEIGHT);
		break;
	case SCRIPT_OBJECT_VAR_TEXT_TEXTALIGN:
		{
			SPString str = var->GetString();
			if (str.CompareNoCase("center") == 0)
				text->SetTextAlign(TEXT_ALIGN_CENTER);
			else if (str.CompareNoCase("right") == 0)
				text->SetTextAlign(TEXT_ALIGN_RIGHT);
			else
				text->SetTextAlign(TEXT_ALIGN_LEFT);
		}
	}
}

////////////////////////////////////////////////////////////////////////

void on_rect_get(int var_id, MmslVariable *var, void *param, int obj_id, MMSL_OBJECT *obj)
{
	if (var == NULL || obj_id != SCRIPT_OBJECT_RECT || obj == NULL)
		return;
	
	Rectangle *rect = (Rectangle *)*obj;
	if (rect == NULL)
		return;

	on_common_get(var_id, var, param, obj_id, obj);

	switch (var_id)
	{
	case SCRIPT_OBJECT_VAR_RECT_COLOR:
		var->Set(rect->color);
		break;
	case SCRIPT_OBJECT_VAR_RECT_BACKCOLOR:
		var->Set(rect->bkcolor);
		break;
	case SCRIPT_OBJECT_VAR_RECT_LINEWIDTH:
		var->Set(rect->linewidth);
		break;
	case SCRIPT_OBJECT_VAR_RECT_ROUND:
		var->Set(rect->round);
		break;
	}
}

void on_rect_set(int var_id, MmslVariable *var, void *param, int obj_id, MMSL_OBJECT *obj)
{
	if (var == NULL || obj_id != SCRIPT_OBJECT_RECT || obj == NULL)
		return;
	
	Rectangle *rect = (Rectangle *)*obj;
	if (rect == NULL)
		return;

	on_common_set(var_id, var, param, obj_id, obj);

	switch (var_id)
	{
	case SCRIPT_OBJECT_VAR_RECT_COLOR:
		rect->SetColor(var->GetInteger());
		break;
	case SCRIPT_OBJECT_VAR_RECT_BACKCOLOR:
		rect->SetBkColor(var->GetInteger());
		break;
	case SCRIPT_OBJECT_VAR_RECT_LINEWIDTH:
		rect->SetLineWidth(var->GetInteger());
		break;
	case SCRIPT_OBJECT_VAR_RECT_ROUND:
		rect->SetRound(var->GetInteger());
		break;
	}
}

