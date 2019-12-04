// returns true if the "filename" has wildcards
static int has_wildcards(const char *filename)
{
	const char *p = filename;
	
	while ( *p ) {
		if ( *p == '*' || *p == '?' || *p == '['
	#ifdef GLOB_TILDE
			 || *p == '~'
	#endif
	#ifdef GLOB_BRACE
			 || *p == '{'
	#endif
		   )
			return 1;
		p ++;
		}
	return 0;
}

// returns the name of the file without the directory and the extension
static char *basename(const char *source)
{
	static char buf[PATH_MAX];
	char		*b;
	const char	*p;

	if ( (p = strrchr(source, '/')) != NULL )
		strcpy(buf, p + 1);
	else
		strcpy(buf, source);
	if ( (b = (char *) strrchr(buf, '.')) != NULL )
		*b = '\0';
	return buf;
}

// returns the directory of the file without the trailing '/'
static char *dirname(const char *source)
{
	static char buf[PATH_MAX];
	const char	*p;

	if ( (p = strrchr(source, '/')) != NULL ) {
		strcpy(buf, source);
		buf[p - source] = '\0';
		}
	else
		buf[0] = '\0';
	return buf;
}

// returns the extension of the file without the '.'
static char *extname(const char *source)
{
	static char buf[PATH_MAX];
	char		*b;
	const char	*p;

	if ( (p = strrchr(source, '/')) != NULL )
		strcpy(buf, p + 1);
	else
		strcpy(buf, source);
	if ( (b = (char *) strrchr(buf, '.')) != NULL )
		return b + 1;
	else
		buf[0] = '\0';
	return buf;
}

