//  $Id$
// 
//  TinyGetText - A small flexible gettext() replacement
//  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
// 
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include <config.h>
#include <vector>
#include <iconv.h>
#include <errno.h>
#include <sstream>
#include <stdexcept>
#include "po_file_reader.hpp"

namespace TinyGetText {

static bool has_prefix(const std::string& lhs, const std::string rhs)
{
  if (lhs.length() < rhs.length())
    return false;
  else
    return lhs.compare(0, rhs.length(), rhs) == 0;
}


class charconv
{
public:
  std::string incharset;
  std::string outcharset;

	charconv() : m_conv(0)
	{}
 
	charconv(const std::string& incharset_, const std::string& outcharset_)
          : incharset(incharset_),
            outcharset(outcharset_),
            m_conv(0)
	{
		create(incharset, outcharset);
	}
 
	~charconv()
	{
		close();
	}
 
	void create(const std::string& incharset, const std::string& outcharset)
	{
		// Create the converter.
		if(!(m_conv = iconv_open(incharset.c_str(), outcharset.c_str())))
		{
			if(errno == EINVAL)
                          {
                            std::ostringstream sstr;
                            sstr << "Unsupported conversion: " << incharset
                                 << " => " << outcharset << "!";
                            throw std::runtime_error(sstr.str());
                          }
			else
                          {
                            throw std::runtime_error(strerror(errno));
                          }
                        std::cout << "Something when very wrong" << std::endl;
			exit(1);
		}
	}
 
	void close()
	{
		// Free, if exists.
		if(m_conv)
		{
			iconv_close(m_conv);
			m_conv = 0;
		}
	}
 
