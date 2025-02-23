#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>


#define BOARD_SIZE 8
#define PORT 8080
#define MAX_CLIENTS 100

// Literele scrise cu majuscule sunt piesele negre, iar celelalte sunt piesele albe.

/*
P/p -> pion
K/k -> rege
Q/q -> regina
T/t -> tura
N/n -> nebun
C/c -> cal
*/

// Pentru jucatorul cu alb avem caracterul a, iar pentru jucatorul cu negru avem caracterul n.



typedef struct {
    int client1_fd;
    int client2_fd;
    char tabla[BOARD_SIZE][BOARD_SIZE];
    int current_turn; // 0 pentru client1, 1 pentru client2
    int session_id;   // ID unic pentru sesiune
} GameSession;

void initializare_tabla(char tabla[BOARD_SIZE][BOARD_SIZE]) {
    char tabla_initiala[BOARD_SIZE][BOARD_SIZE] = {
        {'T', 'C', 'N', 'Q', 'K', 'N', 'C', 'T'},
        {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
        {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.'},
        {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
        {'t', 'c', 'n', 'q', 'k', 'n', 'c', 't'}
    };

    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            tabla[i][j] = tabla_initiala[i][j];
        }
    }
}



// Verificam daca mutarea este in limitele tablei
int is_within_bounds(int x, int y) {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}

/* Regulile de joc pentru pion:  
    1.Mutarea normala inainte:
        ->Pionii se pot muta un patrat inainte (pe aceeasi coloana), doar daca patratul este gol.
    2.Mutarea dubla inainte:
        ->La prima mutare, pionii pot avansa doua patrate inainte, dar doar daca:
            + Se afla in pozitia initiala (linie 6 pentru alb si linie 1 pentru negru).
            + Ambele patrate de pe traseu sunt libere.
    3.Capturarea diagonala:
        ->Pionii pot captura piesele adversarului mutandu-se pe o diagonala adiacenta (o linie inainte si o coloana lateral), doar daca acolo se afla o piesa adversa.
    4.En passant (capturarea in trecere):
        ->Aceasta este o regula speciala, care permite capturarea unui pion advers care a facut o mutare dubla si a trecut pe langa pionul curent.
    5.Promovarea:
        ->Daca pionul ajunge pe ultima linie a adversarului, acesta trebuie promovat intr-o piesa mai puternica (de obicei regina).
*/

int validare_mutare_pion(char tabla[BOARD_SIZE][BOARD_SIZE], int linie_start, int coloana_start, int linie_final, int coloana_final, char jucator) {

    int directie = (jucator == 'a') ? -1 : 1; // Pionul alb merge in sus, pionul negru merge in jos
    char pion = (jucator == 'a') ? 'p' : 'P';

    // Mutare normala inainte
    if (coloana_start == coloana_final) {
        if (tabla[linie_final][coloana_final] == '.' && linie_final == linie_start + directie) {
            printf("Mutare normala inainte validata\n");
            return 1;
        }

        // Mutare dubla inainte
        if ((linie_start == 6 && jucator == 'a') || (linie_start == 1 && jucator == 'n')) {
            if (tabla[linie_final][coloana_final] == '.' && tabla[linie_start + directie][coloana_start] == '.' && linie_final == linie_start + 2 * directie) {
                printf("Mutare dubla inainte validata\n");
                return 1;
            }
        }
    }

    // Capturare pe diagonala
    if (abs(coloana_final - coloana_start) == 1 && linie_final == linie_start + directie) {
        if (tabla[linie_final][coloana_final] != '.' && islower(tabla[linie_final][coloana_final]) != islower(pion)) {
            printf("Capturare pe diagonala validata\n");
            return 1;
        }
    }

    printf("Mutare pion invalida\n");
    return 0;
}



// Functie pentru promovarea pionului.
void verifica_promovare_pion(char tabla[BOARD_SIZE][BOARD_SIZE], int linie_final, int coloana_final, char jucator) {
    if ((jucator == 'a' && linie_final == 0) || (jucator == 'n' && linie_final == 7)) {
        printf("Alege piesa pentru promovare (Q, T, N, C): ");
        char promovare;
        scanf(" %c", &promovare);
        promovare = (jucator == 'a') ? toupper(promovare) : tolower(promovare);
        tabla[linie_final][coloana_final] = promovare;
    }
}

/*Regulile jocului pentru tura
    1.Directia de miscare:
        ->Tura se poate muta doar pe orizontal sau pe vertical.
        ->Nu poate sari peste alte piese.
    2.Capturarea pieselor:
        ->Tura poate captura o piesa adversa daca aceasta se afla pe patratul final, iar drumul este liber.
        ->Captura implica faptul ca piesa de pe patratul final apartine adversarului.
    3.Sah:
        ->Tura poate fi folosita pentru a da sah adversarului, daca drumul pana la rege este liber.
*/

int validare_mutare_tura(char tabla[BOARD_SIZE][BOARD_SIZE], int linie_start, int coloana_start, int linie_final, int coloana_final) {
    // Verificam daca mutarea este pe aceeasi linie sau pe aceeasi coloana.
    if (linie_start != linie_final && coloana_start != coloana_final){
        return 0;
    }

    //Calculam directia in care se muta tura.
    int pas_linie = (linie_final - linie_start) ? ((linie_final - linie_start) > 0 ? 1 : -1) : 0;
    int pas_coloana = (coloana_final - coloana_start) ? ((coloana_final - coloana_start) > 0 ? 1 : -1) : 0;

    int i = linie_start + pas_linie;
    int j = coloana_start + pas_coloana;

    while (i != linie_final || j != coloana_final) {
        if (tabla[i][j] != '.') return 0;
        i += pas_linie;
        j += pas_coloana;
    }
    return 1;
}

/*Regulile de joc pentru nebun:
    1.Directia de miscare:
        ->Nebunul se muta pe diagonala.
        ->Poate traversa mai multe patrate intr-o mutare, dar doar pe aceeasi diagonala.
        ->Nebunul nu poate sari peste alte piese.
        ->Daca exista o piesa pe traseu, mutarea este invalida.
    2.Captura pieselor:
        ->Nebunul poate captura o piesa adversa aflata pe patratul final.
*/

int validare_mutare_nebun(char tabla[BOARD_SIZE][BOARD_SIZE], int linie_start, int coloana_start, int linie_final, int coloana_final, char jucator) {
    // Verificam daca mutarea este  pe diagonala
    if (abs(linie_final - linie_start) != abs(coloana_final - coloana_start)) return 0;

    // Determinam directia mutarii
    int pas_linie = (linie_final > linie_start) ? 1 : -1;
    int pas_coloana = (coloana_final > coloana_start) ? 1 : -1;

    // Verificam daca traseul este liber
    int i = linie_start + pas_linie;
    int j = coloana_start + pas_coloana;
    while (i != linie_final && j != coloana_final) {
        if (tabla[i][j] != '.'){ 
            return 0;
        }
        i = i + pas_linie;
        j = j + pas_coloana;
    }

    // Verificam patratul final
    char piesa_finala = tabla[linie_final][coloana_final];
    if (piesa_finala == '.') {
        return 1;
    } else if ((jucator == 'a' && islower(piesa_finala)) || (jucator == 'n' && isupper(piesa_finala))) {
        return 0;
    } else {
        return 1;
    }

    return 0;
}

/*Regulile de joc pentru cal:
    1.Miscarea:
        ->Calul se misca intotdeauna in forma de "L", adica doua patrate intr-o directie (linie sau coloana) si unul intr-o directie perpendiculara pe acea directie.
    2.Salt peste alte piese:
        ->Calul poate sa sara peste alte piese. Asta inseamna ca, chiar daca pe calea sa se afla piese proprii sau ale adversarului, calul isi poate continua mutarea fara a fi blocat.
    3.Capturarea:
        ->Calul poate captura piesele adverse aflate pe patratul final al mutarii sale, respectand regula de capturare (mutarea pe un patrat ocupat de o piesa adversa).
    4.Raspuns la atacuri:
        ->Calul este capabil sa atace o piesa adversa aflata pe unul dintre patratele tinta ale miscarii sale datorita capacitatii sale de a sari peste alte piese.
*/

int validare_mutare_cal(int linie_start, int coloana_start, int linie_final, int coloana_final) {
    //Calculam diferentele pe linie si coloana
    int delta_linie = abs(linie_final - linie_start);
    int delta_coloana = abs(coloana_final - coloana_start);

    //Verificam regulilor de mutare ale calului
    return (delta_linie == 2 && delta_coloana == 1) || (delta_linie == 1 && delta_coloana == 2);
}

//Functie pentru a avedea daca regele este
int este_atacat(char tabla[BOARD_SIZE][BOARD_SIZE], int linie, int coloana, char jucator) {
    char adversar = (jucator == 'a') ? 'n' : 'a';

    // Verificam toate piesele adversarului pentru a vedea daca se paote ataca patratul
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if ((jucator == 'a' && isupper(tabla[i][j])) || (jucator == 'n' && islower(tabla[i][j]))) {
                char piesa = tabla[i][j];
                int valid = 0;

                switch (piesa) {
                    case 'p': case 'P':
                        valid = validare_mutare_pion(tabla, i, j, linie, coloana, adversar);
                        break;
                    case 't': case 'T':
                        valid = validare_mutare_tura(tabla, i, j, linie, coloana);
                        break;
                    case 'c': case 'C':
                        valid = validare_mutare_cal( i, j, linie, coloana);
                        break;
                    case 'n': case 'N':
                        valid = validare_mutare_nebun(tabla, i, j, linie, coloana, adversar);
                        break;

                }

                if (valid) {
                    return 1; 
                }
            }
        }
    }

    return 0; 
}

