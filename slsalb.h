#ifndef SLSALB_H
#define SLSALB_H

#include "bstr.h"


typedef struct slsalb {

	bstr_t	*sa_artist;
	bstr_t	*sa_name;

	bstr_t	*sa_uri;
	bstr_t	*sa_url;
	
	bstr_t	*sa_covurl_large;
	bstr_t	*sa_covurl_small;

} slsalb_t;

slsalb_t *slsalb_init(void);
void slsalb_uninit(slsalb_t **);

#endif

