/**
 * @file server.c
 *
 * @author Ulrike Schaefer 1327450
 *
 * 
 * @brief the server is a part of the coffeemaker
 *
 * @details the server waits for a request from the client (which specifies what kind of coffee the client wants). the server calculates if the coffee can be made and returns when the coffee will be finished if it can be made. otherwise it will send an error-code to the client.
 *
 * @date 01.04.2017
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <time.h>

#include "coffeemaker.h"


/**
 * @brief Name of the program
 */
static const char *progname = "server"; /* default name */

/**
 * @brief Default amount of liters in the coffee machine
 */
int liters = 1;

/**
 * @brief Default amount of cups that can be stored in a coffee machine
 */
int cups = 10;

/**
 * @brief File descriptor for server socket
 */
static int sockfd = -1;

/**
 * @brief File descriptor for connection socket 
 */
static int connfd = -1;

/**
 * @brief struct that represents a coffee and when it will be finished
 */
struct coffees {
    time_t finish_time;
    int coffee;
};
typedef struct coffees coffees;

/**
 * @brief terminate program on program error
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * @brief free allocated resources
 */
static void free_resources(void);

/**
 * @brief Signal handler
 * @param sig Signal number catched
 */
static void signal_handler(int sig);

/**
 * @brief Parse command line options
 * @param argc The argument counter
 * @param argv The argument vector
 * @param options Struct where parsed arguments are stored
 */
static void parse_args(int argc, char **argv);

/**
 * @brief Send all Data to the client
 * @param fd the socket to which to send to
 * @param buffer the data to be sent
 * @param n the size of the data to be sent
 */
static int send_all(int fd, uint8_t *buffer, size_t n);

/**
 * @brief Read data the client sent
 * @param fd the socket to read from
 * @param the buffer to which to save the data to
 * @param n the amount of bytes to read
 */
static uint8_t *receive_all(int fd, uint8_t *buffer, size_t n);


