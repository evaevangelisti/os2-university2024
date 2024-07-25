#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <locale.h>

#include "tabulate.h"
#include "flatten.h"
#include "hashmap.h"
#include "error_handler.h"
#include "constants.h"

/**
 * Stringa delle opzioni consentite.
 */
#define ALLOWED_OPTIONS ":o:w:mh"

/**
 * Variabili globali per la gestione delle opzioni.
 */
extern char *optarg;
extern int opterr, optind;

/**
 * Enumerazione dei comandi.
 */
typedef enum {
    INVALID = -1,
    EMPTY,
    TABULATE,
    FLATTEN
} CommandCode;

/**
 * Struttura che rappresenta un comando.
 */
typedef struct {
    CommandCode code;
    char *name;
} Command;

/**
 * Struttura che rappresenta le opzioni passate al programma.
 */
typedef struct {
    char *output_filename;
    wchar_t previous_word[MAX_WORD_LENGTH]; 
    bool multiprocess_mode;
    bool help_mode;
} Options;

/**
 * Array dei comandi.
 */
Command commands[] = {
    { TABULATE, "tabulate" },
    { FLATTEN, "flatten" },
};

/**
 * Nome del programma.
 */
char *program_name;

/**
 * Restituisce il comando corrispondente a una stringa.
 *
 * @param command La stringa da confrontare con i comandi.
 * @return Il comando corrispondente alla stringa.
 */
CommandCode get_command(char *command);

/**
 * Verifica se una stringa termina con un suffisso specifico.
 *
 * @param string La stringa da controllare.
 * @param suffix Il suffisso da confrontare con la fine della stringa.
 * @return true se la stringa termina con il suffisso specificato, false altrimenti.
 */
bool ends_with(char *string, char *suffix);

/**
 * Concatena un suffisso a una stringa.
 *
 * @param string La stringa a cui concatenare il suffisso.
 * @param suffix Il suffisso da concatenare alla stringa.
 * @return La stringa con il suffisso concatenato.
 */
char *append_suffix(char *string, char *suffix);

/**
 * Legge un numero da una stringa.
 *
 * @param string La stringa da cui leggere il numero.
 * @return Il numero letto, -1 se la stringa non rappresenta un numero.
 */
int read_number(char *string);

/**
 * Apre un file in base alla modalità specificata.
 *
 * @param filename Il nome del file da aprire.
 * @param extension L'estensione del file da aprire.
 * @param mode La modalità di apertura del file.
 * @return Il puntatore al file aperto.
 */
FILE *open_file(char *filename, char *extension, char mode);

/**
 * Parsa le opzioni passate al programma.
 *
 * @param arguments Gli argomenti passati al programma.
 * @param size La dimensione degli argomenti passati al programma.
 * @param previous_word Indica se è stata specificata la parola precedente.
 * @return Le opzioni passate al programma.
 */
Options parse_options(char *arguments[], int size, bool *previous_word);

/**
 * Gestisce l'opzione di aiuto.
 *
 * @param command Il comando per cui visualizzare l'aiuto.
 * @param command_name Il nome del comando per cui visualizzare l'aiuto.
 */
void help_handler(CommandCode command, char *command_name);

/**
 * Funzione principale del programma.
 *
 * @param argc Il numero di argomenti passati al programma.
 * @param argv Gli argomenti passati al programma.
 * @return Il codice di uscita del programma.
 */
int main(int argc, char *argv[]) {
    // Salva il nome del programma
    program_name = argv[0];

    // Se non è stato specificato alcun argomento, visualizza l'aiuto.
    if (argc < 2) help_handler(EMPTY, NULL); 

    // Imposta la localizzazione in base all'ambiente
    setlocale(LC_ALL, ""); 

    // Getopt non stampa messaggi di errore
    opterr = 0;

    // Indica se è stata specificata la parola precedente
    bool previous_word = false;

    // Parsa le opzioni passate al programma.
    Options options = parse_options(argv, argc, &previous_word); 

    // Legge il comando specificato e ricava il codice corrispondente.
    char *command_name = argv[optind++];
    CommandCode command = get_command(command_name);

    // Gestisce l'opzione per la parola precedente.
    if (previous_word) {
        if (command == FLATTEN) {
            if (wcscmp(options.previous_word, L"") == 0) argument_error_handler(ERR_MISSING_OPTION_ARGUMENT, "-w");
        } else {
            argument_error_handler(ERR_UNKNOWN_OPTION, "-w");
        }
    }

    // Gestisce l'opzione di aiuto.
    if (options.help_mode) help_handler(command, command_name);

    FILE *input_file;
    FILE *output_file;

    switch (command) {
        case TABULATE:
            // Apre il file di input in lettura
            input_file = open_file(argv[optind], ".txt", 'r');

            // Apre il file di output in scrittura
            output_file = open_file(options.output_filename, ".csv", 'w');

            // Esegue il comando tabulate
            tabulate(input_file, output_file, options.multiprocess_mode);

            printf("Tabulazione completata\n\n");
            break;

        case FLATTEN:
            // Apre il file di input in lettura
            input_file = open_file(argv[optind++], ".csv", 'r');

            // Legge il numero di parole da generare
            int words_to_generate;
            if ((words_to_generate = read_number(argv[optind])) == -1) argument_error_handler(ERR_INVALID_PARAMETER, argv[optind]);

            // Apre il file di output in scrittura
            output_file = open_file(options.output_filename, ".txt", 'w');

            // Esegue il comando flatten
            flatten(input_file, words_to_generate, options.previous_word, output_file, options.multiprocess_mode);

            printf("Generazione del testo completata\n\n");
            break;

        case EMPTY:
            // Gestisce il caso in cui non è stato specificato un comando
            error_handler(ERR_MISSING_COMMAND);
            break;

        case INVALID:
        default:
            // Gestisce il caso in cui è stato specificato un comando non valido
            argument_error_handler(ERR_INVALID_COMMAND, command_name);
    }
    
    // Chiude i file
    fclose(input_file);
    fclose(output_file);

    return EXIT_SUCCESS;
}

