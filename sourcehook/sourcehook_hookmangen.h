/* ======== SourceHook ========
* Copyright (C) 2004-2007 Metamod:Source Development Team
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Pavol "PM OnoTo" Marko
* ============================
*/

#ifndef __SOURCEHOOK_HOOKMANGEN_H__
#define __SOURCEHOOK_HOOKMANGEN_H__

#include "sourcehook_impl.h"
#include "windows.h"

namespace SourceHook
{
	namespace Impl
	{

		// Code gen stuff
#if defined HAVE_STDINT_H && !defined WIN32
#include <stdint.h>
		typedef int8_t jit_int8_t;
		typedef uint8_t jit_uint8_t;
		typedef int32_t jit_int32_t;
		typedef uint32_t jit_uint32_t;
		typedef int64_t jit_int64_t;
		typedef uint64_t jit_uint64_t;
#elif defined WIN32
		typedef __int8 jit_int8_t;
		typedef unsigned __int8 jit_uint8_t;
		typedef __int32 jit_int32_t;
		typedef unsigned __int32 jit_uint32_t;
		typedef __int64 jit_int64_t;
		typedef unsigned __int64 jit_uint64_t;
#endif
		typedef unsigned int jitoffs_t;
		typedef signed int jitrel_t;


		class GenBuffer
		{
			static CPageAlloc ms_Allocator;

			unsigned char *m_pData;
			jitoffs_t m_Size;
			jitoffs_t m_AllocatedSize;

		public:
			GenBuffer() : m_pData(NULL), m_Size(0), m_AllocatedSize(0)
			{
			}
			~GenBuffer()
			{
				clear();
			}
			jitoffs_t GetSize()
			{
				return m_Size;
			}
			unsigned char *GetData()
			{
				return m_pData;
			}

			template <class PT> void push(PT what)
			{
				push((const unsigned char *)&what, sizeof(PT));
			}

			void push(const unsigned char *data, jitoffs_t size)
			{
				jitoffs_t newSize = m_Size + size;
				if (newSize > m_AllocatedSize)
				{
					m_AllocatedSize = newSize > m_AllocatedSize*2 ? newSize : m_AllocatedSize*2;
					if (m_AllocatedSize < 64)
						m_AllocatedSize = 64;

					unsigned char *newBuf;
					newBuf = reinterpret_cast<unsigned char*>(ms_Allocator.Alloc(m_AllocatedSize));
					ms_Allocator.SetRW(newBuf);
					if (!newBuf)
					{
						SH_ASSERT(0, ("bad_alloc: couldn't allocate 0x%08X bytes of memory\n", m_AllocatedSize));
						return;
					}
					memset((void*)newBuf, 0xCC, m_AllocatedSize);			// :TODO: remove this !
					memcpy((void*)newBuf, (const void*)m_pData, m_Size);
					if (m_pData)
						ms_Allocator.Free(reinterpret_cast<void*>(m_pData));
					m_pData = newBuf;
				}
				memcpy((void*)(m_pData + m_Size), (const void*)data, size);
				m_Size = newSize;
			}

			template <class PT> void rewrite(jitoffs_t offset, PT what)
			{
				rewrite(offset, (const unsigned char *)&what, sizeof(PT));
			}

			void rewrite(jitoffs_t offset, const unsigned char *data, jitoffs_t size)
			{
				SH_ASSERT(offset + size <= m_AllocatedSize, ("rewrite too far"));

				memcpy((void*)(m_pData + offset), (const void*)data, size);
			}

			void clear()
			{
				if (m_pData)
					ms_Allocator.Free(reinterpret_cast<void*>(m_pData));
				m_pData = NULL;
				m_Size = 0;
				m_AllocatedSize = 0;
			}

			void SetRE()
			{
				ms_Allocator.SetRE(reinterpret_cast<void*>(m_pData));
			}

			operator unsigned char *()
			{
				return GetData();
			}

			void write_ubyte(jit_uint8_t x)			{ push(x); }
			void write_byte(jit_uint8_t x)			{ push(x); }
			
