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

slsalb_t *slsalb_init(void);
void slsalb_uninit(slsalb_t **);


typedef struct slsplaylist {
	bstr_t	*sp_type;

	bstr_t	*sp_name;

	bstr_t	*sp_uri;
	bstr_t	*sp_url;
	
	bstr_t	*sp_caurl_lrg;
	bstr_t	*sp_caurl_sml;

} slsplaylist_t;

slsplaylist_t *slsplaylist_init(void);
void slsplaylist_uninit(slsplaylist_t **);


#endif

