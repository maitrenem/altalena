/*
*	The Altalena Project File
*	Copyright (C) 2009  Boris Ouretskey
*
*	This library is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	This library is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with this library; if not, write to the Free Software
*	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "StdAfx.h"
#include "LuaStaticApi.h"
#include "BridgeMacros.h"
#include "LoggerBridge.h"
#include "ConfBridge.h"
#include "SipCallBridge.h"
#include "RTPProxyBridge.h"
#include "LuaRestoreStack.h"
#include "StreamerBridge.h"
#include "SelectorBridge.h"
#include "LuaTable.h"
#include "MrcpBridge.h"
#include "RtspBridge.h"

namespace ivrworx
{

	class ProcBlockingOperationRunner
		:public LightweightProcess
	{
	public:
		ProcBlockingOperationRunner(LpHandlePair pair, lua_State *L, int &err):
		  LightweightProcess(pair,"LongOp runner"),
			  _L(L),
			  _err(err)
		  {

		  };

	protected:

		void real_run()
		{

			FUNCTRACKER;


			if (lua_isfunction (_L, -1))
			{
				_err  = lua_pcall (_L, 0, 0, 0);
			} 
			else
			{
				LogWarn("ProcBlockingOperationRunner::real_run - wrong param");
				_err = 1;
			}


		}

	protected:

		lua_State * _L;

		int &_err;

	};

	


	IW_IVR_API BOOL
	InitStaticTypes(lua_State *L, LuaTable &ivrworxTable, const Context *ctx)
	{
		FUNCTRACKER;

		ivrworxTable.AddParam(NAME(API_SUCCESS),API_SUCCESS);
		ivrworxTable.AddParam(NAME(API_FAILURE),API_FAILURE);
		ivrworxTable.AddParam(NAME(API_SERVER_FAILURE),API_SERVER_FAILURE);
		ivrworxTable.AddParam(NAME(API_TIMEOUT),API_TIMEOUT);
		ivrworxTable.AddParam(NAME(API_WRONG_PARAMETER),API_WRONG_PARAMETER);
		ivrworxTable.AddParam(NAME(API_WRONG_STATE),API_WRONG_STATE);
		ivrworxTable.AddParam(NAME(API_HANGUP),API_HANGUP);
		ivrworxTable.AddParam(NAME(API_UNKNOWN_DESTINATION),API_UNKNOWN_DESTINATION);
		ivrworxTable.AddParam(NAME(API_UNKNOWN_RESPONSE),API_UNKNOWN_RESPONSE);
		ivrworxTable.AddParam(NAME(API_FEATURE_DISABLED),API_FEATURE_DISABLED);

		ivrworxTable.AddParam(NAME(RCV_DEVICE_FILE_REC_ID),RCV_DEVICE_FILE_REC_ID);
		ivrworxTable.AddParam(NAME(RCV_DEVICE_WINSND_WRITE),RCV_DEVICE_WINSND_WRITE);

		ivrworxTable.AddParam(NAME(SND_DEVICE_TYPE_SND_CARD_MIC),SND_DEVICE_TYPE_SND_CARD_MIC);
		ivrworxTable.AddParam(NAME(SND_DEVICE_TYPE_FILE),SND_DEVICE_TYPE_FILE);

		ivrworxTable.AddParam(NAME(RECOGNIZER),RECOGNIZER);
		ivrworxTable.AddParam(NAME(SYNTHESIZER),SYNTHESIZER);


		
		ivrworxTable.AddFunction("sleep",LuaWait);
		ivrworxTable.AddFunction("wait",LuaWait);
		ivrworxTable.AddFunction("run",LuaRunLongOperation);


		//
		// ivrworx.LOGGER
		//
		
		Luna<LoggerBridge>::RegisterType(L,LUA_RT_ALLOW_GC);
		Luna<LoggerBridge>::RegisterObject(L,new LoggerBridge(L),ivrworxTable.TableRef(),"LOGGER");

		//
		// ivrworx.CONF
		//
		Luna<ConfBridge>::RegisterType(L,LUA_RT_ALLOW_GC);
		Luna<ConfBridge>::RegisterObject(L,new ConfBridge(ctx->_conf),ivrworxTable.TableRef(),"CONF");

		


		
		Luna<rtpproxy>::RegisterType(L,LUA_RT_ALLOW_ALL,&LuaCreateRtpProxy);
		Luna<sipcall>::RegisterType(L,LUA_RT_ALLOW_ALL,&LuaCreateSip);
		Luna<streamer>::RegisterType(L,LUA_RT_ALLOW_ALL,&LuaCreateStreamer);
		Luna<mrcpsession>::RegisterType(L,LUA_RT_ALLOW_ALL,&LuaCreateMrcp);
		Luna<rtspsession>::RegisterType(L,LUA_RT_ALLOW_ALL,&LuaCreateRtspSession);
		Luna<selector>::RegisterType(L,LUA_RT_ALLOW_ALL,&LuaCreateSelector);


	
		return TRUE;
	}


	

	

	int
	LuaCreateMrcp(lua_State *L)
	{
		HandleId service_handle_id = IW_UNDEFINED;
		if (IW_FAILURE(GetConfiguredServiceHandle(service_handle_id, "ivr/mrcp_service", CTX_FIELD(_conf))))
		{
			return 0;
		}

		MrcpSessionPtr call_ptr 
			(new MrcpSession(*(CTX_FIELD(_forking)),service_handle_id));

		Luna<mrcpsession>::PushObject(L,new mrcpsession(call_ptr));

		return 1;

	}

	
	int
	LuaCreateSip(lua_State *L)
	{
		HandleId service_handle_id = IW_UNDEFINED;
		if (IW_FAILURE(GetConfiguredServiceHandle(service_handle_id, "ivr/sip_service", CTX_FIELD(_conf))))
		{
			return 0;
		}

		SipMediaCallPtr media_call_ptr =
			SipMediaCallPtr(new SipMediaCall(*(CTX_FIELD(_forking)), service_handle_id));

		Luna<sipcall>::PushObject(L, new sipcall(media_call_ptr));

		return 1;
	}

	int
	LuaCreateRtpProxy(lua_State *L)
	{
		HandleId service_handle_id = IW_UNDEFINED;
		if (IW_FAILURE(GetConfiguredServiceHandle(service_handle_id, "ivr/rtpproxy_service",  CTX_FIELD(_conf))))
		{
			return 0;
		}

		RtpProxySessionPtr media_call_ptr =
			RtpProxySessionPtr(new RtpProxySession(*(CTX_FIELD(_forking)),service_handle_id));

		Luna<rtpproxy>::PushObject(L, new rtpproxy(media_call_ptr));

		return 1;
	}

	int
	LuaCreateRtspSession(lua_State *L)
	{
		HandleId service_handle_id = IW_UNDEFINED;
		if (IW_FAILURE(GetConfiguredServiceHandle(service_handle_id, "ivr/rtsp_service",  CTX_FIELD(_conf))))
		{
			return 0;
		}

		RtspSessionPtr media_call_ptr =
			RtspSessionPtr(new RtspSession(*(CTX_FIELD(_forking)), service_handle_id));

		Luna<rtspsession>::PushObject(L, new rtspsession(media_call_ptr));

		return 1;
	}

	int
	LuaCreateStreamer(lua_State *L)
	{

		HandleId service_handle_id = IW_UNDEFINED;
		if (IW_FAILURE(GetConfiguredServiceHandle(service_handle_id, "ivr/stream_service", CTX_FIELD(_conf))))
		{
			return 0;
		}

		StreamingSessionPtr media_call_ptr =
			StreamingSessionPtr(new StreamingSession(*(CTX_FIELD(_forking)), service_handle_id));

		Luna<streamer>::PushObject(L, new streamer(media_call_ptr));

		return 1;
	}

	

	int LuaCreateSelector(lua_State *L)
	{
		
		Luna<selector>::PushObject(L, new selector(L));
		return 1;
	}

	int
	LuaWait(lua_State *state)
	{
		FUNCTRACKER;

		LUA_INT_PARAM(state,time_to_sleep,-1);

		LogDebug("IwScript::LuaWait - Wait for " << time_to_sleep);

	#pragma warning (suppress:4244)
		csp::SleepFor(MilliSeconds(time_to_sleep));
		lua_pushnumber (state, API_SUCCESS);

		return 1;
	}

	int
	LuaRunLongOperation(lua_State * state)
	{


		DECLARE_NAMED_HANDLE_PAIR(runner_pair);
		int err = 0;
		csp::Run(new ProcBlockingOperationRunner(runner_pair,state,err));

		err ? lua_pushnumber (state, API_FAILURE): lua_pushnumber (state, API_SUCCESS);

		return 1;
	}

}