/*Regulile de joc pentru rege:
    1.Mutarea simpla:
        ->Regele se poate muta un patrat in orice directie (vertical, orizontal, diagonal).
    2.Rocada:
        ->Regele si tura nu trebuie sa fi fost mutate anterior.
        ->Nu trebuie sa existe piese intre rege si tura.
        ->Regele nu poate trece prin patrate atacate de piesele adverse.
        ->Regele nu trebuie sa fie in sah inainte sau dupa rocada.
    3.Interactiunea cu sahul:
        ->Regele nu poate ramane in sah dupa mutare.
        ->Daca o piesa adversa ameninta patratul in care regele vrea sa se mute, mutarea este invalida.

Conditii pentru:
    1.Rocada: 
        ->Regele trebuie sa fie pe pozitia initiala.
        ->Regele trebuie sa fie in coloana 4 (pozitia standard de start).
        ->Rocada trebuie sa fie posibila.
    2.Rocada mica:
        ->Regele se muta doua patrate spre dreapta.
        ->Toate patratele dintre rege si tura trebuie sa fie goale.
        ->Tura trebuie sa fie pe pozitia initiala. 
    3.Rocada mare:
        ->Regele se muta doua patrate spre stanga.
        ->Toate patratele dintre rege si tura trebuie sa fie goale.
        ->Tura trebuie sa fie pe pozitia initiala
*/

