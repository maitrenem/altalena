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
#include "ProcSipStack.h"
#include "UASAppDialogSetFactory.h"
#include "UASDialogUsageManager.h"






using namespace resip;
using namespace std;


namespace ivrworx
{
	typedef
	boost::char_separator<char> resip_conf_separator;

	typedef 
	boost::tokenizer<resip_conf_separator> resip_conf_tokenizer;

	ResipInterruptor::ResipInterruptor(IwDialogUsageManager *dum):
	_dumPtr(dum)
	{

		

	}

	void 
	ResipInterruptor::SignalDataIn()
	{
		
		boost::mutex::scoped_lock lock(_mutex);

		if (_dumPtr)
		{
			_dumPtr->mFifo.add(NULL, TimeLimitFifo<Message>::InternalElement);
		}
		
	}

	void 
	ResipInterruptor::SignalDataOut()
	{
		


	}

	void 
	ResipInterruptor::Destroy()
	{
		FUNCTRACKER;

		boost::mutex::scoped_lock lock(_mutex);

		_dumPtr = NULL;

	}

	 
	ResipInterruptor::~ResipInterruptor()
	{
		FUNCTRACKER;

	}

	ProcSipStack::ProcSipStack(IN LpHandlePair pair, 
		IN Configuration &conf):
		LightweightProcess(pair,SIP_STACK_Q,"SipStack"),
		_shutDownFlag(false),
		_conf(conf),
		_stack(NULL, resip::DnsStub::EmptyNameserverList, &_si),
		_stackThread(_stack,_si),
		_dumMngr(_stack),
		_dumUas(_conf,_iwHandlesMap,_resipHandlesMap,pair.outbound,_dumMngr),
		_dumUac(_conf,_iwHandlesMap,_resipHandlesMap,_dumMngr)

	{
		FUNCTRACKER;

		Log::initialize(Log::OnlyExternal, Log::Debug, NULL, _logger);
		SetResipLogLevel();
				
	}

    typedef
    map<string,Subsystem *> SubsystemsMap;

	typedef
	map<string,Log::Level> LogLevelMap;

	void
	ProcSipStack::SetResipLogLevel()
	{
		LogLevelMap levelsMap;
		levelsMap["OFF"] = Log::None;
		levelsMap["CRT"] = Log::Crit;
		levelsMap["WRN"] = Log::Warning;	
		levelsMap["INF"] = Log::Info;
		levelsMap["DBG"] = Log::Debug;

		SubsystemsMap subsystemMap;
		subsystemMap["APP"]	= &Subsystem::APP;
		subsystemMap["CONTENTS"] = &Subsystem::CONTENTS;
		subsystemMap["DNS"]	= &Subsystem::DNS;	
		subsystemMap["DUM"]	= &Subsystem::DUM;
		subsystemMap["NONE"] = &Subsystem::NONE; 
		subsystemMap["PRESENCE"] = &Subsystem::PRESENCE; 
		subsystemMap["SDP"] = &Subsystem::SDP;
		subsystemMap["SIP"] = &Subsystem::SIP;
		subsystemMap["TRANSPORT"] = &Subsystem::TRANSPORT;
		subsystemMap["STATS"] = &Subsystem::STATS;
		subsystemMap["REPRO"] = &Subsystem::REPRO;

		// firstly reset all log levels to none
		for (SubsystemsMap::iterator iter = subsystemMap.begin(); 
			iter != subsystemMap.end(); 
			iter++)
		{
			iter->second->setLevel(Log::None);
		}

		

		// dirty parsing
		#pragma warning (suppress: 4129)
		regex e("(APP|CONTENTS|DNS|DUM|NONE|PRESENCE|SDP|SIP|TRANSPORT|STATS|REPRO)\,(OFF|CRT|WRN|INF|DBG)");
		

		resip_conf_separator sep("|");
		const string &resip_conf = _conf.ResipLog();

		resip_conf_tokenizer tokens(resip_conf, sep);

		for (resip_conf_tokenizer::const_iterator  tok_iter = tokens.begin();
			tok_iter != tokens.end(); 
			++tok_iter)
		{
			boost::smatch what;
			if (true == boost::regex_match(*tok_iter, what, e, boost::match_extra))
			{
				const string &subsystem_str = what[1];
				const string &debug_level = what[2];

				if (subsystem_str.length() > 0	&&
					debug_level.length() > 0	&&
					subsystemMap.find(subsystem_str) != subsystemMap.end() &&
					levelsMap.find(debug_level)      != levelsMap.end())
				{
					LogInfo("Resiprocate log level of " << subsystem_str << " set to " << debug_level << ".");
					subsystemMap[subsystem_str]->setLevel(levelsMap[debug_level]);
				} 
				else
				{
					LogWarn("Error while parsing resip debug configuration string - " << resip_conf);
					break;
				}
			}

		}


	}


