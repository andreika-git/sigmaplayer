//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - MMSL interpreter header file
 *  \file       mmsl/mmsl-parser.h
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

#ifndef SP_MMSL_PARSER_H
#define SP_MMSL_PARSER_H

#include <gui/res.h>
#include <mmsl/mmsl.h>

////////////////////////////////////////////////////////////////////

template <typename T, typename BaseT, int n>
class HashList
{
public:
	/// ctor
	HashList()
	{
		hash.SetN(n);
	}

	/// Not thread-safe!
	T *Get(char *name)
	{
		static T tmp;
		tmp.SetConstName(name);
		T *ret = hash.Get(tmp);
		tmp.name = NULL;
		return ret;
	}

public:
	SPHashListAbstract<BaseT, T> hash;
	SPList<T *> l;
};

class MmslToken
{
public:
	MMSL_TOKEN_TYPE type;
	union
	{
		int ival;
		char *sval;
	};

	static int GetPriority(MMSL_TOKEN_TYPE type);
	static MMSL_OPERATOR_TYPE GetOperator(MMSL_TOKEN_TYPE type);
};

/// Built-in or user-defined variable (also object var.)
class MmslParserVariable : public Resource, public MmslVariable
{
public:
	/// ctor
	MmslParserVariable() {}

	/// dtor
	~MmslParserVariable() {}

	MmslParserVariable (const MmslParserVariable & v) : MmslVariable(v)
	{
	}

	MmslParserVariable & operator = (const MmslParserVariable & v)
	{
		MmslVariable::operator =(v);

		return (*this);
	}

public:
};

/// Registered object class (for 'add [object] [name]' command)
class MmslParserClass : public Resource, public MmslClass
{
public:
	MmslParserClass()
	{
		add_callback = 0;
		delete_callback = 0;
		param = NULL;
	}

	/// dtor
	~MmslParserClass()
	{
	}

public:
	MmslObjectCallback add_callback, delete_callback;
	void *param;

};


const int parservar_num_hash = 57;

/// Used for const-strings search optimisation
class MmslParserHashVariable
{
public:
	MmslParserHashVariable(SPString *str)
	{
		prev = next = NULL;
		var = NULL;
		sval = str;
		const char *ss = **str;
		hash = SPStringHashFunc((char * const &)ss, parservar_num_hash);
	}

	MmslParserHashVariable(MmslParserVariable *v)
	{
		prev = next = NULL;
		var = v;
		sval = v->sval;
		hash = v->sval ? SPStringHashFunc(*v->sval, parservar_num_hash) : 0;
	}

	/// compare
	template <typename T>
	bool operator == (const T & phv)
	{
		if (sval == NULL || phv.sval == NULL)
			return false;
		return var->sval->Compare(*phv.sval) == 0;
	}

	inline operator DWORD () const
	{
		return hash;
	}

	const MmslParserHashVariable * const GetItem() const
	{
		return this;
	}

	MmslParserHashVariable *prev, *next;

public:
	MmslParserVariable *var;
	SPString *sval;
	DWORD hash;
};


////////////////////////////////////////////////////////////////

/// MMSL Parser/Interpreter singleton class
class MmslParser : public Mmsl
{
public:
	/// ctor
	MmslParser();
	/// dtor
	~MmslParser();

	/// Register predefined external variable (names must be string constants).
	int RegisterVariable(const char *name, int ID);

	/// Register dynamic object type - to create/delete it from script
	/// (names must be string constants).
	int RegisterObject(const char *name, int ID);

	/// Register object variables
	/// (names must be string constants).
	int RegisterObjectVariable(int obj_ID, const char *var_name, int var_ID);

	/// Read and process MMSL file
	BOOL ParseFile(const char *fname, int start_indent = 0);

	/// Serialization: Save all created data into the buffer
	BYTE *Save(int *buflen);

public:	
	HashList<MmslParserVariable, Resource, resource_num_hash> pvars;

	/// registered object pclasses
	HashList<MmslParserClass, Resource, resource_num_hash> pclasses;

	SPList<MmslParserVariable *> pconstivars;
	HashList<MmslParserHashVariable, MmslParserHashVariable, parservar_num_hash> pconstsvars;

protected:
	friend class MmslParserVariable;
	friend void mmsl_error(const char *, ...);

	SPList<int> parse_events_stack;
	int parse_events_stack_idx;

	SPList<MMSL_TOKEN_TYPE> token_stack;
	int token_stack_idx;
	
	MmslToken GetNextToken(char * &data);
	void UndoToken(char * &data);

	char *varspace;
	int maxvaridx;
	char *last_data;

	SPClassicList<MmslParserVariable> *varscache;
	int varscache_used;

	/// Parsing file info stack (fname, row, col) for nested includes
	SPClassicList<MmslFile> files;
	MmslFile *curfile;

	/// Return a new variable pointer
	char *LockVariableName();
	/// Close variable pointer
	void UnlockVariableName(int len);

	int FindParentEvent(int indent);
	void ParseExpression(char * &b, MmslExpression &expr, int event_idx);

	void AddRow();
	void SetCol(int col, int numtabs);

	int RegisterObjectClass(MmslParserClass *obj);
	int RegisterVariable(char *name);
	int RegisterConstant(char *val);
	int RegisterConstant(int val);
	int RegisterVariable(MmslParserVariable *var);

	MmslParserVariable *GetNewVariable();

	//////////////////////////////
	inline short GetShort(int v);
	inline void WriteChar(BYTE* &buf, int v);
	inline void WriteShort(BYTE* &buf, int v);
	inline void WriteInt(BYTE* &buf, int v);

	int CalcSerVarSize();

private:
	bool was_overflow;
};

extern MmslParser *mmsl_parser;

#endif // of SP_MMSL_PARSER_H
