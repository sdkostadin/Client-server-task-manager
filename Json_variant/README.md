# Client-Server Task Manager (JSON Variant)

This project is a client-server task manager written in C++.
The communication between the client and the server is done over TCP sockets, and all requests and responses are exchanged in JSON format.

The idea of the project is to simulate a small multi-user system where users can register, log in, manage tasks, and execute them. There is also an admin role with additional control over the system, such as assigning tasks, viewing active sessions, kicking users, and shutting down the server.

---

## Tech Stack

The project is built using:

* **C++** – main programming language
* **POSIX sockets** – TCP communication between client and server
* **nlohmann/json** – JSON parsing and serialization
* **OpenSSL (SHA-256)** – password hashing
* **std::thread / mutex / atomic** – multi-threading and synchronization
* **Makefile** – build system
* **JSON files** – simple persistence (`users.json`, `tasks.json`)

---

## What the project does

Once the server is running, multiple clients can connect to it.
Each client can register a new account or log into an existing one. After login, the user can see the tasks available to them, activate tasks, remove active tasks, and execute them.

The server keeps track of active sessions and stores user and task information in JSON files, so the data is preserved between runs.

There are two roles in the system:

* **user** – can manage and execute their own tasks
* **admin** – has access to additional commands for monitoring and controlling the system

---

## Main features

* registration and login system
* password hashing with SHA-256
* role-based access control
* task management with available and active tasks
* task execution through predefined handlers
* session tracking for connected users
* login rate limiting
* graceful server shutdown
* JSON-based communication protocol
* file-based persistence using `users.json` and `tasks.json`

---

## Project structure

```
.
├── include/
│   ├── auth/
│   ├── common/
│   ├── security/
│   ├── server/
│   ├── session/
│   └── tasks/
├── src/
│   ├── auth/
│   ├── client/
│   ├── common/
│   ├── security/
│   ├── server/
│   ├── session/
│   └── tasks/
├── config/
│   └── config.json
├── data/
│   ├── users.json
│   └── tasks.json
├── logs/
├── Makefile
└── README.md
```

---

## How it works

The server listens for incoming TCP connections and creates a separate thread for each connected client.
Each client sends JSON requests such as login, register, add task, remove task, execute task, and so on.

The request is processed by the server, passed to the correct module, and a JSON response is returned back to the client.

The system is split into several modules:

* **server** – accepts connections and manages the server lifecycle
* **request handler** – processes client requests and routes them to the correct functionality
* **auth** – handles registration, login, password validation, and admin creation
* **tasks** – manages user tasks and task state
* **session registry** – tracks currently active sessions
* **rate limiter** – limits repeated failed login attempts
* **common utilities** – config loading, logging, JSON helpers, and protocol validation

---

## Build and run

The project is built with the provided `Makefile`.

### Build everything

```
make
```

### Run the server

```
make run-server
```

or manually:

```
./bin/server
```

### Run the client

```
make run-client
```

or manually:

```
./bin/client
```

### Clean build files

```
make clean
```

### Rebuild from scratch

```
make rebuild
```