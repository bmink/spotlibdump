#include <stdio.h>
#include <libgen.h>
#include "bstr.h"
#include "bcurl.h"
#include "cJSON.h"
#include "cJSON_helper.h"


void usage(char *);

int
main(int argc, char **argv)
{
	int	err;
	int	ret;
	char	*execn;
	bstr_t	*authhdr;
	bstr_t	*resp;
	cJSON	*json;
	cJSON	*items;
	cJSON	*item;
	bstr_t	*addedat;
	cJSON	*album;
	bstr_t	*albnam;

	err = 0;
	resp = 0;
	json = NULL;
	addedat = NULL;
	albnam = NULL;

	execn = basename(argv[0]);
	if(xstrempty(execn)) {
		fprintf(stderr, "Can't get executable name\n");
		err = -1;
		goto end_label;
	}	

	if(argc != 3 || xstrempty(argv[1]) || xstrempty(argv[2])) {
		usage(execn);
		err = -1;
		goto end_label;
	}

	ret = bcurl_init();
	if(ret != 0) {
		fprintf(stderr, "Couldn't initialize curl\n");
		err = -1;
		goto end_label;
	}

	ret = bcurl_header_add("Accept: application/json");
	if(ret != 0) {
		fprintf(stderr, "Couldn't add Accept: header\n");
		err = -1;
		goto end_label;
	}

	authhdr = binit();
	if(!authhdr) {
		fprintf(stderr, "Couldn't allocate authhdr\n");
		err = -1;
		goto end_label;
	}
	bprintf(authhdr, "Authorization: Bearer %s", argv[2]);

	ret = bcurl_header_add(bget(authhdr));
	if(ret != 0) {
		fprintf(stderr, "Couldn't add Authorization: header\n");
		err = -1;
		goto end_label;
	}

	buninit(&authhdr);

	ret = bcurl_get("https://api.spotify.com/v1/me/albums", &resp);
	if(ret != 0) {
		fprintf(stderr, "Couldn't get albums list\n");
		err = -1;
		goto end_label;
	}

	printf("%s\n", bget(resp));

	json = cJSON_Parse(bget(resp));
	if(json == NULL) {
		fprintf(stderr, "Couldn't parse JSON\n");
		err = -1;
		goto end_label;
	}

	items = cJSON_GetObjectItemCaseSensitive(json, "items");
	if(!items) {
		fprintf(stderr, "Didn't find items\n");
		err = -1;
		goto end_label;
	}

	for(item = items->child; item; item = item->next) {
		addedat = binit();
		albnam = binit();
		if(!addedat) {
			fprintf(stderr, "Couldn't allocate addedat\n");
			err = -1;
			goto end_label;
		}

		ret = cjson_get_childstr(item, "added_at", addedat);
		if(ret != 0) {
			fprintf(stderr, "Item didn't contain added_at\n");
			err = -1;
			goto end_label;
		}
			
		album = cJSON_GetObjectItemCaseSensitive(item, "album");
		if(!album) {
			fprintf(stderr, "Item didn't contain album\n");
			err = -1;
			goto end_label;
		}

		ret = cjson_get_childstr(album, "name", albnam);
		if(ret != 0) {
			fprintf(stderr, "Album didn't contain name\n");
			err = -1;
			goto end_label;
		}

		printf("%s\n", bget(albnam));
	

		buninit(&addedat);
		buninit(&albnam);
	}



end_label:
	
	bcurl_uninit();
	buninit(&resp);
	buninit(&addedat);
	buninit(&albnam);

	if(json) {
		cJSON_Delete(json);
		json = NULL;	
	}

	return err;
}


void
usage(char *execn)
{
	if(xstrempty(execn))
		return;

	printf("Usage: %s <Spotify User ID> <token>\n", execn);
}
