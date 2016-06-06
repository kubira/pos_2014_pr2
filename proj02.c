/*
 * Soubor:    proj02.c
 * Autor:     Radim KUBIŠ, xkubis03
 * Vytvořeno: 4. dubna 2014
 *
 * Projekt č. 2 do předmětu Pokročilé operační systémy (POS).
 */

#define _POSIX_C_SOURCE 199309L

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>

// Proměnná pro detekci chyby při vytváření vláken
int threadCreateError = 0;
// Proměnná pro uložení názvu programu
char *programName = "";
// Proměnná pro počet spouštěných vláken
int N = 0;
// Proměnné pro počet průchodů cyklem
int M = 0;
// Proměnná pro uložení identifikátorů vláken
pthread_t *threadsIds = NULL;
// Proměnná pro číslo aktuálního pořadového lístku při generování
int actualTicket = 0;
// Proměnná pro číslo aktuálního pořadového lístku ke zpracování
int actualOrder = 0;
// Mutex pro střežení lístku
pthread_mutex_t *mutexTicket = NULL;
// Mutex pro střežení podmínky
pthread_mutex_t *mutexCondition = NULL;
// Proměnná pro podmínku vláken
pthread_cond_t *threadsCond = NULL;

/*
 * Funkce pro vypsání nápovědy použití programu.
 */
void printHelp() {
    fprintf(stdout, "Usage: %s N M\n\n", programName);
    fprintf(stdout, "  N - number of threads\n");
    fprintf(stdout, "  M - number of pass through the cycle\n");
}

/*
 * Funkce pro převod řetězce na celé číslo.
 *
 * string - řetězec obsahující číslo
 *
 * Návratová hodnota:
 *     číslo reprezentované vstupním řetězcem
 */
int parseInt(char *string) {
    // Proměnná pro index do pole řetězce
    int index = 0;
    // Proměnná pro výsledné číslo
    int number = 0;
    // Dokud nejsem na konci řetězce
    while(string[index] != '\0') {
        // Kontrola, zda je aktuální znak na pozici číslice
        if(isdigit(string[index]) != 0) {
            // Převod aktuální číslice na číslo
            // a přičtení do výsledného čísla
            number = ((number * 10) + (string[index] - '0'));
        } else {
            // Aktuální znak není číslice

            // Tisk chyby
            fprintf(stderr, "error: '%c' is not digit\n", string[index]);
            // Výpis nápovědy použití programu
            printHelp();
            // Ukončení programu s chybovým kódem
            exit(EXIT_FAILURE);
        }
        // Inkrementace indexu řetězce
        index++;
    }
    // Navrácení číselné hodnoty řetězce
    return number;
}

/*
 * Funkce pro získání unikátního pořadového lístku určující pořadí vstupu
 * do kritické sekce.
 *
 * Návratová hodnota:
 *     - unikátní číslo pořadového lístku
 */
int getticket(void){
    // Proměnná pro pořadové číslo lístku
    int ticket = 0;
    // Proměnná pro návratové hodnoty funkcí
    int result = 0;
    // Zamčení mutexu střežícího sdílenou proměnnou
    result = pthread_mutex_lock(mutexTicket);
    // Kontrola zamčení
    if(result != 0) {
        // Tisk chyby zamčení
        perror("getticket pthread_mutex_lock");
    }
    // Uložení aktuálního lístku
    ticket = actualTicket;
    // Inkrementace aktuálního čísla pořadového lístku
    actualTicket++;
    // Odemčení mutexu střežícího sdílenou proměnnou
    result = pthread_mutex_unlock(mutexTicket);
    // Kontrola odemčení
    if(result != 0) {
        // Tisk chyby odemčení
        perror("getticket pthread_mutex_unlock");
    }
    // Navrácení aktuálního čísla pořadového lístku
    return ticket;
}

/*
 * Funkce pro náhodné čekání v intervalu <0,0 s, 0,5 s>.
 *
 * id - identifikátor vlákna, které má čekat
 */
