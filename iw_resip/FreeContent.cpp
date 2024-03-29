#include "StdAfx.h"
#include "FreeContent.h"

namespace ivrworx
{
	FreeContent::FreeContent(const string &body, const string &type):
	Contents(Mime(AbstractOffer::Type(type).c_str(), AbstractOffer::SubType(type).c_str())),
		_body(body),
		_type(type)
	{
		
		
	}

	FreeContent::FreeContent(const AbstractOffer &offer):
	Contents(Mime(AbstractOffer::Type(offer.type).c_str(), AbstractOffer::SubType(offer.type).c_str())),
		_body(offer.body),
		_type(offer.type)
	{


	}

	EncodeStream& 
	FreeContent::encodeParsed(EncodeStream& str) const
	{
		str << _body;
		return str;
	}


	void 
	FreeContent::parse(ParseBuffer& pb)
	{
		_body = pb.getContext().c_str();
	}

	Contents* 
	FreeContent::clone() const
	{
		return new FreeContent(_body,_type);
	}


	FreeContent::~FreeContent(void)
	{
	}




}

