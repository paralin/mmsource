/* ======== SourceHook ========
* Copyright (C) 2004-2005 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Pavol "PM OnoTo" Marko
* ============================
*/

/**
 * @file sourcehook.cpp
 * @brief Contains the implementation of the SourceHook API
*/

#include <algorithm>
#include "sourcehook_impl.h"

namespace SourceHook
{
	CSourceHookImpl::CSourceHookImpl()
	{
	}

	CSourceHookImpl::~CSourceHookImpl()
	{
	}

	bool CSourceHookImpl::IsPluginInUse(Plugin plug)
	{
		// Iterate through all hook managers which are in this plugin
		// Iterate through their vfnptrs, ifaces, hooks
		// If a hook from an other plugin is found, return true
		// Return false otherwise
#define TMP_CHECK_LIST(name) \
	for (hook_iter = iface_iter->name.begin(); hook_iter != iface_iter->name.end(); ++hook_iter) \
		if (hook_iter->plug == plug) \
			return true;

		for (HookManInfoList::iterator hmil_iter = m_HookMans.begin(); hmil_iter != m_HookMans.end(); ++hmil_iter)
		{
			if (hmil_iter->plug != plug)
				continue;
			for (HookManagerInfo::VfnPtrListIter vfnptr_iter = hmil_iter->vfnptrs.begin();
				vfnptr_iter != hmil_iter->vfnptrs.end(); ++vfnptr_iter)
			{
				for (HookManagerInfo::VfnPtr::IfaceListIter iface_iter = vfnptr_iter->ifaces.begin();
					iface_iter != vfnptr_iter->ifaces.end(); ++iface_iter)
				{
					std::list<HookManagerInfo::VfnPtr::Iface::Hook>::iterator hook_iter;
					TMP_CHECK_LIST(hooks_pre);
					TMP_CHECK_LIST(hooks_post);
				}
			}
		}
#undef TMP_CHECK_LIST
		return false;
	}

	void CSourceHookImpl::UnloadPlugin(Plugin plug)
	{
		// 1) Manually remove all hooks by this plugin
		std::list<RemoveHookInfo> hookstoremove;
#define TMP_CHECK_LIST(name, ispost) \
	for (hook_iter = iface_iter->name.begin(); hook_iter != iface_iter->name.end(); ++hook_iter) \
		if (hook_iter->plug == plug) \
			hookstoremove.push_back(RemoveHookInfo(hook_iter->plug, iface_iter->ptr, \
			hmil_iter->func, hook_iter->handler, ispost))
		for (HookManInfoList::iterator hmil_iter = m_HookMans.begin(); hmil_iter != m_HookMans.end(); ++hmil_iter)
		{
			for (HookManagerInfo::VfnPtrListIter vfnptr_iter = hmil_iter->vfnptrs.begin();
				vfnptr_iter != hmil_iter->vfnptrs.end(); ++vfnptr_iter)
			{
				for (HookManagerInfo::VfnPtr::IfaceListIter iface_iter = vfnptr_iter->ifaces.begin();
					iface_iter != vfnptr_iter->ifaces.end(); ++iface_iter)
				{
					std::list<HookManagerInfo::VfnPtr::Iface::Hook>::iterator hook_iter;
					TMP_CHECK_LIST(hooks_pre, false);
					TMP_CHECK_LIST(hooks_post, true);
				}
			}
		}
#undef TMP_CHECK_LIST

		for (std::list<RemoveHookInfo>::iterator rmiter = hookstoremove.begin(); rmiter != hookstoremove.end(); ++rmiter)
			RemoveHook(*rmiter);

		// 2) Other plugins may use hook managers in this plugin.
		// Get a list of hook managers that are in this plugin and are used by other plugins
		// Delete all hook managers that are in this plugin

		HookManInfoList tmphookmans;
		bool erase = false;
		for (HookManInfoList::iterator hmil_iter = m_HookMans.begin(); hmil_iter != m_HookMans.end();
			erase ? hmil_iter=m_HookMans.erase(hmil_iter) : ++hmil_iter)
		{
			if (hmil_iter->plug == plug)
			{
				if (!hmil_iter->vfnptrs.empty())
				{
					// All hooks by this plugin are already removed
					// So if there is a vfnptr, it has to be used by an other plugin
					tmphookmans.push_back(*hmil_iter);
				}
				erase = true;
			}
			else
				erase = false;
		}
		
		// For each hook manager:
		for (HookManInfoList::iterator hmil_iter = tmphookmans.begin(); hmil_iter != tmphookmans.end(); ++hmil_iter)
		{
			// Find a suitable hook manager in an other plugin
			HookManInfoList::iterator newHookMan = FindHookMan(m_HookMans.begin(), m_HookMans.end(),
				hmil_iter->proto, hmil_iter->vtbl_offs, hmil_iter->vtbl_idx);

			// This should _never_ happen.
			// If there is a hook from an other plugin, the plugin must have provided a hook manager as well.
			SH_ASSERT(newHookMan != m_HookMans.end(),
				"Could not find a suitable hook manager in an other plugin!");

			// AddHook should make sure that every plugin only has _one_ hook manager for _one_ proto/vi/vo
			SH_ASSERT(newHookMan->plug != plug, "New hook manager from same plugin!");

			// The first hook manager should be always used - so the new hook manager has to be empty
			SH_ASSERT(newHookMan->vfnptrs.empty(), "New hook manager not empty!");

			// Move the vfnptrs from the old hook manager to the new one
			newHookMan->vfnptrs = hmil_iter->vfnptrs;

			// Unregister the old one, register the new one
			hmil_iter->func(HA_Unregister, NULL);
			newHookMan->func(HA_Register, &(*newHookMan));
		}
	}

