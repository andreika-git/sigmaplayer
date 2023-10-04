//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - MMSL interpreter impl.
 *  \file       mmsl/mmsl-parser.cpp
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

#include <mmsl/mmsl-parser.h>
#include <mmsl/mmsl-file.h>

#include <libsp/containers/membin.h>



#define USE_CONSTANTS_OPTIMISATION


const int num_spaces_in_tab = 4;

MmslParser *mmsl_parser = NULL;


//////////////////////////////
// We combine multiple lists into one global list.
// There macros are wrappers for this list.

#define IDXLIST_ADD(prefix, name, value) { if (prefix idx_##name##1 < 0)		\
		prefix idx_##name##1 = prefix idx_##name##2 = mmsl_parser->name.Add(value);	\
		else mmsl_parser->name.Insert(value, ++prefix idx_##name##2); }
#define IDXLIST_ADD_SHIFT(list,P, i, name, value) { if (list[i] P idx_##name##1 < 0)		\
		list[i] P idx_##name##1 = list[i] P idx_##name##2 = mmsl_parser->name.Add(value);	\
		else { mmsl_parser->name.Insert(value, ++list[i] P idx_##name##2); \
		for (int __i = 0; __i < list.GetN(); __i++) { \
		if (__i == i) continue; \
		if (list[__i] P idx_##name##1 >= list[i] P idx_##name##2) list[__i] P idx_##name##1++; \
		if (list[__i] P idx_##name##2 < 0) list[__i] P idx_##name##2 = list[__i] P idx_##name##1; \
		else if (list[__i] P idx_##name##2 >= list[i] P idx_##name##2) list[__i] P idx_##name##2++; \
		} } }
#define IDXLIST_MERGE_SHIFTPTR(list, i, name, value) { if (mmsl_parser->name.Get(value, list[i]->idx_##name##1, list[i]->idx_##name##2) == -1) { \
		IDXLIST_ADD_SHIFT(list,->,i,name,value); } }

#define IDXLIST_PUT(prefix, name, value, idx) { mmsl_parser->name[prefix idx_##name##1 + idx] = value; }
#define IDXLIST_NUM(prefix, name) (prefix idx_##name##1 < 0 ? 0 : (prefix idx_##name##2 - prefix idx_##name##1 + 1))

///////////////////////////////////////////////////////////////////////////////

/// Output MMSL error message to the console.
void mmsl_error(const char *text, ...)
{
	va_list args;
	va_start(args, text);
	char *msgbuf = (char *)SPalloca(4096);
	vsprintf(msgbuf, text, args);
	va_end(args);
	if (mmsl != NULL)
	{
		if (mmsl_parser->curfile != NULL && mmsl_parser->curfile->row > 0)
		{
			msg("Mmsl (%s:%d,%d): %s\n", *mmsl_parser->curfile->fname, mmsl_parser->curfile->row, mmsl_parser->curfile->col, msgbuf);
			return;
		}
	}
	msg("Mmsl: %s\n", msgbuf);
}

const int max_varspace = 130016;

MmslParser::MmslParser()
{
	varscache_used = 0;
	varscache = new SPClassicList<MmslParserVariable>(4000);

	varspace = new char [max_varspace+256];	// 256 is for safety
	maxvaridx = 0;

	pvars.l.Reserve(4600);
	events.Reserve(900);
	assigned_objs.Reserve(100);
	lut.Reserve(1000);
	clut.Reserve(100);
	assigned_vars.Reserve(1400);
	conditions.Reserve(1500);
	tokens.Reserve(12000);
	execute.Reserve(2200);
	
	parse_events_stack.Reserve(100);
	parse_events_stack_idx = -1;
	
	token_stack.Reserve(256);
	token_stack_idx = -1;

	last_data = NULL;

	curfile = NULL;

	was_overflow = false;
}

MmslParser::~MmslParser()
{
	SPSafeDeleteArray(varspace);

	pvars.hash.Clear();
	for (int i = 0; i < pvars.l.GetN(); i++)
	{
		if (pvars.l[i]->temporary)
		{
			SPSafeDelete(pvars.l[i]);
		}
		else
			pvars.l[i] = NULL;
	}
	
	pclasses.hash.DeleteObjects();

#ifdef USE_CONSTANTS_OPTIMISATION
	pconstsvars.hash.DeleteObjects();
#endif
	
	SPSafeDelete(varscache);
}

char *MmslParser::LockVariableName()
{
	// overflow
	if (maxvaridx >= max_varspace)
	{
		msg_critical("Error! Variable space overflow.\n");
		return NULL;
	}
	return varspace + maxvaridx;
}

void MmslParser::UnlockVariableName(int len)
{
	maxvaridx += len;
}

MmslToken MmslParser::GetNextToken(char * &data)
{
	static bool was_newline = true, was_realnewline = true, was_semicolon = false;
	static int last_indent = -1;
	static char *last_newline = NULL;
	static int num_spaces = 0, num_tabs = 0;
	static MmslFile *curf = NULL;
	static struct  
	{
		MMSL_TOKEN_TYPE type;
		const char *name;
	} lexemes[] = 
	{
		{ MMSL_TOKEN_BREAK, "break" },
		{ MMSL_TOKEN_DELETE, "delete" },
		{ MMSL_TOKEN_INCLUDE, "include" },
		{ MMSL_TOKEN_ON, "on" },
		{ MMSL_TOKEN_ADD, "add" },
		{ MMSL_TOKEN_UNDEFINED, NULL },
	};
	int lexeme_idx = -1, i = 0, j;
	int ignore_sym = false;
	bool hex_number = false;

	if (curfile != NULL && curfile != curf)
	{
		curf = curfile;
		if (curfile->row == 0)
		{
			was_newline = true;
			was_realnewline = true;
			last_newline = NULL;
		} else
		{
			was_newline = false;
			was_realnewline = false;
			last_newline = data;	// wrong but who cares...
		}
		was_semicolon = false;
		last_indent = -1;
		num_spaces = 0;
		num_tabs = 0;
	}

	last_data = data;

	if (last_newline == NULL || was_realnewline)
	{
		last_newline = data;
		AddRow();
	}
	MmslToken token;
	token.type = MMSL_TOKEN_UNDEFINED;
	token.ival = 0;
	
	while (*data != '\0')
	{
cont:
		// comments
		if (*data == '/')
		{
			if (data[1] == '/')
			{
				data+= 2;
				while (*data != '\n' && *data != '\0')
					data++;
				continue;
			}
			else if (data[1] == '*')
			{
				data += 2;
				while ((*data != '*' || data[1] != '/') && *data != '\0')
				{
					if (*data == '\n')
					{
						AddRow();
						last_newline = data + 1;
					}
					data++;
				}
				if (*data != '\0')
					data += 2;
				continue;
			}
		}
		if (was_newline)
		{
			if (*data == ' ')
				num_spaces++;
			else if (*data == '\t')
				num_tabs++;
			else if (*data != '\r' && *data != '\n')
			{
				token.type = MMSL_TOKEN_INDENT;
				token.ival = was_semicolon ? last_indent : num_tabs + num_spaces / num_spaces_in_tab;
				last_indent = token.ival;
				
				was_semicolon = false;
				was_newline = false;
				was_realnewline = false;
				return token;
			}
		}
		switch (token.type)
		{
		case MMSL_TOKEN_UNDEFINED:
			// IDs and built-in commands
			if (isalpha(*data) || *data == '.')
			{
				SetCol(data - last_newline + 1, num_tabs);
				token.sval = LockVariableName();
				if (token.sval == NULL)
				{
					token.type = MMSL_TOKEN_EOF;
					return token;
				}
				token.sval[0] = (char)tolower(*data);
				for (j = 0; lexemes[j].name != NULL; j++)
				{
					if (lexemes[j].name[0] == token.sval[0])
					{
						lexeme_idx = j;
						break;
					}
				}
				token.type = MMSL_TOKEN_ID;
				i = 1;
			}
			// numbers
			else if (isdigit(*data))
			{
				SetCol(data - last_newline + 1, num_tabs);
				token.type = MMSL_TOKEN_NUMBER;
				hex_number = false;
				token.ival = (int)(*data) - '0';
			}
			// strings
			else if (*data == '\"' || *data == '\'')
			{
				SetCol(data - last_newline + 1, num_tabs);
				token.type = MMSL_TOKEN_STRING;
				token.sval = LockVariableName();
				if (token.sval == NULL)
				{
					token.type = MMSL_TOKEN_EOF;
					return token;
				}
				i = 0;
				char term = *data;
				data++;
				while (*data != term && *data != '\0' && *data != '\n')
				{
					if (*data == '\\')
					{
						int k = 0;
						switch (data[1])
						{
						case '\r':
						case '\n':	// line-continuation
							data += 2;
							if (*data == '\r' || *data == '\n')
								data++;
							break;
						case '\"':
						case '\'':
						case '\\':
						case '?':
							data++;
							token.sval[i++] = *data++;
							break;
						case 'a':
							data += 2;
							token.sval[i++] = '\a';
							break;
						case 'b':
							data += 2;
							token.sval[i++] = '\b';
							break;
						case 'f':
							data += 2;
							token.sval[i++] = '\f';
							break;
						case 'n':
							data += 2;
							token.sval[i++] = '\n';
							break;
						case 'r':
							data += 2;
							token.sval[i++] = '\r';
							break;
						case 't':
							data += 2;
							token.sval[i++] = '\t';
							break;
						case 'v':
							data += 2;
							token.sval[i++] = '\v';
							break;
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
							// octal
							token.sval[i] = 0;
							do
							{
								token.sval[i] = (char)(token.sval[i] * 8 + ((int)(*data) - '0'));
								data++;
								k++;
							} while (k < 3 && *data >= '0' && *data <= '7');
							i++;
							break;
						case 'x':
							// hex
							data += 2;
							token.sval[i] = 0;
							while (k < 3 && ((*data >= '0' && *data <= '9') ||
								(*data >= 'a' && *data <= 'f') || 
								(*data >= 'A' && *data <= 'F')))
							{
								token.sval[i] = (char)(token.sval[i] * 16 + hex2char(*data));
								data++;
								k++;
							} 
							i++;
							break;
						default:
							token.sval[i++] = *data++;
						}
						continue;
					}
					token.sval[i++] = *data++;
				}
				if (*data == term)
					data++;
				token.sval[i] = '\0';
				UnlockVariableName(i);
				return token;
			}
			// arithmetic operators
			else switch (*data)
			{
			case '\\':
				if (data[1] == '\n' || data[1] == '\r')
				{
					data += 2;
					if (*data == '\r' || *data == '\n')
						data++;
					if (*data != '\0')
						goto cont;
				}
				break;
			case ';':
				was_semicolon = true;
			case '\n':
				SetCol(data - last_newline + 1, num_tabs);
				token.type = MMSL_TOKEN_EOL;
				was_newline = true;
				num_spaces = 0;
				num_tabs = 0;
				was_realnewline = (*data == '\n');
				if (was_realnewline)
					was_semicolon = false;
				data++;
				return token;
			case '(':
				SetCol(data - last_newline + 1, num_tabs);
				token.type = MMSL_TOKEN_LP;
				data++;
				return token;
			case ')':
				SetCol(data - last_newline + 1, num_tabs);
				token.type = MMSL_TOKEN_RP;
				data++;
				return token;
			case '+':
				SetCol(data - last_newline + 1, num_tabs);
				token.type = MMSL_TOKEN_PLUS;
				data++;
				return token;
			case '-':
				SetCol(data - last_newline + 1, num_tabs);
				token.type = MMSL_TOKEN_MINUS;
				data++;
				return token;
			case '*':
				SetCol(data - last_newline + 1, num_tabs);
				token.type = MMSL_TOKEN_MUL;
				data++;
				return token;
			case '/':
				SetCol(data - last_newline + 1, num_tabs);
				token.type = MMSL_TOKEN_DIV;
				data++;
				return token;
			case '%':
				SetCol(data - last_newline + 1, num_tabs);
				token.type = MMSL_TOKEN_MOD;
				data++;
				return token;
			case '!':
				if (data[1] == '=')
				{
					SetCol(data - last_newline + 1, num_tabs);
					token.type = MMSL_TOKEN_NONEQU;
					data += 2;
					return token;
				}
				break;
			case '&':
				if (data[1] == '&')
				{
					SetCol(data - last_newline + 1, num_tabs);
					token.type = MMSL_TOKEN_ANDAND;
					data += 2;
					return token;
				}
				break;
			case '|':
				if (data[1] == '|')
				{
					SetCol(data - last_newline + 1, num_tabs);
					token.type = MMSL_TOKEN_OROR;
					data += 2;
					return token;
				}
				break;
			case '>':
				SetCol(data - last_newline + 1, num_tabs);
				data++;
				if (*data == '=')
				{
					token.type = MMSL_TOKEN_GTEQU;
					data++;
				} else
					token.type = MMSL_TOKEN_GT;
				return token;
			case '<':
				SetCol(data - last_newline + 1, num_tabs);
				data++;
				if (*data == '=')
				{
					token.type = MMSL_TOKEN_LTEQU;
					data++;
				} else
					token.type = MMSL_TOKEN_LT;
				return token;
			case '=':
				SetCol(data - last_newline + 1, num_tabs);
				data++;
				if (*data == '=')
				{
					token.type = MMSL_TOKEN_EQUEQU;
					data++;
				} else
					token.type = MMSL_TOKEN_EQU;
				return token;
			default:
				if (!ignore_sym && *data != ' ' && *data != '\t' && *data != '\n' && *data != '\r')
				{
					mmsl_error("Ignoring unrecognized symbol.");
					ignore_sym = true;
				}
				break;
			}
			break;
		case MMSL_TOKEN_ID:
			if (!isalnum(*data) && *data != '.' && *data != '_')
			{
				if (lexeme_idx >= 0 && lexemes[lexeme_idx].name[i] == '\0')
				{
					token.type = lexemes[lexeme_idx].type;
					token.ival = 0;
				}
				else 
					token.sval[i] = '\0';
				UnlockVariableName(i);
				
				return token;
			}
			token.sval[i] = (char)tolower(*data);
			// first, check built-in lexemes
			if (lexeme_idx >= 0)
			{
				if (lexemes[lexeme_idx].name[i] != token.sval[i])
					lexeme_idx = -1;
			}
			i++;
			break;
		case MMSL_TOKEN_NUMBER:
			if (*data == 'x')
			{
				hex_number = true;
				break;
			}
			else if (hex_number && ((*data >= '0' && *data <= '9') || 
									(*data >= 'A' && *data <= 'F') || 
									(*data >= 'a' && *data <= 'f')))
			{
				token.ival = token.ival * 16 + hex2char((int)(*data));
			}
			else if (!isdigit(*data))
			{
				return token;
			}
			else
				token.ival = token.ival * 10 + ((int)(*data) - '0');
			break;
		default:
			;
		}
		data++;
	}
	if (token.type == MMSL_TOKEN_UNDEFINED)
		token.type = MMSL_TOKEN_EOF;
	return token;
}

void MmslParser::AddRow()
{
	if (curfile != NULL)
		curfile->row++;
	SetCol(1, 0);
}

void MmslParser::SetCol(int col, int num_tabs)
{
	if (curfile != NULL)
	{
		curfile->col = col + num_tabs * (num_spaces_in_tab - 1);
	}
}

void MmslParser::UndoToken(char * &data)
{
	if (last_data != NULL)
		data = last_data;
}

int MmslToken::GetPriority(MMSL_TOKEN_TYPE type)
{
    switch(type) 
	{
	case MMSL_TOKEN_LP: 
		return 1;
	case MMSL_TOKEN_EQU:
		return 2;
	case MMSL_TOKEN_ANDAND:
	case MMSL_TOKEN_OROR:
		return 3;
	case MMSL_TOKEN_EQUEQU:
	case MMSL_TOKEN_NONEQU:
		return 4;
	case MMSL_TOKEN_GT:
	case MMSL_TOKEN_GTEQU:
	case MMSL_TOKEN_LT:
	case MMSL_TOKEN_LTEQU:
		return 5;
	case MMSL_TOKEN_PLUS:
	case MMSL_TOKEN_MINUS:
		return 6;
	case MMSL_TOKEN_MUL:
	case MMSL_TOKEN_DIV:
	case MMSL_TOKEN_MOD:
		return 7;
	case MMSL_TOKEN_UNARY:
		return 8;
    default: 
		return 0;
    }
}

MMSL_OPERATOR_TYPE MmslToken::GetOperator(MMSL_TOKEN_TYPE type)
{
	MMSL_OPERATOR_TYPE op = (MMSL_OPERATOR_TYPE)(MMSL_OPERATOR_LP + type - MMSL_TOKEN_LP);
	if (op <= MMSL_OPERATOR_UNKNOWN1 || op >= MMSL_OPERATOR_UNKNOWN2)
		return MMSL_OPERATOR_UNKNOWN1;
	return op;
}

///////////////////////////////////////////////////////////////////////

BOOL MmslParser::ParseFile(const char *fname, int start_indent)
{
	int i;
	if (fname == NULL)
		return FALSE;
	FILE *fp = fopen(fname, "rb");
	if (fp == NULL)
	{
		char *newfname = new char [4096];
		sprintf(newfname, "mmsl/%s", fname);
		fp = fopen(newfname, "rb");
		if (fp == NULL)
		{
			mmsl_error("Cannot include file '%s'.", newfname);
			delete [] newfname;
			return FALSE;
		}
		delete [] newfname;
	}
	
	MmslFile fil;
	fil.fname = SPString(fname);
	files.Add(fil);
	curfile = &files[files.GetN() - 1];
	
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	rewind(fp);
	char *buf = (char *)SPmalloc(size + 1);
	fread(buf, size, 1, fp);
	for (i = 0; i < size; i++)
	{
		if (buf[i] == '\0')
			buf[i] = ' ';
	}
	buf[size] = '\0';
	fclose(fp);

	char *b = buf;
	int cur_event;

	if (events.GetN() < 1)
	{
		MmslEvent event;
		IDXLIST_ADD(event.trigger., tokens, MMSL_CONST_TRUE);
		cur_event = events.Add(event);
		parse_events_stack_idx = -1;
		parse_events_stack.Put(cur_event, ++parse_events_stack_idx);
		
	}

	int cur_indent = start_indent;
	cur_event = FindParentEvent(cur_indent);
	
	for (;;)
	{
		MmslToken token = GetNextToken(b);

		if (token.type == MMSL_TOKEN_EOL)
			continue;

		// include MMSL file
		if (token.type == MMSL_TOKEN_INCLUDE)
		{
			MmslToken tfname = GetNextToken(b);
			if (tfname.type != MMSL_TOKEN_STRING)
				mmsl_error("Cannot include file - string expected.");
			else
			{
				if (strcasecmp(tfname.sval, fname) == 0)
				{
					mmsl_error("This file is already included.");
				} else
				{
					ParseFile(tfname.sval, cur_indent);
				}
			}
		}
		
		// debug_print_token(token);

		if (token.type == MMSL_TOKEN_INDENT)
		{
			cur_indent = token.ival + start_indent;
			cur_event = FindParentEvent(cur_indent);
		}

		if (token.type == MMSL_TOKEN_ON)
		{
			MmslEvent event;
			event.indent = cur_indent;
			ParseExpression(b, event.trigger, events.GetN());
			if (IDXLIST_NUM(event.trigger., tokens) < 1)
			{
				mmsl_error("No condition set for event.");
			}
			// do not add root condition (it's always true)
			for (int i = 1; i <= parse_events_stack_idx; i++)
				IDXLIST_ADD(event., conditions, parse_events_stack[i]);
			cur_event = events.Add(event);
			parse_events_stack.Put(cur_event, ++parse_events_stack_idx);
		}

		else if (token.type == MMSL_TOKEN_ADD)
		{
			MmslExpression e;
			IDXLIST_ADD(e., tokens, MMSL_OPERATOR_ADD);

			token = GetNextToken(b);
			if (token.type != MMSL_TOKEN_ID)
				mmsl_error("'Object Type' identifier expected for 'add' operator.");
			else
			{
				MmslParserClass *cls = pclasses.Get(token.sval);
				if (cls == NULL)
					mmsl_error("Wrong object type specified for 'add' operator.");
				else
				{
					IDXLIST_ADD(e., tokens, cls->index);

					token = GetNextToken(b);
					if (token.type != MMSL_TOKEN_ID)
						mmsl_error("'Object Type' identifier expected for 'add' operator.");
					else
					{
						SPString nam(token.sval);
						MmslAssignedObject an;
						// now register variables
						for (int i = 0; i < pvars.l.GetN(); i++)
						{
							if (pvars.l[i]->obj_id == cls->ID)
							{
								if (pvars.l[i]->name && pvars.l[i]->name[0] == '.')	// only abstact vars like ".anyvar"
								{
									int idx = RegisterVariable(nam + pvars.l[i]->name);
									// copy var data
									*pvars.l[idx] = *pvars.l[i];
									IDXLIST_ADD(an., assigned_vars, idx);
								}
							}
						}
						int an_idx = assigned_objs.Add(an);
						IDXLIST_ADD(e., tokens, an_idx);
						IDXLIST_ADD_SHIFT(events,., cur_event, execute, e);
					}
				}
			}
		} 
		else if (token.type == MMSL_TOKEN_DELETE)
		{
			MmslExpression e;
			IDXLIST_ADD(e., tokens, MMSL_OPERATOR_DELETE);
			ParseExpression(b, e, -2);
			if (IDXLIST_NUM(e., tokens) < 1)
			{
				mmsl_error("No condition set for delete.");
			} else
			{
				IDXLIST_ADD_SHIFT(events,.,cur_event, execute, e);
			}
		} 
		else if (token.type == MMSL_TOKEN_ID)
		{
			UndoToken(b);
			MmslExpression e;
			ParseExpression(b, e, -1);
			IDXLIST_ADD_SHIFT(events,., cur_event, execute, e);
		}

		if (token.type == MMSL_TOKEN_EOF)
			break;
	}

	files.Remove(files.GetN() - 1);
	curfile = files.GetN() > 0 ? &files[files.GetN() - 1] : NULL;

	SPfree(buf);
	return TRUE;
}

int MmslParser::FindParentEvent(int indent)
{
	if (indent == 0)
	{
		parse_events_stack_idx = 0;
		return 0;
	}
	for ( ; parse_events_stack_idx >= 0; parse_events_stack_idx--)
	{
		int idx = parse_events_stack[parse_events_stack_idx];
		if (idx >= 0 && idx < events.GetN() && events[idx].indent < indent)
			return idx;
	}
	return 0;
}

void MmslParser::ParseExpression(char * &b, MmslExpression &expr, int event_idx)
{
	int num_par = 0;
	bool was_op = false, was_id = false;
	token_stack_idx = -1;

#ifdef MMSL_DEBUG
	if (curfile != NULL)
		expr.pos = *curfile;
#endif

	for (;;)
	{
		MmslToken token = GetNextToken(b);
		if (token.type == MMSL_TOKEN_EOL || token.type == MMSL_TOKEN_EOF)
			break;
		if (token.type == MMSL_TOKEN_ID)
		{
			int idx = RegisterVariable(token.sval);
			if (event_idx >= 0)
			{
				IDXLIST_MERGE_SHIFTPTR(pvars.l, idx, lut, event_idx);
			}
			IDXLIST_ADD(expr., tokens, idx);
			was_op = false;
			was_id = true;
		} 
		else if (token.type == MMSL_TOKEN_NUMBER)
		{
			int idx = RegisterConstant(token.ival);
			IDXLIST_ADD(expr., tokens, idx);
			was_op = false;
			was_id = true;
		}
		else if (token.type == MMSL_TOKEN_STRING)
		{
			int idx = RegisterConstant(token.sval);
			IDXLIST_ADD(expr., tokens, idx);
			was_op = false;
			was_id = true;
		}
		else if (token.type == MMSL_TOKEN_LP)
		{
			token_stack.Put(MMSL_TOKEN_LP, ++token_stack_idx);
			num_par++;
			was_op = false;
		}
		else if (token.type == MMSL_TOKEN_RP)
		{
			if (was_op)
				mmsl_error("Syntax error.");
			MMSL_TOKEN_TYPE tok;
			while (token_stack_idx >= 0 && (tok = token_stack[token_stack_idx--]) != MMSL_TOKEN_LP && num_par > 0)
			{
				IDXLIST_ADD(expr., tokens, MmslToken::GetOperator(tok));
			}
			num_par--;
			was_op = false;
		}
		else // operator?
		{
			MMSL_OPERATOR_TYPE op = token.GetOperator(token.type);
			if (op != MMSL_OPERATOR_UNKNOWN1)
			{
				MMSL_TOKEN_TYPE type = token.type;
				// fix '==' typos for conditional expression
				if (op == MMSL_OPERATOR_EQU && event_idx != -1)
				{
					op = MMSL_OPERATOR_EQUEQU;
					type = MMSL_TOKEN_EQUEQU;
				}

				// process unary +/- operators
				int cur_pri = MmslToken::GetPriority(type);
				if (!was_id && (op == MMSL_OPERATOR_PLUS || op == MMSL_OPERATOR_MINUS))
				{
					int idx = RegisterConstant(0);
					IDXLIST_ADD(expr., tokens, idx);
					cur_pri = MmslToken::GetPriority(MMSL_TOKEN_UNARY);
				}
				was_op = true;
				was_id = false;
				
				for ( ; token_stack_idx >= 0 
					&& cur_pri <= MmslToken::GetPriority(token_stack[token_stack_idx]); 
					token_stack_idx--)
				{
					IDXLIST_ADD(expr., tokens, MmslToken::GetOperator(token_stack[token_stack_idx]));
				}
				if (token_stack_idx < 0 || cur_pri > MmslToken::GetPriority(token_stack[token_stack_idx]))
					token_stack.Put(type, ++token_stack_idx);
			} else
			{
				mmsl_error("Unknown token in expression.");
			}
		}
	}
	// flush operators
	for ( ; token_stack_idx >= 0; token_stack_idx--)
	{
		IDXLIST_ADD(expr., tokens, MmslToken::GetOperator(token_stack[token_stack_idx]));
	}
}

MmslParserVariable *MmslParser::GetNewVariable()
{
	if (varscache_used < varscache->GetN())
	{
		MmslParserVariable *var = &(*varscache)[varscache_used++];
		var->temporary = false;
		return var;
	}
	return new MmslParserVariable();
}

int MmslParser::RegisterVariable(const char *name, int ID)
{
	if (name == NULL || ID < 0)
		return -1;
	MmslParserVariable *var = GetNewVariable();
	var->ID = ID;
	var->SetConstName(name);
	var->type = MMSL_VARIABLE_INTEGER;
	return RegisterVariable(var);
}

int MmslParser::RegisterObject(const char *name, int ID)
{
	if (name == NULL || ID < 0)
		return -1;
	MmslParserClass *cls = new MmslParserClass();
	cls->ID = ID;
	cls->SetConstName(name);
	return RegisterObjectClass(cls);
}

int MmslParser::RegisterObjectVariable(int obj_ID, const char *var_name, int var_ID)
{
	if (obj_ID < 0 || var_name == NULL || var_ID < 0)
		return -1;
	MmslParserVariable *cvar = pvars.Get((char *)var_name);
	if (cvar == NULL)
	{
		cvar = GetNewVariable();
		if (cvar == NULL)
			return -1;
		cvar->type = MMSL_VARIABLE_OBJECTVAR;
		cvar->SetConstName(var_name);
		// init pclasses LUT
		for (int i = 0; i < pclasses.l.GetN(); i++)
			IDXLIST_ADD(cvar->, clut, -1);
		RegisterVariable(cvar);
	}

	for (int i = 0; i < pclasses.l.GetN(); i++)
	{
		if (pclasses.l[i]->ID == obj_ID)
		{
			MmslParserVariable *var = GetNewVariable();
			if (var == NULL)
				return -1;
			var->ID = var_ID;
			var->obj_id = obj_ID;
			var->obj = NULL;		// will be assigned dynamically for a real variable
			var->type = MMSL_VARIABLE_INTEGER;
			var->SetConstName(var_name);

			var->index = RegisterVariable(var);
			
			// write class index
			IDXLIST_PUT(cvar->, clut, var->index, i);
			
			return var->index;
		}
	}

	return -1;
}

int MmslParser::RegisterVariable(char *name)
{
	MmslParserVariable *var = pvars.Get(name);
	if (var != NULL)
		return var->index;
	var = GetNewVariable();
	var->SetName(name);
	var->type = MMSL_VARIABLE_INTEGER;
	return RegisterVariable(var);
}

int MmslParser::RegisterConstant(char *strval)
{
	SPString *s = new_SPString(strval);
#ifdef USE_CONSTANTS_OPTIMISATION
	MmslParserHashVariable tmphash(s);
	MmslParserHashVariable *ret = pconstsvars.hash.Get(tmphash);
	if (ret)
	{
		delete_SPString(s);
		return ret->var->index;
	}
#endif
	// add hash search?
	MmslParserVariable *var = GetNewVariable();
	var->type = MMSL_VARIABLE_CONST_STRING;
	var->sval = s;
#ifdef USE_CONSTANTS_OPTIMISATION
	MmslParserHashVariable *newphv = new MmslParserHashVariable(var);
	pconstsvars.hash.Add(newphv);
#endif
	return RegisterVariable(var);
}

int MmslParser::RegisterConstant(int val)
{
#ifdef USE_CONSTANTS_OPTIMISATION
	// use raw search
	for (int i = 0; i < pconstivars.GetN(); i++)
	{
		if (pconstivars[i]->ival == val)
			return pconstivars[i]->index;
	}
#endif
	// add hash search?
	MmslParserVariable *var = GetNewVariable();
	var->type = MMSL_VARIABLE_CONST_INTEGER;
	var->ival = val;
#ifdef USE_CONSTANTS_OPTIMISATION
	pconstivars.Add(var);
#endif
	return RegisterVariable(var);
	
}

int MmslParser::RegisterVariable(MmslParserVariable *var)
{
	if (var == NULL)
		return -1;
	pvars.hash.Add(var);
	var->index = pvars.l.Add(var);
/*
	if (var->ID >= 0)
	{
		MmslID *vid = new MmslID;
		vid->ID = var->ID;
		vid->index = var->index;
		idvars.Add(vid);
	}
*/
	return var->index;
}

int MmslParser::RegisterObjectClass(MmslParserClass *obj)
{
	pclasses.hash.Add(obj);
	obj->index = pclasses.l.Add(obj);
	return obj->index;
}

/////////////////////////////////////////////////////////////////////

BYTE *MmslParser::Save(int *buflen)
{
	const int max_buf_size = 512*1024;	// 512 Kb
	BYTE *buf0 = (BYTE *)SPmalloc(max_buf_size);
	BYTE *buf = buf0;
	int i;

	was_overflow = false;

	MmslSerHeader *hdr = (MmslSerHeader *)buf;
	buf += sizeof(MmslSerHeader);
	
	BYTE *varbuf = (BYTE *)buf;
	buf += (hdr->var_buf_size = CalcSerVarSize());
	hdr->num_var = pvars.l.GetN();

	MmslSerClass *serclass = (MmslSerClass *)buf;
	buf += sizeof(MmslSerClass) * (hdr->num_class = pclasses.l.GetN());
	MmslSerEvent *serevent = (MmslSerEvent *)buf;
	buf += sizeof(MmslSerEvent) * (hdr->num_event = events.GetN());
	MmslSerAssignedObject *serassignedobj = (MmslSerAssignedObject *)buf;
	buf += sizeof(MmslSerAssignedObject) * (hdr->num_assignedobj = assigned_objs.GetN());
	MmslSerExpression *serexpr = (MmslSerExpression *)buf;
	buf += sizeof(MmslSerExpression) * (hdr->num_expr = execute.GetN());
	
	short *serlut = (short *)buf;
	buf += sizeof(short) * (hdr->num_lut = lut.GetN());
	short *serclut = (short *)buf;
	buf += sizeof(short) * (hdr->num_clut = clut.GetN());
	short *serassignedvars = (short *)buf;
	buf += sizeof(short) * (hdr->num_assignedvar = assigned_vars.GetN());
	short *sercond = (short *)buf;
	buf += sizeof(short) * (hdr->num_cond = conditions.GetN());
	short *sertoken = (short *)buf;
	buf += sizeof(short) * (hdr->num_token = tokens.GetN());

	int str_len = 0;
	for (i = 0; i < pvars.l.GetN(); i++)
	{
		if (pvars.l[i]->sval)
			str_len += pvars.l[i]->sval->GetLength() + 1;
	}

	char *str_buf = (char *)buf;
	buf += str_len;

	int buf_size = (int)(buf - buf0);
	// at this moment, the buffer is not filled yet, so let's check its size
	if (buf_size > max_buf_size)
	{
		msg("Mmsl: Max. buffer size exceeded.\n");
		SPfree(buf0);
		return NULL;
	}

	char *curstr = str_buf;
	
	for (i = 0; i < pvars.l.GetN(); i++)
	{
		*varbuf++ = (BYTE)pvars.l[i]->type;		// type contains variable bit-fields also (see CalcSerVarSize())
		if (pvars.l[i]->type & MMSL_VARIABLE_HAS_ID)
			WriteShort(varbuf, pvars.l[i]->ID);
		if (pvars.l[i]->type & MMSL_VARIABLE_HAS_LUT)
		{
			WriteShort(varbuf, pvars.l[i]->idx_lut1);
			WriteShort(varbuf, pvars.l[i]->idx_lut2);
		}
		if (pvars.l[i]->type & MMSL_VARIABLE_HAS_CLUT)
		{
			WriteChar(varbuf, pvars.l[i]->idx_clut1);
			WriteChar(varbuf, pvars.l[i]->idx_clut2);
		}
		if (pvars.l[i]->type & MMSL_VARIABLE_HAS_OBJID)
			WriteChar(varbuf, pvars.l[i]->obj_id);

		pvars.l[i]->type = (MMSL_VARIABLE_TYPE)(pvars.l[i]->type & MMSL_VARIABLE_MASK);

		// raw integer or strings index
		if (pvars.l[i]->type == MMSL_VARIABLE_CONST_STRING)
		{
			if (pvars.l[i]->sval)
			{
				WriteInt(varbuf, (int)(curstr - str_buf));
				int l = pvars.l[i]->sval->GetLength() + 1;
				memcpy(curstr, *pvars.l[i]->sval, l);
				curstr += l;
			} else
				WriteInt(varbuf, -1);
		}
		else
			WriteInt(varbuf, pvars.l[i]->ival);
	}

	for (i = 0; i < pclasses.l.GetN(); i++)
	{
		serclass[i].ID = GetShort(pclasses.l[i]->ID);
		serclass[i].idx_vars1 = GetShort(pclasses.l[i]->idx_vars1);
		serclass[i].idx_vars2 = GetShort(pclasses.l[i]->idx_vars2);
	}

	for (i = 0; i < events.GetN(); i++)
	{
		serevent[i].idx_triggertokens1 = GetShort(events[i].trigger.idx_tokens1);
		serevent[i].idx_triggertokens2 = GetShort(events[i].trigger.idx_tokens2);
		serevent[i].idx_conditions1 = GetShort(events[i].idx_conditions1);
		serevent[i].idx_conditions2 = GetShort(events[i].idx_conditions2);
		serevent[i].idx_execute1 = GetShort(events[i].idx_execute1);
		serevent[i].idx_execute2 = GetShort(events[i].idx_execute2);
	}

	for (i = 0; i < assigned_objs.GetN(); i++)
	{
		serassignedobj[i].idx_assigned_vars1 = GetShort(assigned_objs[i].idx_assigned_vars1);
		serassignedobj[i].idx_assigned_vars2 = GetShort(assigned_objs[i].idx_assigned_vars2);
	}

	for (i = 0; i < execute.GetN(); i++)
	{
		serexpr[i].idx_tokens1 = GetShort(execute[i].idx_tokens1);
		serexpr[i].idx_tokens2 = GetShort(execute[i].idx_tokens2);
	}

	//memcpy(serlut, lut.GetData(), sizeof(serlut[0]) * lut.GetN());
	for (i = 0; i < lut.GetN(); i++)
		serlut[i] = GetShort(lut[i]);
	//memcpy(serclut, clut.GetData(), sizeof(serclut[0]) * clut.GetN());
	for (i = 0; i < clut.GetN(); i++)
		serclut[i] = GetShort(clut[i]);
	//memcpy(serassignedvars, assigned_vars.GetData(), sizeof(serassignedvars[0]) * assigned_vars.GetN());
	for (i = 0; i < assigned_vars.GetN(); i++)
		serassignedvars[i] = GetShort(assigned_vars[i]);
	//memcpy(sercond, conditions.GetData(), sizeof(sercond[0]) * conditions.GetN());
	for (i = 0; i < conditions.GetN(); i++)
		sercond[i] = GetShort(conditions[i]);
	//memcpy(sertoken, tokens.GetData(), sizeof(sertoken[0]) * tokens.GetN());
	for (i = 0; i < tokens.GetN(); i++)
		sertoken[i] = GetShort(tokens[i]);

	if (was_overflow)
	{
		msg("MmslParser: Serialization overflow! Not saved!\n");
		SPfree(buf0);
		return NULL;
	}

	*buflen = buf_size;
	return buf0;
}

short MmslParser::GetShort(int v)
{
	short b = (short)v;
	if (v != (int)b)
		was_overflow = true;
	return b;
}

void MmslParser::WriteChar(BYTE* &buf, int v)
{
	if (v != (int)((char)v))
		was_overflow = true;
	*buf++ = (BYTE)((char)v);
}

void MmslParser::WriteShort(BYTE* &buf, int v)
{
	*buf++ = (BYTE)((char)(v & 0xff));
	*buf++ = (BYTE)((char)(v >> 8));
}

void MmslParser::WriteInt(BYTE* &buf, int v)
{
	DWORD d = *((DWORD *)&v);
	*buf++ = (BYTE)d;
	*buf++ = (BYTE)(d >>= 8);
	*buf++ = (BYTE)(d >>= 8);
	*buf++ = (BYTE)(d >>= 8);
}

int MmslParser::CalcSerVarSize()
{
	int ser_var_size = 0;
	for (int i = 0; i < pvars.l.GetN(); i++)
	{
		ser_var_size += 5;	// pvars.l[i]->type + ival/sval_idx

		pvars.l[i]->type = (MMSL_VARIABLE_TYPE)(pvars.l[i]->type & MMSL_VARIABLE_MASK);
		
		if (pvars.l[i]->ID >= 0)
		{
			ser_var_size += 2;
			pvars.l[i]->type = (MMSL_VARIABLE_TYPE)(pvars.l[i]->type | MMSL_VARIABLE_HAS_ID);
		}
		if (pvars.l[i]->idx_lut1 >= 0 || pvars.l[i]->idx_lut2 >= 0)
		{
			ser_var_size += 4;
			pvars.l[i]->type = (MMSL_VARIABLE_TYPE)(pvars.l[i]->type | MMSL_VARIABLE_HAS_LUT);
		}
		if (pvars.l[i]->idx_clut1 >= 0 || pvars.l[i]->idx_clut2 >= 0)
		{
			ser_var_size += 2;
			pvars.l[i]->type = (MMSL_VARIABLE_TYPE)(pvars.l[i]->type | MMSL_VARIABLE_HAS_CLUT);
		}
		if (pvars.l[i]->obj_id >= 0)
		{
			ser_var_size ++;
			pvars.l[i]->type = (MMSL_VARIABLE_TYPE)(pvars.l[i]->type | MMSL_VARIABLE_HAS_OBJID);
		}
	}
	return PAD_EVEN(ser_var_size);
}
