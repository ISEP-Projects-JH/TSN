# TSN: Social Network Backend with Bulgogi Framework

TSN is a backend service for a social network platform, built with C++ and the [Bulgogi](https://github.com/boost-experimental/bulgogi) web framework. It features user modeling, friend recommendation (A* algorithm), interest and profile matching, and provides a RESTful API for user and simulation management.

## Features
- High-performance HTTP server using Boost.Beast and Bulgogi
- User modeling with interests, profiles, and friend lists
- Friend and stranger recommendation based on A* search and cost functions
- MySQL database integration for persistent storage
- RESTful API for user management, simulation, and data refresh
- Docker support for easy deployment

## Directory Structure

- `main.cpp` — Server entry point
- `Web/` — HTTP routing, API handlers, and web utilities
- `Application/` — Core business logic (user model, friend recommendation, DB handlers)
- `Entities/` — User data structures and algorithms
- `Utils/` — Static data, user profile generators
- `docker/` — Dockerfiles for build/runtime
- `docs/` — API documentation, quick start, build instructions

## Quick Start
See [docs/quick_start.md](docs/quick_start.md) for setup and running instructions.

## API Documentation
See [docs/api.md](docs/api.md) for detailed API endpoints and usage.

## Build & Deployment
See [build.md](build.md) for build and deployment instructions, including Docker usage.

## License
See [NOTICE.md](NOTICE.md) for third-party licenses.

## About Bulgogi
This project uses the [Bulgogi](https://github.com/boost-experimental/bulgogi) C++ web framework for high-performance HTTP services.