static void bail_out(int exitcode, const char *fmt, ...) {
    va_list ap;

    (void) fprintf(stderr, "%s: ", progname);
    if (fmt != NULL) {
        va_start(ap, fmt);
        (void) vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    if (errno != 0) {
        (void) fprintf(stderr, ": %s", strerror(errno));
    }
    (void) fprintf(stderr, "\n");

    free_resources();
    exit(exitcode);
}

static void free_resources(void) {
    if(connfd >= 0) {
        (void) close(connfd);
    }
    if(sockfd >= 0) {
        (void) close(sockfd);
    }
}

static void signal_handler(int sig) {
    printf("Freeing Resources. Shutting down server.\n");
    free_resources();
    exit(0);
}

static void parse_args(int argc, char **argv) {
    if(argc > 0) {
        progname = argv[0];
    }
    int opt;
    while ((opt = getopt(argc, argv, "p:l:c:")) != -1) {
        int pflag = 0;
        int lflag = 0;
        int cflag = 0;
        char *endptr;
        switch (opt) {
        case 'p':
            if (pflag) {
                bail_out(EXIT_FAILURE, "only one portnumber - usage: server [-p portno] [-l liters] [-c cups]");
            }
            portno = optarg;
            pflag = 1;
            break;
        case 'l':
            if (lflag) {
                bail_out(EXIT_FAILURE, "only input liters once - usage: server [-p portno] [-l liters] [-c cups]");
            }
            lflag = 1;
            errno = 0;
            liters = strtol(optarg, &endptr, 10);
            if ((errno == ERANGE && (liters == LONG_MAX || liters == LONG_MIN)) || (errno != 0 && liters == 0) || endptr == optarg) {
                bail_out(EXIT_FAILURE, "no valid int as liters");
            }
            if (liters < 1) {
                bail_out(EXIT_FAILURE, "there need to be more than 1 liter in the coffemachine in the start");
            }
            break;
        case 'c':
            if (cflag) {
                bail_out(EXIT_FAILURE, "only input cups once - usage: server [-p portno] [-l liters] [-c cups]");
            }
            cflag = 1;
            errno = 0;
            cups = strtol(optarg, &endptr, 10);
            if ((errno == ERANGE && (cups == LONG_MAX || cups == LONG_MIN)) || (errno != 0 && cups == 0)) {
                bail_out(EXIT_FAILURE, "no valid int as cups");
            }
            if (cups < 1) {
                bail_out(EXIT_FAILURE, "there need to be more than 1 cups in the coffemachine in the start");
            }
            break;
        default:
            bail_out(EXIT_FAILURE, "unknown input - usage: server [-p portno] [-l liters] [-c cups]");
        }
    }
}

static int send_all(int fd, uint8_t *buffer, size_t n) {
  size_t bytes_sent = 0;
  do {
    ssize_t s = send(fd, buffer + bytes_sent, n - bytes_sent, 0);
    if (s <= 0) {
      return -1;
    }
    bytes_sent += s;
  } while (bytes_sent < n);

  if (bytes_sent < n ) {
    return -1;
  }
  return 0;
}

static uint8_t *receive_all(int fd, uint8_t *buffer, size_t n) {
    size_t bytes_recv = 0;
    do {
        ssize_t r;
        r = recv(fd, buffer + bytes_recv, n - bytes_recv, 0);
        if (r <= 0) {
            return NULL;
        }
        bytes_recv += r;
    } while (bytes_recv < n);

    if (bytes_recv < n) {
        return NULL;
    }
    return buffer;
}

int main(int argc, char *argv[]) {

    /* setup signal handlers */
    const int signals[] = {SIGINT, SIGTERM};
    struct sigaction s;

    s.sa_handler = signal_handler;
    s.sa_flags   = 0;
    if(sigfillset(&s.sa_mask) < 0) {
        bail_out(EXIT_FAILURE, "sigfillset");
    }
    for(int i = 0; i < COUNT_OF(signals); i++) {
        if (sigaction(signals[i], &s, NULL) < 0) {
            bail_out(EXIT_FAILURE, "sigaction");
        }
    }

    parse_args(argc, argv);

    int ml = liters*1000;

    /* create socket sockfd */
    /* AF_INET for ipv4
       SOCK_STREAM for a sequenced, reliable, two-way, connection-based byte stream */
    int connfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        bail_out(EXIT_FAILURE, "could not create socket");
    }

    /* set socket option */
    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
        bail_out(EXIT_FAILURE, "could not set sockopt");
    }

    /* bind socket to localhost:1821 (some random free port I chose) */
    /* man 3 getaddrinfo shows an example of doing this by building a complete struct addrinfo */
    /* htons can be used to convert values between host and network byte order */
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    /* getaddrinfo takes the portno as second argument under the name service */
    if (getaddrinfo(NULL, portno, (struct addrinfo *) &hints, &result) != 0) {
        bail_out(EXIT_FAILURE, "could not get addrinfo");
    }

    /* getaddrinfo returns a list of addrinfos - in a loop try to bind to any of them */
    int bind_success = 0;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            bind_success = 1;
            break;
        }
    }

    freeaddrinfo(result);

    if (bind_success == 0) {
        bail_out(EXIT_FAILURE, "could not bind");
    }

    /* listen for incoming connections */
    /* this is non-blocking, it just sets an internal flag that this is a passive listening socket and enables that accept may be called on this socket */
    if (listen(sockfd, 10) != 0) {
        bail_out(EXIT_FAILURE, "setup listen failed");
    }

    /* timestamp which says when the last coffee will be finished */
    time_t last_finished_coffee = time(NULL);

    printf("Initial status : %dml water , %d cups bin\n", ml, cups);
    printf("Waiting for client...\n");

    while (1) {
        /* accept an incoming connection */
        /* second and third parameter are for the addrinfo from the incoming socket */
        /* will just reuse hints which I do not need any longer */
        if ((connfd = accept(sockfd, (struct sockaddr *) &hints, (socklen_t *) &hints)) == -1) {
            bail_out(EXIT_FAILURE, "accept failed");
        }

        printf("Client connected .\n");

        static uint8_t buffer[2];

        /* receive message of what the client wants */
        if (receive_all(connfd, buffer, 2) == NULL) {
            bail_out(EXIT_FAILURE, "could not receive data from client");
        }

        /* OK - 0 coffee can be made
           NOK - 1 coffee cannot be made 
           error : 
                0 - parity bit error at server
                1 - not enough water left for this amount of coffee
                2 - no space for cups left
                3 - no space for cups & not enough water */

        int ok = 0;
        int error = 0;

        /* check the parity bit */
        uint8_t parity_bit = 0;
        for (int i = 0; i < 9; i++) {
            int bit = buffer[1] >> i;
            bit = bit & 1;
            parity_bit = parity_bit ^ bit;
        }
        for (int i = 1; i < 9; i++) {
            int bit = buffer[0] >> i;
            bit = bit & 1;
            parity_bit = parity_bit ^ bit;
        }
        if ((buffer[0]&1) != parity_bit) {
            printf("parity bit does not match\n");
            ok = 1;
            error = 0;
        }

        int seconds = 0;

        if (ok == 0) {
            /* get size & flavor */
            uint16_t total = buffer[1];
            total = total << 8;
            total = total | buffer[0];
            int size = total >> 1;
            size = size & 511;
            int flavor = total >> 10;
            char *coffename = coffeeNames[flavor];

            /* check if enough water & bin space is left for the coffee */
            if ((ml - size) < 0 && (cups - 1) < 0) {
                ok = 1;
                error = 3;
            } else if ((ml - size) < 0) {
                ok = 1;
                error = 1;
            } else if ((cups - 1) < 0) {
                ok = 1;
                error = 2;
            } else {
                /* update status of coffemaker */
                cups --;
                ml = ml - size;
                printf("New status: %dml water, %d cups bin\n", ml, cups);
                /* calculate how long the coffee will take */
                int leftover = 0;
                time_t current_time = time(NULL);
                if ((long) current_time < (long) last_finished_coffee) {
                    leftover = (long) last_finished_coffee - (long) current_time;
                }
                seconds = size;
                if (seconds%10 != 0) {
                     seconds = seconds + (10-(size%10));
                }
                seconds = seconds/10;
                seconds = seconds + leftover;
                last_finished_coffee = current_time + seconds;
                printf("Finish in %ds.\n", seconds);
                printf("Start coffee of %dml cup with flavour '%s'\n", size, coffename);
            }
        }

        /* message to send: 2 bytes 
        use an unsigned integer
        if ok: ------|0|-  time to | ok | wait| parity bit
        if not ok: --|1|- error code | nok | parity bit */

        uint8_t mess;

        if (ok == 0) {
            mess = seconds;
            if (seconds > 63) {
                mess = 63;
            }
            mess = mess << 2;
            uint8_t parity_bit = 0;
            for (int i = 0; i < 8; i++) {
                int bit = mess >> i;
                bit = bit & 1;
                parity_bit = parity_bit ^ bit;
            }
            mess = mess | parity_bit;
        } else {
            mess = error;
            mess = mess << 1;
            mess = mess | 1;
            mess = mess << 1;
            uint8_t parity_bit = 0;
            for (int i = 0; i < 8; i++) {
                int bit = mess >> i;
                bit = bit & 1;
                parity_bit = parity_bit ^ bit;
            }
            mess = mess | parity_bit;
        }

        /* send message to client - it says if the coffee is going to be made and if so when, if not why not */
        if (send_all(connfd, &mess, 1) == -1) {
            bail_out(EXIT_FAILURE, "sending the information to the client did not work");
        }
        printf("Close connection to client.\n");
        if(connfd >= 0) {
            (void) close(connfd);
        }
        printf("Waiting for client...\n");
    }
}