	/// Convert a string from encoding to another.
	std::string convert(std::string text)
	{
		if(!m_conv) return text;
 
		size_t in_size = text.size();
		size_t out_size = 4*in_size; // Worst case scenario: ASCII -> UTF-32?
		std::string result(out_size, ' ');
                ICONV_CONST char* in_str  = &text[0];
		char* out_str = &result[0];
 
		// Try to convert the text.
		if(iconv(m_conv, &in_str, &in_size, &out_str, &out_size) != 0) {
                  std::cout << "TinyGetText: text: \"" << text << "\"" << std::endl;
                  std::cout << "TinyGetText: Error while converting (" 
                            << incharset << " -> " << outcharset 
                            << "): " << strerror(errno) << std::endl;
                  exit(1);
                }
		// Eat off the spare space.
		result.resize(out_str - &result[0]);
		return result;
	}
protected:
	iconv_t m_conv;
};


/** Convert \a which is in \a from_charset to \a to_charset and return it */
std::string convert(const std::string& text,
                    const std::string& from_charset,
                    const std::string& to_charset)           
{
  if (from_charset == to_charset)
    return text;

  charconv *cc = new charconv(from_charset, to_charset);
  std::string ret = cc->convert(text);
  cc->close();
  return ret;
}
/*
  iconv_t cd = iconv_open(to_charset.c_str(), from_charset.c_str());
  
  size_t in_len  = text.length();
  size_t out_len = text.length()*4; // Should be large enough to hold UTF-32

  char*  out_orig = new char[out_len]; // FIXME: cross fingers that this is enough
  char*  in_orig  = new char[in_len+1];
  strcpy(in_orig, text.c_str());

  char* out = out_orig;
  const char* in  = in_orig;

  //std::cout << "IN: " << (int)in << " " << in_len << " " << (int)out << " " << out_len << std::endl;
  int retval = iconv(cd, &in, &in_len, &out, &out_len);
  //std::cout << "OUT: " << (int)in << " " << in_len << " " << (int)out << " " << out_len << std::endl;

  if (retval != 0)
    {
      std::cerr << strerror(errno) << std::endl;
      std::cerr << "Error: conversion from " << from_charset
                << " to " << to_charset << " went wrong: " << retval << std::endl;
    }
  iconv_close(cd);

  
    <dolphin> your code is also buggy
<dolphin> there will be extra spaces at the end of the string
<dolphin> the lenght of the final string should be: out_str - out_orig
<dolphin> or: out_size_before_iconv_call - out_size_after_iconv_call
   
  std::string ret(out_orig, out_len);
  delete[] out_orig;
  delete[] in_orig;
  return ret;
}
*/

POFileReader::POFileReader(std::istream& in, Dictionary& dict_)
  : dict(dict_)
{
  state = WANT_MSGID;
  line_num = 0;
  tokenize_po(in);
}

void
POFileReader::parse_header(const std::string& header)
{
  // Seperate the header in lines
  typedef std::vector<std::string> Lines;
  Lines lines;
    
  std::string::size_type start = 0;
  for(std::string::size_type i = 0; i < header.length(); ++i)
    {
      if (header[i] == '\n')
        {
          lines.push_back(header.substr(start, i - start));
          start = i+1;
        }
    }

  for(Lines::iterator i = lines.begin(); i != lines.end(); ++i)
    {
      if (has_prefix(*i, "Content-Type: text/plain; charset=")) {
        from_charset = i->substr(strlen("Content-Type: text/plain; charset="));
      }
    }

  if (from_charset.empty() || from_charset == "CHARSET")
    {
      std::cerr << "Error: Charset not specified for .po, fallback to ISO-8859-1" << std::endl;
      from_charset = "ISO-8859-1";
    }

  to_charset = dict.get_charset();
  if (to_charset.empty())
    { // No charset requested from the dict, so we use the one from the .po 
      to_charset = from_charset;
      dict.set_charset(from_charset);
    }
}

void
POFileReader::add_token(const Token& token)
{
  switch(state) 
    {
    case WANT_MSGID:
      if (token.keyword == "msgid") 
        {
          current_msgid = token.content;
          state = WANT_MSGID_PLURAL;
        }
      else if (token.keyword.empty())
        {
          //std::cerr << "Got EOF, everything looks ok." << std::endl;
        }
      else
        {
          std::cerr << "tinygettext: expected 'msgid' keyword, got '" << token.keyword 
                    << "' at line " << line_num << std::endl;
        }
      break;
    
    case WANT_MSGID_PLURAL:
      if (token.keyword == "msgid_plural") 
        {
          current_msgid_plural = token.content;
          state = WANT_MSGSTR_PLURAL;
        } 
      else
        {
          state = WANT_MSGSTR;
          add_token(token);
        }
      break;

    case WANT_MSGSTR:
      if (token.keyword == "msgstr") 
        {
          if (current_msgid == "") 
            { // .po Header is hidden in the msgid with the empty string
              parse_header(token.content);
            }
          else
            {
              dict.add_translation(current_msgid, convert(token.content, from_charset, to_charset));
            }
          state = WANT_MSGID;
        } 
      else
        {
          std::cerr << "tinygettext: expected 'msgstr' keyword, got " << token.keyword 
                    << " at line " << line_num << std::endl;
        }
      break;

    case WANT_MSGSTR_PLURAL:
      if (has_prefix(token.keyword, "msgstr[")) 
        {
          int num;
          if (sscanf(token.keyword.c_str(), "msgstr[%d]", &num) != 1) 
            {
              std::cerr << "Error: Couldn't parse: " << token.keyword << std::endl;
            } 
          else 
            {
              msgstr_plural[num] = convert(token.content, from_charset, to_charset);
            }
        }
      else 
        {
          dict.add_translation(current_msgid, current_msgid_plural, msgstr_plural);

          state = WANT_MSGID;
          add_token(token);
        }
      break;
    }
}
  
void
POFileReader::tokenize_po(std::istream& in)
{
  enum State { READ_KEYWORD, 
               READ_CONTENT,
               READ_CONTENT_IN_STRING,
               SKIP_COMMENT };

  State state = READ_KEYWORD;
  int c;
  Token token;

  while((c = getchar(in)) != EOF)
    {
      //std::cout << "Lexing char: '" << char(c) << "' " << c << " state: " << state << std::endl;
      switch(state)
        {
        case READ_KEYWORD:
          if (c == '#')
            {
              state = SKIP_COMMENT;
            }
          else if (isspace(c))
            {
              state = READ_KEYWORD;
            }
          else
            {
              // Read a new token
              token = Token();
                
              do { // Read keyword 
                token.keyword += c;
              } while((c = getchar(in)) != EOF && !isspace(c));
              in.unget();

              state = READ_CONTENT;
            }
          break;

        case READ_CONTENT:
          while((c = getchar(in)) != EOF)
            {
              if (c == '"') { 
                // Found start of content
                state = READ_CONTENT_IN_STRING;
                break;
              } else if (isspace(c)) {
                // skip
              } else { // Read something that may be a keyword
                in.unget();
                state = READ_KEYWORD;
                add_token(token);
                break;
              }
            }
          break;

        case READ_CONTENT_IN_STRING:
          if (c == '\\') {
            c = getchar(in);
            if (c != EOF)
              {
                if (c == 'n') token.content += '\n';
                else if (c == 't') token.content += '\t';
                else if (c == 'r') token.content += '\r';
                else if (c == '"') token.content += '"';
                else
                  {
                    std::cout << "Unhandled escape character: " << char(c) << std::endl;
                  }
              }
            else
              {
                std::cout << "Unterminated string" << std::endl;
              }
          } else if (c == '"') { // Content string is terminated
            state = READ_CONTENT;
          } else {
            token.content += c;
          }
          break;

        case SKIP_COMMENT:
          if (c == '\n')
            state = READ_KEYWORD;
          break;
        }
    }
  // add_token(token);
}

} // namespace TinyGetText

/* EOF */