void randomWait(int id) {
    // Proměnná pro návratovou hodnotu funkcí
    int result = 0;
    // Proměnná pro strukturu aktuálního času
    struct timeval actualTime;
    // Proměnná pro strukturu spícího času
    struct timespec waitTime;
    // Získání aktuálního času
    result = gettimeofday(&actualTime, NULL);
    // Kontrola provedení získání času
    if(result != 0) {
        // Tisk chyby při získání času
        perror("gettimeofday");
    }
    // Proměnná pro semínko generátoru čísel
    unsigned int seed = getpid() + id + actualTime.tv_usec;
    // Proměnná pro náhodné číslo
    int randomInt = rand_r(&seed);
    // Výpočet náhodného času v sekundách pro dobu čekání
    double timeToWait = ((randomInt / (double)RAND_MAX) * 0.5);
    // Nastavení sekund spícího času
    waitTime.tv_sec = 0;
    // Nastavení nanosekund spícího času
    waitTime.tv_nsec = timeToWait * 1000000000;
    // Čekání v nanosekundách
    result = nanosleep(&waitTime, NULL);
    // Kontrola správného čekání
    if(result != 0) {
        // Tisk chyby při čekání
        perror("nanosleep");
    }
}

/*
 * Funkce pro vstup do kritické sekce podle přiděleného pořadového lístku.
 *
 * aenter - číslo pořadového lístku
 */
void await(int aenter){
    // Proměnná pro návratovou hodnotu funkcí
    int result = 0;
    // Uzamčení mutexu před vstupem do kritické sekce
    result = pthread_mutex_lock(mutexCondition);
    // Kontrola správného uzamčení mutexu
    if(result != 0) {
        // Tisk chyby při zamykání
        perror("pthread_mutex_lock");
    }
    // Cyklus kontroly splnění podmínky
    while(actualOrder != aenter) {
        // Pokud podmínka není splněna, čeká
        result = pthread_cond_wait(threadsCond, mutexCondition);
        // Kontrola čekání na podmínku
        if(result != 0) {
            // Tisk chyby při čekání
            perror("pthread_cond_wait");
        }
    }
}

/*
 * Funkce pro výstup z kritické sekce.
 */
void advance(void){
    // Proměnná pro návratovou hodnotu funkcí
    int result = 0;
    // Zvýšení čísla pořadového lístku ke zpracování
    actualOrder++;
    // Pokud existují další pořadové lístky
    if(actualOrder < M) {
        // Signalizace podmínky
        result = pthread_cond_broadcast(threadsCond);
        // Kontrola provedení signalizace
        if(result != 0) {
            // Tisk chyby signalizace
            perror("pthread_cond_broadcast");
        }
    }
    // Odemčení mutexu při výstupu z kritické sekce
    result = pthread_mutex_unlock(mutexCondition);
    // Kontrola odemčení mutexu
    if(result != 0) {
        // Tisk chyby odemčení
        perror("pthread_mutex_unlock");
    }
}

/*
 * Funkce s programem vykonávaným vláknem.
 *
 * id - identifikátor vlákna
 */
void *threadProgram(void *id) {
    // Proměnná pro identifikaci vlákna
    int myId = *((int *) id);
    // Uvolnění paměti po identifikátoru vlákna
    free(id);
    // Proměnná pro pořadové číslo lístku
    int myTicket = 0;
    // Přidělení lístku
    // a kontrola, zda není třeba ukončit vlákno
    while (((myTicket = getticket()) < M) && (threadCreateError == 0)) {
        // Náhodné čekání v intervalu <0,0 s, 0,5 s>
        randomWait(myId);
        // Vstup do kritické sekce
        await(myTicket);
        // Vyprázdnění výstupního bufferu
        if(fflush(stdout) == EOF) {
            // Tisk chyby při vyprazdňování výstupního bufferu
            perror("fflush");
        }
        // Tisk identifikace vlákna a čísla lístku
        fprintf(stdout, "%d\t(%d)\n", myTicket, myId);
        // Výstup z kritické sekce
        advance();
        // Náhodné čekání v intervalu <0,0 s, 0,5 s>
        randomWait(myId);
    }
    // Ukončení vlákna
    pthread_exit("done");
}

/*
 * Hlavní funkce main.
 *
 * argc - počet argumentů příkazového řádku
 * argv - argumenty příkazové řádky
 *
 * Návratová hodnota:
 *         0 - program proběhl v pořádku
 *     jinak - program skončil s chybou
 */
