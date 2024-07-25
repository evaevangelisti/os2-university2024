#include <stdio.h>
#include <stdlib.h>

#include "error_handler.h"

/**
 * Struttura che rappresenta un errore.
 */
typedef struct {
    ErrorCode code;
    char *message;
} Error;

/**
 * Array degli errori.
 */
Error errors[] = {
    { ERR_MISSING_PARAMETER, "parametro mancante" }, 
    { ERR_INVALID_PARAMETER, "parametro non valido" }, 
    { ERR_UNKNOWN_OPTION, "opzione sconosciuta" }, 
    { ERR_MISSING_OPTION_ARGUMENT, "argomento dell'opzione mancante per" },
    { ERR_INVALID_OPTION_ARGUMENT, "argomento dell'opzione non valido per" },
    { ERR_MISSING_COMMAND, "comando mancante" },
    { ERR_INVALID_COMMAND, "comando non valido" },
    { ERR_INVALID_TEXT, "testo fornito non valido" }, 
    { ERR_INVALID_TABLE, "tabella fornita non valida" },
    { ERR_MEMORY_ALLOCATION, "allocazione di memoria fallita" },
    { ERR_PARALLELIZATION, "parallelizzazione fallita" },
    { ERR_INTERNAL_ERROR, "errore interno" }, 
};

/**
 * Gestisce gli errori.
 * 
 * @param code Il codice dell'errore.
 */
void error_handler(ErrorCode code) {
    // Scorre l'array degli errori
    for (int i = 0; i < sizeof(errors) / sizeof(Error); i++) {
        // Se il codice dell'errore corrisponde a quello passato come argomento, stampa il messaggio di errore e termina il programma
        if (errors[i].code == code) {
            fprintf(stderr, "errore: %s\n\n", errors[i].message);        
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * Gestisce gli errori.
 * 
 * @param code Il codice dell'errore.
 * @param argument L'argomento dell'errore.
 */
void argument_error_handler(ErrorCode code, char *argument) {
    // Scorre l'array degli errori
    for (int i = 0; i < sizeof(errors) / sizeof(Error); i++) {
        // Se il codice dell'errore corrisponde a quello passato come argomento, stampa il messaggio di errore e termina il programma
        if (errors[i].code == code) {
            fprintf(stderr, "errore: %s '%s'\n\n", errors[i].message, argument); 
            exit(EXIT_FAILURE);
        }
    }
}