	void CSourceHookImpl::CompleteShutdown()
	{
		std::list<RemoveHookInfo> hookstoremove;
#define TMP_CHECK_LIST(name, ispost) \
	for (hook_iter = iface_iter->name.begin(); hook_iter != iface_iter->name.end(); ++hook_iter) \
		hookstoremove.push_back(RemoveHookInfo(hook_iter->plug, iface_iter->ptr, \
		hmil_iter->func, hook_iter->handler, ispost))
		for (HookManInfoList::iterator hmil_iter = m_HookMans.begin(); hmil_iter != m_HookMans.end(); ++hmil_iter)
		{
			for (HookManagerInfo::VfnPtrListIter vfnptr_iter = hmil_iter->vfnptrs.begin();
				vfnptr_iter != hmil_iter->vfnptrs.end(); ++vfnptr_iter)
			{
				for (HookManagerInfo::VfnPtr::IfaceListIter iface_iter = vfnptr_iter->ifaces.begin();
					iface_iter != vfnptr_iter->ifaces.end(); ++iface_iter)
				{
					std::list<HookManagerInfo::VfnPtr::Iface::Hook>::iterator hook_iter;
					TMP_CHECK_LIST(hooks_pre, false);
					TMP_CHECK_LIST(hooks_post, true);
				}
			}
		}
#undef TMP_CHECK_LIST

		for (std::list<RemoveHookInfo>::iterator rmiter = hookstoremove.begin(); rmiter != hookstoremove.end(); ++rmiter)
			RemoveHook(*rmiter);

		m_HookMans.clear();
	}

