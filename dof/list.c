#ifdef STRDUP
// clone a string
static char *strdup(const char *src)
{
	char *buf = malloc(strlen(src) + 1);
    strcpy(buf, src);
	return buf;
}
#endif

typedef struct s_node node_t;
struct s_node {
	char *str;
	char *data;
	node_t *next;
	};

typedef struct s_list list_t;
struct s_list {
	node_t *root;
	node_t *tail;
	};

// add node to list
// this just stores a string in the list
static void list_add(list_t *list, const char *str)
{
	node_t *np = (node_t *) malloc(sizeof(node_t));
	np->str  = strdup(str);
	np->data = NULL;
	np->next = NULL;
	if ( list->root ) {
		list->tail->next = np;
		list->tail = np;
		}
	else
		list->root = list->tail = np;
}

// add node to list
// this stores a key and a value to the list
static void list_addpair(list_t *list, const char *key, const char *value)
{
	node_t *np = (node_t *) malloc(sizeof(node_t));
	np->str  = strdup(key);
	np->data = strdup(value);
	np->next = NULL;
	if ( list->root ) {
		list->tail->next = np;
		list->tail = np;
		}
	else
		list->root = list->tail = np;
}

// initialize list
static void list_init(list_t *list)
{
	list->root = list->tail = NULL;
}

// clean up memory and reset the list
static void list_clear(list_t *list)
{
	node_t *pre, *cur = list->root;

	while ( cur ) {
		pre = cur;
		cur = cur->next;
		free(pre->str);
		free(pre);
		}
	list->root = list->tail = NULL;
}

// add node to list
// this stores wildcard matches
static void list_addwc(list_t *list, const char *pattern)
{
	glob_t globbuf;
	int flags = GLOB_DOOFFS;
	#ifdef GLOB_TILDE
	flags |= GLOB_TILDE;
	#endif
	#ifdef GLOB_BRACE
	flags |= GLOB_BRACE;
	#endif

	globbuf.gl_offs = 0;
	if ( glob(pattern, flags, NULL, &globbuf) == 0 ) {
		for ( int i = 0; globbuf.gl_pathv[i]; i ++ ) {
			const char *name = globbuf.gl_pathv[i];
			if ( ! (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) )
				list_add(list, name);
			}
		globfree(&globbuf);
		}
}

// convert string list to string
static char *list_to_string(list_t *list, const char *delim)
{
	char	*ret;
	int		len = 0, delim_len = 0, first = 1;
	node_t	*cur = list->root;

	if ( delim )
		delim_len = strlen(delim);
	ret = (char *) malloc(1);
	*ret = '\0';
	while ( cur ) {
		len += delim_len + strlen(cur->str);
		ret = (char *) realloc(ret, len + 1);
		if ( first )
			first = 0;
		else if ( delim )
			strcat(ret, delim);
		strcat(ret, cur->str);
		cur = cur->next;
		}
	return ret;
}

