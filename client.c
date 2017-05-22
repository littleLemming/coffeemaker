/**
 * @file client.c
 *
 * @author Ulrike Schaefer 1327450
 *
 * 
 * @brief the client is a part of the coffeemaker
 *
 * @details the client connects to the server and request a flavor and amount of coffee. then the client waits for a response from the server which contains if the coffee can be made and how long it will take. If the coffee can't be produced the client receives an error-code.
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

#include "coffeemaker.h"


/**
 * @brief Name of the program
 */
static const char *progname = "client"; /* default name */

/**
 * @brief File descriptor for client socket
 */
static int sockfd = -1;

/**
 * @brief Default Hostname to connect to
 */
char *hostname = "localhost";

/**
 * @brief coffee flavor - default is -1 which means not set
 */
int flavor = -1;

/**
 * @brief the string of the coffee name
 */
char* flavor_str;

/**
 * @brief siz of cup - default is -1 which means not set
 */
int size = -1;


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
 * @brief Parse command line options
 * @param argc The argument counter
 * @param argv The argument vector
 * @param options Struct where parsed arguments are stored
 */
static void parse_args(int argc, char **argv);

/**
 * @brief Send all Data to the server
 * @param fd the socket to which to send to
 * @param buffer the data to be sent
 * @param n the size of the data to be sent
 */
static int send_all(int fd, uint8_t *buffer, size_t n);

/**
 * @brief Read data the server sent
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
    if(sockfd >= 0) {
        (void) close(sockfd);
    }
}

static void parse_args(int argc, char **argv) {
    if(argc > 0) {
        progname = argv[0];
    }
    int opt;
    while ((opt = getopt(argc, argv, "p:h:")) != -1) {
        int pflag = 0;
        int hflag = 0;
        switch (opt) {
        case 'p':
            if (pflag) {
                bail_out(EXIT_FAILURE, "only one portnumber - usage: client [-h hostname] [-p portno] size flavor");
            }
            portno = optarg;
            pflag = 1;
            break;
        case 'h':
            if (hflag) {
                bail_out(EXIT_FAILURE, "only one hostnumber - usage: client [-h hostname] [-p portno] size flavor");
            }
            portno = optarg;
            hflag = 1;
            break;
        default:
            bail_out(EXIT_FAILURE, "unknown input - usage: client [-h hostname] [-p portno] size flavor");
        }
    }
    if (optind != argc-2) {
        bail_out(EXIT_FAILURE, "enter size and flavor- usage: client [-h hostname] [-p portno] size flavor");
    }
    char* size_str = argv[optind];
    char *endptr;
    errno = 0;
    size = strtol(size_str, &endptr, 10);
    if ((errno == ERANGE && (size == LONG_MAX || size == LONG_MIN)) || (errno != 0 && size == 0) || endptr == optarg) {
        bail_out(EXIT_FAILURE, "no valid int as size");
    }
    if (size < 0 || size > 330) {
        bail_out(EXIT_FAILURE, "no valid size - must be between 0 and 330 (inclusive)");
    }
    flavor_str = argv[optind+1];
    if (strcmp(flavor_str, "Kazaar") == 0) {
        flavor = Kazaar;
    } else if (strcmp(flavor_str, "Dharkan") == 0) {
        flavor = Dharkan;
    } else if (strcmp(flavor_str, "Roma") == 0) {
        flavor = Roma;
    } else if (strcmp(flavor_str, "Livanto") == 0) {
        flavor = Livanto;
    } else if (strcmp(flavor_str, "Volluto") == 0) {
        flavor = Volluto;
    } else if (strcmp(flavor_str, "Cosi") == 0) {
        flavor = Cosi;
    } else if (strcmp(flavor_str, "Cappricio") == 0) {
        flavor = Cappricio;
    } else if (strcmp(flavor_str, "Appregio") == 0) {
        flavor = Appregio;
    } else if (strcmp(flavor_str, "Caramelito") == 0) {
        flavor = Caramelito;
    } else if (strcmp(flavor_str, "Vanilio") == 0) {
        flavor = Vanilio;
    } else if (strcmp(flavor_str, "Ciocattino") == 0) {
        flavor = Ciocattino;
    } else {
        bail_out(EXIT_FAILURE, "no known flavor - usage: client [-h hostname] [-p portno] size flavor");
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

    parse_args(argc, argv);

    /* create socket sockfd */
    /* AF_INET for ipv4
       SOCK_STREAM for a sequenced, reliable, two-way, connection-based byte stream */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        bail_out(EXIT_FAILURE, "could not create socket");
    }

    /* connect socket by default to localhost:1821 (some random free port I chose) */
    /* man 3 getaddrinfo shows an example of doing this by building a complete struct addrinfo */
    /* htons can be used to convert values between host and network byte order */
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    if (getaddrinfo(hostname, portno, (struct addrinfo *) &hints, &result) != 0) {
        bail_out(EXIT_FAILURE, "could not getaddrinfo");
    }

    /* getaddrinfo returns a list of addrinfos - in a loop try to connect to any of them */
    int connect_success = 0;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            connect_success = 1;
            break;
        }
    }

    freeaddrinfo(result);

    if (connect_success == 0 || rp == NULL) {
        bail_out(EXIT_FAILURE, "connect − Connection refused");
    }

    printf("Requesting a %dml cup of coffee of flavour '%s' (id=%d)\n", size, flavor_str, flavor);

    /* message to send: 2 bytes 
        use an unsigned integer
        -----|---------|- : 5 bits flavor, 9 bits size, 1 parity bit */

    uint16_t mess = flavor;
    mess = mess << 9;
    mess = mess | size;
    mess = mess << 1;
    uint16_t parity_bit = 0;
    for (int i = 0; i < 15; i++) {
        int bit = mess >> i;
        bit = bit & 1;
        parity_bit = parity_bit ^ bit;
    }
    mess = mess | parity_bit;

    uint8_t buff[2];
    buff[0] = mess;
    buff[1] = mess >> 8;

    /* send message with all needed information to the server */
    if (send_all(sockfd, buff, 2) == -1) {
        bail_out(EXIT_FAILURE, "sending the information to the server did not work");
    }

    uint8_t buffer[1];

    /* receive message of server with feedback */
    if (receive_all(sockfd, buffer, 1) == NULL) {
        bail_out(EXIT_FAILURE, "could not receive data from server");
    }

    /* check the parity bit */
    uint8_t parity_bit_check = 0;
    for (int i = 1; i < 9; i++) {
        int bit = buffer[0] >> i;
        bit = bit & 1;
        parity_bit_check = parity_bit_check ^ bit;
    }
    if ((buffer[0]&1) != parity_bit_check) {
        bail_out(EXIT_FAILURE, "parity bit does not match\n");
    }
    int ok = buffer[0];
    ok = ok >> 1;
    ok = ok & 1;
    if (ok == 0) {
        int seconds = buffer[0];
        seconds = seconds >> 2;
        seconds = seconds & 63;
        if (seconds < 63) {
            printf("Coffee ready in %ds.\n", seconds);
        } else {
            printf("Coffee ready in 63 seconds or more.\n");
        }
    } else {
        int error = buffer[0];
        error = error >> 2;
        error = error & 3;
        char* error_name;
        if (error == 0) {
            error_name = "server_parity_bit_error";
        } if (error == 1) {
            error_name = "no_water";
        } if (error == 2) {
            error_name = "full_bin";
        } if (error == 3) {
            error_name = "no_water_and_full_bin";
        }
        printf("Error %d - %s\n", error, error_name);
    }

    free_resources();
}