	ProcSipStack::~ProcSipStack(void)
	{
		FUNCTRACKER;
		ShutDown();
	}


	ApiErrorCode
	ProcSipStack::Init()
	{
		FUNCTRACKER;

		try 
		{
			
			
			//
			// Prepare SIP stack
			//
			CnxInfo ipAddr = _conf.IvrCnxInfo();

			_stack.addTransport(
				UDP,
				ipAddr.port_ho(),
				V4,
				StunDisabled,
				ipAddr.iptoa(),
				Data::Empty, // only used for TLS based stuff 
				Data::Empty,
				SecurityTypes::TLSv1 );



			_stack.addTransport(
				TCP,
				ipAddr.port_ho(),
				V4,
				StunDisabled,
				ipAddr.iptoa(),
				Data::Empty, // only used for TLS based stuff 
				Data::Empty,
				SecurityTypes::TLSv1 );


			//
			// Prepare interruptor
			//
			_dumInt = ResipInterruptorPtr(new ResipInterruptor(&_dumMngr));
			_inbound->HandleInterruptor(_dumInt);


			string uasUri = "sip:" + _conf.From() + "@" + ipAddr.ipporttos();
			NameAddr uasAor	(uasUri.c_str());


			_dumMngr.setMasterProfile(SharedPtr<MasterProfile>(new MasterProfile()));

			auto_ptr<ClientAuthManager> uasAuth (new ClientAuthManager());
			_dumMngr.setClientAuthManager(uasAuth);
			

			_dumMngr.getMasterProfile()->setDefaultFrom(uasAor);
			_dumMngr.getMasterProfile()->setDefaultRegistrationTime(70);


			if (_conf.EnableSessionTimer())
			{
				_dumMngr.getMasterProfile()->addSupportedOptionTag(Token(Symbols::Timer));

				_confSessionTimerModeMap["prefer_uac"]	  = Profile::PreferUACRefreshes;
				_confSessionTimerModeMap["prefer_uas"]	  = Profile::PreferUASRefreshes;
				_confSessionTimerModeMap["prefer_local"]  = Profile::PreferLocalRefreshes;
				_confSessionTimerModeMap["prefer_remote"] = Profile::PreferRemoteRefreshes;

				ConfSessionTimerModeMap::iterator  i = _confSessionTimerModeMap.find(_conf.SipRefreshMode());
				if (i == _confSessionTimerModeMap.end())
				{
					LogInfo("Setting refresh mode to 'none'");
				}
				else
				{
					LogInfo("Setting refresh mode to " << i->first);
					_dumMngr.getMasterProfile()->setDefaultSessionTimerMode(i->second);
					_dumMngr.getMasterProfile()->setDefaultSessionTime(_conf.SipDefaultSessionTime());

				}

			}

			_dumMngr.setClientRegistrationHandler(this);
			_dumMngr.setInviteSessionHandler(this);
			_dumMngr.addClientSubscriptionHandler("refer",this);
			_dumMngr.addOutOfDialogHandler(OPTIONS, this);


			auto_ptr<AppDialogSetFactory> uas_dsf(new UASAppDialogSetFactory());
			_dumMngr.setAppDialogSetFactory(uas_dsf);
			

			LogInfo("UAS started on " << ipAddr.ipporttos());


			_stackThread.run();

			return API_SUCCESS;

		}
		catch (BaseException& e)
		{
			LogWarn("Error while starting sip stack - " << e.getMessage().c_str());
			return API_FAILURE;
		}

	}

