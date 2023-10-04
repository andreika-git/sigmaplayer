//////////////////////////////////////////////////////////////////////////
/**
 * SigmaPlayer source project - MMSL interpreter header file
 *  \file       mmsl/mmsl.h
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

#ifndef SP_MMSL_H
#define SP_MMSL_H


#define MMSL_VERSION "0.94"

#ifdef WIN32
//#define MMSL_DEBUG
#endif

class MmslVariable;

typedef void* MMSL_OBJECT;
typedef int MMSL_EXPR_TOKEN;

typedef void (*MmslObjectCallback)(int obj_id, MMSL_OBJECT *obj, void *param);
typedef void (*MmslVariableCallback)(int var_id, MmslVariable *var, void *param, int obj_id, MMSL_OBJECT *obj);

SPString *new_SPString();
SPString *new_SPString(const SPString &);
void delete_SPString(SPString * &);

const int mmsl_num_hash = 347;

enum MMSL_VARIABLE_CLASS
{
	MMSL_VARIABLECLASS_INTEGER = 0x4,
	MMSL_VARIABLECLASS_STRING = 0x8,
};


enum MMSL_VARIABLE_TYPE
{
	MMSL_VARIABLE_INTEGER = MMSL_VARIABLECLASS_INTEGER,
	MMSL_VARIABLE_CONST_INTEGER = MMSL_VARIABLECLASS_INTEGER | 1,
	MMSL_VARIABLE_OBJECT_INTEGER = MMSL_VARIABLECLASS_INTEGER | 2,
	
	MMSL_VARIABLE_STRING = MMSL_VARIABLECLASS_STRING,
	MMSL_VARIABLE_CONST_STRING = MMSL_VARIABLECLASS_STRING | 1,
	MMSL_VARIABLE_RET_STRING = MMSL_VARIABLECLASS_STRING | 2,
	MMSL_VARIABLE_OBJECT_STRING = MMSL_VARIABLECLASS_STRING | 3,

	MMSL_VARIABLE_OBJECTVAR,				// '.varname'

	// these used only for serialization
	MMSL_VARIABLE_HAS_ID = 0x10,
	MMSL_VARIABLE_HAS_LUT = 0x20,
	MMSL_VARIABLE_HAS_CLUT = 0x40,
	MMSL_VARIABLE_HAS_OBJID = 0x80,

	MMSL_VARIABLE_MASK = ~(MMSL_VARIABLE_HAS_ID | MMSL_VARIABLE_HAS_LUT | MMSL_VARIABLE_HAS_CLUT | MMSL_VARIABLE_HAS_OBJID),
};

enum MMSL_OPERATOR_TYPE
{
	MMSL_OPERATOR_UNKNOWN1 = -100,
	
	MMSL_OPERATOR_LP,		// (
	MMSL_OPERATOR_RP, 		// )
	MMSL_OPERATOR_PLUS, 	// +
    MMSL_OPERATOR_MINUS, 	// -
    MMSL_OPERATOR_MUL, 		// *
    MMSL_OPERATOR_DIV, 		// /
	MMSL_OPERATOR_MOD, 		// %
    MMSL_OPERATOR_NONEQU, 	// !=
    MMSL_OPERATOR_GT, 		// >
    MMSL_OPERATOR_LT, 		// <
    MMSL_OPERATOR_GTEQU, 	// >=
    MMSL_OPERATOR_LTEQU, 	// <=
    MMSL_OPERATOR_EQUEQU, 	// ==
	MMSL_OPERATOR_OROR,		// ||
	MMSL_OPERATOR_ANDAND,	// &&
	MMSL_OPERATOR_EQU, 		// =
	
	MMSL_OPERATOR_UNKNOWN2,

	// 'add object' operator
	// arg1 = index into classes
	// arg2 = index into 'assigned_objects'
	MMSL_OPERATOR_ADD,

	// 'delete objects' operator
	// following arguments = deletion condition
	MMSL_OPERATOR_DELETE,

	// special operator (no args) - returns true.
	// used for 'default' event condition
	MMSL_CONST_TRUE,
};

// lexemes for tokens are case-insensitive!
enum MMSL_TOKEN_TYPE
{
	MMSL_TOKEN_UNDEFINED = 0,

	MMSL_TOKEN_ID,		// [a-z][a-z0-9]+
	MMSL_TOKEN_NUMBER,	// [0-9]+
	MMSL_TOKEN_STRING,	// ['"][^"']*["']

	MMSL_TOKEN_ADD,		// "add"
	MMSL_TOKEN_BREAK,	// "break"
	MMSL_TOKEN_DELETE,	// "delete"
	MMSL_TOKEN_INCLUDE,	// "include"
	MMSL_TOKEN_ON,		// "on"

	// operators
	MMSL_TOKEN_LP, 		// (
	MMSL_TOKEN_RP, 		// )
	MMSL_TOKEN_PLUS, 	// +
	MMSL_TOKEN_MINUS, 	// -
	MMSL_TOKEN_MUL, 	// *
	MMSL_TOKEN_DIV, 	// /
	MMSL_TOKEN_MOD, 	// %
	MMSL_TOKEN_NONEQU, 	// !=
	MMSL_TOKEN_GT, 		// >
	MMSL_TOKEN_LT, 		// <
	MMSL_TOKEN_GTEQU, 	// >=
	MMSL_TOKEN_LTEQU, 	// <=
	MMSL_TOKEN_EQUEQU, 	// ==
	MMSL_TOKEN_OROR, 	// ||
	MMSL_TOKEN_ANDAND,	// &&
	MMSL_TOKEN_EQU, 	// =


	MMSL_TOKEN_OPERATOR_MAX,

	// special tokens

	MMSL_TOKEN_INDENT,	// indent at the start of a line
	MMSL_TOKEN_EOL,		// end of line [\r\n]
	MMSL_TOKEN_EOF,		// end of file
	// comments are skipped:
	// 		\/\/[^\n]*		c++ comments 
	// 		\/\*(.*?)\*\/	c-like comments

	// fake 'unary operator' token definition - for correct priorities
	MMSL_TOKEN_UNARY,
};



////////////////////////////////////////////////////////////////////

/// Built-in or user-defined variable (also object var.)
class MmslVariable
{
public:
	/// ctor
	MmslVariable();

	/// dtor
	~MmslVariable()
	{
		delete_SPString(sval);
		//SPSafeDelete(lut);
		//SPSafeDelete(clut);
	}

	void Set(int value);
	void Set(const SPString & value);
	void Set(const MmslVariable & from);

	int GetInteger();
	/// not thread-safe
	SPString &GetString();
	
	void Update(bool immediate_trigger);
	
	MmslVariable (const MmslVariable & v)
	{
		type = v.type;
		ival = v.ival;
		sval = v.sval != NULL ? new_SPString(*v.sval) : NULL;
		ID = v.ID;
	}

	MmslVariable & operator = (const MmslVariable & v)
	{
		type = v.type;
		ival = v.ival;
		sval = v.sval != NULL ? new_SPString(*v.sval) : NULL;
		get_callback = v.get_callback;
		set_callback = v.set_callback;
		param = v.param;
		obj = v.obj;
		obj_id = v.obj_id;
		ID = v.ID;
		//idx_lut1 = idx_lut2 = -1;
		//idx_clut1 = idx_clut2 = -1;

		return (*this);
	}

public:
	/// set for tmp vars - references to the real ones
	MmslVariable *ref;

	MMSL_VARIABLE_TYPE type;

	int ID;

	int ival;
	SPString *sval;

	/// 1) lut='event_deps' - A list of dependent event indexes.
	///    If variable changes, they should be revised.
	//SPClassicList<int>	lut;
	int idx_lut1, idx_lut2;
	/// 2) lut='class_var_idx' for MMSL_VARIABLE_OBJECTVAR
	///    lut[class_idx] = class_var_idx (MmslClass::vars)
	///    or -1 if variable not used in class
	//SPClassicList<int>	clut;
	int idx_clut1, idx_clut2;

	/// object ID (for object vars only)
	int obj_id;
	/// object pointer
	MMSL_OBJECT obj;

	/// For 'system' pre-set variables
	MmslVariableCallback get_callback, set_callback;
	void *param;
};

/// Registered object class (for 'add [object] [name]' command)
class MmslClass
{
public:
	MmslClass()
	{
		add_callback = 0;
		delete_callback = 0;
		param = NULL;
		ID = -1;

		idx_vars1 = idx_vars2 = -1;
		idx_idobjvars1 = idx_idobjvars2 = -1;
	}

	/// dtor
	~MmslClass()
	{
	}

public:
	MmslObjectCallback add_callback, delete_callback;
	void *param;

	int ID;
	
	// idobjvars[classes[i].idx_idobjvars1 + var_ID] = vars_idx
	int idx_idobjvars1, idx_idobjvars2;		// listed by ID

	/// Abstract Variables (like ".var")
	int idx_vars1, idx_vars2;
};

/// Saved file position (for error reports)
class MmslFile
{
public:
	/// ctor
	MmslFile()
	{
		row = col = 0;
	}

	/// dtor
	~MmslFile() {}

public:
	SPString fname;
	int row, col;
};

/// Expression storage
class MmslExpression
{
public:
	/// ctor
	MmslExpression()
	{
		idx_tokens1 = idx_tokens2 = -1;
		value = -1;
	}
	
	// Tokens are in PPN order.
	// Operators ADD and DELETE come first.
	// Contains variable/constant indexes or operators (< 0).
	
	//SPList<MMSL_EXPR_TOKEN> tokens;	
	int idx_tokens1, idx_tokens2;

	/// Last calculated value
	int value;

#ifdef MMSL_DEBUG
	/// expression location (for error/debug)
	MmslFile pos;
#endif
};

/// Conditional 'on' block
class MmslEvent
{
public:
	/// ctor
	MmslEvent()
	{
		counter = 0;
		indent = 0;
		idx_conditions1 = idx_conditions2 = -1;
		idx_execute1 = idx_execute2 = -1;
	}

public:
	/// Iteration counter
	int counter;

	/// event statement indent (used to align sub-statements)
	int indent;

	/// This one triggers event
	MmslExpression trigger;
	
	/// These conditions (indexes stored) must be true to trigger this event
	/// triggers of the parent events)
	/// Pointers to the trigger conditions of other events
	//SPClassicList<int> conditions;
	int idx_conditions1, idx_conditions2;

	/// We need to execute these if event triggered
	//SPClassicList<MmslExpression> execute;
	int idx_execute1, idx_execute2;
};

class MmslObject;

/// An object with assigned name (i.e. used in 'add obj name' operator).
/// Used to re-assign objects (MmslObject) to names.
/// Created each time 'add obj' operator encounted.
class MmslAssignedObject
{
public:
	/// ctor
	MmslAssignedObject()
	{
		obj = NULL;
		idx_assigned_vars1 = idx_assigned_vars2 = -1;
	}

public:
	/// Link to the object 
	MmslObject *obj;

	/// Indexes of created object variables 
	/// Created from MmslClass::vars, the same size!
	/// Used to access object variables via 'Update' mechanism
	// SPList<int> vars;
	int idx_assigned_vars1, idx_assigned_vars2;
};

/// Dynamically created object
class MmslObject
{
public:
	MmslObject()
	{
		class_idx = -1;
		obj = NULL;
		assigned_obj = NULL;
		prev = next = NULL;
	}

	/// dtor
	~MmslObject();

public:
	friend class MmslAssignedObject;
	
	int class_idx;
	MMSL_OBJECT obj;

	/// Set if a name exists 
	MmslAssignedObject *assigned_obj;

public:
	const MmslObject * const GetItem() const
	{
		return this;
	}

	template <typename T>
	bool operator == (const T & o)
	{
		return obj == o.obj;
	}

	inline operator DWORD () const
	{
		return (DWORD)obj;
	}


	// linked list support
	MmslObject *prev, *next;
};

////////////////////////////////////////////////////////////////

/// MMSL Interpreter singleton class
class Mmsl
{
public:
	/// ctor
	Mmsl();
	/// dtor
	~Mmsl();

	// user land:

	/// Serialization: Save all created data into the buffer
	BOOL Load(BYTE *buf, int buflen);

	/// Bind callbacks to the predefined external variable.
	int BindVariable(int ID, MmslVariableCallback Get = NULL, MmslVariableCallback Set = NULL,
					void *param = NULL);

	/// Bind callbacks to the dynamic object type - to create/delete it from script
	int BindObject(int ID, MmslObjectCallback Add, MmslObjectCallback Delete, void *param);
	
	/// Bind callbacks to the object variables
	int BindObjectVariable(int obj_ID, int var_ID, MmslVariableCallback Get, MmslVariableCallback Set, 
					void *param);
	
	/// Run script - call this from your program loop every time
	BOOL Run();

	/// Call this to trigger script events when one of variables changed
	BOOL UpdateVariable(int ID);

	/// Call this to trigger script events when one of object variables changed
	BOOL UpdateObjectVariable(MMSL_OBJECT obj, int ID);

	MmslVariable *GetVariable(int ID);
	MmslClass *GetClass(int ID);

	void Dump();

public:	
	/// Created objects
	SPHashListAbstract<MmslObject,MmslObject> objects;

	/// registered/created variables
	SPClassicList<MmslVariable> vars;

	/// registered object classes
	SPClassicList<MmslClass> classes;

	/// Events list (contains ALL events). Also used for triggering events.
	SPClassicList<MmslEvent> events;

	/// name-assigned objects (linked to objects)
	SPClassicList<MmslAssignedObject> assigned_objs;

	/// Expressions
	SPClassicList<MmslExpression> execute;

	// internal lists for events/variables/assigned_objs:
	SPList<int>	lut;
	SPList<int>	clut;	// clut[var_offs + class_idx] = var_idx
	SPList<int> assigned_vars;
	SPList<int> conditions;
	SPList<MMSL_EXPR_TOKEN> tokens;
	
	/// LUT for update-vars mechanism
	SPList<int> idvars;
	/// LUT for update-objectvars mechanism
	SPList<int> idobjvars;
	/// LUT for classes
	SPList<int> idclasses;

protected:
	friend class MmslVariable;
	friend void mmsl_run_error(const char *, ...);

	/// Current execution pass counter
	int cur_counter;
	bool need_to_run_events;

	void SetVariable(int var_idx, bool immediate_trigger);

	void TriggerEvent(int event_idx, bool immediate_trigger);

	bool Evaluate(MmslExpression &e);

	/// Not thread-safe!
	int EvaluateMath(const MmslExpression & e, int start, MmslObject *obj, bool del = false);
	/// Not thread-safe!
	MmslVariable *Calc(MMSL_OPERATOR_TYPE op, MmslVariable *arg1, MmslVariable *arg2, bool last_op);

private:
	inline int ReadChar(BYTE* &buf);
	inline int ReadShort(BYTE* &buf);
	inline int ReadInt(BYTE* &buf);

	template<typename L>
	void DumpIntList(const char *n, const L &list);
};

extern Mmsl *mmsl;

#endif // of SP_MMSL_H