/**
 * Restituisce il comando corrispondente a una stringa.
 *
 * @param command La stringa da confrontare con i comandi.
 * @return Il comando corrispondente alla stringa.
 */
CommandCode get_command(char *command) {
    // Se non è stata specificata alcuna stringa, restituisce il comando vuoto
    if (!command) return EMPTY;

    // Scorre l'array dei comandi
    for (int i = INVALID + 1; i < sizeof(commands) / sizeof(Command); i++) {
        // Se la stringa corrisponde a un comando, restituisce il codice del comando
        if (strcmp(command, commands[i].name) == 0) return commands[i].code;
    }
    
    // Restituisce il comando non valido
    return INVALID;
}

/**
 * Verifica se una stringa termina con un suffisso specifico.
 *
 * @param string La stringa da controllare.
 * @param suffix Il suffisso da confrontare con la fine della stringa.
 * @return true se la stringa termina con il suffisso specificato, false altrimenti.
 */
bool ends_with(char *string, char *suffix) {
    // Se non è stata specificata alcuna stringa o alcun suffisso, restituisce false
    if (!string || !suffix) return false;

    // Calcola la lunghezza della stringa e del suffisso
    size_t string_length = strlen(string);
    size_t suffix_length = strlen(suffix);

    // Se la lunghezza del suffisso è maggiore della lunghezza della stringa, restituisce false
    if (suffix_length > string_length) return false;

    // Restituisce true se la stringa termina con il suffisso, false altrimenti
    return strncmp(string + string_length - suffix_length, suffix, suffix_length) == 0;
}

/**
 * Concatena un suffisso a una stringa.
 *
 * @param string La stringa a cui concatenare il suffisso.
 * @param suffix Il suffisso da concatenare alla stringa.
 * @return La stringa con il suffisso concatenato.
 */
char *append_suffix(char *string, char *suffix) {
    // Alloca la memoria per la nuova stringa
    char *result = malloc(strlen(string) + strlen(suffix) + 1);
    if (!result) error_handler(ERR_MEMORY_ALLOCATION);

    // Concatena la stringa e il suffisso
    strcpy(result, string);
    strcat(result, suffix);

    // Restituisce la nuova stringa
    return result;
}

/**
 * Legge un numero da una stringa.
 *
 * @param string La stringa da cui leggere il numero.
 * @return Il numero letto, -1 se la stringa non rappresenta un numero.
 */
int read_number(char *string) {
    // Se non è stata specificata alcuna stringa, errore
    if (!string) argument_error_handler(ERR_MISSING_PARAMETER, "words_to_generate");

    // Scorre la stringa
    for (int i = 0; string[i] != '\0'; i++) {
        // Se il carattere corrente non è un numero, restituisce -1
        if (!isdigit(string[i])) return -1;
    }

    // Restituisce il numero letto
    return atoi(string);
}

/**
 * Apre un file in base alla modalità specificata.
 *
 * @param filename Il nome del file da aprire.
 * @param extension L'estensione del file da aprire.
 * @param mode La modalità di apertura del file.
 * @return Il puntatore al file aperto.
 */
FILE *open_file(char *filename, char *extension, char mode) {
    // Puntatore al file
    FILE *file;

    switch (mode) {
        case 'r':
            // Se non è stata specificato il filename, errore
            if (!filename) argument_error_handler(ERR_MISSING_PARAMETER, "input_file");

            // Se il filename non finisce con l'estensione specificata, aggiunge l'estensione
            if (!ends_with(filename, extension)) filename = append_suffix(filename, extension);

            // Apre il file in lettura
            if (!(file = fopen(filename, "r"))) argument_error_handler(ERR_INVALID_PARAMETER, filename);

            break;

        case 'w':
            // Se il filename è una stringa vuota, imposta il filename di default
            if (strcmp(filename, "") == 0) filename = "output";
            
            // Se il filename non finisce con l'estensione specificata, aggiunge l'estensione
            if (!ends_with(filename, extension)) filename = append_suffix(filename, extension);

            // Apre il file in scrittura
            if (!(file = fopen(filename, "w"))) argument_error_handler(ERR_INVALID_PARAMETER, filename);

            break;
        
        default:
            // Errore interno
            error_handler(ERR_INTERNAL_ERROR);
    }

    // Restituisce il puntatore al file
    return file;
}

