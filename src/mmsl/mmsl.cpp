//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - MMSL interpreter impl.
 *  \file       mmsl/mmsl.cpp
 *  \author     bombur
 *  \version    0.1
 *  \date       4.10.2006
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <libsp/sp_misc.h>
#include <libsp/sp_msg.h>
#include <libsp/sp_memory.h>

#include <mmsl/mmsl.h>
#include <mmsl/mmsl-file.h>

#include <libsp/containers/membin.h>

const int num_spaces_in_tab = 4;

#ifdef WIN32
//#define MMSL_DEBUG
#endif

//#define MMSL_DUMP

Mmsl *mmsl = NULL;

//////////////////////////////
// We combine multiple lists into one global list.
// There macros are wrappers for this list.

#define IDXLIST_NUM(prefix, name) (prefix idx_##name##1 < 0 ? 0 : (prefix idx_##name##2 - prefix idx_##name##1 + 1))
#define IDXLIST_LAST(i, prefix, name) (prefix idx_##name##1 < 0 ? false : (i == prefix idx_##name##2))
#define IDXLIST_FOR(i, prefix, name) for (i = prefix idx_##name##1; i >= 0 && i <= prefix idx_##name##2; i++)
#define IDXLIST_FOR_OFFS(i, prefix, name, start) for (i = prefix idx_##name##1 + start; i >= 0 && i <= prefix idx_##name##2; i++)
#define IDXLIST(prefix, name, idx) (mmsl->name[prefix idx_##name##1 + idx])
#define IDXLISTABS(prefix, name, idx) (mmsl->name[idx])

///////////////////////////////////////////////////////////////////////////////

/// Output MMSL error message to the console.
void mmsl_run_error(const char *text, ...)
{
	va_list args;
	va_start(args, text);
	char *msgbuf = (char *)SPalloca(4096);
	vsprintf(msgbuf, text, args);
	va_end(args);
	msg("Mmsl: %s\n", msgbuf);
}

const int max_varspace = 86016;

Mmsl::Mmsl()
{
	cur_counter = 0;
	need_to_run_events = false;

	objects.SetN(mmsl_num_hash);
}

Mmsl::~Mmsl()
{
	objects.DeleteObjects();
}

int Mmsl::BindVariable(int ID, MmslVariableCallback Get, MmslVariableCallback Set, void *param)
{
	if (ID < 0 || Get == NULL || Set == NULL)
		return -1;
	MmslVariable *var = GetVariable(ID);
	if (var)
	{
		var->get_callback = Get;
		var->set_callback = Set;
		var->param = param;
		var->type = MMSL_VARIABLE_INTEGER;
		return 0;
	}
	return -1;
}

int Mmsl::BindObject(int ID, MmslObjectCallback Add, MmslObjectCallback Delete, void *param)
{
	if (ID < 0 || Add == NULL || Delete == NULL)
		return -1;
	MmslClass *cls = GetClass(ID);
	if (cls)
	{
		cls->add_callback = Add;
		cls->delete_callback = Delete;
		cls->param = param;
		return 0;
	}
	return -1;
}

int Mmsl::BindObjectVariable(int obj_ID, int var_ID,
					MmslVariableCallback Get, MmslVariableCallback Set, void *param)
{
	int i;
	bool was_any = false;
	if (obj_ID < 0 || var_ID < 0 || Get == NULL || Set == NULL)
		return -1;
	for (i = 0; i < vars.GetN(); i++)
	{
		MmslVariable &var = vars[i];

		if (var.obj_id == obj_ID && var.type != MMSL_VARIABLE_OBJECTVAR)
		{
			var.get_callback = Get;
			var.set_callback = Set;
			var.param = param;
			var.obj = NULL;		// will be assigned dynamically for a real variable
			var.type = MMSL_VARIABLE_INTEGER;
			was_any = true;
		}
	}
	return was_any ? 0 : -1;
}


///////////////////////////////////////////////////////////////////////

MmslVariable::MmslVariable()
{
	type = MMSL_VARIABLE_CONST_INTEGER;
	ival = 0;
	sval = NULL;
	get_callback = NULL;
	set_callback = NULL;
	param = NULL;

	obj_id = -1;
	obj = NULL;

	idx_lut1 = idx_lut2 = -1;
	idx_clut1 = idx_clut2 = -1;

	ref = NULL;
	
	ID = -1;
}

void MmslVariable::Set(int value)
{
	// cannot change constants
	if (type == MMSL_VARIABLE_CONST_INTEGER || type == MMSL_VARIABLE_CONST_STRING)
		return;

	if (type == MMSL_VARIABLE_STRING || type == MMSL_VARIABLE_RET_STRING)
		delete_SPString(sval);
	type = MMSL_VARIABLE_INTEGER;
	ival = value;
	//Update(false);
}