int main(int argc, char *argv[]) {
    // Proměnná pro použití v cyklu
    int i = 0;
    // Proměnná pro návratové hodnoty volaných funkcí
    int result = 0;
    // Uložení názvu programu
    programName = argv[0];
    // Pokud nemá program dostatek parametrů příkazové řádky
    if(argc != 3) {
        // Výpis nápovědy užití programu
        printHelp();
        // Ukončení programu s chybovým kódem
        exit(EXIT_SUCCESS);
    }
    // Získání počtu požadovaných vláken
    N = parseInt(argv[1]);
    // Získání počtu průchodů cyklem
    M = parseInt(argv[2]);
    // Alokace mutexu střežícího podmínku
    mutexCondition = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    // Kontrola alokace mutexu
    if(mutexCondition == NULL) {
        // Tisk chyby alokace
        perror("mutexCondition malloc");
        // Ukončení programu s chybou
        exit(EXIT_FAILURE);
    }
    // Alokace mutexu střežícího sdílenou proměnnou
    mutexTicket = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    // Kontrola alokace mutexu
    if(mutexTicket == NULL) {
        // Tisk chyby alokace
        perror("mutexTicket malloc");
        // Uvolnění paměti po mutexu podmínky
        free(mutexCondition);
        // Ukončení programu s chybou
        exit(EXIT_FAILURE);
    }
    // Inicializace mutexu podmínky
    result = pthread_mutex_init(mutexCondition, NULL);
    // Kontrola inicializace mutexu
    if(result != 0) {
        // Tisk chyby inicializace
        perror("mutexCondition pthread_mutex_init");
        // Uvolnění paměti po mutexu podmínky
        free(mutexCondition);
        // Uvolnění paměti po mutexu sdílené proměnné
        free(mutexTicket);
        // Ukončení programu s chybou
        exit(EXIT_FAILURE);
    }
    // Inicializace mutexu sdílené proměnné
    result = pthread_mutex_init(mutexTicket, NULL);
    // Kontrola inicializace mutexu
    if(result != 0) {
        // Tisk chyby inicializace
        perror("mutexTicket pthread_mutex_init");
        // Odinicializace mutexu podmínky
        result = pthread_mutex_destroy(mutexCondition);
        // Kontrola odinicializace
        if(result != 0) {
            // Tisk chyby odinicializace
            perror("mutexCondition pthread_mutex_destroy");
        }
        // Uvolnění paměti po mutexu podmínky
        free(mutexCondition);
        // Uvolnění paměti po mutexu sdílené proměnné
        free(mutexTicket);
        // Ukončení programu s chybou
        exit(EXIT_FAILURE);
    }
    // Alokace prostoru pro identifikátory všech vláken
    threadsIds = (pthread_t *)malloc(N * sizeof(pthread_t));
    // Kontrola alokace
    if(threadsIds == NULL) {
        // Tisk chyby alokace
        perror("threadsIds malloc");
        // Odinicializace mutexu podmínky
        result = pthread_mutex_destroy(mutexCondition);
        // Kontrola odinicializace
        if(result != 0) {
            // Tisk chyby odinicializace
            perror("mutexCondition pthread_mutex_destroy");
        }
        // Odinicializace mutexu sdílené proměnné
        result = pthread_mutex_destroy(mutexTicket);
        // Kontrola odinicializace
        if(result != 0) {
            // Tisk chyby odinicializace
            perror("mutexTicket pthread_mutex_destroy");
        }
        // Uvolnění paměti po mutexu podmínky
        free(mutexCondition);
        // Uvolnění paměti po mutexu sdílené proměnné
        free(mutexTicket);
        // Ukončení programu s chybovým kódem
        exit(EXIT_FAILURE);
    }
    // Alokace prostoru pro podmínku vláken
    threadsCond = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    // Kontrola alokace
    if(threadsCond == NULL) {
        // Tisk chyby alokace
        perror("threadsCond malloc");
        // Uvolnění paměti pro threadsIds
        free(threadsIds);
        // Odinicializace mutexu podmínky
        result = pthread_mutex_destroy(mutexCondition);
        // Kontrola odinicializace
        if(result != 0) {
            // Tisk chyby odinicializace
            perror("mutexCondition pthread_mutex_destroy");
        }
        // Odinicializace mutexu sdílené proměnné
        result = pthread_mutex_destroy(mutexTicket);
        // Kontrola odinicializace
        if(result != 0) {
            // Tisk chyby odinicializace
            perror("mutexTicket pthread_mutex_destroy");
        }
        // Uvolnění paměti po mutexu podmínky
        free(mutexCondition);
        // Uvolnění paměti po mutexu sdílené proměnné
        free(mutexTicket);
        // Ukončení programu s chybovým kódem
        exit(EXIT_FAILURE);
    }
    // Inicializace podmínky
    result = pthread_cond_init(threadsCond, NULL);
    // Kontrola inicializace podmínky
    if(result != 0) {
        // Tisk chyby inicializace
        perror("pthread_cond_init");
        // Odinicializace mutexu podmínky
        result = pthread_mutex_destroy(mutexCondition);
        // Kontrola odinicializace
        if(result != 0) {
            // Tisk chyby odinicializace
            perror("mutexCondition pthread_mutex_destroy");
        }
        // Odinicializace mutexu sdílené proměnné
        result = pthread_mutex_destroy(mutexTicket);
        // Kontrola odinicializace
        if(result != 0) {
            // Tisk chyby odinicializace
            perror("mutexTicket pthread_mutex_destroy");
        }
        // Uvolnění paměti po mutexu podmínky
        free(mutexCondition);
        // Uvolnění paměti po mutexu sdílené proměnné
        free(mutexTicket);
        // Uvolnění paměti pro threadsIds
        free(threadsIds);
        // Uvolnění paměti pro threadsCond
        free(threadsCond);
        // Ukončení programu s chybovým kódem
        exit(EXIT_FAILURE);
    }
    // Cyklus vytváření N vláken
    for(i = 0; i < N; i++) {
        // Vytvoření proměnné pro identifikaci vlákna
        int *arg = malloc(sizeof(*arg));
        // Kontrola alokace proměnné
        if(arg == NULL) {
            // Tisk chyby alokace
            perror("arg malloc");
            // Nastavení příznaku chyby vytváření vláken
            threadCreateError = 1;
            // Ukončení cyklu vytváření vláken
            break;
        }
        // Uložení pořadového čísla vlákna do proměnné arg
        *arg = (i + 1);
        // Vytvoření jednoho vlákna
        result = pthread_create(&threadsIds[i], NULL, threadProgram, (void *) arg);
        // Kontrola vytvoření vlákna
        if(result != 0) {
            // Tisk chyby vytvoření vlákna
            perror("pthread_create");
            // Nastavení příznaku chyby vytváření vláken
            threadCreateError = 1;
            // Uvolnění paměti po identifikátoru pro aktuálně vytvářené vlákno
            free(arg);
            // Ukončení cyklu vytváření vláken
            break;
        }
    }
    // Pokud je nastaven příznak chyby vytváření vláken
    if(threadCreateError == 1) {
        // Nastavení počtu vláken na i
        N = i;
        // Ukončení generování lístků
        actualTicket = M;
        // Uzamčení mutexu kvůli signalizaci podmínky
        result = pthread_mutex_lock(mutexCondition);
        // Kontrola uzamčení
        if(result != 0) {
            // Tisk chyby uzamčení
            perror("pthread_mutex_lock after error");
        }
        // Signalizace podmínky
        result = pthread_cond_broadcast(threadsCond);
        // Kontrola signalizace
        if(result != 0) {
            // Tisk chyby signalizace
            perror("pthread_cond_broadcast after error");
        }
        // Odemčení mutexu po signalizaci
        result = pthread_mutex_unlock(mutexCondition);
        // Kontrola odemčení
        if(result != 0) {
            // Tisk chyby odemčení
            perror("pthread_mutex_unlock after error");
        }
    }
    // Cyklus pro čekání na ukončení vláken
    for(i = 0; i < N; i++) {
        // Čekání na ukončení vlákna
        result = pthread_join(threadsIds[i], NULL);
        // Kontrola čekání na ukončení vlákna
        if(result != 0) {
            // Tisk chyby při čekání na ukončení vlákna
            perror("pthread_join");
        }
    }
    // Dealokace podmínky
    result = pthread_cond_destroy(threadsCond);
    // Kontrola dealokace
    if(result != 0) {
        // Tisk chyby při dealokaci
        perror("pthread_cond_destroy");
    }
    // Odinicializace mutexu podmínky
    result = pthread_mutex_destroy(mutexCondition);
    // Kontrola odinicializace
    if(result != 0) {
        // Tisk chyby odinicializace
        perror("mutexCondition pthread_mutex_destroy");
    }
    // Odinicializace mutexu sdílené proměnné
    result = pthread_mutex_destroy(mutexTicket);
    // Kontrola odinicializace
    if(result != 0) {
        // Tisk chyby odinicializace
        perror("mutexTicket pthread_mutex_destroy");
    }
    // Uvolnění paměti po mutexu podmínky
    free(mutexCondition);
    // Uvolnění paměti po mutexu sdílené proměnné
    free(mutexTicket);
    // Uvolnění paměti po identifikátorech vláken
    free(threadsIds);
    // Uvolnění paměti po podmínkách vláken
    free(threadsCond);
    // Kontrola příznaku podmínky
    if(threadCreateError == 1) {
        // Ukončení programu s chybou
        exit(EXIT_FAILURE);
    } else {
        // Ukončení programu bez chyby
        exit(EXIT_SUCCESS);
    }
}