	bool CSourceHookImpl::AddHook(Plugin plug, void *iface, int thisptr_offs, HookManagerPubFunc myHookMan, ISHDelegate *handler, bool post)
	{
		void *adjustediface = reinterpret_cast<void*>(reinterpret_cast<char*>(iface) + thisptr_offs);
		// 1) Get info about the hook manager
		HookManagerInfo tmp;
		if (myHookMan(HA_GetInfo, &tmp) != 0)
			return false;

		// Add the proposed hook manager to the _end_ of the list if the plugin doesn't have a hook manager with this proto/vo/vi registered
		HookManInfoList::iterator hkmi_iter;
		for (hkmi_iter = m_HookMans.begin(); hkmi_iter != m_HookMans.end(); ++hkmi_iter)
		{
			if (hkmi_iter->plug == plug && strcmp(hkmi_iter->proto, tmp.proto) == 0 &&
				hkmi_iter->vtbl_offs == tmp.vtbl_offs && hkmi_iter->vtbl_idx == tmp.vtbl_idx)
				break;
		}
		if (hkmi_iter == m_HookMans.end())
		{
			// No such hook manager from this plugin yet, add it!
			tmp.func = myHookMan;
			tmp.plug = plug;
			m_HookMans.push_back(tmp);
		}

		// Then, search for a suitable hook manager (from the beginning)
		HookManInfoList::iterator hookman = FindHookMan(m_HookMans.begin(), m_HookMans.end(), tmp.proto, tmp.vtbl_offs, tmp.vtbl_idx);
		SH_ASSERT(hookman != m_HookMans.end(), "No hookman found - but if there was none, we've just added one!");

		// Tell it to store the pointer if it's not already active
		if (hookman->vfnptrs.empty())
			hookman->func(HA_Register, &(*hookman));

		void **cur_vtptr = *reinterpret_cast<void***>(
			reinterpret_cast<char*>(adjustediface) + tmp.vtbl_offs);
		void *cur_vfnptr = reinterpret_cast<void*>(cur_vtptr + tmp.vtbl_idx);

		HookManagerInfo::VfnPtrListIter vfnptr_iter = std::find(
			hookman->vfnptrs.begin(), hookman->vfnptrs.end(), cur_vfnptr);

		if (vfnptr_iter == hookman->vfnptrs.end())
		{
			// Add a new one
			HookManagerInfo::VfnPtr vfp;
			vfp.vfnptr = cur_vfnptr;
			vfp.orig_entry = *reinterpret_cast<void**>(cur_vfnptr);
			hookman->vfnptrs.push_back(vfp);

			// Alter vtable entry
			SetMemAccess(cur_vtptr, sizeof(void*) * (tmp.vtbl_idx + 1), SH_MEM_READ | SH_MEM_WRITE);
			*reinterpret_cast<void**>(cur_vfnptr) = *reinterpret_cast<void**>(hookman->hookfunc_vfnptr);

			// Make vfnptr_iter point to the new element
			vfnptr_iter = hookman->vfnptrs.end();
			--vfnptr_iter;

			// Now that it is done, check whether we have to update any callclasses
			for (Impl_CallClassList::iterator cciter = m_CallClasses.begin(); cciter != m_CallClasses.end(); ++cciter)
				if (cciter->cc.ptr == iface)
					ApplyCallClassPatch(*cciter, tmp.vtbl_offs, tmp.vtbl_idx, vfnptr_iter->orig_entry);
		}

		HookManagerInfo::VfnPtr::IfaceListIter iface_iter = std::find(
			vfnptr_iter->ifaces.begin(), vfnptr_iter->ifaces.end(), adjustediface);
		if (iface_iter == vfnptr_iter->ifaces.end())
		{
			// Add a new one
			HookManagerInfo::VfnPtr::Iface ifs;
			ifs.ptr = adjustediface;
			vfnptr_iter->ifaces.push_back(ifs);

			// Make iface_iter point to the new element
			iface_iter = vfnptr_iter->ifaces.end();
			--iface_iter;
		}

		// Add the hook
		HookManagerInfo::VfnPtr::Iface::Hook hookinfo;
		hookinfo.handler = handler;
		hookinfo.plug = plug;
		hookinfo.paused = false;
		if (post)
   			iface_iter->hooks_post.push_back(hookinfo);
		else
			iface_iter->hooks_pre.push_back(hookinfo);


		return true;
	}

	bool CSourceHookImpl::RemoveHook(RemoveHookInfo info)
	{
		return RemoveHook(info.plug, info.iface, info.hookman, info.handler, info.post);
	}

	bool CSourceHookImpl::RemoveHook(Plugin plug, void *iface, int thisptr_offs, HookManagerPubFunc myHookMan, ISHDelegate *handler, bool post)
	{
		return RemoveHook(plug, reinterpret_cast<void*>(reinterpret_cast<char*>(iface)+thisptr_offs),
			myHookMan, handler, post);
	}

