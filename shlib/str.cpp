/*
 * string class
 */
 
#include <cstdio>
#include <climits>
#include <cctype>
#include <cstring>
#include <memory>
#include <algorithm>
#include <vector>
#include <list>

#ifndef MINMAX_T
	#define MIN(a,b)	((a<b)?a:b)
	#define MAX(a,b)	((a>b)?a:b)
	#define MINMAX_T
#endif

class str {
private:
	char	*s;
	
public:
	inline str()
		{ s = (char *) malloc(16); *s = '\0'; }
	inline str(const char *source)
		{ s = strdup(source); }
	inline str(const str &source)
		{ s = strdup(source.cptr()); }
	virtual ~str()
		{ free(s); }
	
	inline const char *cptr() const
		{ return s; }
	inline char *ptr()
		{ return s; }
	inline int len() const
		{ return strlen(s); }
	inline void clear()
		{ *s = '\0'; }
	inline bool empty() const
		{ return (*s == '\0'); }
	inline char& at(int index)
		{ return s[index]; }
	str& alloc(int newsize) {
		if ( newsize > len() )
			s = (char *) realloc(s, newsize);
		return *this;
		}
		
	str& append(const char *source) {
		if ( source ) {
			s = (char *) realloc(s, len() + strlen(source) + 1);
			strcat(s, source);
			}
		return *this;
		}
	inline str& append(const str& src)
		{ return append(src.cptr()); }
		
	str& set(const char *newstr) {
		free(s);
		s = strdup(newstr);
		return *this;
		}
	inline str& set(const str& newstr)
		{ return set(newstr.cptr()); }
		
	inline str& operator= (const str& src)
		{ set(src.cptr()); return *this; }
	inline str& operator= (const char *src)
		{ set(src); return *this; }
	inline str& operator+= (const str& src)
		{ return append(src.cptr()); }
	inline str& operator+= (const char *src)
		{ return append(src); }
	inline const char& operator[] (size_t pos) const
		{ return s[pos]; }
	inline char& operator[] (size_t pos)
		{ return s[pos]; }

	str& chop(char ch = '\n') {
		if ( !empty() ) {
			int	i = len();
			if ( s[i-1] == ch )
				s[i-1] = '\0';
			}
		return *this;
		}
	str& ltrim() {
		char *p = s;
		while ( *p == ' ' || *p == '\t' )	p ++;
		if ( p != s )
			strcpy(s, p);
		return *this;
		}
	str& rtrim() {
		if ( !empty() ) {
			int i = len() - 1;
			for ( ; i >= 0; i -- ) {
				if ( s[i] == ' ' || s[i] == '\t' )
					s[i] = '\0';
				else
					break;
				}
			}
		return *this;
		}
	str& trim()
		{ return ltrim().rtrim(); }
		
	//
	inline int toInt() const
		{ return atoi(s); }
	inline float toFloat() const
		{ return atof(s); }

	//
	str& replace(char c1, char c2) {
		char *p = s;
		while ( *p ) {
			if ( *p == c1 )
				*p = c2;
			p ++;
			}
		return *this;
		}

	//
	str& replace(const char *search_for, const char *replace_with) {
		intptr_t sf_l = strlen(search_for);
		intptr_t rw_l = strlen(replace_with);
		intptr_t diff_l, remain_l;
		char *pos = s, *ns;

		do {
			if ( (pos = strstr(pos, search_for)) != NULL ) {
				diff_l = pos - s;
				remain_l = strlen(pos + sf_l);
				ns = (char *) malloc(diff_l + rw_l + remain_l + 1);
				
				memcpy(ns, s, diff_l);
				memcpy(ns + diff_l, replace_with, rw_l);
				memcpy(ns + diff_l + rw_l, pos + sf_l, remain_l + 1);

				// exchange
				free(s);
				s = ns;
				pos = s + diff_l + rw_l;
				}
			} while ( pos );
		return *this;
		}
		
	friend str replace(const char *source, const char *search_for, const char *replace_with);
	};

