#ifndef BJSON_H
#define BJSON_H


#define BJSON_STRING	0
#define BJSON_INT	1
#define BJSON_BOOL	2
#define BJSON_STRUCT	3
#define BJSON_ARRAY	4


typedef struct bjson_field {
	char			*bf_name;
	int			bf_type;
	int			bf_required;

	size_t			bf_offset;

				/* If the value is a struct */
	size_t			bf_struct_size;
	struct bjson_field	*bf_struct_elems;
				
				/* If the value is an array. */
	int			bf_array_elem_type;
	size_t			*bf_array_struct_size;
	struct bjson_field	*bf_array_struct_elems;
} bjson_field_t;


typedef struct bjson_struct {

	char		*bs_name;
	size_t		bs_size;

	bjson_field_t	*bs_fields;
}









#endif
