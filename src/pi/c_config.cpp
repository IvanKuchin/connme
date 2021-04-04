#include "c_config.h"

bool					c_config_cache::IsInCache(const string &file)
{
	MESSAGE_DEBUG("", "", "start");

	auto	result = (content.find(file) != content.end());

	MESSAGE_DEBUG("", "", "finish (" + to_string(result) + ")");

	return result;
}

bool					c_config_cache::IsInCache(const string &file, const vector<string> &entries)
{
	MESSAGE_DEBUG("", "", "start");

	auto	result				= true;

	if(IsInCache(file))
	{
		auto	file_content = content.find(file)->second;

		for(auto &entry: entries)
		{
			result &= (file_content.find(entry) != file_content.end());
		}
	}
	else
	{
		MESSAGE_DEBUG("", "", "file " + file + " not in cache");
	}

	MESSAGE_DEBUG("", "", "finish (" +  to_string(result) + ")");

	return result;
}

map<string, string>		c_config_cache::Get(const string &file, const vector<string> &entries)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, string>	result;

	if(IsInCache(file))
	{
		auto	file_content = content.find(file)->second;

		for(auto &entry: entries)
		{
			if(file_content.find(entry) != file_content.end())
			{
				result[entry] = file_content.find(entry)->second;
				MESSAGE_DEBUG("", "", "actual entry taken from the file(" + file + "): " + entry + " = " + (entry.find("PASSWORD") != string::npos ? "xxxxxxxx" : result[entry]));
			}
			else if(file_content.find(DEFAULT_KEY) != file_content.end())
			{
				result[entry] = file_content.find(DEFAULT_KEY)->second;
				MESSAGE_DEBUG("", "", "default entry taken from the file(" + file + "): " + entry + " = " + (entry.find("PASSWORD") != string::npos ? "xxxxxxxx" : result[entry]));
			}
			else
			{
				MESSAGE_ERROR("", "", "no entry(" + entry + ") found in file(" + file + ")");
			}
		}
	}
	else
	{
		MESSAGE_DEBUG("", "", "file " + file + " not in cache");
	}

	MESSAGE_DEBUG("", "", "finish (" + to_string(result.size()) + ")");

	return result;
}


// =========================================================================================


string	c_config::trim(string line)
{
	line.erase(0, line.find_first_not_of(" \t\f\v\n\r"));	   //prefixing spaces
	line.erase(line.find_last_not_of(" \t\f\v\n\r") + 1);		 //suffixing spaces

	return line;
}

tuple<string, string> c_config::ExtractKeyValue(const string &line)
{
	MESSAGE_DEBUG("", "", "start (" + line + ")");

	tuple<string, string>	result{""s, ""s};
	auto					equal_pos	= line.find('=');

	if(equal_pos != string::npos)
	{
		result = make_tuple(trim(line.substr(0, equal_pos)), trim(line.substr(equal_pos + 1)));
	}

	MESSAGE_DEBUG("", "", "finish (" + get<0>(result) + ", " + get<1>(result) + ")");

	return result;
}
/*
string				c_config::GetKeyValue(ifstream &f)
{
}
*/

string				c_config::RemoveComment(string line)
{
	MESSAGE_DEBUG("", "", "start (" + line + ")");

	auto	comment_pos = line.find('#');

	if(comment_pos != string::npos)
		line.resize(comment_pos);

	MESSAGE_DEBUG("", "", "finish (" + line + ")");

	return line;
}

bool c_config::IsMultilineMarkerPresent(const string &line)
{
	return (line.find(MULTILINE_MARKER) != string::npos);
}

bool c_config::IsStartFromMultilineMarker(const string &line)
{
	return (line.find(MULTILINE_MARKER) == 0);
}

bool c_config::IsLineEndsWithMultilineMarker(const string &line)
{
	return ((line.rfind(MULTILINE_MARKER) + MULTILINE_MARKER.length()) == line.length());
}

string c_config::StripMultilineMarker(string line)
{
	try
	{
		line = line.replace(line.find(MULTILINE_MARKER), MULTILINE_MARKER.length(), "");
	}
	catch(...)
	{
		MESSAGE_ERROR("", "", "line (" + line + ") doesn't contain MULTILINE_MARKER (" + MULTILINE_MARKER + ")");
	}

	return line;
}

map<string, string>	c_config::ReadFileContent(const string &file)
{
	MESSAGE_DEBUG("", "", "start (" + file + ")");

	map<string, string>		result;
	ifstream				f(file);

	if(f.is_open())
	{
		auto	line			= ""s;
		auto	multiline_key	= ""s;
		auto	multiline_value	= ""s;
		auto	state			= KEY_VALUE_PAIR;


		while( getline (f,line) )
		{
			line = RemoveComment(line);
			
			if(trim(line).length())
			{
				if(state == KEY_VALUE_PAIR)
				{
					MESSAGE_DEBUG("", "", "key value pair (" + line + ")");

					auto	key_value = ExtractKeyValue(line);

					if(get<0>(key_value).length())
					{
						if(IsMultilineMarkerPresent(get<1>(key_value)))
						{
							if(IsStartFromMultilineMarker(get<1>(key_value)))
							{
								state = MULTILINE_VALUE;
								multiline_key = get<0>(key_value);
								multiline_value = StripMultilineMarker(get<1>(key_value));
							}
							else
							{
								MESSAGE_ERROR("", "", "multiline marker must be located at a beginning of a value (" + get<1>(key_value) + ")");
								break;
							}
						}
						else
						{
							// --- key and value define on a single line, not value could be empty
							result[get<0>(key_value)] = get<1>(key_value);
						}
					}
					else
					{
						MESSAGE_DEBUG("", "", "key is empty in line (" + line + ") -> skip the line");
					}
				}
				else if(state == MULTILINE_VALUE)
				{
					MESSAGE_DEBUG("", "", "multiline progressing (" + line + ")");

					// look for multiline value
					if(IsMultilineMarkerPresent(line))
					{
						// --- supposedly end of multiline 
						if(IsLineEndsWithMultilineMarker(line))
						{
							multiline_value += StripMultilineMarker(line);
							result[multiline_key] = multiline_value;

							MESSAGE_DEBUG("", "", "multiline value: " + multiline_key + " -> (" + multiline_value + ")");
							multiline_key = "";
							multiline_value = "";
							state = KEY_VALUE_PAIR;
						}
						else
						{
							MESSAGE_ERROR("", "", "multiline marker must be located at an end of a line (" + line + ")");
							break;
						}
					}
					else
					{
						multiline_value += line + "\n";
					}
				}
				else
				{
					MESSAGE_ERROR("", "", "unknown state");
					break;
				}
			}
		}

		f.close();
	}
	else
	{
		MESSAGE_ERROR("", "", "can't open " + file)
	}

	MESSAGE_DEBUG("", "", "finish (result size: " + to_string(result.size()) + ")");

	return result;
}

map<string, string>		c_config::GetFromFile(const string &file, const vector<string> &params)
{
	MESSAGE_DEBUG("", "", "start");

	map<string, string>		result;

	if(!cache.IsInCache(file))
	{
		auto file_content = ReadFileContent(GetConfigFolder() + file);
		cache.Add(file, file_content);
	}

	result = cache.Get(file, params);

	MESSAGE_DEBUG("", "", "finish (result size: " + to_string(result.size()) + ")");

	return result;
}

string					c_config::GetFromFile(const string &file, const string &param)
{
	vector<string>	vec_param = {param};
	return GetFromFile(file, vec_param)[param];
}

ostream& operator<<(ostream& os, const c_config &var)
{
	os << string("test line");
	return os;
}

