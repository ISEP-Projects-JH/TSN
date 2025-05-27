# 📦 Build & Deployment Guide

> This document explains how to build and run **TSN (Tailored Social Network)** — a C++20-powered social simulation
> backend.
>
> It supports **development containers**, **slim runtime images**, and optionally embeds a **MySQL database** for fully
> isolated deployment.

---

## 🧱 Overview

TSN is a RESTful C++ service that simulates and evolves a synthetic user network.
It offers friend-of-friend and stranger recommendation systems, compact pod-serialization, and lightweight persistence
with MySQL.

> 💡 **Note**: This project is designed to run in Docker. Local builds are not supported on Windows.  
> **ONLY** clang based is Tested (gcc may work but is not guaranteed).

---

## 🛠️ Development Image (`docker/Dockerfile`)

This is a full-featured environment for debugging and iteration.

### Features

* C++20 toolchain (Clang or GCC)
* Boost 1.88.0 (JSON, ASIO, etc.)
* jh-toolkit POD (`1.3.x-LTS`)
* MySQL **client libraries only**

### Build

```bash
docker buildx build \
  --platform linux/amd64 \
  -t tsn-dev:latest \
  -f docker/Dockerfile \
  --load \
  .
```

### Run

```bash
docker run --rm -p 8080:8080 tsn-dev:latest
```

> 🧠 Note: The dev version **does not embed** a MySQL server. Use `set_db_connection` to connect manually.

---

## 🚀 Runtime Image (`docker/Dockerfile.runtime`)

This is a **multi-stage, minimal deployment** image.

### Features

* Only includes final C++ binary
* Statically linked Boost
* **Optional embedded MySQL** via `--build-arg InnerMySQL=true`

### Build

```bash
docker buildx build \
  --platform linux/amd64 \
  -t tsn-prod:latest \
  -f docker/Dockerfile.runtime \
  --build-arg InnerMySQL=true \
  --load \
  .
```

### Run

```bash
docker run --rm -p 8080:8080 tsn-prod:latest
```

If built with `InnerMySQL=true`, a local `mysqld` is launched and automatically available to the backend at
`localhost:3306`.

---

## 🔐 Connecting to MySQL

### In Dev Mode

You must connect manually:

```bash
POST /api/set_db_connection?renew=true
```

With body:

```json
{
  "host": "host.docker.internal",
  "user": "usr_mdl_usr",
  "password": "example",
  "database": "synthetic_users_local",
  "port": 3306
}
```

### In Prod Mode

With `InnerMySQL=true`, connect to:

```json
{
  "host": "host.docker.internal",
  "user": "usr_mdl_usr",
  "password": "example",
  "database": "synthetic_users_local",
  "port": 3306
}
```

If the DB is empty, use `?renew=true` to auto-create schema.

---

## 🐳 Docker & MySQL Access

| Environment    | MySQL Host             | Notes                                    |
|----------------|------------------------|------------------------------------------|
| Dev container  | `host.docker.internal` | On macOS/Windows with Docker Desktop     |
| Linux host     | via bridge/IP          | Use exposed port or user-defined network |
| Prod container | `localhost`            | Only when built with `InnerMySQL=true`   |

---

## 🧪 Full Test Flow

```bash
bash ./test_api.sh
```

Steps:

1. Connects to DB

> Assuming using the local MySQL instance:
```json
{
  "host": "host.docker.internal",
  "user": "usr_mdl_usr",
  "password": "example",
  "database": "synthetic_users_local",
  "port": 3306
}
```

2. Resets schema
3. Populates random users
4. Simulates 1 day
5. Prints profile and recommendations

---

## 🧩 Native Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/BulgogiAPP
```

Make sure:

* You're using a C++20 compiler
* `libmysqlclient-dev` is installed
* Boost ≥ 1.80 is available
* Always run `set_db_connection` to initialize MySQL
* Use `curl -X POST http://localhost:8080/shutdown_server` to stop the server gracefully

---

## 📦 Image Sizes

| Variant       | Boost   | MySQL          | Image Size | Use Case        |
|---------------|---------|----------------|------------|-----------------|
| `tsn-dev`     | dynamic | external       | \~1.4 GB   | Development     |
| `tsn-prod`    | static  | optional embed | \~90 MB    | Deployment      |
| Native binary | dynamic | local          | <10 MB     | Intranet/Server |

---

## 📄 Related Docs

* [`api.md`](docs/api.md): All backend endpoints
