// PS IS1 321 LAB09
// Sebastian Ratańczuk
// sebastian-ratanczuk@zut.edu.pl

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MaxClients 1000
#define BYTES 8192
#define MaxSize 99999

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

char NotFoundResponse[] = "HTTP/1.1 404 Not Found\r\n\r\n";
char ForbiddenResponse[] = "HTTP/1.1 501 Not Implemented\r\n\r\n";

char *SERV_ROOT;
char *SERV_PORT;
char SERV_LOG[MaxSize];

int sock;
int clients[MaxClients];
pthread_t client_threads[MaxClients];
struct sockaddr_in cli_addr;
socklen_t sin_len = sizeof(cli_addr);
sem_t mutex;

typedef struct ThreadData ThreadData;
struct ThreadData {
    int slot;
    char ip[MaxSize];
};

typedef struct MessageData MessageData;
struct MessageData {
    pthread_t pid;
    const char *ip;
    const char *status;
    char *request;
    char *message;
};

void stop(char *programName);

void start(char *programName);

void initializeServer();

void *respond(void *rawData);

void sendMime(int slot, char *request);

char *getMime(const char *dup);

void writeLog(char *message);

char *simpleMessage(char *message);

char *messageBuilder(MessageData data);

void testRootFolder();

int CheckGet(char *const *request);

int CheckHttp(char *const *request);

void Send404(int slot, const char *ip, char *const *request);

void Send501(int slot, const char *ip, char *const *request);

void Log(const char *ip, const char *status, char *const *request);

int main(int argc, char **argv) {
    SERV_ROOT = "/mnt/c/Users/sebol/CLionProjects/ps/httptest";
    SERV_PORT = "8080";

    int opt;
    int startFlag = 0;
    int stopFlag = 0;

    while ((opt = getopt(argc, argv, "sqp:d:")) != -1) {
        switch (opt) {
            case 'p':
                SERV_PORT = optarg;
                break;
            case 'd':
                SERV_ROOT = optarg;
                break;
            case 's':
                startFlag = 1;
                break;
            case 'q':
                stopFlag = 1;
                break;
            default:
                break;
        }
    }

    if (startFlag)
        start(argv[0]);

    if (stopFlag)
        stop(argv[0]);
}

void start(char *programName) {
    strcpy(SERV_LOG, programName);
    strcpy(&SERV_LOG[strlen(programName)], ".log");
    sem_init(&mutex, 0, 1);

    testRootFolder();
    initializeServer();

#ifndef DEBUG
    daemon(-1, 0);
#endif

    int slot = 0;
    while (1) {
        clients[slot] = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);

        if (clients[slot] == -1) {
            printf("%s\n", strerror(errno));
            writeLog(simpleMessage(strerror(errno)));
            continue;
        }

        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cli_addr.sin_addr), str, INET_ADDRSTRLEN);

        ThreadData *threadData = malloc(sizeof(struct ThreadData));
        threadData->slot = slot;
        strcpy(threadData->ip, str);

        pthread_create(&client_threads[slot], NULL, respond, threadData);

        while (clients[slot] != -1)
            slot = (slot + 1) % MaxClients;
    }
}

void testRootFolder() {
    int fd = open(SERV_ROOT, O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        printf("%s\n", strerror(errno));
        writeLog(simpleMessage(strerror(errno)));
        exit(1);
    }
    close(fd);
}

void initializeServer() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("%s\n", strerror(errno));
        writeLog(simpleMessage(strerror(errno)));
        exit(1);
    }
    struct addrinfo *res;

    if (getaddrinfo("0.0.0.0", SERV_PORT, NULL, &res) != 0) {
        printf("%s\n", strerror(errno));
        writeLog(simpleMessage(strerror(errno)));
        exit(1);
    }

    if (bind(sock, res->ai_addr, res->ai_addrlen) != 0) {
        close(sock);
        printf("%s\n", strerror(errno));
        writeLog(simpleMessage(strerror(errno)));
        exit(1);
    }

    listen(sock, MaxSize);

    for (int i = 0; i < MaxClients; i++)
        clients[i] = -1;
}

