#include <stdio.h>
#include "str.h"

int main()
{
	char *str = "This is not easy !";
	printf("%s\n", delete(str, 8, 4));
	char *t = "1234567";
	printf("%s\n", copy(t,0,2));
	printf("%s\n", copy(t,3,2));
	printf("%s\n", copy(t,3,8));
	char *s = "Free Pascal is difficult to use !";
	printf("%s\n", insert(s, strpos(s, "difficult"),"NOT "));
	return 0;
}