int validare_mutare_rege(char tabla[BOARD_SIZE][BOARD_SIZE], int linie_start, int coloana_start, int linie_final, int coloana_final, char jucator) {

    int rocada_posibila[2] = {1, 1};
    // Verificam mutarea simpla
    if (abs(linie_final - linie_start) <= 1 && abs(coloana_final - coloana_start) <= 1) {
        // Verificam daca patratul final este atacat
        if (!este_atacat(tabla, linie_final, coloana_final, jucator)) {
            return 1;
        }
        return 0;
    }

    // Verificam rocada
    if ((linie_start == 7 && coloana_start == 4 && jucator == 'a' && rocada_posibila[0] == 1) ||
        (linie_start == 0 && coloana_start == 4 && jucator == 'n' && rocada_posibila[1] == 1)) {
        // Rocada mica
        if (coloana_final == 6 && tabla[linie_start][5] == '.' && tabla[linie_start][6] == '.' && tabla[linie_start][7] == 't') {
            if (!este_atacat(tabla, linie_start, 4, jucator) && 
                !este_atacat(tabla, linie_start, 5, jucator) && 
                !este_atacat(tabla, linie_start, 6, jucator)) { 
                // Mutam tura
                tabla[linie_start][5] = tabla[linie_start][7];
                tabla[linie_start][7] = '.';
                return 1;
            }
            return 0;
        }
        // Rocada mare
        if (coloana_final == 2 && tabla[linie_start][1] == '.' && tabla[linie_start][2] == '.' && tabla[linie_start][3] == '.' && tabla[linie_start][0] == 't') {
            if (!este_atacat(tabla, linie_start, 4, jucator) && 
                !este_atacat(tabla, linie_start, 3, jucator) && 
                !este_atacat(tabla, linie_start, 2, jucator)) { 
        
                tabla[linie_start][3] = tabla[linie_start][0];
                tabla[linie_start][0] = '.';
                return 1;
            }
            return 0;
        }
    }

    return 0;
}

/*Reguli de joc pentru regina:
    1.Miscari permise:
        ->Regina poate sa se mute oricate patrate:
            +Pe o linie dreapta (orizontala sau verticala), ca tura.
            +Pe o diagonal, ca nebunul.
    2.Blocarea traseului:
        ->Mutarea este permisa doar daca toate patratele intre pozitia de start si cea final sunt goale.
        ->Nu poate sa sara peste alte piese.
    3.Capturare:
        ->Poate captura o piesa adversa, dar numai daca patratul final este ocupat de acea piesa.
*/