void *respond(void *rawData) {
    ThreadData data = *(ThreadData *) rawData;

    int slot = data.slot;
    char *ip = data.ip;
    char msg[MaxSize], *request[3], data_to_send[BYTES], path[MaxSize];

    memset((void *) msg, (int) '\0', MaxSize);
    long err = recv(clients[slot], msg, MaxSize, 0);

    if (err <= 0) {
        printf("%s\n", strerror(errno));
        writeLog(simpleMessage(strerror(errno)));
        exit(1);
    }

    request[0] = strtok(msg, " \t\n");
    request[1] = strtok(NULL, " \t");
    request[2] = strtok(NULL, " \t\n");

    if (CheckGet(request) || CheckHttp(request)) {
        Send501(slot, ip, request);

        shutdown(clients[slot], SHUT_RDWR);
        close(clients[slot]);
        clients[slot] = -1;
        pthread_exit(0);
    }

    if (strcmp(request[1], "/\0") == 0)
        request[1] = "/index.html";

    strcpy(path, SERV_ROOT);
    strcpy(&path[strlen(SERV_ROOT)], request[1]);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        Send404(slot, ip, request);

        shutdown(clients[slot], SHUT_RDWR);
        close(clients[slot]);
        clients[slot] = -1;
        pthread_exit(0);
    }

    Log(ip, "200", request);
    send(clients[slot], "HTTP/1.1 200 OK\r\n", 17, 0);
    sendMime(slot, request[1]);

    long bytes_read;
    while ((bytes_read = read(fd, data_to_send, BYTES)) > 0) {
        write(clients[slot], data_to_send, bytes_read);
    }

    shutdown(clients[slot], SHUT_RDWR);
    close(clients[slot]);
    clients[slot] = -1;

    pthread_exit(0);
}

char *messageBuilder(MessageData messageData) {
    time_t *t = malloc(sizeof(time_t));
    time(t);
    char *newMessage = malloc(sizeof(char) * MaxSize);
    char pid[999];
    snprintf(pid, 999, "%lu", messageData.pid);

    strcpy(newMessage, strtok(ctime(t), "\n"));
    strcpy(&newMessage[strlen(newMessage)], "\t");
    strcpy(&newMessage[strlen(newMessage)], pid);
    strcpy(&newMessage[strlen(newMessage)], "\t");
    strcpy(&newMessage[strlen(newMessage)], messageData.ip);
    strcpy(&newMessage[strlen(newMessage)], "\t");
    strcpy(&newMessage[strlen(newMessage)], messageData.status);
    strcpy(&newMessage[strlen(newMessage)], "\t");
    strcpy(&newMessage[strlen(newMessage)], messageData.request);
    strcpy(&newMessage[strlen(newMessage)], "\t");
    strcpy(&newMessage[strlen(newMessage)], messageData.message);
    strcpy(&newMessage[strlen(newMessage)], "\n");

    free(t);
    return newMessage;
}

char *simpleMessage(char *message) {
    time_t *t = malloc(sizeof(time_t));
    time(t);
    char *newMessage = malloc(sizeof(char) * MaxSize);

    strcpy(newMessage, strtok(ctime(t), "\n"));
    strcpy(&newMessage[strlen(newMessage)], "\t");
    strcpy(&newMessage[strlen(newMessage)], message);
    strcpy(&newMessage[strlen(newMessage)], "\n");
    return newMessage;
}

void writeLog(char *message) {
    sem_wait(&mutex);
    int fd = open(SERV_LOG, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    write(fd, message, strlen(message));
    close(fd);
    sem_post(&mutex);
    free(message);
}

void Send501(int slot, const char *ip, char *const *request) {
    Log(ip, "501", request);
    send(clients[slot], ForbiddenResponse, strlen(ForbiddenResponse), 0);
}

void Send404(int slot, const char *ip, char *const *request) {
    Log(ip, "404", request);
    send(clients[slot], NotFoundResponse, strlen(NotFoundResponse), 0);
}

int CheckHttp(char *const *request) {
    return strcmp(request[2], "HTTP/1.1\r") != 0;
}

int CheckGet(char *const *request) {
    return strcmp(request[0], "GET\0") != 0;
}

void Log(const char *ip, const char *status, char *const *request) {
    writeLog(messageBuilder((MessageData) {
            .pid=pthread_self(),
            .ip=ip,
            .status=status,
            .request=request[0],
            .message=request[1]}));
}

void sendMime(int slot, char *request) {
    char *dup = malloc(MaxSize);
    char *response;

    strcpy(dup, request);
    strtok(dup, ".");
    dup = strtok(NULL, ".");
    response = getMime(dup);

    write(clients[slot], response, strlen(response));
}

char *getMime(const char *dup) {
    if (strcmp(dup, "html") == 0) {
        return "Content-Type: text/html; charset=UTF-8\r\n\r\n";
    } else if (strcmp(dup, "jpg") == 0) {
        return "Content-Type: image/jpeg\r\n\r\n";
    } else if (strcmp(dup, "png") == 0) {
        return "Content-Type: image/png\r\n\r\n";
    } else if (strcmp(dup, "gif") == 0) {
        return "Content-Type: image/gif\r\n\r\n";
    } else {
        return "\r\n";
    }
}

void stop(char *programName) {
    char command[MaxSize];
    strcpy(command, "pkill -o -f ");
    strcpy(&command[strlen(command)], programName);
    system(command);
}

#pragma clang diagnostic pop