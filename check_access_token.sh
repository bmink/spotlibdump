#!/bin/bash

# Based on:
# https://gist.github.com/hughrawlinson/1d24595d3648d53440552436dc215d90#
#
# Usage: authorize.sh <client_id> <client_secret>
#
# Generates an access and a refresh token. The access token will be good for
# an hour only, so we don't really care about it. The important thing is the
# refresh token. It should be put into a file named .refreshtoken. The program
# will read it and acquire a new access token before doing anything else.
#
#
# On your Spotify dev dashboard, your app must have "http://localhost:8082/'
# as its redirect_uri.

if [[ -z "$REDIS_ADDR" ]]; then
	echo "REDIS_ADDR is not set."
	exit -1
fi

REDIS_KEY_CREDS="spotlibdump:credentials"
REDIS_KEY_ACCESSTOK="spotlibdump:access_token"

KEY_EXISTS=`redis-cli -h $REDIS_ADDR --raw exists "$REDIS_KEY_CREDS"`

if [[ "$KEY_EXISTS" -ne "1" ]]; then
	echo "No credentials found in redis, run do_authorize.sh first"
	exit -1
fi

CREDS=`redis-cli -h $REDIS_ADDR --raw get "$REDIS_KEY_CREDS"`

CLIENT_ID=$(echo "$CREDS" | jq -r '.client_id')
CLIENT_SECRET=$(echo "$CREDS" | jq -r '.client_secret')
REFRESH_TOKEN=$(echo "$CREDS" | jq -r '.refresh_token')

#echo Refreshing

URL=https://accounts.spotify.com/api/token

RESPONSE=$(curl -s "$URL" -H "Content-Type:application/x-www-form-urlencoded" \
  -H "Authorization: Basic $(echo -n "$CLIENT_ID:$CLIENT_SECRET" | base64)" \
  -d "grant_type=refresh_token&refresh_token=$REFRESH_TOKEN")


#echo $RESPONSE

ACCESS_TOKEN=`echo $RESPONSE | jq -r '.access_token'`

redis-cli -h "$REDIS_ADDR" set "$REDIS_KEY_ACCESSTOK" "$ACCESS_TOKEN" >/dev/null

#echo Done
