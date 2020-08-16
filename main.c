#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include <unistd.h>
#include "bstr.h"
#include "bcurl.h"
#include "bfs.h"
#include "blog.h"
#include "cJSON.h"
#include "cJSON_helper.h"
#include "hiredis_helper.h"
#include "slsobj.h"

bstr_t	*datadir;
bstr_t	*access_tok;

void usage(char *);

#define ALBMODE_SAVED_ALBUMS	0
#define ALBMODE_LIKED_TRACKS	1

int load_access_tok(void);
int dump_albums(int);
int dump_playlists(void);
int unset_repeat(void);

#define MODE_LIBDUMP	0
#define MODE_UNSETREP	1

#define SLSOBJ_TYPE_SPOT	"spotify"

#define REDIS_KEY_ACCESSTOK	"spotlibdump:access_token"
#define REDIS_KEY_S_ALBUMS_ALL	"spotlibdump:saved_albums:all"
#define REDIS_KEY_LT_ALBUMS_ALL	"spotlibdump:liked_track_albums:all"


int
main(int argc, char **argv)
{
	int		err;
	int		ret;
	char		*execn;
	bstr_t		*authhdr;
	int		mode;

	err = 0;
	datadir = NULL;
	mode = MODE_LIBDUMP;

	execn = basename(argv[0]);
	if(xstrempty(execn)) {
		fprintf(stderr, "Can't get executable name\n");
		err = -1;
		goto end_label;
	}	

	ret = blog_init(execn);
	if(ret != 0) {
		fprintf(stderr, "Could not initialize logging: %s\n",
		    strerror(ret));
		goto end_label;
	}

	ret = hiredis_init();
	if(ret != 0) {
		fprintf(stderr, "Could not connect to redis\n");
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

	if(argc > 1) {
		if((argc == 2) && (!xstrcmp(argv[1], "unsetrep"))) {
			mode = MODE_UNSETREP;
		} else {
			fprintf(stderr, "Wrong arguments specified\n");
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

	ret = load_access_tok();
	if(ret != 0) {
		fprintf(stderr, "Couldn't load access token\nn");
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
	bprintf(authhdr, "Authorization: Bearer %s", bget(access_tok));

	ret = bcurl_header_add(bget(authhdr));
	if(ret != 0) {
		fprintf(stderr, "Couldn't add Authorization: header\n");
		err = -1;
		goto end_label;
	}
	buninit(&authhdr);

	switch(mode) {

	case MODE_LIBDUMP:
	default:
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
	
		printf("Getting playlists...\n");
		fflush(stdout);
		ret = dump_playlists();
		if(ret != 0) {
			fprintf(stderr, "Could not get playlists.\n");
			err = -1;
			goto end_label;
		}
	 	printf("Done.\n");
		
		break;

	case MODE_UNSETREP:
		printf("Unsetting repeat...\n");
		fflush(stdout);
		ret = unset_repeat();
		if(ret != 0) {
			fprintf(stderr, "Could not unset repeat.\n");
			err = -1;
			goto end_label;
		}
	 	printf("Done.\n");
		
		break;

	
	}


end_label:
	
	bcurl_uninit();
	buninit(&datadir);
	buninit(&access_tok);

	hiredis_uninit();
	blog_uninit();

	return err;
}


void
usage(char *execn)
{
	if(xstrempty(execn))
		return;

	printf("Usage: %s [unsetrep]\n", execn);
}



int
load_access_tok(void)
{
	int	err;
	int	ret;

	err = 0;

	access_tok = binit();
	if(access_tok == NULL) {
		fprintf(stderr, "Couldn't allocate access_tok\n");
		err = ENOMEM;
		goto end_label;
	}

	ret = hiredis_get(REDIS_KEY_ACCESSTOK, access_tok);
	if(ret != 0 || bstrempty(access_tok)) {
		fprintf(stderr, "Couldn't load access token: %s\n",
		    strerror(ret));
		err = ret;
		goto end_label;
	}

	ret = bstrchopnewline(access_tok);
	if(ret != 0) {
		fprintf(stderr, "Couldn't chop newline from access token: %s\n",
		    strerror(ret));
		err = ret;
		goto end_label;
	}

end_label:

	if(err != 0) {
		buninit(&access_tok);
	}

	return err;


}


#define URL_ALBUMS	"https://api.spotify.com/v1/me/albums"
#define URL_TRACKS	"https://api.spotify.com/v1/me/tracks"
#define FILEN_ALBUMS	"spotlib_saved_albums.txt"
#define FILEN_LT_ALBUMS	"spotlib_liked_track_albums.txt"

int process_items_album(int, cJSON *, bstr_t *, bstr_t *);


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
	bstr_t		*rediskey;
	bstr_t		*rediskey_tmp;

	err = 0;
	resp = 0;
	json = NULL;
	url = NULL;
	out = NULL;
	filen = NULL;
	filen_tmp = NULL;
	rediskey = NULL;
	rediskey_tmp = NULL;

	if(mode != ALBMODE_SAVED_ALBUMS && mode != ALBMODE_LIKED_TRACKS)
		return EINVAL;

	url = binit();
	if(!url) {
		fprintf(stderr, "Couldn't allocate url\n");
		err = ENOMEM;
		goto end_label;
	}

	if(mode == ALBMODE_SAVED_ALBUMS) {
		bprintf(url, URL_ALBUMS);
	} else
	if(mode == ALBMODE_LIKED_TRACKS) {
		bprintf(url, URL_TRACKS);
	}

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

	rediskey = binit();
	if(!rediskey) {
		fprintf(stderr, "Couldn't allocate rediskey\n");
		err = ENOMEM;
		goto end_label;
	}
	if(mode == ALBMODE_SAVED_ALBUMS)
		bprintf(rediskey, "%s", REDIS_KEY_S_ALBUMS_ALL);
	else
	if(mode == ALBMODE_LIKED_TRACKS)
		bprintf(rediskey, "%s", REDIS_KEY_LT_ALBUMS_ALL);

	rediskey_tmp = binit();
	if(!rediskey_tmp) {
		fprintf(stderr, "Couldn't allocate rediskey_tmp\n");
		err = ENOMEM;
		goto end_label;
	}
	bprintf(rediskey_tmp, "%s:tmp:%d", bget(rediskey), getpid());


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

		ret = process_items_album(mode, items, out, rediskey_tmp);
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

	ret = hiredis_rename(bget(rediskey_tmp), bget(rediskey));
	if(ret != 0) {
		fprintf(stderr, "Couldn't rename redis key '%s' to '%s'\n",
		    bget(rediskey_tmp), bget(rediskey));
		err = ret;
		goto end_label;
	}

end_label:

	buninit(&resp);
	buninit(&url);
	buninit(&out);
	buninit(&filen);
	buninit(&rediskey);
	buninit(&rediskey_tmp);

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
process_items_album(int mode, cJSON *items, bstr_t *out, bstr_t *rediskey)
{
	cJSON		*item;
	cJSON		*album;
	cJSON		*track;
	cJSON		*artists;
	cJSON		*artist;
	cJSON		*external_urls;
	bstr_t		*artnam_sub;
	cJSON		*images;
	cJSON		*image;
	int		width;
	bstr_t		*imgurl_targ;
	int		err;
	int		ret;
	slsalb_t	*slsalb;
	bstr_t		*slsalb_json;
	int		nadded;

	err = 0;
	slsalb = NULL;
	artnam_sub = NULL;
	slsalb_json = NULL;

	if(items == NULL || out == NULL || bstrempty(rediskey))
		return EINVAL;

	if(mode != ALBMODE_SAVED_ALBUMS && mode != ALBMODE_LIKED_TRACKS)
		return EINVAL;

	slsalb = slsalb_init(SLSOBJ_TYPE_SPOT);
	if(slsalb == NULL) {
		fprintf(stderr, "Couldn't allocate slsalb\n");
		err = ENOMEM;
		goto end_label;
	}

	slsalb_json = binit();
	if(!slsalb_json) {
		fprintf(stderr, "Couldn't allocate slsalb_json\n");
		err = ENOMEM;
		goto end_label;
	}

	artnam_sub = binit();
	if(!artnam_sub) {
		fprintf(stderr, "Couldn't allocate artnam_sub\n");
		err = ENOMEM;
		goto end_label;
	}

	for(item = items->child; item; item = item->next) {

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

		ret = cjson_get_childstr(album, "uri", slsalb->sa_uri);
		if(ret != 0) {
			fprintf(stderr, "Album didn't contain uri\n");
			err = ENOENT;
			goto end_label;
		}

		ret = cjson_get_childstr(album, "name", slsalb->sa_name);
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

		for(artist = artists->child; artist; artist = artist->next) {

			ret = cjson_get_childstr(artist, "name", artnam_sub);
			if(ret != 0) {
				fprintf(stderr, "Artist didn't contain name\n");
				err = ENOENT;
				goto end_label;
			}

			if(!bstrempty(slsalb->sa_artist))
				bstrcat(slsalb->sa_artist, ", ");

			bstrcat(slsalb->sa_artist, bget(artnam_sub));

			bclear(artnam_sub);
		}

		external_urls = cJSON_GetObjectItemCaseSensitive(album,
		    "external_urls");
		if(!external_urls) {
			fprintf(stderr, "Didn't find external_urls\n");
			err = ENOENT;
			goto end_label;
		}

		ret = cjson_get_childstr(external_urls, "spotify",
		    slsalb->sa_url);
		if(ret != 0) {
			fprintf(stderr, "Album didn't contain URL\n");
			err = ENOENT;
			goto end_label;
		}

		images = cJSON_GetObjectItemCaseSensitive(album,
		    "images");
		if(images) {
			for(image = images->child; image; image = image->next) {
				ret = cjson_get_childint(image, "width",
				    &width);
				if(ret != 0) {
					fprintf(stderr, "Image didn't contain"
					    " width\n");
					err = ENOENT;
					goto end_label;
				}

				imgurl_targ = NULL;
				if(width == 640) {
					imgurl_targ = slsalb->sa_caurl_lrg;
				} else
				if(width == 300) {
					imgurl_targ = slsalb->sa_caurl_med;
				} else

				if(width == 64) {
					imgurl_targ = slsalb->sa_caurl_sml;
				}

				if(imgurl_targ == NULL) {
					/* Unknown size */
					continue;
				}

				ret = cjson_get_childstr(image, "url",
				    imgurl_targ);
				if(ret != 0) {
					fprintf(stderr, "Image didn't contain"
					    " URL\n");
					err = ENOENT;
					goto end_label;
				}

			}
		}

#if 0
		printf("art=%s\n", bget(slsalb->sa_artist));
		printf("alb=%s\n", bget(slsalb->sa_name));
		printf("uri=%s\n", bget(slsalb->sa_uri));
		printf("\n");
#endif
		bprintf(out, "%s - %s | %s\n", bget(slsalb->sa_artist),
		    bget(slsalb->sa_name), bget(slsalb->sa_uri));

		ret = slsalb_tojson(slsalb, slsalb_json);
		if(ret != 0) {
			blogf("Couldn't render slsalb to JSON");
			err = ret;
			goto end_label;
		}
		printf("%s\n", bget(slsalb_json));

		nadded = 0;
		ret = hiredis_sadd(bget(rediskey), slsalb_json, &nadded);
		if(ret != 0 || nadded != 1) {
			blogf("Couldn't add album to redis!");
		}

		bclear(slsalb_json);

		slsalb_uninit(&slsalb);

		slsalb = slsalb_init(SLSOBJ_TYPE_SPOT);
		if(slsalb == NULL) {
			fprintf(stderr, "Couldn't allocate slsalb\n");
			err = ENOMEM;
			goto end_label;
		}

	}

end_label:
	
	buninit(&artnam_sub);
	buninit(&slsalb_json);

	slsalb_uninit(&slsalb);

	return err;
}


int process_items_pl(cJSON *, bstr_t *);

#define URL_PLAYLISTS	"https://api.spotify.com/v1/me/playlists"
#define FILEN_PLAYLISTS	"spotlib_playlists.txt"


int
dump_playlists(void)
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


	url = binit();
	if(!url) {
		fprintf(stderr, "Couldn't allocate url\n");
		err = ENOMEM;
		goto end_label;
	}
	bprintf(url, URL_PLAYLISTS);

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
	bprintf(filen, "%s/%s", bget(datadir), FILEN_PLAYLISTS);

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
			fprintf(stderr, "Couldn't get playlist list\n");
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

		ret = process_items_pl(items, out);
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
process_items_pl(cJSON *items, bstr_t *out)
{
	cJSON		*item;
	cJSON		*owner;
	bstr_t		*plnam;
	bstr_t		*pluri;
	bstr_t		*ownernam;
	int		err;
	int		ret;

	err = 0;
	plnam = NULL;
	pluri = NULL;
	ownernam = NULL;

	if(items == NULL)
		return EINVAL;

	for(item = items->child; item; item = item->next) {

		plnam = binit();
		if(!plnam) {
			fprintf(stderr, "Couldn't allocate plnam\n");
			err = ENOMEM;
			goto end_label;
		}

		pluri = binit();
		if(!pluri) {
			fprintf(stderr, "Couldn't allocate pluri\n");
			err = ENOMEM;
			goto end_label;
		}

		ownernam = binit();
		if(!ownernam) {
			fprintf(stderr, "Couldn't allocate ownernam\n");
			err = ENOMEM;
			goto end_label;
		}

		ret = cjson_get_childstr(item, "name", plnam);
		if(ret != 0) {
			fprintf(stderr, "Item didn't contain name\n");
			err = ENOENT;
			goto end_label;
		}

		ret = cjson_get_childstr(item, "uri", pluri);
		if(ret != 0) {
			fprintf(stderr, "Item didn't contain uri\n");
			err = ENOENT;
			goto end_label;
		}

		owner = cJSON_GetObjectItemCaseSensitive(item, "owner");
		if(!owner) {
			fprintf(stderr, "Item didn't contain owner\n");
			err = ENOMEM;
			goto end_label;
		}

		ret = cjson_get_childstr(owner, "display_name", ownernam);
		if(ret != 0) {
			fprintf(stderr, "Owner didn't contain display_name\n");
			err = ENOENT;
			goto end_label;
		}

		bprintf(out, "%s | %s | %s\n", bget(ownernam), bget(plnam),
		    bget(pluri));

		buninit(&plnam);
		buninit(&pluri);
		buninit(&ownernam);
	}

end_label:
	
	buninit(&plnam);
	buninit(&pluri);
	buninit(&ownernam);

	return err;
}


#define URL_PLAYER_UNSET_REPEAT \
	"https://api.spotify.com/v1/me/player/repeat?state=off"

int
unset_repeat(void)
{
	bstr_t		*resp;
	bstr_t		*url;
	int		err;
	int		ret;

	err = 0;
	resp = 0;
	url = NULL;

	url = binit();
	if(!url) {
		fprintf(stderr, "Couldn't allocate url\n");
		err = ENOMEM;
		goto end_label;
	}
	bprintf(url, URL_PLAYER_UNSET_REPEAT);

	ret = bcurl_put(bget(url), NULL, &resp);
	if(ret != 0) {
		fprintf(stderr, "Couldn't PUT unset URL\n");
		err = ret;
		goto end_label;
	}

#if 0
	printf("%d\n%s\n", ret, bget(resp));
	exit(0);
#endif

end_label:

	buninit(&resp);
	buninit(&url);

	return err;
}