void MmslVariable::Set(const SPString & value)
{
	// cannot change constants
	if (type == MMSL_VARIABLE_CONST_INTEGER || type == MMSL_VARIABLE_CONST_STRING)
		return;
	
	if (type == MMSL_VARIABLE_STRING || type == MMSL_VARIABLE_RET_STRING)
	{
		*sval = value;
	} else
	{
		sval = new_SPString(value);
	}
	type = MMSL_VARIABLE_STRING;
	ival = 0;
	//Update(false);
}

void MmslVariable::Set(const MmslVariable & from)
{
	// cannot change constants
	if (type == MMSL_VARIABLE_CONST_INTEGER || type == MMSL_VARIABLE_CONST_STRING)
		return;
	// cannot set from 'objectvars'
	if (type == MMSL_VARIABLE_OBJECTVAR || from.type == MMSL_VARIABLE_OBJECTVAR)
		return;

	if ((from.type & MMSL_VARIABLECLASS_STRING) == MMSL_VARIABLECLASS_STRING)
	{
		if (type == MMSL_VARIABLE_STRING || type == MMSL_VARIABLE_RET_STRING)
		{
			if (sval != NULL)
			{
				if (from.sval == NULL)
					*sval = "";
				else
					*sval = *from.sval;
			}
			else
				sval = (from.sval == NULL) ? new_SPString() : new_SPString(*from.sval);
		} else
			sval = (from.sval == NULL) ? new_SPString() : new_SPString(*from.sval);
		ival = 0;
		type = (from.type == MMSL_VARIABLE_RET_STRING) ? MMSL_VARIABLE_RET_STRING : MMSL_VARIABLE_STRING;
	}
	else if ((from.type & MMSL_VARIABLECLASS_INTEGER) == MMSL_VARIABLECLASS_INTEGER)
	{
		if (type == MMSL_VARIABLE_STRING || type == MMSL_VARIABLE_RET_STRING)
			delete_SPString(sval);

		ival = from.ival;
		sval = NULL;
		type = MMSL_VARIABLE_INTEGER;
	}

	// pass new value for registered vars
	if (ref == NULL)
	{
		if (ID >= 0 && set_callback != NULL)
			set_callback(ID, this, param, obj_id, &obj);
		Update(false);
	}
}

void MmslVariable::Update(bool immediate_trigger)
{
	if (mmsl == NULL)
		return;
	if (type == MMSL_VARIABLE_OBJECTVAR)
		return;
	
	// UpdateTriggers()
	int i, num_e = mmsl->events.GetN();
	for (i = idx_lut1; i >= 0 && i <= idx_lut2; i++)
	{
		int event_idx = mmsl->lut[i];
		if (event_idx < 0 || event_idx >= num_e)
			continue;

		mmsl->Evaluate(mmsl->events[event_idx].trigger);
	}
	
	for (i = idx_lut1; i >= 0 && i <= idx_lut2; i++)
	{
		mmsl->TriggerEvent(mmsl->lut[i], immediate_trigger);
	}
}

int MmslVariable::GetInteger()
{
	if ((type & MMSL_VARIABLECLASS_INTEGER) == MMSL_VARIABLECLASS_INTEGER)
		return ival;
	if ((type & MMSL_VARIABLECLASS_STRING) == MMSL_VARIABLECLASS_STRING)
	{
		if (sval != NULL)
			return atol(*sval);
	}
	return 0;
}

SPString &MmslVariable::GetString()
{
	if ((type & MMSL_VARIABLECLASS_STRING) == MMSL_VARIABLECLASS_STRING)
		return *sval;
	static SPString str;
	str.FromInteger(ival);
	return str;
}

////////////////////////////////////////////////////

MmslObject::~MmslObject()
{
	// call 'on_delete'
	if (class_idx >= 0 && class_idx < mmsl->classes.GetN())
	{
		MmslClass *cls = &mmsl->classes[class_idx];
		if (cls->delete_callback != NULL)
			cls->delete_callback(cls->ID, &obj, cls->param);
	}
}

////////////////////////////////////////////////////

MmslVariable *Mmsl::GetVariable(int ID)
{
	if (ID >= 0 && ID < idvars.GetN())
	{
		return &vars[idvars[ID]];
	}
	return NULL;
}

MmslClass *Mmsl::GetClass(int ID)
{
	if (ID >= 0 && ID < idclasses.GetN())
	{
		return &classes[idclasses[ID]];
	}
	return NULL;
}

BOOL Mmsl::UpdateVariable(int ID)
{
	MmslVariable *ret = GetVariable(ID);
	if (ret != NULL)
	{
		ret->Update(false);
		return TRUE;
	}
	return FALSE;
}