	bool CSourceHookImpl::RemoveHook(Plugin plug, void *adjustediface, HookManagerPubFunc myHookMan, ISHDelegate *handler, bool post)
	{
		HookManagerInfo tmp;
		if (myHookMan(HA_GetInfo, &tmp) != 0)
			return false;

		// Find the hook manager and the hook
		HookManInfoList::iterator hookman = FindHookMan(m_HookMans.begin(), m_HookMans.end(), 
			tmp.proto, tmp.vtbl_offs, tmp.vtbl_idx);
		if (hookman == m_HookMans.end())
			return false;

		void **cur_vtptr = *reinterpret_cast<void***>(
			reinterpret_cast<char*>(adjustediface) + tmp.vtbl_offs);
		void *cur_vfnptr = reinterpret_cast<void*>(cur_vtptr + tmp.vtbl_idx);

		for (HookManagerInfo::VfnPtrListIter vfnptr_iter = hookman->vfnptrs.begin();
			vfnptr_iter != hookman->vfnptrs.end(); ++vfnptr_iter)
		{
			if (vfnptr_iter->vfnptr == cur_vfnptr)
			{
				for (HookManagerInfo::VfnPtr::IfaceListIter iface_iter = vfnptr_iter->ifaces.begin();
					iface_iter != vfnptr_iter->ifaces.end(); ++iface_iter)
				{
					std::list<HookManagerInfo::VfnPtr::Iface::Hook> &hooks =
						post ? iface_iter->hooks_post : iface_iter->hooks_pre;

					bool erase;
					for (std::list<HookManagerInfo::VfnPtr::Iface::Hook>::iterator hookiter = hooks.begin();
						hookiter != hooks.end(); erase ? hookiter = hooks.erase(hookiter) : ++hookiter)
					{
						erase = hookiter->plug == plug && hookiter->handler->IsEqual(handler);
						if (erase)
							hookiter->handler->DeleteThis();			// Make the _plugin_ delete the handler object
					}
					if (iface_iter->hooks_post.empty() && iface_iter->hooks_pre.empty())
					{
						vfnptr_iter->ifaces.erase(iface_iter);
						if (vfnptr_iter->ifaces.empty())
						{
							// Deactivate the hook
							*reinterpret_cast<void**>(vfnptr_iter->vfnptr) = vfnptr_iter->orig_entry;
							
							hookman->vfnptrs.erase(vfnptr_iter);

							// Remove callclass patch
							for (Impl_CallClassList::iterator cciter = m_CallClasses.begin(); cciter != m_CallClasses.end(); ++cciter)
								if (cciter->cc.ptr == adjustediface)
									RemoveCallClassPatch(*cciter, tmp.vtbl_offs, tmp.vtbl_idx);

							if (hookman->vfnptrs.empty())
							{
								// Unregister the hook manager
								hookman->func(HA_Unregister, NULL);
							}

						}
					}
				}
				return true;
			}
		}
		return false;
	}

	GenericCallClass *CSourceHookImpl::GetCallClass(void *iface)
	{
		for (Impl_CallClassList::iterator cciter = m_CallClasses.begin(); cciter != m_CallClasses.end(); ++cciter)
		{
			if (cciter->cc.ptr == iface)
			{
				++cciter->refcounter;
				return &cciter->cc;
			}
		}

		CallClassInfo tmp;
		tmp.refcounter = 1;
		tmp.cc.ptr = iface;

		for (HookManInfoList::iterator hookman = m_HookMans.begin(); hookman != m_HookMans.end(); ++hookman)
			for (HookManagerInfo::VfnPtrListIter vfnptr_iter = hookman->vfnptrs.begin();
				vfnptr_iter != hookman->vfnptrs.end(); ++vfnptr_iter)
				for (HookManagerInfo::VfnPtr::IfaceListIter iface_iter = vfnptr_iter->ifaces.begin();
					iface_iter != vfnptr_iter->ifaces.end(); ++iface_iter)
					if (iface_iter->ptr == iface)
						ApplyCallClassPatch(tmp, hookman->vtbl_offs, hookman->vtbl_idx,
						vfnptr_iter->orig_entry);

		m_CallClasses.push_back(tmp);
		return &m_CallClasses.back().cc;
	}

	void CSourceHookImpl::ReleaseCallClass(GenericCallClass *ptr)
	{
		Impl_CallClassList::iterator iter = std::find(m_CallClasses.begin(), m_CallClasses.end(), ptr);
		if (iter == m_CallClasses.end())
			return;
		--iter->refcounter;
		if (iter->refcounter < 1)
			m_CallClasses.erase(iter);
	}

	void CSourceHookImpl::ApplyCallClassPatch(CallClassInfo &cc, int vtbl_offs, int vtbl_idx, void *orig_entry)
	{
		OrigFuncs &tmpvec = cc.cc.vt[vtbl_offs];
		if (tmpvec.size() <= (size_t)vtbl_idx)
			tmpvec.resize(vtbl_idx+1);
		tmpvec[vtbl_idx] = orig_entry;
	}

	void CSourceHookImpl::RemoveCallClassPatch(CallClassInfo &cc, int vtbl_offs, int vtbl_idx)
	{
		OrigVTables::iterator iter = cc.cc.vt.find(vtbl_offs);
		if (iter != cc.cc.vt.end())
		{
			if (iter->second.size() > (size_t)vtbl_idx)
			{
				iter->second[vtbl_idx] = 0;
				// Free some memory if possible
				OrigFuncs::reverse_iterator riter;
				for (riter = iter->second.rbegin(); riter != iter->second.rend(); ++riter)
				{
					if (*riter != 0)
						break;
				}
				iter->second.resize(iter->second.size() - (riter - iter->second.rbegin()));
				if (!iter->second.size())
					cc.cc.vt.erase(iter);
			}
		}
	}

