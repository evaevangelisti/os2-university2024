#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>

#include "tabulate.h"
#include "hashmap.h"
#include "error_handler.h"
#include "constants.h"

#define BUFFER_SIZE 1024

/**
 * Verifica se un carattere è una punteggiatura non valida.
 *
 * @param character Il carattere da verificare.
 * @return true se il carattere è una punteggiatura non valida, false altrimenti.
 */
bool is_invalid_punctuation(wchar_t character);

/**
 * Processa un carattere.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param character Il carattere da processare.
 * @param previous_word La parola precedente.
 * @param current_word La parola corrente.
 * @param index L'indice della parola corrente.
 * @param first_word La prima parola del testo.
 * @param first_iteration Indica se è la prima iterazione.
 */
void process_character(HashMap *word_frequencies, wchar_t character, wchar_t *previous_word, wchar_t *current_word, int *index, wchar_t *first_word);

/**
 * Scrive una tabella di frequenze su un file CSV.
 *
 * @param word_frequencies La tabella delle frequenze da stampare.
 * @param output_file Il file su cui stampare la tabella delle frequenze.
 */
void hashmap_to_csv(HashMap *word_frequencies, FILE *output_file);

/*
 * Legge il testo da un file e lo scrive su un pipe.
 *
 * @param input_file Il file di input.
 * @param pipe_fd Il file descriptor del pipe.  
*/
void read_text(FILE *input_file, int pipe_fd[2]);

/*
 * Legge il testo da un pipe e lo processa.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param pipe_fd Il file descriptor del pipe.
*/
void process_text(HashMap *word_frequencies, int pipe_fd[2]);

/**
 * Converte un file di testo in una tabella di frequenze utilizzando un singolo processo.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param input_file Il file di input.
 * @param output_file Il file di output.
 */
void tabulate_single_process(HashMap *word_frequencies, FILE *input_file, FILE *output_file);

/**
 * Converte un file di testo in una tabella di frequenze.
 *
 * @param input_file Il file di input.
 * @param output_file Il file di output.
 * @param multiprocess_mode La modalità multiprocessore.
 */