	void 
	ProcSipStack::UponBlindXferReq(IwMessagePtr req)
	{
		FUNCTRACKER;

		_dumUac.UponBlindXferReq(req);

	}

	void 
	ProcSipStack::UponHangupCallReq(IwMessagePtr ptr)
	{
		FUNCTRACKER;

		_dumUac.UponHangupReq(ptr);
		
		
	}

	void
	ProcSipStack::ShutDown()
	{
		FUNCTRACKER;

		if (_shutDownFlag)
		{
			return;
		}

		_shutDownFlag = true;

 		_dumMngr.forceShutdown(NULL);
		_stackThread.shutdown();
		_stackThread.join();
		_stack.shutdown();
		_dumInt->Destroy();
	}

	void
	ProcSipStack::ShutDown(IwMessagePtr req)
	{
		FUNCTRACKER;

		ShutDown();

	}

	void
	ProcSipStack::UponCallOfferedAck(IwMessagePtr req)
	{
		FUNCTRACKER;

		_dumUas.UponCallOfferedAck(req);

	}

	void
	ProcSipStack::UponCallOfferedNack(IwMessagePtr req)
	{
		FUNCTRACKER;

		_dumUas.UponCallOfferedNack(req);

	}

	void
	ProcSipStack::UponMakeCallReq(IwMessagePtr req)
	{
		FUNCTRACKER;

		_dumUac.UponMakeCallReq(req);
	}

	void
	ProcSipStack::UponMakeCallAckReq(IwMessagePtr req)
	{
		FUNCTRACKER;

		_dumUac.UponMakeCallAckReq(req);
	}

	bool 
	ProcSipStack::ProcessApplicationMessages()
	{

		FUNCTRACKER;
		
		bool shutdown = false;
		if (InboundPending())
		{
			ApiErrorCode res;
			IwMessagePtr msg;

			msg = GetInboundMessage(Seconds(0),res);
			if (IW_FAILURE(res))
			{
				throw;
			}

			switch (msg->message_id)
			{
			case MSG_CALL_BLIND_XFER_REQ:
				{
					UponBlindXferReq(msg);
					break;
				}
			case MSG_PROC_SHUTDOWN_REQ:
				{
					ShutDown(msg);
					shutdown = true;
					SendResponse(msg, new MsgShutdownAck());
					break;
				}
			case MSG_HANGUP_CALL_REQ:
				{
					UponHangupCallReq(msg);
					break;

				}
			case MSG_CALL_OFFERED_ACK:
				{
					UponCallOfferedAck(msg);
					break;
				}
			case MSG_CALL_OFFERED_NACK:
				{
					UponCallOfferedNack(msg);
					break;
				}
			case MSG_MAKE_CALL_REQ:
				{
					UponMakeCallReq(msg);
					break;
				}
			case MSG_MAKE_CALL_ACK:
				{
					UponMakeCallAckReq(msg);
					break;
				}
			default:
				{ 
					if (HandleOOBMessage(msg) == FALSE)
					{
						LogCrit("Received unknown message " << msg->message_id_str);
						throw;
					}
				}
			}
		}

		return shutdown;
	}


	void 
	ProcSipStack::real_run()
	{
		FUNCTRACKER;

		ApiErrorCode res = Init();
		if (IW_FAILURE(res))
		{
			LogCrit("Cannot start sip stack res:" << res);
			return;
		}

		I_AM_READY;

		BOOL shutdown_flag = FALSE;
		while (shutdown_flag == FALSE)
		{
			// ***
			// IX_PROFILE_CHECK_INTERVAL(11000);
			// ***

			// taken from DumThread
			std::auto_ptr<Message> msg(_dumMngr.mFifo.getNext(60000));  // Only need to wake up to see if we are shutdown
			if (msg.get())
			{
				_dumMngr.internalProcess(msg);
			} 
			else
			{
				if (InboundPending())
				{
					shutdown_flag = ProcessApplicationMessages();
					if (shutdown_flag)
					{
						break;
					}
				}
				else
				{
					LogInfo("Sip keep alive.");
				}
			}
		}
	}

	void 
	ProcSipStack::onNewSession(
		IN ClientInviteSessionHandle s, 
		IN InviteSession::OfferAnswerType oat, 
		IN const SipMessage& msg)
	{
		FUNCTRACKER;
		_dumUac.onNewSession(s,oat,msg);
	}

