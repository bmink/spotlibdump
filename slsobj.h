#ifndef SLSOBJ_H
#define SLSOBJ_H

#include "bstr.h"


typedef struct slsalb {
	bstr_t	*sa_type;

	bstr_t	*sa_artist;
	bstr_t	*sa_name;

	bstr_t	*sa_uri;
	bstr_t	*sa_url;
	
	bstr_t	*sa_caurl_lrg;
	bstr_t	*sa_caurl_sml;

} slsalb_t;

slsalb_t *slsalb_init(const char *);
void slsalb_uninit(slsalb_t **);
int slsalb_tojson(slsalb_t *, bstr_t *);


typedef struct slspl {
	bstr_t	*sp_type;

	bstr_t	*sp_name;

	bstr_t	*sp_uri;
	bstr_t	*sp_url;
	
	bstr_t	*sp_caurl_lrg;
	bstr_t	*sp_caurl_sml;

} slspl_t;

slspl_t *slsplaylist_init(void);
void slspl_uninit(slspl_t **);

#endif

