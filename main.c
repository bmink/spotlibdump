#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <unistd.h>
#include "bstr.h"
#include "bcurl.h"
#include "bfs.h"
#include "cJSON.h"
#include "cJSON_helper.h"

bstr_t	*datadir;
void usage(char *);

#define ALBMODE_SAVED_ALBUMS	0
#define ALBMODE_LIKED_TRACKS	1

int dump_albums(int);
int get_access_token(int);


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

	ret = get_access_token();
	if(ret != 0) {
		fprintf(stderr, "Couldn't get access token\nn");
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

	printf("Getting saved albums...\n");
	fflush(stdout);
	ret = dump_albums(ALBMODE_SAVED_ALBUMS);
	if(ret != 0) {
		fprintf(stderr, "Could not get albums.\n");
		err = -1;
		goto end_label;
	}
	printf("Done.\n");

	printf("Getting albums from liked tracks...\n");
	fflush(stdout);
	ret = dump_albums(ALBMODE_LIKED_TRACKS);
	if(ret != 0) {
		fprintf(stderr, "Could not get albums from saved tracks.\n");
		err = -1;
		goto end_label;
	}
	printf("Done.\n");



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
get_access_token()
{


end_label:


}


#define URL_ALBUMS	"https://api.spotify.com/v1/me/albums"
#define URL_TRACKS	"https://api.spotify.com/v1/me/tracks"
#define FILEN_ALBUMS	"spotlib_saved_albums.txt"
#define FILEN_LT_ALBUMS	"spotlib_liked_track_albums.txt"

int process_items(int, cJSON *, bstr_t *);


int
dump_albums(int mode)
{
	bstr_t		*resp;
	cJSON		*json;
	cJSON		*items;
	bstr_t		*url;
	int		err;
	int		ret;
	bstr_t		*out;
	bstr_t		*filen;
	bstr_t		*filen_tmp;

	err = 0;
	resp = 0;
	json = NULL;
	url = NULL;
	out = NULL;
	filen = NULL;
	filen_tmp = NULL;

	if(mode != ALBMODE_SAVED_ALBUMS && mode != ALBMODE_LIKED_TRACKS)
		return EINVAL;

	url = binit();
	if(!url) {
		fprintf(stderr, "Couldn't allocate url\n");
		err = ENOMEM;
		goto end_label;
	}

	if(mode == ALBMODE_SAVED_ALBUMS)
		bprintf(url, URL_ALBUMS);
	else
	if(mode == ALBMODE_LIKED_TRACKS)
		bprintf(url, URL_TRACKS);

	out = binit();
	if(!out) {
		fprintf(stderr, "Couldn't allocate out\n");
		err = ENOMEM;
		goto end_label;
	}

	filen = binit();
	if(!filen) {
		fprintf(stderr, "Couldn't allocate filen\n");
		err = ENOMEM;
		goto end_label;
	}
	if(mode == ALBMODE_SAVED_ALBUMS)
		bprintf(filen, "%s/%s", bget(datadir), FILEN_ALBUMS);
	else
	if(mode == ALBMODE_LIKED_TRACKS)
		bprintf(filen, "%s/%s", bget(datadir), FILEN_LT_ALBUMS);

	filen_tmp = binit();
	if(!filen_tmp) {
		fprintf(stderr, "Couldn't allocate filen_tmp\n");
		err = ENOMEM;
		goto end_label;
	}
	bprintf(filen_tmp, "%s.%d", bget(filen), getpid());

	while(1) {

		ret = bcurl_get(bget(url), &resp);
		if(ret != 0) {
			fprintf(stderr, "Couldn't get albums list\n");
			err = ret;
			goto end_label;
		}

#if 0
		printf("%s\n", bget(resp));
#endif

		json = cJSON_Parse(bget(resp));
		if(json == NULL) {
			fprintf(stderr, "Couldn't parse JSON\n");
			err = ENOEXEC;
			goto end_label;
		}

		items = cJSON_GetObjectItemCaseSensitive(json, "items");
		if(!items) {
			fprintf(stderr, "Didn't find items\n");
			err = ENOENT;
			goto end_label;
		}

		ret = process_items(mode, items, out);
		if(ret != 0) {
			fprintf(stderr, "Couldn't process items\n");
			err = ret;
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

	ret = btofile(bget(filen_tmp), out);
	if(ret != 0) {
		fprintf(stderr, "Couldn't write file: %s\n", bget(filen_tmp));
		err = ret;
		goto end_label;
	}

	ret = rename(bget(filen_tmp), bget(filen));
	if(ret != 0) {
		fprintf(stderr, "Couldn't rename file to: %s\n", bget(filen));
		err = ret;
		goto end_label;
	}

end_label:

	buninit(&resp);
	buninit(&url);
	buninit(&out);
	buninit(&filen);

	if(!bstrempty(filen_tmp)) {
		unlink(bget(filen_tmp));
	}

	buninit(&filen_tmp);

	if(json) {
		cJSON_Delete(json);
		json = NULL;	
	}

	return err;
}


int
process_items(int mode, cJSON *items, bstr_t *out)
{
	cJSON		*item;
	bstr_t		*addedat;
	cJSON		*album;
	cJSON		*track;
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

	if(mode != ALBMODE_SAVED_ALBUMS && mode != ALBMODE_LIKED_TRACKS)
		return EINVAL;

	for(item = items->child; item; item = item->next) {

		addedat = binit();
		if(!addedat) {
			fprintf(stderr, "Couldn't allocate addedat\n");
			err = ENOMEM;
			goto end_label;
		}

		alburi = binit();
		if(!alburi) {
			fprintf(stderr, "Couldn't allocate alburi\n");
			err = ENOMEM;
			goto end_label;
		}

		albnam = binit();
		if(!albnam) {
			fprintf(stderr, "Couldn't allocate albnam\n");
			err = ENOMEM;
			goto end_label;
		}

		ret = cjson_get_childstr(item, "added_at", addedat);
		if(ret != 0) {
			fprintf(stderr, "Item didn't contain added_at\n");
			err = ENOENT;
			goto end_label;
		}

		if(mode == ALBMODE_SAVED_ALBUMS) {
			album = cJSON_GetObjectItemCaseSensitive(item, "album");
			if(!album) {
				fprintf(stderr, "Item didn't contain album\n");
				err = ENOMEM;
				goto end_label;
			}
	 	} else if(mode == ALBMODE_LIKED_TRACKS) {

			track = cJSON_GetObjectItemCaseSensitive(item, "track");
			if(!track) {
				fprintf(stderr, "Item didn't contain track\n");
				err = ENOMEM;
				goto end_label;
			}

			album = cJSON_GetObjectItemCaseSensitive(track,
			    "album");
			if(!album) {
				fprintf(stderr, "Track didn't contain album\n");
				err = ENOMEM;
				goto end_label;
			}
		} 

		ret = cjson_get_childstr(album, "uri", alburi);
		if(ret != 0) {
			fprintf(stderr, "Album didn't contain id\n");
			err = ENOENT;
			goto end_label;
		}

		ret = cjson_get_childstr(album, "name", albnam);
		if(ret != 0) {
			fprintf(stderr, "Album didn't contain name\n");
			err = ENOENT;
			goto end_label;
		}

		artists = cJSON_GetObjectItemCaseSensitive(album, "artists");
		if(!artists) {
			fprintf(stderr, "Didn't find artists\n");
			err = ENOENT;
			goto end_label;
		}

		artnam = binit();
		if(!artnam) {
			fprintf(stderr, "Couldn't allocate artnam\n");
			err = ENOMEM;
			goto end_label;
		}

		for(artist = artists->child; artist; artist = artist->next) {

			artnam_sub = binit();
			if(!artnam_sub) {
				fprintf(stderr, "Couldn't allocate"
				    " artnam_sub\n");
				err = ENOMEM;
				goto end_label;
			}
			
			ret = cjson_get_childstr(artist, "name", artnam_sub);
			if(ret != 0) {
				fprintf(stderr, "Artist didn't contain name\n");
				err = ENOENT;
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
		bprintf(out, "%s - %s | %s\n", bget(artnam), bget(albnam),
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