			void write_ushort(unsigned short x)		{ push(x); }
			void write_short(signed short x)		{ push(x); }

			void write_uint32(jit_uint32_t x)		{ push(x); }
			void write_int32(jit_uint32_t x)		{ push(x); }

			jitoffs_t get_outputpos()
			{
				return m_Size;
			}

			void start_count(jitoffs_t &offs)
			{
				offs = get_outputpos();
			}
			void end_count(jitoffs_t &offs)
			{
				offs = get_outputpos() - offs;
			}
		};

		class GenContext
		{
			const static int SIZE_MWORD = 4;
			const static int SIZE_PTR = sizeof(void*);

			CProto m_Proto;
			int m_VtblOffs;
			int m_VtblIdx;
			ISourceHook *m_SHPtr;

			GenBuffer m_HookFunc;
			GenBuffer m_PubFunc;

			ProtoInfo *m_BuiltPI;
			PassInfo *m_BuiltPI_Params;
			PassInfo::V2Info *m_BuiltPI_Params2;

			// For hookfunc
			void **m_pHI;
			void **m_HookfuncVfnptr;

			// Level 3 - Helpers
			int m_RegCounter;
			jit_int8_t NextRegEBX_ECX_EDX();

			// size info
			jit_int32_t GetRealSize(const IntPassInfo &info);
			jit_int32_t GetStackSize(const IntPassInfo &info);
			short GetParamsStackSize();		// sum(GetStackSize(i), 0 <= i < numOfParams)

			// Helpers
			void BitwiseCopy_Setup();
			void BitwiseCopy_Do(size_t size);

			// Param push
			jit_int32_t PushParams(jit_int32_t param_base_offset, jit_int32_t save_ret_to, 
				jit_int32_t v_place_for_memret);		// save_ret_to and v_place_for_memret only used for memory returns
			jit_int32_t PushRef(jit_int32_t param_offset, const IntPassInfo &pi);
			jit_int32_t PushBasic(jit_int32_t param_offset, const IntPassInfo &pi);
			jit_int32_t PushFloat(jit_int32_t param_offset, const IntPassInfo &pi);
			jit_int32_t PushObject(jit_int32_t param_offset, const IntPassInfo &pi);

			// Ret val processing
			void SaveRetVal(jit_int32_t v_where, jit_int32_t v_place_for_memret);
			void ProcessPluginRetVal(jit_int32_t v_cur_res, jit_int32_t v_pContext, jit_int32_t v_plugin_ret);

			void PrepareReturn(jit_int32_t v_status, jit_int32_t v_pContext, jit_int32_t v_retptr);
			void DoReturn(jit_int32_t v_retptr, jit_int32_t v_memret_outaddr);

			bool MemRetWithTempObj();			// do we do a memory return AND need a temporary place for it?

			// Call hooks
			void GenerateCallHooks(int v_status, int v_prev_res, int v_cur_res, int v_iter,
				int v_pContext, int base_param_offset, int v_plugin_ret, int v_place_for_memret);

			// Call orig
			void GenerateCallOrig(int v_status, int v_pContext, int param_base_offs, int v_this,
				int v_vfnptr_origentry, int v_orig_ret, int v_override_ret, int v_place_for_memret);

			// Hook loop
			void CallSetupHookLoop(int v_orig_ret, int v_override_ret, 
				int v_cur_res, int v_prev_res, int v_status, int v_vnfptr_origentry,
				int v_this, int v_pContext);

			void CallEndContext(int v_pContext);

			// Level 2 -> called from Generate()
			void AutoDetectRetType();
			bool PassInfoSupported(const IntPassInfo &pi, bool is_ret);
			void Clear();
			void BuildProtoInfo();
			unsigned char *GenerateHookFunc();
			unsigned char *GeneratePubFunc();
		public:
			// Level 1 -> Public interface
			GenContext(const ProtoInfo *proto, int vtbl_offs, int vtbl_idx, ISourceHook *pSHPtr);
			~GenContext();

			HookManagerPubFunc Generate();
		};
	}
}


#endif