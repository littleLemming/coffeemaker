# coffeemaker

The purpose of this program was to understand how sockets and bitwise operations work.

## Creating a socket:

```
int sockfd;
sockfd = socket(AF_INET, SOCK_STREAM, 0);
```

The socket will from this moment on be refered to by `sockfd`.
In this case with `AF_INET` a ipv4 socket and with `SOCK_STREAM` a tcp socket will be created.
If `connfd` after creating the socket turn out to be smaller than 0 an error occured while creating the socket.

```
int optval = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval);
```

This sets a variable for the operating system. If the program should crash the operating system will keep the used socket reserved for some time.

```
struct addrinfo hints;
memset(&hints, 0, sizeof(struct addrinfo));
hints.ai_family = AF_INET;
hints.ai_socktype = SOCK_STREAM;
hints.ai_protocol = 0;
```
This reserves an addressinfo in the memory and sets it to being ipv4 and an TCP socket. 
Afterwards getaddrinfo can be used to get a list of addressinfos. 

```
struct addrinfo *result;
getaddrinfo(NULL, portno, (struct addrinfo *) &hints, &result)
```

`portno` is the port number as a `char *`. Usually it is a good idea to check if this is actually a valid portnumber by parsing and checking its size. `getaddrinfo` fails if the result is not 0.

## Binding to an addressinfo:

This is only relevant for servers.

```
struct addrinfo *rp;
for (rp = result; rp != NULL; rp = rp->ai_next) {
  if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
    break;
  }
}
```

This loop tries to connect to any of the generated addressinfos. As soon as it connected to any of them it stops the loop. It is a good idea to additionally check if after it looped through all of the addressinfos if it actually connected to any of them which can be done in a few different ways.

Afterwards it is possible to `listen` for incoming connections.

## Listening at a specific port:

This again is relevant for servers. It can be executed after `bind`. When listening to a ceratin port the server passively listens to imcoming connections. The server listens to the port specified in the addressinfo.

```
listen(sockfd, 10)
```

Failure is indicatend by `listen` by a return value not equal to 0.
The number (in this case 10) indicates how large the queue of outstanding connections can be.

## Accepting an incoming connection:

After the server has executed `listen` it can `accept` connections. For instance this can be done in an loop to accept multiple client connections. This will work like a first-come-first-serve queue. The server will not be able to communicate with multiple clients at the same time.

```
int connfd;
connfd = accept(sockfd, (struct sockaddr *) &hints, (socklen_t *) &hints)
```

`connfd` represents the client-socket. If accept returns -1 an error has occured.

## Connecting to a socket:

This is relevant for clients - this operation tries to connect the socket to another socket.

```
struct addrinfo *rp;
for (rp = result; rp != NULL; rp = rp->ai_next) {
  if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
    break;
  }
}
```

This works basically like `bind`. Again checking if the connect actually worked after the loop is a good idea.
After this operation a connection between two sockets has been set up. They now can `send` and `recv` (reveive) messages from each other.

## Freeing an addressinfo

As C does not have a garbage collector one should clean up all no longer used addressinfos. This can be done by:

```
freeaddrinfo(addrinfo);
```

`addrinfo` here stands for the specific name of the addressinfo.

## Send and Receive

As soon as a connection has been established `send` and `recv` can be called.

```
recv(sockfd, buffer_to_write_to,length_of_buffer, 0);
```

This returns the length of the received message. As it is not clear how much that will be `recv` should be ideally called in a loop until the whole message has been received.

```
recv(sockfd, buffer_to_send_from,length_of_buffer, 0);
```

`send` works basically the same as `recv` only that the buffer is not being written to but whatever is written there gets sent.
