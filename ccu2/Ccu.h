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

#pragma once

#include "CcuCommon.h"
#include "CcuMessage.h"



using namespace std;
using namespace boost;
using namespace boost::serialization;

enum CcuWellKnownProcesses
{
	RTP_RELAY_Q,			
	RTP_RELAY_IPC_Q,			
	IMS_Q,						
	VCS_Q,						
	COMMANDER_Q,					
	IPC_DISPATCHER_Q,			
	AIS_Q,						
	LAST_WELL_KNOWN_Q_MARKER    

};


#define IsWellKnownPid(x)  ((x) < LAST_WELL_KNOWN_Q_MARKER)


typedef long CcuConnectionId;

#define CCU_MAX_IP_LEN 64

namespace boost {
	namespace serialization {
		template<class Archive>
		void serialize(Archive & ar, sockaddr_in & g, const unsigned int version)
		{
			ar & make_nvp("sockadr_in",make_binary_object(&g,sizeof(sockaddr_in)));
		}
	}
}


class CnxInfo
{
private:

	BOOST_SERIALIZATION_REGION
	{
		SERIALIZE_FIELD(saddr);
		SERIALIZE_FIELD(addr);
	}

	sockaddr_in addr;

	wstring wsaddr;

	string saddr;

	wstring wsaddrport;

	string saddrport;

	string sport;

	wstring wsport;
	
public:


	CnxInfo(wstring s, int p_port)
	{
		addr.sin_family = AF_INET;
		addr.sin_port = ::htons(p_port);
		addr.sin_addr.s_addr = ::inet_addr(WStringToString(s).c_str());
		
		init_strings();
	}

	CnxInfo(string s, int p_port)
	{
		addr.sin_family = AF_INET;
		addr.sin_port = ::htons(p_port);
		addr.sin_addr.s_addr = ::inet_addr(s.c_str());
		
		init_strings();
	}

	CnxInfo(char *ip, int p_port)
	{
		addr.sin_family = AF_INET;
		addr.sin_port = ::htons(p_port);
		addr.sin_addr.s_addr = ::inet_addr(ip);

		init_strings();
	}

 	CnxInfo(in_addr p_in_addr, int p_port)
 	{
 		addr.sin_family = AF_INET;;
		addr.sin_addr = p_in_addr;
		addr.sin_port = ::htons(p_port);
 		

		init_strings();
 	}

	CnxInfo()
	{
		addr.sin_addr.s_addr = INADDR_NONE;
		addr.sin_port = CCU_UNDEFINED;
	}

	bool is_ip_valid() const
	{
		return (addr.sin_addr.s_addr != INADDR_NONE );
	}

	bool is_port_valid() const
	{
		return (addr.sin_port != CCU_UNDEFINED);
	}

	CnxInfo(const CnxInfo &x)
	{
		addr = x.addr;
		saddr = x.saddr;

		init_strings();
	}

	int port_ho() const
	{
		return ::ntohs(addr.sin_port);
	}

	int port_no() const
	{
		return addr.sin_port;
	}

	in_addr inaddr()
	{
		return addr.sin_addr;
	}

	long iaddr_ho() const
	{
		return ::ntohl(addr.sin_addr.s_addr);
	}

	long iaddr_no() const
	{
		return addr.sin_addr.s_addr;
	}

	sockaddr_in sockaddr() const
	{
		return addr;
	}

	string ipporttos() const
	{
		return saddrport;
	}

	wstring ipporttows() const
	{
		return wsaddrport;
	}

	string iptos() const
	{
		return saddr;
	}

	string porttos() const
	{
		return	sport;
	}

	const char *ipporttoa(char *buffer, int len) const
	{
		return saddrport.c_str();
	}

	const char *iptoa() const
	{
		return saddr.c_str();
	}

private:

	void init_strings()
	{
		// convert port
		char buffer[10];
		buffer[0] = '\0';

		if ( _itoa_s(port_ho(),buffer,10,10) != 0)
		{
			sport = "INVALID";
		} 
		else
		{
			sport = buffer;
		}

		// convert address
		saddr = string(::inet_ntoa(addr.sin_addr));

		// build full address
		saddrport.append(saddr);
		saddr.append(":");
		saddr.append(sport);

		wsport = StringToWString(sport);
		wsaddr = StringToWString(saddr);
		wsaddrport = StringToWString(saddrport);



	}


	friend int operator == (const CnxInfo &right,const CnxInfo &left);

};
BOOST_CLASS_EXPORT(CnxInfo);

typedef 
list<CnxInfo> CcuMediaDataList;


wostream& operator << (wostream &ostream, const CnxInfo *ptr);

wostream& operator << (wostream &ostream, const CnxInfo &ptr);


//
// Business Objects
//
struct Agent
{	
	Agent();

	Agent(const Agent &other);

	wstring name;

	wstring media_address;
};

typedef 
list<Agent> AgentsList;

