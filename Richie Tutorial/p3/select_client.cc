/**
 * select_client.cc
 *
 * Select_client is half of a client/server pair that demonstrates how a server
 * can use select() so that a single-threaded server can manage multiple client
 * communications.  The client connects to the server, sends a message, waits,
 * and sends another message.  By waiting, there is an opportunity for another
 * client to connect in the meantime.
 *
 * After starting the server, run multiple copies of this program from a loop:
 *
 *   bash:> for i in `seq 12`; do ./obj64/select_client.exe -p [server port] -s
 *            [server name] -w $i & done
 *
 *   (Note: all of the above, from 'for' to 'done', should be on a single line
 *    in the terminal.)
 *
 * If you look at the output of the server, you'll see interesting interleavings
 * among the clients' messages.
 */

#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <netdb.h>
#include <string>
#include <unistd.h>

/**
 * Display a help message to explain how the command-line parameters for this
 * program work
 *
 * @progname The name of the program
 */
void usage(char *progname) {
  printf("%s: Client half of a client/server program to demonstrate the use of "
         "select().\n",
         basename(progname));
  printf("  -s [string] Name of the server (probably 'localhost')\n");
  printf("  -p [int]    Port number of the server\n");
  printf("  -w [int]    Time to wait between messages\n");
  printf("  -h          Print help (this message)\n");
}

/** arg_t is used to store the command-line arguments of the program */
struct arg_t {
  /** The name of the server to which the parent program will connect */
  std::string server_name = "";

  /** The port on which the program will connect to the above server */
  size_t port = 0;

  /** The time to wait between sending the first and second message */
  int wait = 0;

  /** Is the user requesting a usage message? */
  bool usage = false;
};

/**
 * Parse the command-line arguments, and use them to populate the provided args
 * object.
 *
 * @param argc The number of command-line arguments passed to the program
 * @param argv The list of command-line arguments
 * @param args The struct into which the parsed args should go
 */
void parse_args(int argc, char **argv, arg_t &args) {
  long opt;
  while ((opt = getopt(argc, argv, "p:s:w:h")) != -1) {
    switch (opt) {
    case 's':
      args.server_name = std::string(optarg);
      break;
    case 'p':
      args.port = atoi(optarg);
      break;
    case 'w':
      args.wait = atoi(optarg);
      break;
    case 'h':
      args.usage = true;
      break;
    }
  }
}

/**
 * Print an error message that combines some provided text (prefix) with the
 * standard unix error message that accompanies errno, and then exit the
 * program.  This routine makes it easier to see the logic in our program while
 * still correctly handling errors.
 *
 * @param code   The exit code to return from the program
 * @param err    The error code that was generated by the program
 * @param prefix The text to display before the error message
 */
void error_message_and_exit(std::size_t code, std::size_t err,
                            const char *prefix) {
  char buf[1024];
  printf("%s %s\n", prefix, strerror_r(err, buf, sizeof(buf)));
  exit(code);
}

/**
 * Connect to a server so that we can have bidirectional communication on the
 * socket (represented by a file descriptor) that this function returns
 *
 * @param hostname The name of the server (ip or DNS) to connect to
 * @param port     The server's port that we should use
 */
int connect_to_server(std::string hostname, std::size_t port) {
  // figure out the IP address that we need to use and put it in a sockaddr_in
  struct hostent *host = gethostbyname(hostname.c_str());
  if (host == nullptr) {
    fprintf(stderr, "connect_to_server():DNS error %s\n", hstrerror(h_errno));
    exit(0);
  }
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr =
      inet_addr(inet_ntoa(*(struct in_addr *)*host->h_addr_list));
  addr.sin_port = htons(port);
  // create the socket and try to connect to it
  int sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0) {
    error_message_and_exit(0, errno, "Error making client socket: ");
  }
  if (connect(sd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    close(sd);
    error_message_and_exit(0, errno, "Error connecting socket to address: ");
  }
  return sd;
}

/**
 * Send a message over a socket
 *
 * @param sd  The file descriptor of the socket on which to write
 * @param msg The message to send
 */
void write_to_server(int sd, const char *msg) {
  // By now, the process of writing over a socket should be familiar :)
  const char *next_byte = msg;
  size_t remain = strlen(msg);
  while (remain) {
    size_t sent = write(sd, next_byte, remain);
    if (sent <= 0) {
      if (errno != EINTR) {
        error_message_and_exit(0, errno, "Error in write(): ");
      }
    } else {
      next_byte += sent;
      remain -= sent;
    }
  }
}

int main(int argc, char **argv) {
  // parse the command line arguments
  arg_t args;
  parse_args(argc, argv, args);
  if (args.usage) {
    usage(argv[0]);
    exit(0);
  }

  // We will use the program's process id to uniquely identify it, so that the
  // output of the program shows that lots of clients can run at once
  printf("Starting client %d\n", getpid());

  // Set up the client socket for communicating.  This will exit the program on
  // any error.
  int sd = connect_to_server(args.server_name, args.port);

  // Get a random number between 1 and 8, based on the wait time
  unsigned seed = args.wait;
  int s = 1 + (rand_r(&seed) % 8);

  // Send two messages, with enough time between them that the server should be
  // able to see other clients also sending messages
  write_to_server(sd, "Hello");
  sleep(s);
  // NB: second message is longer... watch what happens on the server, where the
  //     buffer size is just 16...
  write_to_server(sd, "Thanks for all the good times.  Farewell.");
  close(sd);

  printf("Closing client %d\n", getpid());
  return 0;
}
