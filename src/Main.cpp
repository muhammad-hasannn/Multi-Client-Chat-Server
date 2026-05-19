#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <vector>

#define _POSIX_C_SOURCE 200112L
#define PORT "9034"

const char *inet_ntop2(void *addr, char *buf, size_t size){
    struct sockaddr_storage *sas = (struct sockaddr_storage *)addr;
    struct sockaddr_in *sa4;
    struct sockaddr_in6 *sa6;
    void *src;

    switch (sas->ss_family){
    case AF_INET:
        sa4 = (struct sockaddr_in *)addr;
        src = &(sa4->sin_addr);
        break;
    case AF_INET6:
        sa6 = (struct sockaddr_in6 *)addr;
        src = &(sa6->sin6_addr);
        break;
    default:
        return NULL;
    }

    return inet_ntop(sas->ss_family, src, buf, size);
}

int get_listener_socket(void){
    int listener; // Listening socket descriptor
    int yes = 1;  // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0){
        fprintf(stderr, "pollserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next){
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0){
            continue;
        }

        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0){
            close(listener);
            continue;
        }

        break;
    }

    if (p == NULL){
        return -1;
    }

    freeaddrinfo(ai); // freeing the linkde list returned by getaddrinfo().

    // Listen
    if (listen(listener, 10) == -1){
        return -1;
    }

    return listener;
}

void add_to_pfd(std::vector<pollfd> &pollfdStore, int newfd){
    struct pollfd pfd;
    pfd.fd = newfd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    pollfdStore.push_back(pfd);
}

void del_from_pfd(std::vector<pollfd> &pollfdStore, int i){
    pollfdStore[i] = pollfdStore.back();
    pollfdStore.pop_back();
}

void handle_new_connection(int listener, std::vector<pollfd> &pfdStore){
    struct sockaddr_storage remoteaddr; // Client address
    socklen_t addrlen;
    int newfd; // Newly accept()ed socket descriptor
    char remoteIP[INET6_ADDRSTRLEN];

    addrlen = sizeof remoteaddr;
    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

    if (newfd == -1){
        perror("error while accepting a new connection ");
    }
    else{
        add_to_pfd(pfdStore, newfd);

        printf("pollserver: new connection from %s on socket %d\n",
               inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP),
               newfd);
    }
}

void handle_client_data(int listener, std::vector<pollfd> &pfdStore, int *pfd_i){
    char buf[256]; // Buffer for client data

    int nbytes = recv(pfdStore[*pfd_i].fd, buf, sizeof buf, 0);
    int sender_fd = pfdStore[*pfd_i].fd;

    if (nbytes <= 0){ // Got error or connection closed by client
        
        if (nbytes == 0){
            // Connection closed
            printf("pollserver: socket %d hung up\n", sender_fd);
        }
        else{
            perror("recv");
        }

        close(pfdStore[*pfd_i].fd); // Bye!

        del_from_pfd(pfdStore, *pfd_i);

        // reexamine the slot we just deleted
        (*pfd_i)--;
    }
    else{ // We got some good data from a client
        printf("pollserver: recv from fd %d: %.*s", sender_fd, nbytes, buf);
        // Send to everyone!
        for (int j = 0; j < pfdStore.size(); j++){
            int dest_fd = pfdStore[j].fd;

            // Except the listener and ourselves
            if (dest_fd != listener && dest_fd != sender_fd){
                if (send(dest_fd, buf, nbytes, 0) == -1){
                    perror("send");
                }
            }
        }
    }
}

void process_connections(int listener, std::vector<pollfd> &pfdStore){
    for (int i = 0; i < pfdStore.size() ; i++)    {

        // Check if someone's ready to read
        if ((pfdStore)[i].revents & (POLLIN | POLLHUP)){
            // We got one!!

            if ((pfdStore)[i].fd == listener){
                // If we're the listener, it's a new connection
                handle_new_connection(listener, pfdStore);
            }
            else{
                // Otherwise we're just a regular client
                handle_client_data(listener, pfdStore, &i);
            }
        }
    }
}

int main(void){
    int listener; // Listening socket descriptor

    int fd_size = 5;
    // int fd_count = 0;

    std::vector<pollfd> pfdStore;
    pfdStore.reserve(500); // avoiding reallocations. assuming we won't have more than 500 clients at a time.      cuz we are passing the vector.data() to poll() which expects a pointer to an array of pollfd structs.

    // Set up and get a listening socket
    listener = get_listener_socket();

    if (listener == -1){
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }
    add_to_pfd(pfdStore, listener);

    // fd_count = 1; // For the listener

    puts("pollserver: waiting for connections...");

    // Main loop
    for ( ; ; ){
        int poll_count = poll(pfdStore.data(), pfdStore.size(), -1);

        if (poll_count == -1){
            perror("poll");
            exit(1);
        }

        // Run through connections looking for data to read
        process_connections(listener, pfdStore);
    }
}