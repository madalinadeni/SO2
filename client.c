#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BOARD_SIZE 8

#define WHITE_BACKGROUND "\033[48;5;15m"
#define BLACK_BACKGROUND "\033[48;5;233m"
#define RESET_COLOR "\033[0m"

#define WHITE_TEXT "\033[38;5;15m"
#define BLACK_TEXT "\033[38;5;0m"

void afisare_tabla(char tabla[BOARD_SIZE][BOARD_SIZE]) {
    printf("   A B C D E F G H\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%d  ", 8 - i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            if ((i + j) % 2 == 0) {
                printf(WHITE_BACKGROUND BLACK_TEXT);
            } else {
                printf(BLACK_BACKGROUND WHITE_TEXT);
            }
            if (tabla[i][j] == '.') {
                printf("  ");
            } else {
                printf("%c ", tabla[i][j]);
            }
            printf(RESET_COLOR);
        }
        printf(" %d", 8 - i);
        printf("\n");
    }
    printf("   A B C D E F G H\n");
}

int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Eroare la crearea socket-ului!");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(client_fd);
        return 1;
    }

    printf("Connected to server\n");

    char tabla[BOARD_SIZE][BOARD_SIZE];
    while (1) {
        int bytes_received = recv(client_fd, tabla, sizeof(char) * BOARD_SIZE * BOARD_SIZE, 0);
        if (bytes_received <= 0) {
            perror("Eroare la primirea tablei!");
            break;
        }

        afisare_tabla(tabla);

        printf("Enter your move (e.g., e2 e4): ");
        char move[10];
        fgets(move, sizeof(move), stdin);
        move[strcspn(move, "\n")] = 0; 

        printf("Move read: '%s' (length: %lu)\n", move, strlen(move));

        if (strlen(move) != 5 || move[2] != ' ') {
            printf("Invalid move format.\n");
            continue;
        }

        int bytes_sent = send(client_fd, move, 5, 0); 
        if (bytes_sent <= 0) {
            perror("Eroare la trimiterea mutarii!");
            break;
        }
    }

    close(client_fd);
    return 0;
}
