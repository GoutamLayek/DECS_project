# DECS Autograder version 4

## Project File Structure

```txt
project/
├── client
│   ├── bin
│   │   ├── async-submit
│   │   └── normal-submit
│   ├── include
│   │   └── socket-utils.h
│   ├── loadtest
│   │   ├── calculate.sh
│   │   ├── datagen.sh
│   │   ├── final.awk
│   │   ├── loadtest.sh
│   │   └── plot.sh
│   ├── Makefile
│   ├── obj
│   │   └── socket-utils.o
│   ├── src
│   │   ├── helper
│   │   │   └── socket-utils.c
│   │   ├── async-client.c
│   │   └── normal-client.c
│   └── tests
│       ├── compiler_error.c
│       ├── long_sleep.c
│       ├── output_error.c
│       ├── pass.c
│       └── runtime_error.c
├── install-dependencies.sh
├── README.md
└── server
    ├── autograder.db
    ├── bin
    │   └── server
    ├── include
    │   ├── database.h
    │   ├── grader-utils.h
    │   ├── persistent_queue.h
    │   ├── project_file_paths.h
    │   └── socket-utils.h
    ├── logs
    ├── Makefile
    ├── obj
    │   ├── database.o
    │   ├── grader-utils.o
    │   ├── persistent_queue.o
    │   ├── server.o
    │   └── socket-utils.o
    ├── public
    │   ├── <request-id>
    │   │   ├── results
    │   │   │   ├── compiler_error.txt
    │   │   │   ├── final_output.txt
    │   │   │   ├── output_diff.txt
    │   │   │   ├── program_output.txt
    │   │   │   └── runtime_error.txt
    │   │   └── submissions
    │   │       ├── program
    │   │       └── program.c
    │   └── expected_output.txt
    ├── src
    │   ├── database.c
    │   ├── grader-utils.c
    │   ├── persistent_queue.c
    │   ├── server.c
    │   └── socket-utils.c
    └── tests
        └── expected_output.txt
```

## Submission Structure

```txt
<request-id>
├── results
│   ├── compiler_error.txt
│   ├── final_output.txt
│   ├── output_diff.txt
│   ├── program_output.txt
│   └── runtime_error.txt
└── submissions
    ├── program
    └── program.c
```

## Install Dependencies

```sh
# On machine 1
cd client
./install-dependencies.sh

# On machine 2
cd server 
./install-dependencies.sh

```

## Build the program (Compile & Link)

```sh
# On machine 1
cd client
make

# On machine 2
cd server
make
```

## Run the server (v4)

```sh
cd ./server/bin
./server 9000 10
# ./server <port-no> <no-of-threads>
```

## Run the simple client (v4)

```sh
cd ./client/bin
./submit <new|status> <hostname> <portno> <filename>
```

## Run the load generating client (v4)

```sh
./load-generating-submit <new|status> <hostname> <portno> <filename|requestID> <loopnum> <sleep> <timeout>
```

## Run loadtest

```sh
./datagen.sh
```

## Contributors

`Goutam Layek` (23M0776)

`Chaitanya Shinge` (23M2116)