int validare_mutare_regina(char tabla[BOARD_SIZE][BOARD_SIZE], int linie_start, int coloana_start, int linie_final, int coloana_final, char jucator) {
    //Verificam daca mutarea este valida ca si cum regina ar fi o tura
    //Verificam daca mutarea este valida ca si cum regina ar fi un nebun
    return validare_mutare_tura(tabla, linie_start, coloana_start, linie_final, coloana_final) ||
           validare_mutare_nebun(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator);
}


int validare_mutare(char tabla[BOARD_SIZE][BOARD_SIZE], int linie_start, int coloana_start, int linie_final, int coloana_final, char jucator) {
    
    char piesa = tabla[linie_start][coloana_start];
    

    //Verificam daca piesa apartine jucatorului corespunzator
    if ((jucator == 'a' && isupper(piesa)) || (jucator == 'n' && islower(piesa))) {
        printf("Mutare invalida: piesa nu apartine jucatorului\n");
        return 0;
    }

    // Verificam sa nu se captureze o piesa proprie
    if (tabla[linie_final][coloana_final] != '.' &&
        ((jucator == 'a' && islower(tabla[linie_final][coloana_final])) ||
         (jucator == 'n' && isupper(tabla[linie_final][coloana_final])))) {
        printf("Mutare invalida: nu se poate captura o piesa proprie\n");
        return 0;
    }

    int valid = 0;
    switch (tolower(piesa)) {
        case 'p': 
            valid = validare_mutare_pion(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator); 
            break;
        case 'k': 
            valid = validare_mutare_rege(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator); 
            break;
        case 'q': 
            valid = validare_mutare_regina(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator); 
            break;
        case 't': 
            valid = validare_mutare_tura(tabla, linie_start, coloana_start, linie_final, coloana_final); 
            break;
        case 'n': 
            valid = validare_mutare_nebun(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator); 
            break;
        case 'c': 
            valid = validare_mutare_cal(linie_start, coloana_start, linie_final, coloana_final); 
            break;
        default: 
            printf("Mutare invalida: piesa necunoscuta\n");
            return 0;
    }

    if (valid) {
        printf("Mutare validata\n");
    } else {
        printf("Mutare invalida conform regulilor piesei\n");
    }
    return valid;
}


int muta_piesa(char tabla[BOARD_SIZE][BOARD_SIZE], int linie_start, int coloana_start, int linie_final, int coloana_final, char jucator){

    tabla[linie_final][coloana_final] = tabla[linie_start][coloana_start];
    tabla[linie_start][coloana_start] = '.';

    return 0;
}

int gaseste_regele(char tabla[BOARD_SIZE][BOARD_SIZE], char jucator) {
    char rege = (jucator == 'a') ? 'K' : 'k'; // Regele alb sau negru
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (tabla[i][j] == rege) {
                return i * BOARD_SIZE + j; // Returneaza pozitia regelui 
            }
        }
    }
    return -1; 
}

int este_in_sah(char tabla[BOARD_SIZE][BOARD_SIZE], char jucator) {
    int pozitie_rege = gaseste_regele(tabla, jucator);
    if (pozitie_rege == -1) return 0; 

    int linie_rege = pozitie_rege / BOARD_SIZE;
    int coloana_rege = pozitie_rege % BOARD_SIZE;

    // Verificam daca piesele inamice pot captura regele
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char piesa_inamica = (jucator == 'a') ? 'n' : 'a'; 
            if (isupper(tabla[i][j]) == (jucator == 'a')) {
                continue; // Sarim peste piesele proprii
            }
            if (validare_mutare(tabla, i, j, linie_rege, coloana_rege, piesa_inamica)) {
                return 1; 
            }
        }
    }
    return 0;
}

int este_sah_mat(char tabla[BOARD_SIZE][BOARD_SIZE], char jucator) {
    if (!este_in_sah(tabla, jucator)) {
        return 0; 
    }

    // Verificam toate posibilele mutari ale regelui
    int pozitie_rege = gaseste_regele(tabla, jucator);
    int linie_rege = pozitie_rege / BOARD_SIZE;
    int coloana_rege = pozitie_rege % BOARD_SIZE;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0){ 
                continue;
            } 
            int linie_noua = linie_rege + i;
            int coloana_noua = coloana_rege + j;
            if (linie_noua >= 0 && linie_noua < BOARD_SIZE && coloana_noua >= 0 && coloana_noua < BOARD_SIZE) {
                if (tabla[linie_noua][coloana_noua] == '.' || (isupper(tabla[linie_noua][coloana_noua]) == (jucator == 'a'))) {
                    if (!este_in_sah(tabla, jucator)) {
                        return 0; 
                    }
                }
            }
        }
    }

    return 1; 
}