void tabulate(FILE *input_file, FILE *output_file, bool multiprocess_mode) {
    // Creazione della hashmap
    HashMap *word_frequencies = hashmap_create();

    if (multiprocess_mode) {
        // Modalità multiprocess

        // Creazione della pipe (pipe_fd[0] per la lettura del file di input, pipe_fd[1] per la scrittura su file di output)
        int pipe_fd[2][2];
        if (pipe(pipe_fd[0]) == -1 || pipe(pipe_fd[1]) == - 1) error_handler(ERR_PARALLELIZATION);

        // Creazione di un processo per la lettura del testo
        pid_t read_pid = fork();
        if (read_pid == 0) {
            // Chiusura dei file descriptor della pipe di scrittura su file di output
            close(pipe_fd[1][0]);
            close(pipe_fd[1][1]);

            // Lettura del testo e scrittura sulla pipe
            read_text(input_file, pipe_fd[0]);

            // Fine del processo di lettura
            exit(EXIT_SUCCESS);
        } else if (read_pid == -1) {
            error_handler(ERR_PARALLELIZATION);
        }

        // Creazione di un processo per il processamento del testo
        pid_t process_pid = fork();
        if (process_pid == 0) {
            // Chisura del lato di lettura della pipe di scrittura su file di output
            close(pipe_fd[1][0]);

            // Processamento del testo
            process_text(word_frequencies, pipe_fd[0]);

            // Conversione della hashmap in un buffer
            wchar_t *buffer = hashmap_serialize(word_frequencies);

            // Dimensione del buffer
            size_t buffer_size = sizeof(wchar_t) * (wcslen(buffer) + 1);

            // Scrittura della dimensione del buffer sulla pipe
            write(pipe_fd[1][1], &buffer_size, sizeof(buffer_size));
            
            // Totale dei byte scritti
            ssize_t total = 0;

            // Continua a scrivere finché non ha scritto tutto il buffer
            while (total < buffer_size) {
                // Scrittura del buffer sulla pipe
                ssize_t written_size = write(pipe_fd[1][1], buffer + total / sizeof(wchar_t), buffer_size - total);
                if (written_size == -1) error_handler(ERR_PARALLELIZATION);

                // Viene incrementato il totale dei byte scritti
                total += written_size;
            }

            // Deallocazione del buffer
            free(buffer);
            
            // Chisura del lato di scrittura della pipe di scrittura su file di output
            close(pipe_fd[1][1]);

            // Fine del processo di processamento
            exit(EXIT_SUCCESS);
        } else if (process_pid == -1) {
            error_handler(ERR_PARALLELIZATION);
        }

        // Chiusura dei file descriptor della pipe di lettura del file di input
        close(pipe_fd[0][0]);
        close(pipe_fd[0][1]);

        // Creazione di un processo per la scrittura della tabella delle frequenze
        pid_t write_pid = fork();
        if (write_pid == 0) {
            // Chisura del lato di scrittura della pipe di scrittura su file di output
            close(pipe_fd[1][1]);

            // Dimensione del buffer
            size_t buffer_size;

            // Lettura della dimensione del buffer dalla pipe
            read(pipe_fd[1][0], &buffer_size, sizeof(buffer_size));

            // Allocazione del buffer
            wchar_t *buffer = malloc(buffer_size);
            if (!buffer) error_handler(ERR_MEMORY_ALLOCATION);
            
            // Totale dei byte letti
            ssize_t total = 0;

            // Continua a leggere finché non ha letto tutto il buffer
            while (total < buffer_size) {
                // Scrittura del buffer sulla pipe
                ssize_t read_size = read(pipe_fd[1][0], buffer + total / sizeof(wchar_t), buffer_size - total);
                if (read_size == -1) error_handler(ERR_PARALLELIZATION);

                // Viene incrementato il totale dei byte letti
                total += read_size;
            }

            // Conversione del buffer in una hashmap
            hashmap_deserialize(word_frequencies, buffer);
            
            // Deallocazione del buffer
            free(buffer);

            // Scrittura della hashmap su file di output
            hashmap_to_csv(word_frequencies, output_file);

            // Chisura del lato di lettura della pipe di scrittura su file di output
            close(pipe_fd[1][0]);

            // Fine del processo di scrittura
            exit(EXIT_SUCCESS);
        } else if (write_pid == -1) {
            error_handler(ERR_PARALLELIZATION);
        }

        // Chiusura dei file descriptor della pipe di scrittura su file di output
        close(pipe_fd[1][0]);
        close(pipe_fd[1][1]);

        // Status dei processi
        int status;
        
        // Attesa del processo di lettura
        waitpid(read_pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) exit(EXIT_FAILURE);

        // Attesa del processo di processamento
        waitpid(process_pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) exit(EXIT_FAILURE);

        // Attesa del processo di scrittura
        waitpid(write_pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) exit(EXIT_FAILURE);
    } else {
        // Modalità single process
        tabulate_single_process(word_frequencies, input_file, output_file);
    }

    // Deallocazione della hashmap
    hashmap_destroy(word_frequencies);
}

/**
 * Verifica se un carattere è una punteggiatura non valida.
 *
 * @param character Il carattere da verificare.
 * @return true se il carattere è una punteggiatura non valida, false altrimenti.
 */
bool is_invalid_punctuation(wchar_t character) {
    // Array dei segni di punteggiatura
    const wchar_t punctation[] = {L',', L';', L':', L'-', L'(', L')', L'[', L']', L'{', L'}', L'<', L'>', L'"', L'/', L'\\', L'|', L'@', L'#', L'$', L'%', L'^', L'&', L'*', L'_', L'+', L'=', L'~', L'`'};

    // Scorre l'array
    for (int i = 0; i < sizeof(punctation) / sizeof(wchar_t); i++) {
        // Se il carattere è un segno di punteggiatura, restituisce true
        if (character == punctation[i]) return true;
    }

    // Altrimenti restituisce false
    return false;
}

/**
 * Processa un carattere.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param character Il carattere da processare.
 * @param previous_word La parola precedente.
 * @param current_word La parola corrente.
 * @param index L'indice della parola corrente.
 * @param first_word La prima parola del testo.
 */
void process_character(HashMap *word_frequencies, wchar_t character, wchar_t *previous_word, wchar_t *current_word, int *index, wchar_t *first_word) {
    // Controllo della lunghezza massima della parola
    if (*index == MAX_WORD_LENGTH) error_handler(ERR_INVALID_TEXT);

    if (*index != 0 && (iswblank(character) || character == '\'' || character == '.' || character == '?' || character == '!' || character == '\n' || character == '\0')) {
        // Se la parola corrente è un'apostrofo, viene concatenata alla parola precedente
        if (character == '\'') current_word[(*index)++] = character;
        
        // Terminazione della parola corrente
        current_word[*index] = '\0';

        // Se la parola precedente è non è vuota allora viene effettuato l'inserimento, altrimenti la imposta (e imposta la prima parola come la parola corrente)
        if (wcscmp(previous_word, L"") != 0) {
            // Inserimento della parola corrente nella tabella delle frequenze
            hashmap_insert(word_frequencies, previous_word, current_word);

            // Se il carattere è un segno di punteggiatura, viene inserito nella tabella delle frequenze
            if (character == '.' || character == '?' || character == '!') {
                wcscpy(previous_word, (wchar_t[]){ character, '\0' });
                hashmap_insert(word_frequencies, current_word, previous_word);
            } else {
                wcscpy(previous_word, current_word);
            }
        } else {
            wcscpy(previous_word, current_word);
            wcscpy(first_word, current_word);
        }

        *index = 0;
    } else if (!(is_invalid_punctuation(character) || iswblank(character) || character == '\n' || character == '\0')) {
        // Inserimento del carattere nella parola corrente
        current_word[(*index)++] = towlower(character);
    }
}

