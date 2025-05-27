# Quick Start

## Prerequisites
- Docker (required; local C++ build is not supported on Windows)

## 1. Clone the Repository

```bash
git clone https://github.com/ISEP-Projects-JH/TSN.git
cd TSN
```
## 2. Build and Run with Docker

This project is designed to run entirely in Docker, including the MySQL server.

Build the Docker image using the provided runtime Dockerfile:

```bash
docker build -f docker/Dockerfile.runtime -t tsn-runtime .
```
Run the container (the MySQL server will be started automatically inside the container):
```bash
docker run -p 8080:8080 tsn-runtime
```
> **Note:** No local C++ toolchain or MySQL installation is required. All dependencies are handled inside the Docker container.

## 3. Initialize Database Connection (MUST DO)

After the server starts, you **must** call the `/set_db_connection` API (POST) to initialize the MySQL connection and handlers. If you skip this step, all API calls will return:

## 3. API Usage

Refer to [docs/api.md](api.md) for available endpoints and usage examples.


## 4. API Usage

Refer to [docs/api.md](api.md) for available endpoints and usage examples.

## 5. Refreshing Simulation Data

Use the `/refresh_db` API to clear all data and regenerate the initial simulation dataset. This endpoint is designed for simulation purposes and will reset the database to its default mock state.

---

For more details, see [build.md](../build.md) and [NOTICE.md](../NOTICE.md).
