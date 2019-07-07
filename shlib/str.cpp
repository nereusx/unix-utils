#include <cstdio>
#include <string>

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
		{ return p[index]; }
	str& alloc(int newsize) {
		if ( newsize > len() )
			s = (char *) realloc(s, newsize));
		return *this;
		}
	str& append(const char *source) {
		if ( source ) {
			s = (char *) realloc(s, len() + strlen(source) + 1);
			strcat(s, source);
			}
		return *this;
		}
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
		if ( p > s )
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
		char *pos, *ns;

		do {
			if ( (pos = strstr(pos, search_for)) != NULL ) {
				diff_l = pos - s;
				remain_l = strlen(pos + sf_l);
				ns = (char *) malloc(diff_l + rw_l + remain_l + 1);
				
				memcpy(ns, s, diff_l);
				memcpy(ns + diff_l, replace_width, rw_l);
				memcpy(ns + diff_l + rw_l, pos + sf_l, remain_l + 1);

				// exchange
				free(s);
				s = ns;
				pos = s + diff_l + rw_len;
				}
			} while ( pos );
		return *this;
		}
	};

// start >= 0 starts from the beginning
// start <  0 starts from the end, -1 = the last character
// clen  >  0 copy clen characters
// clen  <  0 copy clen characters backward
str substr(const char *s, int start, int clen = 0)
{
	intptr_t ln = len();
	if ( start < 0 )  start = ln - start;
	if ( start < 0 )  start = 0;
	if ( start >= ln )	return str("");
	if ( clen == 0 )	return str(s + start);
	if ( clen > 0 ) {
		// copy min(clen,ln-start)
		intptr_t n = min(clen, ln - start);
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

int		split(const char *source, vector<str> &items, const char *sep = ';');
void	join(const char *source, const vector<str> &items, const char *sep = ';');