/**
 * Scrive una tabella di frequenze su un file CSV.
 *
 * @param word_frequencies La tabella delle frequenze da stampare.
 * @param output_file Il file su cui stampare la tabella delle frequenze.
 */
void hashmap_to_csv(HashMap *word_frequencies, FILE *output_file) {
    // Scorre i bucket della hashmap
    for (int i = 0; i < word_frequencies->size; i++) {
        Entry *entry = word_frequencies->buckets[i];

        // Scorre le entry
        while (entry) {
            // Stampa nel file la parola relativa all'entry
            fwprintf(output_file, L"%ls", entry->word);

            Node *node = entry->next_words;

            // Scorre i nodi
            while (node) {
                // Stampa nel file la parola successiva e la frequenza
                fwprintf(output_file, L",%ls,%.5f", node->next_word, node->frequency);

                // Nodo successivo
                node = node->next;
            }

            // Stampa nel file un carattere di nuova riga
            fwprintf(output_file, L"\n");
            
            // Entry successiva
            entry = entry->next;
        }
    }
}

/*
 * Legge il testo da un file e lo scrive su un pipe.
 *
 * @param input_file Il file di input.
 * @param pipe_fd Il file descriptor del pipe.  
*/
void read_text(FILE *input_file, int pipe_fd[2]) {
    // Chiusura del lato di lettura della pipe
    close(pipe_fd[0]);

    wchar_t character;

    // Lettura dal file e scrittura sulla pipe
    while ((character = fgetwc(input_file)) != WEOF) {
        write(pipe_fd[1], &character, sizeof(character));
    }

    // Ultima iterazione (poiché WEOF non viene processato dal ciclo while )
    character = L'\0';
    write(pipe_fd[1], &character, sizeof(character));

    // Chiusura del lato di scrittura della pipe
    close(pipe_fd[1]);
}

/*
 * Legge il testo da un pipe e lo processa.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param pipe_fd Il file descriptor del pipe.
*/
void process_text(HashMap *word_frequencies, int pipe_fd[2]) {
    // Chisura del lato di scrittura della pipe
    close(pipe_fd[1]);

    wchar_t previous_word[MAX_WORD_LENGTH] = L"";
    wchar_t current_word[MAX_WORD_LENGTH];

    wchar_t first_word[MAX_WORD_LENGTH];

    wchar_t character;
    int index = 0;

    // Lettura dalla pipe e processamento del testo
    while (read(pipe_fd[0], &character, sizeof(character)) > 0) {
        process_character(word_frequencies, character, previous_word, current_word, &index, first_word);
    }

    // L'ultima parola viene collegata alla prima
    hashmap_insert(word_frequencies, previous_word, first_word);

    // Chiusura del lato di lettura della pipe
    close(pipe_fd[0]);
}

/**
 * Converte un file di testo in una tabella di frequenze utilizzando un singolo processo.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param input_file Il file di input.
 * @param output_file Il file di output.
 */
void tabulate_single_process(HashMap *word_frequencies, FILE *input_file, FILE *output_file) {
    wchar_t previous_word[MAX_WORD_LENGTH] = L"";
    wchar_t current_word[MAX_WORD_LENGTH];

    wchar_t first_word[MAX_WORD_LENGTH];

    wchar_t character;
    int index = 0;

    // Lettura dal file e processamento del testo
    while((character = fgetwc(input_file)) != WEOF) {
        process_character(word_frequencies, character, previous_word, current_word, &index, first_word);
    }

    // Ultima iterazione (poiché WEOF non viene processato dal ciclo while )
    character = L'\0';
    process_character(word_frequencies, character, previous_word, current_word, &index, first_word);

    // L'ultima parola viene collegata alla prima
    hashmap_insert(word_frequencies, previous_word, first_word);
    
    // Scrittura della tabella delle frequenze
    hashmap_to_csv(word_frequencies, output_file);
}