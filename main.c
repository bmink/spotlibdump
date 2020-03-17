#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include "bstr.h"
#include "bcurl.h"
#include "bfs.h"
#include "cJSON.h"
#include "cJSON_helper.h"

bstr_t	*datadir;


void usage(char *);


int dump_albums(void);

int process_items(cJSON *);


int
main(int argc, char **argv)
{
	int		err;
	int		ret;
	char		*execn;
	bstr_t		*authhdr;

	err = 0;
	datadir = NULL;

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

	datadir = binit();
	if(datadir == NULL) {
		fprintf(stderr, "Can't allocate datadir.\n");
		err = -1;
		goto end_label;
	}

	bstrcat(datadir, getenv("HOME"));
	if(bstrempty(datadir)) {
		fprintf(stderr, "Could not obtain home directory.\n");
		err = -1;
		goto end_label;
	}
	bprintf(datadir, "/.%s", execn);
	if(!bfs_isdir(bget(datadir))) {
		ret = bfs_mkdir(bget(datadir));
		if(ret != 0) {
			fprintf(stderr, "Could not create data directory: %s\n",
			   strerror(ret));
			err = -1;
			goto end_label;
		}
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

	ret = dump_albums();
	if(ret != 0) {
		fprintf(stderr, "Couldn't dump albums.\n");
		err = -1;
		goto end_label;
	}


end_label:
	
	bcurl_uninit();
	buninit(&datadir);

	return err;
}


void
usage(char *execn)
{
	if(xstrempty(execn))
		return;

	printf("Usage: %s <Spotify User ID> <token>\n", execn);
}


int
dump_albums(void)
{
	bstr_t		*resp;
	cJSON		*json;
	cJSON		*items;
	bstr_t		*url;
	int		err;
	int		ret;

	err = 0;
	resp = 0;
	json = NULL;
	url = NULL;


	url = binit();
	if(!url) {
		fprintf(stderr, "Couldn't allocate url\n");
		err = -1;
		goto end_label;
	}
	bprintf(url, "https://api.spotify.com/v1/me/albums");

	while(1) {

		ret = bcurl_get(bget(url), &resp);
		if(ret != 0) {
			fprintf(stderr, "Couldn't get albums list\n");
			err = -1;
			goto end_label;
		}

#if 0
		printf("%s\n", bget(resp));
#endif

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

		ret = process_items(items);
		if(ret != 0) {
			fprintf(stderr, "Couldn't process items\n");
			err = -1;
			goto end_label;
		}

		bclear(url);
		ret = cjson_get_childstr(json, "next", url);

		cJSON_Delete(json);
		json = NULL;

		if(ret != 0)
			break;
#if 0
		printf("next url: %s\n", bget(url));
#endif
	}


end_label:

	buninit(&resp);
	buninit(&url);

	if(json) {
		cJSON_Delete(json);
		json = NULL;	
	}

	return err;
}


int
process_items(cJSON *items)
{
	cJSON		*item;
	bstr_t		*addedat;
	cJSON		*album;
	bstr_t		*alburi;
	bstr_t		*albnam;
	cJSON		*artists;
	cJSON		*artist;
	bstr_t		*artnam;
	bstr_t		*artnam_sub;
	int		err;
	int		ret;

	err = 0;
	addedat = NULL;
	alburi = NULL;
	albnam = NULL;
	artnam = NULL;
	artnam_sub = NULL;

	if(items == NULL)
		return EINVAL;

	for(item = items->child; item; item = item->next) {
		addedat = binit();
		alburi = binit();
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

		ret = cjson_get_childstr(album, "uri", alburi);
		if(ret != 0) {
			fprintf(stderr, "Album didn't contain id\n");
			err = -1;
			goto end_label;
		}

		ret = cjson_get_childstr(album, "name", albnam);
		if(ret != 0) {
			fprintf(stderr, "Album didn't contain name\n");
			err = -1;
			goto end_label;
		}

		artists = cJSON_GetObjectItemCaseSensitive(album, "artists");
		if(!artists) {
			fprintf(stderr, "Didn't find artists\n");
			err = -1;
			goto end_label;
		}

		artnam = binit();
		if(!artnam) {
			fprintf(stderr, "Couldn't allocate artnam\n");
			err = -1;
			goto end_label;
		}

		for(artist = artists->child; artist; artist = artist->next) {

			artnam_sub = binit();
			if(!artnam_sub) {
				fprintf(stderr, "Couldn't allocate"
				    " artnam_sub\n");
				err = -1;
				goto end_label;
			}
			
			ret = cjson_get_childstr(artist, "name", artnam_sub);
			if(ret != 0) {
				fprintf(stderr, "Artist didn't contain name\n");
				err = -1;
				goto end_label;
			}

			if(!bstrempty(artnam))
				bstrcat(artnam, ", ");

			bstrcat(artnam, bget(artnam_sub));

			buninit(&artnam_sub);
		}

#if 0
		printf("art=%s\n", bget(artnam));
		printf("alb=%s\n", bget(albnam));
		printf("uri=%s\n", bget(alburi));
		printf("added_at=%s\n", bget(addedat));
		printf("\n");
#endif
		printf("%s - %s | %s\n", bget(artnam), bget(albnam),
		    bget(alburi));

		buninit(&artnam);
		buninit(&addedat);
		buninit(&alburi);
		buninit(&albnam);
	}

end_label:
	
	buninit(&addedat);
	buninit(&alburi);
	buninit(&albnam);
	buninit(&artnam);
	buninit(&artnam_sub);

	return err;
}
