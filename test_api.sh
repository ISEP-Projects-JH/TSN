#!/bin/bash

BASE_URL=http://localhost:8080/api

# Step 0: Set database connection
echo "🔐 Connecting to MySQL..."
curl -s -X POST "${BASE_URL}/set_db_connection?renew=true" \
  -H "Content-Type: application/json" \
  -d '{
    "host": "host.docker.internal",
    "user": "usr_mdl_usr",
    "password": "example",
    "database": "synthetic_users_local",
    "port": 3306
  }' | jq


# Step 1: Refresh DB
echo "⏳ Resetting DB..."
curl -s -X POST ${BASE_URL}/refresh_db | jq

# Step 2: Get a random user
echo "🎲 Fetching a random user ID..."
USER_ID=$(curl -s ${BASE_URL}/random_user_id | jq '.user_id')
echo "👉 User ID: $USER_ID"

# Step 3: Get user profile
echo "🧑 User profile:"
curl -s "${BASE_URL}/get_user_profile?id=$USER_ID" | jq

# Step 4: Simulate one day
echo "📆 Simulating a day of activity..."
curl -s -X POST ${BASE_URL}/simulate_day | jq

# Step 5: Get FOF recommendations
echo "🔗 FOF Recommendations:"
FOF=$(curl -s "${BASE_URL}/recommend_fof?id=$USER_ID" | tee /tmp/fof.json | jq '.recommendations')
echo "$FOF"

# Step 6: Get stranger recommendations
echo "🧍 Stranger Recommendations:"
STRANGERS=$(curl -s "${BASE_URL}/recommend_strangers?id=$USER_ID" | tee /tmp/strangers.json | jq '.recommendations')
echo "$STRANGERS"

# Step 7: Pick one from Stranger and get their info
RECOMMENDED_ID=$(jq '.recommendations[0] // empty' /tmp/strangers.json)
if [ -n "$RECOMMENDED_ID" ]; then
    echo "🔍 Getting profile of recommended ID: $RECOMMENDED_ID"
    curl -s "${BASE_URL}/get_user_profile_simple?id=$RECOMMENDED_ID" | jq
else
    echo "❗ No stranger recommendations found."
fi