#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include "bstr.h"
#include "bcurl.h"
#include "bjson.h"
#include "cJSON.h"
#include "cJSON_helper.h"


void usage(char *);

typedef struct spoti_album {
	bstr_t	*sa_id;
	bstr_t	*sa_name;
	bstr_t	*sa_uri;
	barr_t	*sa_artists;
} spoti_album_t;



bjson_field_t	bjson_album_fields[] = {
	{ "id",		BJSON_STRING,	1, offsetof(spoti_album_t, sa_id) },
	{ "name",	BJSON_STRING,	1, offsetof(spoti_album_t, sa_name) },
	{ "uri",	BJSON_STRING,	1, offsetof(spoti_album_t, sa_uri) },
//	{ "artists",	BJSON_ARRAY,	1, offsetof(spoti_album_t, sa_artists) },
	{ NULL }
};


int
conv_fields(cJSON *json, void *target, bjson_field_t *fields)
{
	bjson_field_t		*field;
	bstr_t			*strval;
	int			ret;

	if(!json || !target || !fields)
		return EINVAL;

	strval = NULL;

	for(field = fields; field->bf_name; ++field) {

		printf("Converting '%s'\n", field->bf_name);

		switch(field->bf_type) {
		case BJSON_STRING:

			strval = binit();
			if(!strval) {
				fprintf(stderr, "Couldn't allocate strval\n");
				return ENOMEM;
			}

			ret = cjson_get_childstr(json, field->bf_name, strval);
			if(ret != 0 && field->bf_required) {
				fprintf(stderr, "Didn't contain required %s\n",
				    field->bf_name);
				buninit(&strval);
				return ENOENT;
			}


			memcpy((target + field->bf_offset), &strval,
				sizeof(bstr_t *));

			strval = NULL;

			break;

		default:
			printf("Unknown type for '%s'\n", field->bf_name);
			break;
		}


	

		strval = binit();
		if(!strval) {
			fprintf(stderr, "Couldn't allocate strval");
			return ENOMEM;
		}

	}


	return 0;
}



int
conv_album(cJSON *album, spoti_album_t *salb)
{
	int	ret;

	if(!album || !salb)
		return EINVAL;

	ret = conv_fields(album, (void *)salb, bjson_album);

	return ret;
}



int
main(int argc, char **argv)
{
	int		err;
	int		ret;
	char		*execn;
	bstr_t		*authhdr;
	bstr_t		*resp;
	cJSON		*json;
	cJSON		*items;
	cJSON		*item;
	bstr_t		*addedat;
	cJSON		*album;
	bstr_t		*alburi;
	bstr_t		*albnam;
	cJSON		*artists;
	cJSON		*artist;
	bstr_t		*artnam;
	spoti_album_t	salb;

	err = 0;
	resp = 0;
	json = NULL;
	addedat = NULL;
	alburi = NULL;
	albnam = NULL;
	artnam = NULL;

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


		memset(&salb, 0, sizeof(spoti_album_t));
	
		ret = conv_album(album, &salb);
		if(ret != 0) {
			fprintf(stderr, "Could not convert album: %s\n",
			    strerror(ret));
			err = -1;
			goto end_label;
		}

		printf("uri=%s\nid=%s\nname=%s\n\n", bget(salb.sa_uri),
		    bget(salb.sa_id), bget(salb.sa_name));


#if 0
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

		for(artist = artists->child; artist; artist = artist->next) {
			artnam = binit();
			if(!artnam) {
				fprintf(stderr, "Couldn't allocate artnam\n");
				err = -1;
				goto end_label;
			}

			ret = cjson_get_childstr(artist, "name", artnam);
			if(ret != 0) {
				fprintf(stderr, "Artist didn't contain name\n");
				err = -1;
				goto end_label;
			}

			printf("art=%s\n", bget(artnam));

			buninit(&artnam);
		}

		printf("alb=%s\n", bget(albnam));
		printf("uri=%s\n", bget(alburi));
		printf("added_at=%s\n", bget(addedat));
		printf("\n");
#endif

		buninit(&addedat);
		buninit(&alburi);
		buninit(&albnam);

	}



end_label:
	
	bcurl_uninit();
	buninit(&resp);
	buninit(&addedat);
	buninit(&alburi);
	buninit(&albnam);
	buninit(&artnam);

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