	CSourceHookImpl::HookManInfoList::iterator CSourceHookImpl::FindHookMan(HookManInfoList::iterator begin,
		HookManInfoList::iterator end, const char *proto, int vtblofs, int vtblidx)
	{
		for (HookManInfoList::iterator hookmaniter = m_HookMans.begin(); hookmaniter != m_HookMans.end(); ++hookmaniter)
		{
			if (strcmp(hookmaniter->proto, proto) == 0 && hookmaniter->vtbl_offs == vtblofs &&
				hookmaniter->vtbl_idx == vtblidx)
				break;
		}
		return hookmaniter;
	}

	void CSourceHookImpl::PausePlugin(Plugin plug)
	{
		// Go through all hook managers, all interfaces, and set the status of all hooks of this plugin to paused
		for (HookManInfoList::iterator hookmaniter = m_HookMans.begin(); hookmaniter != m_HookMans.end(); ++hookmaniter)
			for (HookManagerInfo::VfnPtrListIter vfnptr_iter = hookmaniter->vfnptrs.begin();
				vfnptr_iter != hookmaniter->vfnptrs.end(); ++vfnptr_iter)
				for (HookManagerInfo::VfnPtr::IfaceListIter ifaceiter = vfnptr_iter->ifaces.begin();
					ifaceiter != vfnptr_iter->ifaces.end(); ++ifaceiter)
				{
					for (std::list<HookManagerInfo::VfnPtr::Iface::Hook>::iterator hookiter = ifaceiter->hooks_pre.begin();
						hookiter != ifaceiter->hooks_pre.end(); ++hookiter)
						if (plug == hookiter->plug)
							hookiter->paused = true;

					for (std::list<HookManagerInfo::VfnPtr::Iface::Hook>::iterator hookiter = ifaceiter->hooks_post.begin();
						hookiter != ifaceiter->hooks_post.end(); ++hookiter)
						if (plug == hookiter->plug)
							hookiter->paused = true;
				}
	}

	void CSourceHookImpl::UnpausePlugin(Plugin plug)
	{
		// Go through all hook managers, all interfaces, and set the status of all hooks of this plugin to paused
		for (HookManInfoList::iterator hookmaniter = m_HookMans.begin(); hookmaniter != m_HookMans.end(); ++hookmaniter)
			for (HookManagerInfo::VfnPtrListIter vfnptr_iter = hookmaniter->vfnptrs.begin();
				vfnptr_iter != hookmaniter->vfnptrs.end(); ++vfnptr_iter)
				for (HookManagerInfo::VfnPtr::IfaceListIter ifaceiter = vfnptr_iter->ifaces.begin();
					ifaceiter != vfnptr_iter->ifaces.end(); ++ifaceiter)
				{
					for (std::list<HookManagerInfo::VfnPtr::Iface::Hook>::iterator hookiter = ifaceiter->hooks_pre.begin();
						hookiter != ifaceiter->hooks_pre.end(); ++hookiter)
						if (plug == hookiter->plug)
							hookiter->paused = false;

					for (std::list<HookManagerInfo::VfnPtr::Iface::Hook>::iterator hookiter = ifaceiter->hooks_post.begin();
						hookiter != ifaceiter->hooks_post.end(); ++hookiter)
						if (plug == hookiter->plug)
							hookiter->paused = false;
				}
	}

	void CSourceHookImpl::SetRes(META_RES res)
	{
		m_CurRes = res;
	}

	META_RES CSourceHookImpl::GetPrevRes()
	{
		return m_PrevRes;
	}

	META_RES CSourceHookImpl::GetStatus()
	{
		return m_Status;
	}

	const void *CSourceHookImpl::GetOrigRet()
	{
		return m_OrigRet;
	}

	const void *CSourceHookImpl::GetOverrideRet()
	{
		return m_OverrideRet;
	}

	META_RES &CSourceHookImpl::GetCurResRef()
	{
		return m_CurRes;
	}

	META_RES &CSourceHookImpl::GetPrevResRef()
	{
		return m_PrevRes;
	}

	META_RES &CSourceHookImpl::GetStatusRef()
	{
		return m_Status;
	}

	void CSourceHookImpl::SetOrigRet(const void *ptr)
	{
		m_OrigRet = ptr;
	}

	void CSourceHookImpl::SetOverrideRet(const void *ptr)
	{
		m_OverrideRet = ptr;
	}

	void CSourceHookImpl::SetIfacePtr(void *ptr)
	{
		m_IfacePtr = ptr;
	}
	void *CSourceHookImpl::GetIfacePtr()
	{
		return m_IfacePtr;
	}
}
