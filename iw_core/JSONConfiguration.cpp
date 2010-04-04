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
#include "JSONConfiguration.h"


using namespace json_spirit;

namespace ivrworx
{

	static bool 
	same_name(const Pair& pair, const string& name )
	{
		return pair.name_ == name;
	}

	static const Value& 
	find_value( const Object& obj, const string& name )
	{
		Object::const_iterator i = find_if( obj.begin(), obj.end(), bind( same_name, _1, ref( name ) ) );

		if( i == obj.end() ) return Value::null;

		return i->value_;
	}

	static const Array& 
	find_array( const Object& obj, const string& name )
	{
		const Value & val = find_value(obj, name );
		if ( val.type() != array_type)
		{
			throw configuration_exception (name + " does not exist or not array type");
		};

		return val.get_array(); 
	}

	static int 
	find_int( const Object& obj, const string& name )
	{
		const Value & val = find_value(obj, name );
		if ( val.type() != int_type)
		{
			throw configuration_exception (name + " does not exist or not int type");
		};
		return val.get_int();
	}

	static bool
	find_bool( const Object& obj, const string& name)
	{
		const Value & val = find_value(obj, name );
		if ( val.type() != bool_type)
		{
			throw configuration_exception (name + " does not exist or not bool type");
		};
		return val.get_bool();
	}

	string
	get_env_variable_as_str(const char *var_name)
	{
        
		char* var_value = NULL;
		size_t requiredSize;

		::getenv_s( &requiredSize, NULL, 0, var_name);
		var_value = (char*)::malloc(requiredSize * sizeof(char));

		// Get the value of the LIB environment variable.
		::getenv_s( &requiredSize, var_value, requiredSize, var_name);

		string res(var_value);

		::free(var_value);

		return res;


	}

	static string 
	find_str( const Object& obj, const string& name )
	{
		
		
		const Value & val = find_value(obj, name );
		if ( val.type() != str_type)
		{
			throw configuration_exception (name + " does not exist or not string type");
		};

		string value_str = val.get_str();
		if (value_str.length() > 0 && 
			 *value_str.begin() == '$')
		{
			return get_env_variable_as_str(value_str.c_str() + 1);
		}

		return value_str;
	}

	JSONConfiguration::JSONConfiguration(void)
	{
		
	}

	JSONConfiguration::~JSONConfiguration(void)
	{
	}

	

	const Object&
	JSONConfiguration::FindObject(IN const string &name, const Object &parent_object, OUT string &leaf_name)
	{
		size_t found = name.find_first_of("/");
		if (found == string::npos)
		{
			return parent_object;
			leaf_name = name;
		}

		string array_name =  
			name.substr (found); 
		string remainder 
			= name.substr(found + 1, name.length());

		const Value& val = 
			find_value(parent_object,array_name);

		return FindObject(remainder,val.get_obj(),leaf_name);

	}

	int
	JSONConfiguration::GetInt(IN const string &name)
	{
		string leaf_name = name;
		const Object &obj = FindObject(name,_rootValue.get_obj(),leaf_name);

		return find_int(obj,leaf_name);
	}

	string
	JSONConfiguration::GetString(IN const string &name)
	{

		string leaf_name = name;
		const Object &obj = FindObject(name,_rootValue.get_obj(),leaf_name);

		return find_str(obj, leaf_name);
				
	}

	BOOL
	JSONConfiguration::GetBool(IN const string &name)
	{

		string leaf_name = name;
		const Object &obj = FindObject(name,_rootValue.get_obj(),leaf_name);

		return find_bool(obj, leaf_name);

	}

	BOOL 
	JSONConfiguration::HasOption(const string &name)
	{
		string leaf_name = name;
		const Object &obj = FindObject(name,_rootValue.get_obj(),leaf_name);

		try
		{
			find_value(obj, leaf_name);
			return true;
		}
		catch (exception e)
		{
			return false;
		}
		
	}