BOOL Mmsl::UpdateObjectVariable(MMSL_OBJECT obj, int ID)
{
	MmslObject tmp, *ret;
	tmp.obj = obj;
	ret = objects.Get(tmp);
	if (ret != NULL && ret->assigned_obj != NULL && ret->class_idx >= 0 && ret->class_idx < classes.GetN())
	{
		MmslClass &c = classes[ret->class_idx];
		if (ID >= 0 && ID < IDXLIST_NUM(c., idobjvars))
		{
			int vi = IDXLIST(c., idobjvars, ID);
			if (vi >= 0 && vi < IDXLIST_NUM(ret->assigned_obj->, assigned_vars))
			{
				int vidx = IDXLIST(ret->assigned_obj->, assigned_vars, vi);
				if (vidx >= 0 && vidx < vars.GetN())
				{
					vars[vidx].Update(false);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

void Mmsl::TriggerEvent(int event_idx, bool immediate_trigger)
{
	if (event_idx < 0 || event_idx >= events.GetN())
		return;
	int i;
	MmslEvent &e = events[event_idx];

	// we don't care if there's nothing to execute...
	if (IDXLIST_NUM(e., execute) < 1)
		return;
	
	// check additional conditions
	IDXLIST_FOR(i, e., conditions)
	{
		int idx = IDXLISTABS(e., conditions, i);
		if (idx < 0 || idx >= events.GetN())
			continue;

#ifdef MMSL_DEBUG
		if (events[idx].trigger.value >= 0)
		{
			int oldval = events[idx].trigger.value;

			Evaluate(events[idx].trigger);
			if (events[idx].trigger.value != oldval)
			{
				//MmslFile *oldcurfile = curfile;
				//curfile = &events[idx].trigger.pos;
				
				mmsl_run_error("Trigger expression mismatch for event %d!", idx);
				
				//curfile = oldcurfile;
				
				events[idx].trigger.value = -1;
			}
		}
#endif
		if (events[idx].trigger.value == 0 || (events[idx].trigger.value < 0 && !Evaluate(events[idx].trigger)))
			return;
	}
	
	// check trigger condition
	if (e.trigger.value == 1 || (e.trigger.value < 0 && Evaluate(e.trigger)))
	{
		if (!immediate_trigger && e.counter >= cur_counter)
		{
			need_to_run_events = true;
			e.counter = cur_counter + 1;
			return;
		}
		e.counter = cur_counter;
		IDXLIST_FOR(i, e., execute)
		{
			Evaluate(IDXLISTABS(e., execute, i));
		}
		if (e.counter <= cur_counter)
			e.counter = 0;
	}
}

bool Mmsl::Evaluate(MmslExpression &e)
{
	e.value = 0;
	if (IDXLIST_NUM(e., tokens) < 1)
		return false;
	if (IDXLIST(e., tokens, 0) == MMSL_CONST_TRUE)
	{
		e.value = 1;
		return true;
	}
	if (IDXLIST(e., tokens, 0) == MMSL_OPERATOR_ADD)
	{
		// tokens[1] = type (index into classes)
		if (IDXLIST(e., tokens, 1) < 0 || IDXLIST(e., tokens, 1) > classes.GetN())
			return false;
		// tokens[2] = name (index into assigned_objs)
		if (IDXLIST(e., tokens, 2) < 0 || IDXLIST(e., tokens, 2) > assigned_objs.GetN())
			return false;
		MmslAssignedObject *an = &assigned_objs[IDXLIST(e., tokens, 2)];
		if (an == NULL)
			return false;
		// remove old object reference
		if (an->obj != NULL)
			an->obj->assigned_obj = NULL;
		int index = IDXLIST(e., tokens, 1);
		MmslClass *cls = &classes[index];
		if (cls == NULL || cls->add_callback == NULL)
			return false;
		MMSL_OBJECT obj = NULL;
		cls->add_callback(cls->ID, &obj, cls->param);
		MmslObject *newobj = new MmslObject();
		if (newobj == NULL)
			return false;
		newobj->class_idx = index;
		newobj->obj = obj;
		// set new object reference
		newobj->assigned_obj = an;
		an->obj = newobj;
		int i;
		IDXLIST_FOR(i, an->, assigned_vars)
		{
			int vi = IDXLISTABS(an->, assigned_vars, i);
			if (vi >= 0 && vi < vars.GetN())
			{
				vars[vi].obj = obj;
			}
		}
		objects.Add(newobj);
		return true;
	}
	if (IDXLIST(e., tokens, 0) == MMSL_OPERATOR_DELETE)
	{
		MmslObject *cur = objects.GetFirst(), *next = NULL;
		bool was_del = false;
		while (cur != NULL)
		{
			next = objects.GetNext(*cur);
			if (EvaluateMath(e, 1, cur, true))
			{
				// unset object variables
				if (cur->assigned_obj != NULL)
				{
					cur->assigned_obj->obj = NULL;
					int i;
					IDXLIST_FOR(i, cur->assigned_obj->, assigned_vars)
					{
						int vi = IDXLISTABS(cur->assigned_obj->, assigned_vars, i);
						if (vi > 0 && vi < vars.GetN())
						{
							vars[vi].obj = NULL;
						}
					}
				}
				objects.Delete(*cur);
				was_del = true;
			}
			cur = next;
		}
		return was_del;
	}
	e.value = EvaluateMath(e, 0, NULL) != 0 ? 1 : 0;
	return e.value != 0;
}


int Mmsl::EvaluateMath(const MmslExpression & e, int start, MmslObject *obj, bool del)
{
	//msg("----------------- EVAL %d %d\n", e.pos.row, e.pos.col);

	const int max_vars_in_stack = 64;
	/// this is a bit dirty way, because ctor is not called, but it's ok
	MmslVariable *var_stack = (MmslVariable *)SPalloca(max_vars_in_stack * sizeof(MmslVariable));
	if (var_stack == NULL)
	{
		mmsl_run_error("Cannot allocate variable stack.");
		return 0;
	}
	int i, var_stack_idx = -1;

	// for correct error/debug report
	bool was_err = false;
#ifdef MMSL_DEBUG
	//curfile = (MmslFile *)&e.pos;
#endif

	IDXLIST_FOR_OFFS(i, e., tokens, start)
	{
		int tokens_i = IDXLISTABS(e., tokens, i);
		if (tokens_i > MMSL_OPERATOR_UNKNOWN1 && tokens_i < MMSL_OPERATOR_UNKNOWN2)	// operator
		{
			if (var_stack_idx < 1)
			{
				mmsl_run_error("Cannot evaluate expression (operands expected).");
				was_err = true;
				break;
			}
			MMSL_OPERATOR_TYPE op = (MMSL_OPERATOR_TYPE)tokens_i;
			MmslVariable *ret = Calc(op, var_stack[var_stack_idx - 1].ref,
										var_stack[var_stack_idx].ref, IDXLIST_LAST(i, e., tokens));
			if (ret == NULL)
			{
				was_err = true;
				break;
			}

			// clean-up top-stack var
			if (var_stack[var_stack_idx].type == MMSL_VARIABLE_STRING
				 || var_stack[var_stack_idx].type == MMSL_VARIABLE_RET_STRING)
				delete_SPString(var_stack[var_stack_idx].sval);
			var_stack[--var_stack_idx].Set(*ret);
			var_stack[var_stack_idx].ref = &var_stack[var_stack_idx];
		} else
		{
			if (tokens_i >= 0 && tokens_i < vars.GetN())
			{
				MmslVariable *v = &vars[tokens_i];
				// evaluate abstract object variable using given object idx
				if (v->type == MMSL_VARIABLE_OBJECTVAR && obj != NULL)
				{
					if (obj->class_idx < 0 || obj->class_idx >= IDXLIST_NUM(v->, clut))
					{
						mmsl_run_error("Cannot find object variable.");
						was_err = true;
						break;
					}
					
					int vidx = IDXLIST(v->, clut, obj->class_idx);
					if (vidx < 0 || vidx >= vars.GetN())
					{
						// for delete condition it's normal situation
						if (del)
							return 0;
						mmsl_run_error("Cannot find object variable.");
						was_err = true;
						break;
					}
					//v = &IDXLIST(classes[obj->class_idx]., vars, vidx);
					v = &vars[vidx];
				}
				// get current value of the variable or object's variable
				// TODO: l-values are also queried - it's no good...
				if (v->type == MMSL_VARIABLE_INTEGER || v->type == MMSL_VARIABLE_STRING ||
					v->type == MMSL_VARIABLE_OBJECT_INTEGER || v->type == MMSL_VARIABLE_OBJECT_STRING)
				{
					if (v->get_callback != NULL)
						v->get_callback(v->ID, v, v->param, v->obj_id, obj != NULL ? &obj->obj : &v->obj);
				}
				if (var_stack_idx >= max_vars_in_stack)
				{
					mmsl_run_error("Too long expression, stack overflow.");
					return 0;
				}
				++var_stack_idx;
				var_stack[var_stack_idx].type = MMSL_VARIABLE_INTEGER;
				var_stack[var_stack_idx].ref = v;
				var_stack[var_stack_idx].ID = -1;
			}
		}
	}

	// stack vars clean-up
	for (i = 0; i <= var_stack_idx; i++)
	{
		if (var_stack[i].type == MMSL_VARIABLE_STRING || 
			var_stack[i].type == MMSL_VARIABLE_RET_STRING)
			delete_SPString(var_stack[i].sval);
	}

#ifdef MMSL_DEBUG	
	//curfile = NULL;
#endif

	if (was_err)
		return 0;
	if (var_stack_idx >= 0)
	{
		return var_stack[var_stack_idx].ival;
	}
	return 0;
}

MmslVariable *Mmsl::Calc(MMSL_OPERATOR_TYPE op, MmslVariable *arg1, MmslVariable *arg2, bool last_op)
{
	static MmslVariable ret;
	
	// ret is static so we need to clean-up
	delete_SPString(ret.sval);
	ret.ival = 0;
	
	if (arg1 == NULL || arg2 == NULL)
	{
		mmsl_run_error("Wrong arithm. arguments.");
		return NULL;
	}

	int d;
	switch (op)
	{
	///////////////////////////////////////////////////
	// arithmetic:

	case MMSL_OPERATOR_MINUS:
		ret.type = MMSL_VARIABLE_INTEGER;
		ret.ival = arg1->GetInteger() - arg2->GetInteger();
		break;
	case MMSL_OPERATOR_MUL:
		ret.type = MMSL_VARIABLE_INTEGER;
		ret.ival = arg1->GetInteger() * arg2->GetInteger();
		break;
	case MMSL_OPERATOR_DIV:
		ret.type = MMSL_VARIABLE_INTEGER;
		d = arg2->GetInteger();
		if (d == 0)
		{
			mmsl_run_error("Division by zero.");
			break;
		}
		ret.ival = arg1->GetInteger() / d;
		break;
	case MMSL_OPERATOR_MOD:
		ret.type = MMSL_VARIABLE_INTEGER;
		d = arg2->GetInteger();
		if (d == 0)
		{
			mmsl_run_error("Modulus Division by zero.");
			break;
		}
		ret.ival = arg1->GetInteger() % d;
		break;

	///////////////////////////////////////////////////
	// comparison:

	case MMSL_OPERATOR_GT:
		ret.type = MMSL_VARIABLE_INTEGER;
		ret.ival = arg1->GetInteger() > arg2->GetInteger();
		break;
	case MMSL_OPERATOR_LT:
		ret.type = MMSL_VARIABLE_INTEGER;
		ret.ival = arg1->GetInteger() < arg2->GetInteger();
		break;
	case MMSL_OPERATOR_GTEQU:
		ret.type = MMSL_VARIABLE_INTEGER;
		ret.ival = arg1->GetInteger() >= arg2->GetInteger();
		break;
	case MMSL_OPERATOR_LTEQU:
		ret.type = MMSL_VARIABLE_INTEGER;
		ret.ival = arg1->GetInteger() <= arg2->GetInteger();
		break;
	case MMSL_OPERATOR_OROR:
		ret.type = MMSL_VARIABLE_INTEGER;
		ret.ival = arg1->GetInteger() || arg2->GetInteger();
		break;
	case MMSL_OPERATOR_ANDAND:
		ret.type = MMSL_VARIABLE_INTEGER;
		ret.ival = arg1->GetInteger() && arg2->GetInteger();
		break;

	///////////////////////////////////////////////////
	// special:
	case MMSL_OPERATOR_PLUS:
		if (arg1->type == MMSL_VARIABLE_CONST_INTEGER || arg2->type == MMSL_VARIABLE_CONST_INTEGER)
		{
			ret.type = MMSL_VARIABLE_INTEGER;
			ret.ival = arg1->GetInteger() + arg2->GetInteger();
		} 
		else if (arg1->type == MMSL_VARIABLE_CONST_STRING || arg2->type == MMSL_VARIABLE_CONST_STRING
			|| arg1->type == MMSL_VARIABLE_RET_STRING || arg2->type == MMSL_VARIABLE_RET_STRING)
		{
			ret.type = MMSL_VARIABLE_RET_STRING;
			ret.sval = new_SPString(arg1->GetString());
			*ret.sval += arg2->GetString();
		}
		else if (arg1->type == MMSL_VARIABLE_INTEGER || arg2->type == MMSL_VARIABLE_INTEGER)
		{
			ret.type = MMSL_VARIABLE_INTEGER;
			ret.ival = arg1->GetInteger() + arg2->GetInteger();
		} 
		else 
		{
			ret.type = MMSL_VARIABLE_RET_STRING;
			ret.sval = new_SPString(arg1->GetString());
			*ret.sval += arg2->GetString();
		}
		break;

	case MMSL_OPERATOR_EQUEQU:
		ret.type = MMSL_VARIABLE_INTEGER;
		if (arg1->type == MMSL_VARIABLE_CONST_INTEGER || arg2->type == MMSL_VARIABLE_CONST_INTEGER)
			ret.ival = arg1->GetInteger() == arg2->GetInteger();
		else if (arg1->type == MMSL_VARIABLE_CONST_STRING || arg2->type == MMSL_VARIABLE_CONST_STRING)
			ret.ival = arg1->GetString().CompareNoCase(arg2->GetString()) == 0;
		else if (arg1->type == MMSL_VARIABLE_INTEGER || arg2->type == MMSL_VARIABLE_INTEGER)
			ret.ival = arg1->GetInteger() == arg2->GetInteger();
		else 
			ret.ival = arg1->GetString().CompareNoCase(arg2->GetString()) == 0;
		break;

	case MMSL_OPERATOR_NONEQU:
		ret.type = MMSL_VARIABLE_INTEGER;
		if (arg1->type == MMSL_VARIABLE_CONST_INTEGER || arg2->type == MMSL_VARIABLE_CONST_INTEGER)
			ret.ival = arg1->GetInteger() != arg2->GetInteger();
		else if (arg1->type == MMSL_VARIABLE_CONST_STRING || arg2->type == MMSL_VARIABLE_CONST_STRING)
			ret.ival = arg1->GetString().CompareNoCase(arg2->GetString()) != 0;
		else if (arg1->type == MMSL_VARIABLE_INTEGER || arg2->type == MMSL_VARIABLE_INTEGER)
			ret.ival = arg1->GetInteger() != arg2->GetInteger();
		else 
			ret.ival = arg1->GetString().CompareNoCase(arg2->GetString()) != 0;
		break;

	case MMSL_OPERATOR_EQU:
		{
			arg1->Set(*arg2);
			// we're not in conditional, and not need the result
			if (!last_op)
				ret.Set(*arg2);
		}
		break;
	default:
		break;
	}

	return &ret;
}

BOOL Mmsl::Run()
{
	bool more_events = false;

	cur_counter++;
	
	if (cur_counter == 1) // startup
	{
		TriggerEvent(0, false);
	}
	
	if (need_to_run_events)
	{
		need_to_run_events = false;
		for (int i = 1; i < events.GetN(); i++)
		{
			if (events[i].counter >= cur_counter)
			{
				events[i].counter = 0;
				if (IDXLIST_NUM(events[i]., execute) > 0)
				{
					TriggerEvent(i, false);
					more_events = true;
				}
			}
		}
	}
	return more_events;
}

//////////////////////////////////////////////////

BOOL Mmsl::Load(BYTE *buf, int buflen)
{
	/// \WARNING: This code is unprotected from corrupted data!

	int i;
	BYTE *buf0 = buf;
	MmslSerHeader *hdr = (MmslSerHeader *)buf;
	buf += sizeof(MmslSerHeader);
	vars.SetN(hdr->num_var);
	classes.SetN(hdr->num_class);
	events.SetN(hdr->num_event);
	assigned_objs.SetN(hdr->num_assignedobj);
	execute.SetN(hdr->num_expr);
	lut.SetN(hdr->num_lut);
	clut.SetN(hdr->num_clut);
	assigned_vars.SetN(hdr->num_assignedvar);
	conditions.SetN(hdr->num_cond);
	tokens.SetN(hdr->num_token);

	BYTE *serbuf0 = (BYTE *)buf;
	buf += hdr->var_buf_size;
	MmslSerClass *serclass = (MmslSerClass *)buf;
	buf += sizeof(MmslSerClass) * classes.GetN();
	MmslSerEvent *serevent = (MmslSerEvent *)buf;
	buf += sizeof(MmslSerEvent) * events.GetN();
	MmslSerAssignedObject *serassignedobj = (MmslSerAssignedObject *)buf;
	buf += sizeof(MmslSerAssignedObject) * assigned_objs.GetN();
	MmslSerExpression *serexpr = (MmslSerExpression *)buf;
	buf += sizeof(MmslSerExpression) * execute.GetN();
	short *serlut = (short *)buf;
	buf += sizeof(short) * lut.GetN();
	short *serclut = (short *)buf;
	buf += sizeof(short) * clut.GetN();
	short *serassignedvars = (short *)buf;
	buf += sizeof(short) * assigned_vars.GetN();
	short *sercond = (short *)buf;
	buf += sizeof(short) * conditions.GetN();
	short *sertoken = (short *)buf;
	buf += sizeof(short) * tokens.GetN();
	char *str_buf = (char *)buf;

	if (buflen < (int)(buf - buf0))
	{
		msg("Mmsl: It seems that MMSO buffer is damaged!\n");
	}

	BYTE *serbuf = serbuf0;
	for (i = 0; i < vars.GetN(); i++)
	{
		vars[i].type = (MMSL_VARIABLE_TYPE)ReadChar(serbuf);

		if (vars[i].type & MMSL_VARIABLE_HAS_ID)
			vars[i].ID = ReadShort(serbuf);
		if (vars[i].type & MMSL_VARIABLE_HAS_LUT)
		{
			vars[i].idx_lut1 = ReadShort(serbuf);
			vars[i].idx_lut2 = ReadShort(serbuf);
		}
		if (vars[i].type & MMSL_VARIABLE_HAS_CLUT)
		{
			vars[i].idx_clut1 = ReadChar(serbuf);
			vars[i].idx_clut2 = ReadChar(serbuf);
		}
		if (vars[i].type & MMSL_VARIABLE_HAS_OBJID)
			vars[i].obj_id = ReadChar(serbuf);

		vars[i].type = (MMSL_VARIABLE_TYPE)(vars[i].type & MMSL_VARIABLE_MASK);

		int val = ReadInt(serbuf);
		if (vars[i].type == MMSL_VARIABLE_CONST_STRING)
		{
			vars[i].ival = 0;
			vars[i].sval = (val >= 0) ? new_SPString(str_buf + val) : new_SPString();
		} else
		{
			vars[i].ival = val;
			vars[i].sval = NULL;
		}
	}

	if (PAD_EVEN(serbuf - serbuf0) != hdr->var_buf_size)
	{
		msg("Mmsl: It seems that MMSO var.buffer is damaged!\n");
	}

	int max_ID = 0;
	for (i = 0; i < vars.GetN(); i++)
	{
		if (vars[i].ID > max_ID)
			max_ID = vars[i].ID;
	}
	idvars.SetN(max_ID + 1);
	for (i = 0; i <= max_ID; i++)
		idvars[i] = -1;

	SPList<int> num_idobjvars;
	int max_num_idobjvars = 0;
	num_idobjvars.SetN(classes.GetN());
	for (i = 0; i < classes.GetN(); i++)
		num_idobjvars[i] = 0;
	for (i = 0; i < vars.GetN(); i++)
	{
		if (vars[i].obj_id >= 0 && vars[i].obj_id < num_idobjvars.GetN())
		{
			num_idobjvars[vars[i].obj_id]++;
			if (max_num_idobjvars < vars[i].ID + 1)
				max_num_idobjvars = vars[i].ID + 1;
		}
		if (vars[i].ID >= 0 && vars[i].obj_id < 0)
			idvars[vars[i].ID] = i;
	}

	max_ID = 0;
	for (i = 0; i < classes.GetN(); i++)
	{
		if (serclass[i].ID > max_ID)
			max_ID = serclass[i].ID;
	}
	idclasses.SetN(max_ID + 1);
	for (i = 0; i <= max_ID; i++)
		idclasses[i] = -1;

	idobjvars.SetN(max_num_idobjvars * classes.GetN());
	for (i = 0; i < idobjvars.GetN(); i++)
		idobjvars[i] = -1;

	for (i = 0; i < classes.GetN(); i++)
	{
		classes[i].ID = serclass[i].ID;
		classes[i].idx_vars1 = serclass[i].idx_vars1;
		classes[i].idx_vars2 = serclass[i].idx_vars2;

		if (classes[i].ID >= 0)
			idclasses[classes[i].ID] = i;

		classes[i].idx_idobjvars1 = i * max_num_idobjvars;
		classes[i].idx_idobjvars2 = (i + 1) * max_num_idobjvars - 1;

		int local_idx = 0;
		for (int j = 0; j < vars.GetN(); j++)
		{
			if (vars[j].obj_id == classes[i].ID)
			{
				int k = classes[i].idx_idobjvars1 + vars[j].ID;
				if (idobjvars[k] == -1)
					idobjvars[k] = local_idx++;
			}
		}
	}
	
	for (i = 0; i < events.GetN(); i++)
	{
		events[i].trigger.idx_tokens1 = serevent[i].idx_triggertokens1;
		events[i].trigger.idx_tokens2 = serevent[i].idx_triggertokens2;
		events[i].idx_conditions1 = serevent[i].idx_conditions1;
		events[i].idx_conditions2 = serevent[i].idx_conditions2;
		events[i].idx_execute1 = serevent[i].idx_execute1;
		events[i].idx_execute2 = serevent[i].idx_execute2;
	}

	for (i = 0; i < assigned_objs.GetN(); i++)
	{
		assigned_objs[i].idx_assigned_vars1 = serassignedobj[i].idx_assigned_vars1;
		assigned_objs[i].idx_assigned_vars2 = serassignedobj[i].idx_assigned_vars2;
	}

	for (i = 0; i < execute.GetN(); i++)
	{
		execute[i].idx_tokens1 = serexpr[i].idx_tokens1;
		execute[i].idx_tokens2 = serexpr[i].idx_tokens2;
	}

	for (i = 0; i < lut.GetN(); i++)
		lut[i] = serlut[i];
	for (i = 0; i < clut.GetN(); i++)
		clut[i] = serclut[i];
	for (i = 0; i < assigned_vars.GetN(); i++)
		assigned_vars[i] = serassignedvars[i];
	for (i = 0; i < conditions.GetN(); i++)
		conditions[i] = sercond[i];
	for (i = 0; i < tokens.GetN(); i++)
		tokens[i] = sertoken[i];

	msg("Mmsl Loaded: %d vars, %d cls, %d evnts, %d exec, %d asno, %d asnv\n",
			vars.GetN(), classes.GetN(), events.GetN(), execute.GetN(), assigned_objs.GetN(), assigned_vars.GetN());
	msg("             %d conds, %d tokens, %d lut, %d clut\n", 
			conditions.GetN(), tokens.GetN(), lut.GetN(), clut.GetN());
	msg("             %d idv, %d ido, %d idc\n", 
		idvars.GetN(), idobjvars.GetN(), idclasses.GetN());

	return TRUE;
}

int Mmsl::ReadChar(BYTE* &buf)
{
	return (int)(*buf++);
}

int Mmsl::ReadShort(BYTE* &buf)
{
	int v = (buf[1] << 8) | buf[0];
	buf += 2;
	return v;
}

int Mmsl::ReadInt(BYTE* &buf)
{
	int v = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	buf += 4;
	return v;
}

#ifdef MMSL_DUMP

void Mmsl::Dump()
{
	int i;
	for (i = 0; i < vars.GetN(); i++)
	{
		msg("V[%d] %d %d, %d \"%s\" %d %d %d %d, %d, %c %c\n",
			i, vars[i].type, vars[i].ID, vars[i].ival, (vars[i].sval ? **vars[i].sval : ""),
			vars[i].idx_lut1, vars[i].idx_lut2, vars[i].idx_clut1, vars[i].idx_clut2,
			vars[i].obj_id,
			(vars[i].get_callback ? 'G' : '.'), (vars[i].set_callback ? 'S' : '.'));
	}

	msg("\n\n-----------------------------------\n\n");

	for (i = 0; i < classes.GetN(); i++)
	{
		msg("C[%d] %d, %d %d %d %d, %c %c\n",
			i, classes[i].ID, 
			classes[i].idx_idobjvars1, classes[i].idx_idobjvars2, classes[i].idx_vars1, classes[i].idx_vars2,
			classes[i].add_callback ? 'A' : '.', classes[i].delete_callback ? 'S' : '.');
		
	}

	msg("\n\n-----------------------------------\n\n");

	for (i = 0; i < events.GetN(); i++)
	{
		msg("E[%d] %d %d, %d %d, %d %d\n",
			i, events[i].trigger.idx_tokens1, events[i].trigger.idx_tokens2, 
			events[i].idx_conditions1, events[i].idx_conditions2,
			events[i].idx_execute1, events[i].idx_execute2);
	}

	msg("\n\n-----------------------------------\n\n");

	for (i = 0; i < execute.GetN(); i++)
	{
		msg("X[%d] %d %d\n",
			i, execute[i].idx_tokens1, execute[i].idx_tokens2);
	}

	msg("\n\n-----------------------------------\n\n");

	for (i = 0; i < assigned_objs.GetN(); i++)
	{
		msg("A[%d] %d %d\n",
			i, assigned_objs[i].idx_assigned_vars1, assigned_objs[i].idx_assigned_vars2);
	}

	DumpIntList("AV", assigned_vars);
	DumpIntList("CO", conditions);
	DumpIntList("TO", tokens);
	DumpIntList("LU", lut);
	DumpIntList("CL", clut);
	DumpIntList("IV", idvars);
	DumpIntList("IO", idobjvars);
	DumpIntList("IC", idclasses);

}

template<typename L>
void Mmsl::DumpIntList(const char *n, const L &list)
{
	int i;
	msg("\n\n-----------------------------------\n\n");
	for (i = 0; i < list.GetN() / 16 * 16; i+=16)
	{
		msg("%s[%d] %4d %4d %4d %4d %4d %4d %4d %4d  %4d %4d %4d %4d %4d %4d %4d %4d\n", n, i, 
			list[i+0], list[i+1], list[i+2], list[i+3], list[i+4], list[i+5], list[i+6], list[i+7],
			list[i+8], list[i+9], list[i+10], list[i+11], list[i+12], list[i+13], list[i+14], list[i+15]);
	}
	for (; i < list.GetN(); i++)
	{
		msg("%s[%d] %d\n", n, i, list[i]);
	}
}

#endif

//////////////////////////////////////////////////

SPString *new_SPString()
{
	return new SPString();
}

SPString *new_SPString(const SPString & s)
{
	return new SPString(s);
}

void delete_SPString(SPString * & s)
{
	SPSafeDelete(s);
	s = NULL;
}
