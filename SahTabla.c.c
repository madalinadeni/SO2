#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BOARD_SIZE 8

#define WHITE_BACKGROUND "\033[48;5;15m"  
#define BLACK_BACKGROUND "\033[48;5;233m" 
#define RESET_COLOR "\033[0m"             

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

int rocada_posibila[2] = {1, 1};
int sah_mat = 0;
int piese_capturate_alb = 0; 
int piese_capturate_negru = 0; 

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

void afisare_tabla(char tabla[BOARD_SIZE][BOARD_SIZE]) {
    printf("   A B C D E F G H\n");

    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%d  ", 8 - i); 
        for (int j = 0; j < BOARD_SIZE; j++) {
            if ((i + j) % 2 == 0) {
                printf(WHITE_BACKGROUND);
            } else {
                printf(BLACK_BACKGROUND);
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
    static int en_passant_linie = -1; 
    static int en_passant_coloana = -1; 
    
    int directie = (jucator == 'a') ? -1 : 1;
    char pion = (jucator == 'a') ? 'p' : 'P';

    // Mutare normala inainte
    if (coloana_start == coloana_final) {
        if (tabla[linie_final][coloana_final] == '.' && linie_final == linie_start + directie) {
            en_passant_linie = -1; 
            en_passant_coloana = -1;
            return 1;
        }

        // Mutare dubla inainte
        if ((linie_start == 6 && jucator == 'a') || (linie_start == 1 && jucator == 'n')) {
            if (tabla[linie_final][coloana_final] == '.' && tabla[linie_start + directie][coloana_start] == '.' && linie_final == linie_start + 2 * directie) {
                en_passant_linie = linie_start + directie; 
                en_passant_coloana = coloana_start;
                return 1;
            }
        }
    }

    // Capturare pe diagonala
    if (abs(coloana_final - coloana_start) == 1 && linie_final == linie_start + directie) {
        if (tabla[linie_final][coloana_final] != '.' && islower(tabla[linie_final][coloana_final]) != islower(pion)) {
            en_passant_linie = -1; 
            en_passant_coloana = -1;
            return 1;
        }

        // Capturare en passant
        if (linie_final == en_passant_linie && coloana_final == en_passant_coloana) {
            tabla[linie_start][coloana_final] = '.'; // Eliminam pionul capturat
            en_passant_linie = -1; 
            en_passant_coloana = -1;
            return 1;
        }
    }

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

int validare_mutare_rege(char tabla[BOARD_SIZE][BOARD_SIZE], int linie_start, int coloana_start, int linie_final, int coloana_final, char jucator) {
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
        return 0;
    }

    // Verificam sa nu se captureze o piesa proprie
    if (tabla[linie_final][coloana_final] != '.' &&
        ((jucator == 'a' && islower(tabla[linie_final][coloana_final])) ||
         (jucator == 'n' && isupper(tabla[linie_final][coloana_final])))) {
        return 0;
    }

    switch (tolower(piesa)) {
        case 'p': return validare_mutare_pion(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator);
        case 'k': return validare_mutare_rege(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator);
        case 'q': return validare_mutare_regina(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator);
        case 't': return validare_mutare_tura(tabla, linie_start, coloana_start, linie_final, coloana_final);
        case 'n': return validare_mutare_nebun(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator);
        case 'c': return validare_mutare_cal(linie_start, coloana_start, linie_final, coloana_final);
        default: return 0;
    }
}


// Functie unde am implementat si contorul pentru numarul de piese capturate
void muta_piesa(char tabla[BOARD_SIZE][BOARD_SIZE], int linie_start, int coloana_start, int linie_final, int coloana_final, char jucator) {
    char piesa_capturata = tabla[linie_final][coloana_final];
    if (piesa_capturata != '.') { 
        if (jucator == 'a') {
            piese_capturate_alb++;
        } else {
            piese_capturate_negru++;
        }
    }
    tabla[linie_final][coloana_final] = tabla[linie_start][coloana_start];
    tabla[linie_start][coloana_start] = '.';
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


int main() {
    char tabla[BOARD_SIZE][BOARD_SIZE];
    char jucator_curent = 'a';
    char input[10];

    initializare_tabla(tabla);

    while (1) {
        afisare_tabla(tabla);
        printf("Piese capturate: Alb - %d, Negru - %d\n", piese_capturate_alb, piese_capturate_negru);

        printf("Este randul jucatorului %s.\n", (jucator_curent == 'a') ? "Alb" : "Negru");
        printf("Introdu mutarea (exemplu: A7 H4): ");
        fgets(input, sizeof(input), stdin);

        // Interpretam mutarile: avem 8 lini, iar fiecare input reprezinta caracterul la care face referire.
        int linie_start = 8 - (input[1] - '0');
        int coloana_start = input[0] - 'A';
        int linie_final = 8 - (input[4] - '0');
        int coloana_final = input[3] - 'A';

        if (validare_mutare(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator_curent)) {
            muta_piesa(tabla, linie_start, coloana_start, linie_final, coloana_final, jucator_curent);
            verifica_promovare_pion(tabla, linie_final, coloana_final, jucator_curent);
            jucator_curent = (jucator_curent == 'a') ? 'n' : 'a';
        } else {
            printf("Mutarea este invalida! Incearca din nou.\n");
        }

        if (este_sah_mat(tabla, jucator_curent)) {
            printf("Sah Mat! Jucatorul %s a castigat!\n", (jucator_curent == 'a') ? "Alb" : "Negru");
            break; 
        }
    }

    return 0;
}
