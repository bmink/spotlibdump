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

#REFRESH_MIN=0
REFRESH_MIN=45

if [[ ! -f .credentials.json ]]; then
	echo "No credentials file, run do_authorize.sh first"
	exit -1
fi

if [[ $(find .access_token -mmin -$REFRESH_MIN) ]]; then
	# Token is still good, nothing to do.
	exit 0
fi

CLIENT_ID=$(cat .credentials.json | jq -r '.client_id')
CLIENT_SECRET=$(cat .credentials.json | jq -r '.client_secret')
REFRESH_TOKEN=$(cat .credentials.json | jq -r '.refresh_token')

echo Refreshing

URL=https://accounts.spotify.com/api/token

RESPONSE=$(curl -s "$URL" -H "Content-Type:application/x-www-form-urlencoded" \
  -H "Authorization: Basic $(echo -n "$CLIENT_ID:$CLIENT_SECRET" | base64)" \
  -d "grant_type=refresh_token&refresh_token=$REFRESH_TOKEN")


#echo $RESPONSE

echo $RESPONSE | jq -r '.access_token'  > .access_token
chmod 600 .access_token

echo Done
