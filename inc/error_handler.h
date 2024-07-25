#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

/**
 * Enumerazione degli errori.
 */
typedef enum ErrorCode {
    ERR_MISSING_PARAMETER,
    ERR_INVALID_PARAMETER,
    ERR_UNKNOWN_OPTION,
    ERR_MISSING_OPTION_ARGUMENT,
    ERR_INVALID_OPTION_ARGUMENT,
    ERR_MISSING_COMMAND,
    ERR_INVALID_COMMAND,
    ERR_INVALID_TEXT,
    ERR_INVALID_TABLE,
    ERR_MEMORY_ALLOCATION,
    ERR_PARALLELIZATION,
    ERR_INTERNAL_ERROR,
} ErrorCode;

/**
 * Gestisce gli errori.
 * 
 * @param code Il codice dell'errore.
 */
void error_handler(ErrorCode code);

/**
 * Gestisce gli errori.
 * 
 * @param code Il codice dell'errore.
 * @param argument L'argomento dell'errore.
 */
void argument_error_handler(ErrorCode code, char *argument);

#endif