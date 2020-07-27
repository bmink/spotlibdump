#!/bin/bash

if [[ -z "$REDIS_ADDR" ]]; then
	echo "REDIS_ADDR is not set."
	exit -1
fi

REDIS_CLI="/home/bmink/bin/redis-cli"
REDIS_KEY_CREDS="spotlibdump:credentials"
REDIS_KEY_ACCESSTOK="spotlibdump:access_token"

KEY_EXISTS=`$REDIS_CLI -h $REDIS_ADDR --raw exists "$REDIS_KEY_CREDS"`

if [[ "$KEY_EXISTS" -ne "1" ]]; then
	echo "No credentials found in redis, run do_authorize.sh first"
	exit -1
fi

CREDS=`$REDIS_CLI -h $REDIS_ADDR --raw get "$REDIS_KEY_CREDS"`

CLIENT_ID=$(echo "$CREDS" | jq -r '.client_id')
CLIENT_SECRET=$(echo "$CREDS" | jq -r '.client_secret')
REFRESH_TOKEN=$(echo "$CREDS" | jq -r '.refresh_token')

#echo Refreshing

URL="https://accounts.spotify.com/api/token"

RESPONSE=$(curl -s "$URL" -H "Content-Type:application/x-www-form-urlencoded" \
  -H "Authorization: Basic $(echo -n "$CLIENT_ID:$CLIENT_SECRET" | base64 -w 0)" \
  -d "grant_type=refresh_token&refresh_token=$REFRESH_TOKEN")

ACCESS_TOKEN=`echo $RESPONSE | jq -r '.access_token'`

$REDIS_CLI -h "$REDIS_ADDR" set "$REDIS_KEY_ACCESSTOK" "$ACCESS_TOKEN" >/dev/null

#echo Done
