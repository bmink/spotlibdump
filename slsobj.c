#include <errno.h>
#include "slsobj.h"
#include "blog.h"
#include "cJSON.h"
#include "cJSON_helper.h"

slsalb_t
*slsalb_init(const char *type)
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
	if(!xstrempty(type))
		bstrcat(alb->sa_type, type);

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

	alb->sa_caurl_med = binit();
	if(alb->sa_caurl_med == NULL) {
		blogf("Couldn't allocate caurl_med");
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
	buninit(&(*alb)->sa_caurl_med);
	buninit(&(*alb)->sa_caurl_sml);
}


int
slsalb_tojson(slsalb_t *alb, bstr_t *buf)
{
	cJSON	*albj;
	cJSON	*child;
	int	err;
	char	*rendered;

	if(alb == NULL || buf == NULL)
		return EINVAL;

	err = 0;
	albj = NULL;
	rendered = NULL;

	albj = cJSON_CreateObject();
	if(albj == NULL) {
		blogf("Couldn't create JSON object");
		err = ENOMEM;
		goto end_label;
	}

	child = cJSON_AddStringToObject(albj, "type", bget(alb->sa_type));
	if(child == NULL) {
		err = ENOEXEC;
		goto end_label;
	}
	
	child = cJSON_AddStringToObject(albj, "artist", bget(alb->sa_artist));
	if(child == NULL) {
		err = ENOEXEC;
		goto end_label;
	}

	child = cJSON_AddStringToObject(albj, "name", bget(alb->sa_name));
	if(child == NULL) {
		err = ENOEXEC;
		goto end_label;
	}

	child = cJSON_AddStringToObject(albj, "uri", bget(alb->sa_uri));
	if(child == NULL) {
		err = ENOEXEC;
		goto end_label;
	}

	child = cJSON_AddStringToObject(albj, "url", bget(alb->sa_url));
	if(child == NULL) {
		err = ENOEXEC;
		goto end_label;
	}
				
	child = cJSON_AddStringToObject(albj, "caurl_lrg",
	    bget(alb->sa_caurl_lrg));
	if(child == NULL) {
		err = ENOEXEC;
		goto end_label;
	}

	child = cJSON_AddStringToObject(albj, "caurl_med",
	    bget(alb->sa_caurl_med));
	if(child == NULL) {
		err = ENOEXEC;
		goto end_label;
	}

	child = cJSON_AddStringToObject(albj, "caurl_sml",
	    bget(alb->sa_caurl_sml));
	if(child == NULL) {
		err = ENOEXEC;
		goto end_label;
	}

	rendered = cJSON_Print(albj);
	if(xstrempty(rendered)) {
		blogf("Could not render into string");
		err = ENOEXEC;
		goto end_label;
	}

	bclear(buf);
	bstrcat(buf, rendered);

end_label:

	if(albj != NULL) {
		cJSON_Delete(albj);
		albj = NULL;
	}

	if(rendered != NULL) {
		free(rendered);	
		rendered = NULL;
	}

	return err;
}
