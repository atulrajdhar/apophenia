#ifndef __apop_name__
#define __apop_name__

#include <string.h>

typedef struct apop_nnn{
	char ** colnames;
	char ** rownames;
	int colnamect, rownamect;
} apop_name;

apop_name * apop_name_alloc(void);
int apop_name_add(apop_name * n, char *add_me, int is_column);
void  apop_name_free(apop_name * free_me);
void  apop_name_print(apop_name * n);

#endif
