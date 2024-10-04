// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once

#include "smsdk_ext.h"
#include "rust_exports.h"


extern bool (*MyQueryInterfaceDrop)(SMInterface*);
extern void (*MyNotifyInterfaceDrop)(SMInterface*);

extern CGlobalVars* gpGlobals;
#ifndef META_NO_HL2SDK
class IFileSystem;
extern IFileSystem *filesystem;
#endif


inline const char** GlobalsMapname()
{
#ifdef SOURCE_ENGINE
	return (const char**)&gpGlobals->mapname;
#else
	return &(((const char**)gpGlobals)[0x3C/4]); // TODO: WHAT THE FUCK! STOP HARDCODING SHIT!!!
#endif
}

// NOTE: I use atcprintf instead of ISourceMod::FormatString...
//       I don't want to have unnecessary string copying if there's no varargs...
// TODO: Look into turning this into a function... this is used a lot in natives_json.cpp & it probably bloats the code...
#define MAYBE_FORMAT(arg, var)        \
	int fmtlen = 0;                   \
	char fmtbuffer[1024];             \
	if (var && params[0] > (arg))     \
	{                                 \
		int varargparam = (arg) + 1;  \
		DetectExceptions eh(ctx);     \
		fmtlen = (int)atcprintf(fmtbuffer, sizeof(fmtbuffer), var, ctx, params, &varargparam); \
		if (eh.HasException())        \
			return 0;                 \
		var = fmtbuffer;              \
	}

class MyExtension : public SDKExtension
{
public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlen	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);

	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	virtual void SDK_OnAllLoaded();

		/**
	 * @brief Called on server activation before plugins receive the OnServerLoad forward.
	 *
	 * @param pEdictList		Edicts list.
	 * @param edictCount		Number of edicts in the list.
	 * @param clientMax			Maximum number of clients allowed in the server.
	 */
	virtual void OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax);

	/**
	 * @brief Called on level shutdown
	 *
	 */
	virtual void OnCoreMapEnd();

	/**
	 * @brief Called when the pause state is changed.
	 */
	//virtual void SDK_OnPauseChange(bool paused);

	/**
	 * @brief Asks the extension whether it's safe to remove an external
	 * interface it's using.  If it's not safe, return false, and the
	 * extension will be unloaded afterwards.
	 *
	 * NOTE: It is important to also hook NotifyInterfaceDrop() in order to clean
	 * up resources.
	 *
	 * @param pInterface		Pointer to interface being dropped.  This
	 * 							pointer may be opaque, and it should not
	 *							be queried using SMInterface functions unless
	 *							it can be verified to match an existing
	 *							pointer of known type.
	 * @return					True to continue, false to unload this
	 * 							extension afterwards.
	 */
	virtual bool QueryInterfaceDrop(SMInterface *pInterface);

	/**
	 * @brief Notifies the extension that an external interface it uses is being removed.
	 *
	 * @param pInterface		Pointer to interface being dropped.  This
	 * 							pointer may be opaque, and it should not
	 *							be queried using SMInterface functions unless
	 *							it can be verified to match an existing
	 */
	virtual void NotifyInterfaceDrop(SMInterface *pInterface);

	/**
	 * @brief this is called when Core wants to know if your extension is working.
	 *
	 * @param error		Error message buffer.
	 * @param maxlen	Size of error message buffer.
	 * @return			True if working, false otherwise.
	 */
	//virtual bool QueryRunning(char *error, size_t maxlen);
public:
#if defined SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodUnload(char *error, size_t maxlen);

	/**
	 * @brief Called when Metamod's pause state is changing.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param paused		Pause state being set.
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	//virtual bool SDK_OnMetamodPauseChange(bool paused, char *error, size_t maxlen);
#endif

public: //IExtensionInterface
	virtual const char *GetExtensionName();
	virtual const char *GetExtensionURL();
	virtual const char *GetExtensionTag();
	virtual const char *GetExtensionAuthor();
	virtual const char *GetExtensionVerString();
	virtual const char *GetExtensionDescription();
	virtual const char *GetExtensionDateString();

public: //ISmmPlugin
	/** Returns the license to MM */
	virtual const char *GetLicense();
};
