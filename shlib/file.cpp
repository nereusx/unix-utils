	
	/*
	 * file utilities
	 */
	
const char *getFileExt() const
		{ return strrchr(s, '.'); }

	void	setFileExt(const char *newext = NULL) {
		char *p = strrchr(s, '.');
		if ( p ) *p = '\0';
		if ( newext ) {
			s = (char *) realloc(s, len() + strlen(newext) + 1);
			strcat(s, newext);
			}
		}

	const char *getFileName() const {
		const char *p = strrchr(s, '/');
		return ( p ) ? p + 1 : s;
		}
