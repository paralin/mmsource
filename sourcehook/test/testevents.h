/* ======== SourceHook ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Pavol "PM OnoTo" Marko
* ============================
*/

#ifndef __TESTEVENTS_H__
#define __TESTEVENTS_H__

#include <typeinfo>
#include <stdarg.h>
#include <iostream>
#include <string>

extern bool g_Verbose;

namespace
{

	struct State
	{
		virtual ~State()
		{

		}

		virtual bool IsEqual(State *other)
		{
			return (typeid(other) == typeid(this)) ? true : false;
		}

		virtual void Dump() = 0;
	};

	typedef std::list<State*> StateList;

	#define ADD_STATE(name) g_States.push_back(new name)


	void DumpStates(StateList *sl)
	{
		for (StateList::iterator iter = sl->begin(); iter != sl->end(); ++iter)
			(*iter)->Dump();
	}

	bool StatesOk(StateList *sl, ...)
	{
		StateList requiredstates;
		va_list argptr;
		va_start(argptr, sl);
		while (true)
		{
			State *cs = va_arg(argptr, State*);
			if (!cs)
				break;
			requiredstates.push_back(cs);
		}
		va_end(argptr);

		if (requiredstates.size() != sl->size())
		{
			if (g_Verbose)
			{
				std::cout << std::endl << "FAIL: Should be:" << std::endl;
				DumpStates(&requiredstates);
				std::cout << std::endl << "FAIL: Is:" << std::endl;
				DumpStates(sl);
			}

			for (StateList::iterator iter = requiredstates.begin(); iter != requiredstates.end(); ++iter)
				delete *iter;
			for (StateList::iterator iter = sl->begin(); iter != sl->end(); ++iter)
				delete *iter;
			sl->clear();
			return false;
		}

		bool ok = true;
		StateList::iterator req_iter = requiredstates.begin();
		for (StateList::iterator o_iter = sl->begin(); o_iter != sl->end(); ++o_iter, ++req_iter)
		{
			if (!(*o_iter)->IsEqual(*req_iter))
			{
				ok = false;
				break;
			}
		}

		for (StateList::iterator iter = requiredstates.begin(); iter != requiredstates.end(); ++iter)
			delete *iter;
		for (StateList::iterator iter = sl->begin(); iter != sl->end(); ++iter)
			delete *iter;
		sl->clear();

		return ok;
	}
}

#define CHECK_STATES(mwah, myerr) if (!StatesOk mwah) { error=myerr; return false; } else if (g_Verbose) { std::cout << "No error: " << myerr << std::endl; }

#define MAKE_STATE(name) struct name : State { \
		virtual void Dump() { \
			std::cout << "  " << #name << std::endl; } \
	};

#define MAKE_STATE_1(name, p1_type) struct name : State { \
		p1_type m_Param1; \
		name(p1_type param1) : m_Param1(param1) {} \
		virtual bool IsEqual(State *other) { \
			name *other2 = dynamic_cast<name*>(other); \
			if (!other2) \
				return false; \
			return other2->m_Param1 == m_Param1;\
		} \
		virtual void Dump() { \
			std::cout << "  " << #name << "; Param1=" << m_Param1 << std::endl; } \
	}

#define MAKE_STATE_2(name, p1_type, p2_type) struct name : State { \
		p1_type m_Param1; \
		p2_type m_Param2; \
		name(p1_type param1, p2_type param2) : m_Param1(param1), m_Param2(param2) {} \
		virtual bool IsEqual(State *other) { \
			name *other2 = dynamic_cast<name*>(other); \
			if (!other2) \
				return false; \
			return other2->m_Param1 == m_Param1 && other2->m_Param2 == m_Param2;\
		} \
		virtual void Dump() { \
			std::cout << "  " << #name << "; Param1=" << m_Param1 << "; Param2=" << m_Param2 << std::endl; } \
	}
#endif

