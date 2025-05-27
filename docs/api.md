# 📡 API Documentation

This document describes all available backend API endpoints, including usage, parameters, expected responses, and initialization behavior.

> ℹ️ Note: For Docker-specific SQL setup, see [`build.md`](../build.md)

---

## 🔐 `/api/set_db_connection`

**Purpose**: Establish a database connection dynamically (for local or containerized environments).
**Method**: `POST`

**Query parameter**:

* `renew=true` (optional) → if provided, **creates all required tables** using the embedded SQL schema.

**Request body (JSON)**:

```json
{
  "host": "localhost",
  "user": "usr_mdl_usr",
  "password": "example",
  "database": "synthetic_users_local",
  "port": 3306
}
```

**Successful response**:

```json
{
  "status": "connected",
  "db": "synthetic_users_local"
}
```

**Error cases**:

* Invalid credentials or unreachable server
* Missing tables (if `renew` is not used)
* Server not initialized

---

## 🧪 `/api/ping`

Check if the server is alive.
**Method**: `GET`

**Response**:

```json
{ "status": "alive" }
```

---

## 🚨 `/api/shutdown_server`

Shut down the backend gracefully.
**Method**: `POST`

---

## 🔁 `/api/refresh_db`

Clear the database and repopulate it with synthetic users.
**Method**: `POST`

**Response**:

```json
{
  "status": "database_refreshed",
  "new_users": 123
}
```

---

## 🎲 `/api/random_user_id`

Get a randomly selected valid user ID.
**Method**: `GET`

**Response**:

```json
{ "user_id": 42 }
```

---


## 🔢 `/api/get_total_count`

Get the **total number of users** currently stored in the database.

**Method**: `GET`

**Response**:

```json
{
  "total_users": 12345
}
```

**Error response** (e.g. MySQL not ready or handler not initialized):

```json
{
  "error": "Fabric handler not ready"
}
```

---

## 📦 `/api/batch_get_simple_profiles`

Get **lightweight profiles** for multiple users in one call.

**Method**: `POST`
**Request body**: A JSON array of user IDs (integers)

```json
[42, 84, 128]
```

**Response**:

```json
{
  "profiles": [
    {
      "user_id": 42,
      "first_name": "Alice",
      "surname": "Wang",
      "gender": true,
      "avatar_url": "base64..."
    },
    {
      "user_id": 84,
      "first_name": "Bob",
      "surname": "Chen",
      "gender": false,
      "avatar_url": "base64..."
    }
  ]
}
```

**Error response** (invalid input or server error):

```json
{
  "error": "Invalid request format"
}
```

or

```json
{
  "error": "some exception message"
}
```


---

## 👤 `/api/get_user_profile`

Get the **full user profile** for a given ID.
**Method**: `GET`
**Query param**: `id={number}`

> gender: `true` for female, `false` for male

**Response**:

```json
{
  "user_id": 42,
  "first_name": "Alice",
  "surname": "Wang",
  "gender": true,
  "avatar_url": "base64...",
  "interests": [
    { "name": "Music", "value": 9 },
    "..."
  ],
  "tags": ["Is Creator", "Night Owl", "..."]
}
```

---

## 👤 `/api/get_user_profile_simple`

Get a **lightweight profile** (name/avatar only).
**Method**: `GET`
**Query param**: `id={number}`

**Response**:

```json
{
  "user_id": 42,
  "first_name": "Alice",
  "surname": "Wang",
  "gender": true,
  "avatar_url": "base64..."
}
```

---

## 🤝 `/api/get_user_friends`

Get a list of user IDs this user interacts with regularly.
**Method**: `GET`
**Query param**: `id={number}`

**Response**:

```json
{ "friends": [8, 15, 21] }
```

---

## ⏳ `/api/simulate_day`

Simulate one day of activity:

* New interactions
* Friendships
* New users
  **Method**: `POST`

**Response**:

```json
{
  "new_users": 18,
  "new_friendships": 274,
  "total_interactions": 3921
}
```

---

## 🧭 `/api/recommend_fof`

Recommend **friends-of-friends** using A\* traversal.
**Method**: `GET`
**Query param**: `id={number}`

**Response**:

```json
{
  "recommendations": [51, 87, 90, "..."]
}
```

---

## 🧍 `/api/recommend_strangers`

Recommend strangers with **similar interests and traits**, excluding known friends.
**Method**: `GET`
**Query param**: `id={number}`

**Response**:

```json
{
  "recommendations": [73, 118, 103, "..."]
}
```

---

## 🧱 Initialization Behavior

If you are using a **new/empty database**, you must call:

```http
POST /api/set_db_connection?renew=true
```

This will create the required tables:

* `UsersFabric`
* `UserModels`
* With proper constraints and schema

> This logic is embedded directly in the backend and **does not require any external `.sql` file.**

---

## 🐳 Docker Considerations

If the backend runs **inside Docker**, remember:

* `localhost` from within Docker refers to the container itself.

* To connect to your **host MySQL**, use:

  * `host.docker.internal` (available on **Windows** and **macOS** with Docker Desktop)
  * On **Linux**, this may not work by default. You can:

    * Manually expose host IP (e.g., `172.17.0.1`)
    * Or use `--add-host=host.docker.internal:host-gateway` on Docker 20.10+

* Alternatively, define a **Docker network** and connect by service name (`mysql`, `api`, etc.) if both containers are in the same `docker-compose` file.

For more information, see [`build.md`](../build.md).