//
typedef std::list<str>			strlist;
typedef strlist::iterator		strlist_iterator;
typedef strlist::const_iterator	strlist_const_iterator;

//
str operator+ (const str& lhs, const char *rhs)
{ str x(lhs); x.append(rhs); return x; }

//
str operator+ (const char *lhs, const str& rhs)
{ str x(lhs); x.append(rhs); return x; }

// remove spaces from left
str	ltrim(const char *s)
{
	const char *p = s;
	while ( *p == ' ' || *p == '\t' )	p ++;
	if ( p != s )
		return str(p);
	return str(s);
}

// remove spaces from right
str rtrim(const char *s)
{
	if ( s && *s ) {
		str ns(s);
		int i = ns.len() - 1;
		
		for ( ; i >= 0; i -- ) {
			if ( ns[i] == ' ' || ns[i] == '\t' )
				ns[i] = '\0';
			else
				break;
			}
		return ns;
		}
	return str(s);
}

// remove spaces (space & tab) from the begin and end of the string
str trim(const char *s)
{
	return rtrim(ltrim(s).cptr());
}

// Returns part of the string 's', starting from 'start' with lenght 'clen'
//
// start >= 0 starts from the beginning
// start <  0 starts from the end, -1 = the last character
// clen  >  0 copy clen characters
// clen  <  0 copy clen characters backward
str substr(const char *s, int start, int clen = 0)
{
	intptr_t ln = strlen(s);
	if ( start < 0 )  start = ln - start;
	if ( start < 0 )  start = 0;
	if ( start >= ln )	return str("");
	if ( clen == 0 )	return str(s + start);
	if ( clen > 0 ) {
		// copy min(clen,ln-start)
		intptr_t n = MIN(clen, ln - start);
		str ns = str(s + start);
		ns.at(n) = '\0';
		return ns;
		}
	// copy backward, start -= clen
	clen = -clen;
	start -= clen;
	if ( start < 0 )  start = 0;
	return substr(s, start, clen);
}

// replace the characters of string src
str	replace(const char *src, char c1, char c2)
{
	str  ns(src);
	char *p = ns.ptr();
	while ( *p ) {
		if ( *p == c1 )
			*p = c2;
		p ++;
		}
	return ns;
}

// replace string 'search_for' with 'replace_with'
str replace(const char *source, const char *search_for, const char *replace_with)
{
	str		 rs(source);
	intptr_t sf_l = strlen(search_for);
	intptr_t rw_l = strlen(replace_with);
	intptr_t diff_l, remain_l;
	char *pos = rs.s, *ns;

	do {
		if ( (pos = strstr(pos, search_for)) != NULL ) {
			diff_l = pos - rs.s;
			remain_l = strlen(pos + sf_l);
			ns = (char *) malloc(diff_l + rw_l + remain_l + 1);
			
			memcpy(ns, rs.s, diff_l);
			memcpy(ns + diff_l, replace_with, rw_l);
			memcpy(ns + diff_l + rw_l, pos + sf_l, remain_l + 1);

			// exchange
			free(rs.s);
			rs.s = ns;
			pos = rs.s + diff_l + rw_l;
			}
		} while ( pos );
	return rs;
}

//
int split(const char *source, strlist &lst, const char *sep)
{
	const char *p = source;
	char *dest, *d;

	dest = (char *) malloc(strlen(source) + 1);
	d = dest;
	while ( *p ) {
		if ( strchr(sep, *p) ) {
			*d = '\0';
			d = dest;
			lst.push_back(dest);
			}
		else
			*d ++ = *p;
		p ++;
		}
	if ( d != dest ) {
		*d = '\0';
		lst.push_back(dest);
		}
	free(dest);
	return lst.size();
}

// joins strings
str join(const strlist &lst, const char *sep = " ")
{
	str	rs;
	strlist_const_iterator cit = lst.begin();
	if ( cit != lst.end() ) {
		do {
			rs.append(*cit);
			cit ++;
			if ( cit == lst.end() )
				break;
			rs.append(sep);
			} while ( 1 );
		}
	return rs;
}