/**
 * Parsa le opzioni passate al programma.
 *
 * @param arguments Gli argomenti passati al programma.
 * @param size La dimensione degli argomenti passati al programma.
 * @param previous_word Indica se è stata specificata la parola precedente.
 * @return Le opzioni passate al programma.
 */
Options parse_options(char *arguments[], int size, bool *previous_word) {
    // Opzioni di default
    Options options = { "", L"", false, false };

    // Opzione corrente
    int option;

    // Scorre le opzioni passate al programma
    while ((option = getopt(size, arguments, ALLOWED_OPTIONS)) != EOF) {
        switch (option) {
            case 'o':
                // Imposta il nome del file di output
                options.output_filename = optarg;
                break;

            case 'w':
                // Imposta la parola precedente
                if (mbstowcs(options.previous_word, optarg, MAX_WORD_LENGTH) == -1) error_handler(ERR_INTERNAL_ERROR);

                // Indica che è stata specificata la parola precedente
                *previous_word = true;
                break;

            case 'm':
                // Abilita la modalità multiprocesso
                options.multiprocess_mode = true;
                break;

            case 'h':
                // Abilita la modalità di aiuto
                options.help_mode = true;
                break;

            case ':':
                // La gestione dell'opzione -w viene effettuata in seguito
                if (optopt != 'w') {
                    // Se l'opzione richiede un argomento, ma non è stato specificato, errore
                    argument_error_handler(ERR_MISSING_OPTION_ARGUMENT, (char []){ '-', optopt, '\0' });
                } else {
                    // Indica che è stata specificata la parola precedente
                    *previous_word = true;
                }
                
                break;

            default:
                // Se l'opzione non è riconosciuta, errore
                argument_error_handler(ERR_UNKNOWN_OPTION, (char []){ '-', optopt, '\0' });
        }
    }

    // Restituisce le opzioni
    return options;
}

/**
 * Gestisce l'opzione di aiuto.
 *
 * @param command Il comando per cui visualizzare l'aiuto.
 * @param command_name Il nome del comando per cui visualizzare l'aiuto.
 */
void help_handler(CommandCode command, char *command_name) {
    switch (command) {
        case TABULATE:
            // Visualizza l'aiuto per il comando tabulate
            printf("usage: %s tabulate [-h] [-o <output_file>] [m] <input_file>\n\n", program_name);
            printf("Descrizione:\n");
            printf("  converte un file di testo in una tabella di frequenze.\n\n");
            printf("Opzioni:\n");
            printf("  -h     Visualizza questo messaggio di aiuto ed esce.\n");
            printf("  -o     Specifica il percorso per il file di output (default './output.csv').\n");
            printf("  -m     Abilita il multiprocessing.\n\n");
            printf("Argomenti:\n");
            printf("  input_file    File di input.\n\n");

            break;

        case FLATTEN:
            // Visualizza l'aiuto per il comando flatten
            printf("usage: %s flatten [-h] [-w <previous_word] [-o <output_file>] [m] <input_file> <words_to_generate>\n\n", program_name);
            printf("Descrizione:\n");
            printf("  genera un testo casuale a partire da una tabella di frequenze.\n\n");
            printf("Opzioni:\n");
            printf("  -h                   Visualizza questo messaggio di aiuto ed esce.\n");
            printf("  -w                   Specifica la parola precedente (default '.', '?' o '!').\n");
            printf("  -o                   Specifica il percorso per il file di output (default './output.txt').\n");
            printf("  -m                   Abilita il multiprocessing.\n\n");
            printf("Argomenti:\n");
            printf("  input_file           File di input.\n");
            printf("  words_to_generate    Numero di parole da generare.\n\n");

            break;

        case EMPTY:
            // Se non è stato specificato alcun comando, visualizza l'aiuto generale
            printf("usage: %s [-h] [-o <output_file>] [m] <command>\n\n", program_name); 
            printf("Descrizione:\n");
            printf("  gestisce la realizzazione dei compiti tabulate e flatten fornendo un'implementazione a singolo processo e multiprocesso.\n\n"); // Scrivere descrizione
            printf("Opzioni:\n");
            printf("  -h          Visualizza un messaggio di aiuto relativo a un comando.\n");
            printf("  -o          Specifica il percorso per il file di output (default './output').\n");
            printf("  -m          Abilita il multiprocessing.\n\n");
            printf("Comandi:\n");
            printf("  tabulate    Converte un file di testo in una tabella di frequenze.\n");
            printf("  flatten     Genera un testo casuale a partire da una tabella di frequenze.\n\n");

            break;

        case INVALID:
        default:
            // Se il comando non è riconosciuto, errore
            argument_error_handler(ERR_INVALID_COMMAND, command_name);
    }

    // Termina il programma
    exit(EXIT_SUCCESS);
}