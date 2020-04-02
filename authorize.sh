#!/bin/bash

# Based on:
# https://gist.github.com/hughrawlinson/1d24595d3648d53440552436dc215d90#
#
# Usage: authorize.sh <client_id> <client_secret>
#
# Generates an access and a refresh token. Credentials will be put in
# .credentials, while the access token will be put in .accesstoken.
# check_accesstoken.sh should be run periodically to refresh the
# access token.
#
# On your Spotify dev dashboard, your app must have "http://localhost:8082/'
# as its redirect_uri.

if [[ -z "$1" || -z "$2" ]]; then
	echo "Invalid arguments"
	exit -1
fi	

CLIENT_ID=$1
CLIENT_SECRET=$2
PORT=8082
REDIRECT_URI="http%3A%2F%2Flocalhost%3A$PORT%2F"
SCOPES="playlist-read-private user-library-read"
AUTH_URL="https://accounts.spotify.com/authorize/?response_type=code&client_id=$CLIENT_ID&redirect_uri=$REDIRECT_URI"

if [[ ! -z $SCOPES ]]; then
	ENCODED_SCOPES=$(echo $SCOPES| tr ' ' '%' | sed s/%/%20/g)
	AUTH_URL="$AUTH_URL&scope=$ENCODED_SCOPES"
fi

# Start user authentication
open "$AUTH_URL"


# Serve up a response once the redirect happens.
RESPONSE=$(echo -e "HTTP/1.1 200 OK\nAccess-Control-Allow-Origin:*\nCache-Control: no-cache, no-store, must-revalidate\nContent-Length:77\n\n<html><body>Authorization successful, please close this page.</body></html>\n" | nc -l -c $PORT)

#echo $RESPONSE

CODE=$(echo "$RESPONSE" | grep "code=" | sed -e 's/^.*code=//' | sed -e 's/ .*$//')

RESPONSE=$(curl -s https://accounts.spotify.com/api/token \
  -H "Content-Type:application/x-www-form-urlencoded" \
  -H "Authorization: Basic $(echo -n "$CLIENT_ID:$CLIENT_SECRET" | base64)" \
  -d "grant_type=authorization_code&code=$CODE&redirect_uri=http%3A%2F%2Flocalhost%3A$PORT%2F")

#echo $RESPONSE
#echo "Expires:"
#echo $RESPONSE | jq -r '.expires_in'
#
#echo
#echo "Access token:"
#echo $RESPONSE | jq -r '.access_token'
#echo
#echo "Refresh token:"
#echo $RESPONSE | jq -r '.refresh_token'

echo "{" > .credentials.json
echo "   \"client_id\" : \"$CLIENT_ID\"," >> .credentials.json
echo "   \"client_secret\" : \"$CLIENT_SECRET\"," >> .credentials.json
echo -n "   \"refresh_token\": \"" >> .credentials.json
echo $RESPONSE | jq -j '.refresh_token'  >> .credentials.json
echo "\"" >> .credentials.json
echo "}" >> .credentials.json

chmod 600 .credentials.json

echo $RESPONSE | jq -r '.access_token'  > .access_token
chmod 600 .access_token