int recv_full(int sockfd, char *buf, int len) {
    int total_received = 0;
    while (total_received < len) {
        int bytes_received = recv(sockfd, buf + total_received, len - total_received, 0);
        if (bytes_received <= 0) {
            perror("Eroare la recv_full");
            return bytes_received;
        }
        total_received += bytes_received;

        for (int i = 0; i < bytes_received; ++i) {
            printf("%02x ", (unsigned char)buf[total_received - bytes_received + i]);
        }
        printf("\n");
    }
    return total_received;
}



void *handle_game(void *arg) {
    GameSession *session = (GameSession *)arg;
    int client1 = session->client1_fd;
    int client2 = session->client2_fd;
    char (*tabla)[8] = session->tabla;
    int current_turn = session->current_turn;
    int session_id = session->session_id;

    printf("Session %d started.\n", session_id);

    while (1) {
        int current_client = (current_turn == 0) ? client1 : client2;
        char current_player_color = (current_turn == 0) ? 'a' : 'n';

        printf("Session %d: Waiting for move from player %d (%c).\n", session_id, current_turn + 1, current_player_color);

        if (send(current_client, tabla, sizeof(char) * BOARD_SIZE * BOARD_SIZE, 0) < 0) {
            perror("Eroare la trimiterea tablei!");
            break;
        }

        char move[10];
        printf("Session %d: Waiting for move...\n", session_id);

        int recv_status = recv_full(current_client, move, 5); 
        if (recv_status <= 0) {
            printf("Session %d: Player disconnected.\n", session_id);
            break;
        }

        printf("Session %d: Move received: '%s' (length: %lu)\n", session_id, move, strlen(move));
        for (int i = 0; i < strlen(move); i++) {
            printf("Character %d: '%c' (ASCII: %d)\n", i, move[i], move[i]);
        }

        if (strlen(move) != 5 || move[2] != ' ') {
            send(current_client, "Invalid move format", 20, 0);
            printf("Session %d: Invalid move format\n", session_id);
            continue;
        }

        int linie_start, coloana_start, linie_final, coloana_final;
        linie_start = 8 - (move[1] - '0');
        coloana_start = move[0] - 'a'; 
        linie_final = 8 - (move[4] - '0');
        coloana_final = move[3] - 'a'; 

        int valid = validare_mutare(tabla, linie_start, coloana_start, linie_final, coloana_final, current_turn == 0? 'a':'n');
        printf("Session %d: Result from validare_mutare: %d\n", session_id, valid);

        if (!valid) {
            send(current_client, "Mutare invalida\n", 15, 0);
            printf("Session %d: Invalid move by player %d.\n", session_id, current_turn + 1);
            continue;
        } else {
            muta_piesa(tabla, linie_start, coloana_start, linie_final, coloana_final, current_turn == 0 ? 'a' : 'n');
            verifica_promovare_pion(tabla, linie_final, coloana_final, current_turn == 0 ? 'a' : 'n');
            printf("Session %d: Move applied.\n", session_id);
        }

        send(client1, tabla, sizeof(char) * BOARD_SIZE * BOARD_SIZE, 0);
        send(client2, tabla, sizeof(char) * BOARD_SIZE * BOARD_SIZE, 0); 
        

        current_turn = 1 - current_turn;
    }

    printf("Session %d ended.\n", session_id);
    free(session);

    close(client1);
    close(client2);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    int session_counter = 0; // Contor pentru sesiuni

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Eroare la bind!");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Eroare la listen!");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is on port %d\n", PORT);

    while (1) {
        int client1 = accept(server_fd, NULL, NULL);
        if (client1 < 0) {
            perror("Jucatorul nu a fost acceptat!");
            continue;
        }

        printf("Primul jucator a fost conectat cu succes!\n");

        int client2 = accept(server_fd, NULL, NULL);
        if (client2 < 0) {
            perror("Al doilea jucator nu a fost acceptat!");
            close(client1);
            continue;
        }
        printf("Al doilea jucator a fost conectat cu succes!\n");

        GameSession *session = (GameSession *)malloc(sizeof(GameSession));
        session->session_id = __sync_fetch_and_add(&session_counter, 1); // ID unic
        session->client1_fd = client1;
        session->client2_fd = client2;
        initializare_tabla(session->tabla);
        session->current_turn = 0;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_game, session);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
