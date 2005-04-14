/* ======== SourceHook ========
* By PM
* No warranties of any kind
*
* This file declares necessary stuff for the sourcehook implementation.
* ============================
*/

#ifndef __SOURCEHOOK_IMPL_H__
#define __SOURCEHOOK_IMPL_H__

#include "sourcehook.h"

// Set this to 1 to enable runtime code generation (faster)
#define SH_RUNTIME_CODEGEN 1

namespace SourceHook
{
	/**
	*	@brief The SourceHook implementation class
	*/
	class CSourceHookImpl : public ISourceHook
	{
		/**
		*	@brief A list of HookerInfo structures
		*/
		typedef std::list<HookerInfo> HookerInfoList;


		/**
		*	@brief A list of Impl_CallClass structures
		*/
		typedef std::list<CallClass> Impl_CallClassList;

		Impl_CallClassList m_CallClasses;			//!< A list of already generated callclasses
		HookerInfoList m_Hookers;					//!< A list of hookers

		int m_PageSize;								//!< Stores the system's page size

		/**
		*	@brief Finds a hooker for a function based on a text-prototype, a vtable offset and a vtable index
		*/
		HookerInfoList::iterator FindHooker(HookerInfoList::iterator begin, HookerInfoList::iterator end, 
			const char *proto, int vtblofs, int vtblidx, int thisptrofs);

		void FreeCallClass(CallClass &cc);
		bool ApplyCallClassPatch(CallClass &cc, int vtbl_offs, int vtbl_idx, void *orig_entry);

		static const int MAX_VTABLE_LEN = 4096;		//!< Maximal vtable length in bytes

		META_RES m_Status, m_PrevRes, m_CurRes;
		const void *m_OrigRet;
		const void *m_OverrideRet;
		void *m_IfacePtr;
	public:
		CSourceHookImpl();
		~CSourceHookImpl();

		/**
		*	@brief Make sure that a plugin is not used by any other plugins anymore, and unregister all its hookers
		*/
		void UnloadPlugin(Plugin plug);

		/**
		*	@brief Shut down the whole system, unregister all hookers
		*/
		void CompleteShutdown();

		/**
		*	@brief Add a hook.
		*
		*	@return True if the function succeeded, false otherwise
		*
		*	@param plug The unique identifier of the plugin that calls this function
		*	@param iface The interface pointer
		*	@param myHooker A hooker (hook manager) function that should be capable of handling the corresponding function
		*	@param handler A pointer to a FastDelegate containing the hook handler
		*	@param post Set to true if you want a post handler
		*/
		bool AddHook(Plugin plug, void *iface, int ifacesize, Hooker myHooker, ISHDelegate *handler, bool post);

		/**
		*	@brief Removes a hook.
		*
		*	@return True if the function succeeded, false otherwise
		*
		*	@param plug The unique identifier of the plugin that calls this function
		*	@param iface The interface pointer
		*	@param vtableoffset Offset to the vtable, in bytes
		*	@param vtableidx Pointer to the vtable index of the function
		*	@param proto A text version of the function prototype; generated by the macros
		*	@param handler A pointer to a FastDelegate containing the hook handler
		*	@param post Set to true if you want a post handler
		*/
		bool RemoveHook(Plugin plug, void *iface, Hooker myHooker, ISHDelegate *handler, bool post);

		/**
		*	@brief Checks whether a plugin has (a) hooker(s) that is/are currently used by other plugins
		*
		*	@param plug The unique identifier of the plugin in question
		*/
		bool IsPluginInUse(Plugin plug);

		/**
		*	@brief Return a pointer to a callclass. Generate a new one if required.
		*
		*	@param iface The interface pointer
		*/
		void *GetCallClass(void *iface, size_t size);

		/**
		*	@brief Release a callclass
		*
		*	@param ptr Pointer to the callclass
		*/
		virtual void ReleaseCallClass(void *ptr);

		virtual void SetRes(META_RES res);				//!< Sets the meta result
		virtual META_RES GetPrevRes();					//!< Gets the meta result of the previously called handler
		virtual META_RES GetStatus();					//!< Gets the highest meta result
		virtual const void *GetOrigRet();				//!< Gets the original result. If not in post function, undefined
		virtual const void *GetOverrideRet();			//!< Gets the override result. If none is specified, NULL
		virtual void *GetIfacePtr();					//!< Gets the interface pointer

		//////////////////////////////////////////////////////////////////////////
		// For hookers
		virtual META_RES &GetCurResRef();				//!< Gets the pointer to the current meta result
		virtual META_RES &GetPrevResRef();				//!< Gets the pointer to the previous meta result
		virtual META_RES &GetStatusRef();				//!< Gets the pointer to the status variable
		virtual void SetOrigRet(const void *ptr);		//!< Sets the original return pointer
		virtual void SetOverrideRet(const void *ptr);	//!< Sets the override result pointer
		virtual void SetIfacePtr(void *ptr);			//!< Sets the interface this pointer
	};
}

#endif