# Structure du Dépôt Git

```
webserv/

├── .gitignore
├── Makefile
├── README.md
│
├── config/                    # Fichiers de configuration
│   ├── default.conf
│   └── ...
│
├── includes/                  # Headers (.hpp)
│   ├── Config.hpp
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── Request.hpp
│   ├── Response.hpp
│   └── ...
│
├── srcs/                      # Sources (.cpp)
│   ├── main.cpp
│   ├── Config/
│   │   ├── Config.cpp
│   │   └── ...
│   ├── Server/
│   │   ├── Server.cpp
│   │   └── ...
│   ├── Client/
│   │   └── Client.cpp
│   ├── HTTP/
│   │   ├── Request.cpp
│   │   ├── Response.cpp
│   │   └── Router.cpp
│   ├── Handlers/
│   │   ├── FileHandler.cpp
│   │   ├── CGIHandler.cpp
│   │   └── ...
│   └── Utils/
│       ├── utils.cpp
│       └── ...
│
├── www/                       # Fichiers web à servir
│   ├── index.html
│   └── error/
│       ├── 404.html
│       └── 500.html
│
├── cgi-bin/                   # Scripts CGI
│   ├── test.php
│   ├── test.py
│   └── ...
│
└──  tests/                     # Tests
    └── test_server.py

```