	void 
	ProcSipStack::onConnected(
		IN ClientInviteSessionHandle is, 
		IN const SipMessage& msg)
	{
		FUNCTRACKER;
		_dumUac.onConnected(is,msg);
	}


	void 
	ProcSipStack::onNewSession(
		IN ServerInviteSessionHandle sis, 
		IN InviteSession::OfferAnswerType oat, 
		IN const SipMessage& msg)
	{
		FUNCTRACKER;
		_dumUas.onNewSession(sis,oat,msg);
	}

	void 
	ProcSipStack::onFailure(IN ClientInviteSessionHandle is, IN const SipMessage& msg)
	{
		FUNCTRACKER;
		_dumUac.onFailure(is,msg);

	}

	void 
	ProcSipStack::onOffer(
		IN InviteSessionHandle is, 
		IN const SipMessage& msg, 
		IN const SdpContents& sdp)
	{
		FUNCTRACKER;
		
		// On offer is actually called when first SDP is received
		// it may be in first invite, or in response to empty invite
		// in this case it is not correct to send it to uas.
		// this case is not handled.
		_dumUas.onOffer(is,msg,sdp);
	}

	/// called when ACK (with out an answer) is received for initial invite (UAS)
	void 
	ProcSipStack::onConnectedConfirmed( 
		IN InviteSessionHandle is, 
		IN const SipMessage &msg)
	{

		FUNCTRACKER;
		_dumUas.onConnectedConfirmed(is,msg);
	}

	void 
	ProcSipStack::onReceivedRequest(
		IN ServerOutOfDialogReqHandle ood, 
		IN const SipMessage& request)
	{
		FUNCTRACKER;
		_dumUas.onReceivedRequest(ood,request);
	}

	void 
	ProcSipStack::onTerminated(
		IN InviteSessionHandle is, 
		IN InviteSessionHandler::TerminatedReason reason, 
		IN const SipMessage* msg)
	{
		
		FUNCTRACKER;
		// this logic is not UAS/UAC specific
		
		IwStackHandle ixhandle = IW_UNDEFINED;
		ResipDialogHandlesMap::iterator iter  = _resipHandlesMap.find(is->getAppDialog());
		if (iter != _resipHandlesMap.end())
		{
			// early termination
			SipDialogContextPtr ctx_ptr = (*iter).second;
			ixhandle = ctx_ptr->stack_handle;
			ctx_ptr->call_handler_inbound->Send(new MsgCallHangupEvt());

			LogDebug("onTerminated:: " << LogHandleState(ctx_ptr,is));
			_iwHandlesMap.erase(ctx_ptr->stack_handle);

			if (ctx_ptr->uas_invite_handle.isValid())
			{
				_resipHandlesMap.erase(ctx_ptr->uas_invite_handle->getAppDialog());
			}
			if (ctx_ptr->uac_invite_handle.isValid())
			{
				_resipHandlesMap.erase(ctx_ptr->uac_invite_handle->getAppDialog());
			}
			

		} 
	}

	IwResipLogger::~IwResipLogger()
	{

	}

	/** return true to also do default logging, false to suppress default logging. */
	bool 
	IwResipLogger::operator()(Log::Level level,
		const Subsystem& subsystem, 
		const Data& appName,
		const char* file,
		int line,
		const Data& message,
		const Data& messageWithHeaders)
	{

		if (subsystem.getLevel() == Log::None)
		{
			return false;
		}

		switch (level)
		{
		case Log::Info:
		case Log::Warning:
			{
				LogInfo(subsystem.getSubsystem().c_str() << " " << message.c_str());
				break;
			}
		case Log::Debug:
			{
				LogDebug(subsystem.getSubsystem().c_str() << " " << message.c_str());
				break;
			}
		case Log::Err:
			{
				LogWarn(subsystem.getSubsystem().c_str() << " " << message.c_str());
				break;
			}
		case Log::Crit:
			{
				LogCrit(subsystem.getSubsystem().c_str() << " " << message.c_str());
				break;
			}
		default:
			{
				LogDebug(subsystem.getSubsystem().c_str() << " " << message.c_str());
			}
		}

		return false;

	}




}