	void 
	JSONConfiguration::GetArray(IN const string &name, OUT list<any> &out_list)
	{
		string leaf_name = name;
		const Object &obj = FindObject(name,_rootValue.get_obj(),leaf_name);

		const Array &val_array = 
			find_array(obj,leaf_name);

		out_list.clear();

		Array::const_iterator iter = 
			val_array.begin();

		while(iter != val_array.end())
		{
			switch (iter->type())
			{
			case str_type: 
				{
					out_list.push_back(iter->get_str());
					break;
				}
			case bool_type: 
				{
					out_list.push_back(iter->get_bool());
					break;
				}
			case int_type: 
				{
					out_list.push_back(iter->get_int());
					break;
				}
			case real_type: 
				{
					out_list.push_back(iter->get_real());
					break;
				}
			case obj_type:
			case null_type:
			case array_type:
			default:
				{
					throw new configuration_exception("not supported JSON type. (TBD)");

				}
			} // switch
		}
	}



	ApiErrorCode
	JSONConfiguration::InitFromFile(IN const string &filename)
	{
		ifstream is(filename.c_str());

		if (read(is, _rootValue) == false)
		{
			cerr << "Error reading json configuration file '" << filename << "'. Check that file exists, accessible and json valid." << endl;
			return API_FAILURE;
		}

		return API_SUCCESS;
	}

// 
// 	ApiErrorCode
// 	JSONConfiguration::InitFromString(IN const string &is)
// 	{
// 
// 		if (read(is, _value) == false)
// 		{
// 			return API_FAILURE;
// 		}
// 
// 		ApiErrorCode res = InitDb();
// 		return res;
// 
// 	}

	

// 	ApiErrorCode
// 	JSONConfiguration::InitDb()
// 	{
// 
// 		Object root_obj(_value.get_obj());
// 
// 		//
// 		// resiprocate logging
// 		//
// 		_resipLog = find_str(root_obj, "resip_log");
// 
// 		//
// 		// debug level
// 		//
// 		_debugLevel = find_str(root_obj, "debug_level");
// 		_debugOutputs = find_str(root_obj, "debug_outputs");
// 
// 		//
// 		// syslog
// 		//
// 		_sysloghost = find_str(root_obj, "syslogd_host");
// 		_syslogport = find_int(root_obj, "syslogd_port");
// 
// 		//
// 		// ivr 
// 		//
// 		const string ivr_host_str = find_str(root_obj, "ivr_sip_host" );
// 		const int ivr_ip_int	= find_int(root_obj, "ivr_sip_port" );
// 
// 		
// 		
// 		_ivrCnxInfo = CnxInfo(
// 			convert_hname_to_addrin(ivr_host_str.c_str()),
// 			ivr_ip_int);
// 
// 		// configuration file
// 		_scriptFile = find_str(root_obj, "script_file");
// 
// 		// codecs
// 		const Array &codecs_array = find_array(root_obj, "codecs");
// 
// 		Array::const_iterator iter = codecs_array.begin();
// 		while(iter != codecs_array.end())
// 		{
// 			const MediaFormat *codec = read_codec(iter->get_obj());
// 			if (codec == NULL)
// 			{
// 				return API_FAILURE;
// 			}
// 
// 			_mediaFormatPtrsList.push_back(codec);
// 			iter++;
// 		}
// 
// 		// from 
// 		_from		 = find_str(root_obj, "from_id");
// 		_fromDisplay = find_str(root_obj, "from_display_name");
// 
// 		// basic sound path
// 		_soundsPath	= find_str(root_obj, "sounds_dir");
// 
// 		// refresh mode
// 		_sipRefreshMode = find_str(root_obj, "sip_refresh_mode");
// 
// 		// default refresh time
// 		_sipDefaultSessionTime = find_int(root_obj, "sip_default_session_time");
// 		_sipDefaultSessionTime = _sipDefaultSessionTime < 90 ? 90 : _sipDefaultSessionTime;
// 
// 		// enable session timer
// 		_enableSessionTimer = find_bool(root_obj,"sip_session_timer_enabled");
// 
// 		// read super script
// 		_superScript = find_str(root_obj, "super_script");
// 		_superMode	 = find_str(root_obj, "super_mode");
// 
// 		_syncLog = find_bool(root_obj, "sync_log");
// 
// 		return API_SUCCESS;
// 
// 	}

}
