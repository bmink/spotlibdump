#include <errno.h>
#include "slsobj.h"
#include "blog.h"

slsalb_t
*slsalb_init(void)
{
	slsalb_t	*alb;
	int		err;

	err = 0;
	alb = NULL;
	
	alb = malloc(sizeof(slsalb_t));
	if(alb == NULL) {
		blogf("Couldn't allocate alb");
		err = ENOMEM;
		goto end_label;
	}
	memset(alb, 0, sizeof(slsalb_t));

	alb->sa_type = binit();
	if(alb->sa_type == NULL) {
		blogf("Couldn't allocate type");
		err = ENOMEM;
		goto end_label;
	}

	alb->sa_artist = binit();
	if(alb->sa_artist == NULL) {
		blogf("Couldn't allocate artist");
		err = ENOMEM;
		goto end_label;
	}

	alb->sa_name = binit();
	if(alb->sa_name == NULL) {
		blogf("Couldn't allocate name");
		err = ENOMEM;
		goto end_label;
	}

	alb->sa_uri = binit();
	if(alb->sa_uri == NULL) {
		blogf("Couldn't allocate uri");
		err = ENOMEM;
		goto end_label;
	}

	alb->sa_url = binit();
	if(alb->sa_url == NULL) {
		blogf("Couldn't allocate url");
		err = ENOMEM;
		goto end_label;
	}

	alb->sa_caurl_lrg = binit();
	if(alb->sa_caurl_lrg == NULL) {
		blogf("Couldn't allocate caurl_lrg");
		err = ENOMEM;
		goto end_label;
	}

	alb->sa_caurl_sml = binit();
	if(alb->sa_caurl_sml == NULL) {
		blogf("Couldn't allocate caurl_sml");
		err = ENOMEM;
		goto end_label;
	}


end_label:

	if(err != 0) {
		if(alb)
			slsalb_uninit(&alb);
		return NULL;
	}
	
	return alb;
}

void
slsalb_uninit(slsalb_t **alb)
{

	if(alb == NULL || *alb == NULL)
		return;

	buninit(&(*alb)->sa_type);
	buninit(&(*alb)->sa_artist);
	buninit(&(*alb)->sa_name);
	buninit(&(*alb)->sa_uri);
	buninit(&(*alb)->sa_url);
	buninit(&(*alb)->sa_caurl_lrg);
	buninit(&(*alb)->sa_caurl_sml);
}
