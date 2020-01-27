#ifndef BJSON_H
#define BJSON_H


#define BJSON_STRING	0
#define BJSON_INT	1
#define BJSON_BOOL	2
#define BJSON_ARRAY	3

typedef struct bjson_def_elem {
	char	*be_name;
	int	be_type;
	int	be_required;
	int	be_offset;
} bjson_def_elem_t;



#endif
