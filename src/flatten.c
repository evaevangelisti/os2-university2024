#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>
#include <time.h> 
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <sys/wait.h>
#include <errno.h>

#include "flatten.h"
#include "hashmap.h"
#include "error_handler.h"
#include "constants.h"

extern int errno;

/**
 * Parsa una stringa.
 *
 * @param string La stringa da parsare.
 */
void parse_string(wchar_t *string);

/**
 * Processa una cella.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param string La stringa da processare.
 * @param hash_value Il valore hash.
 * @param next_word La prossima parola.
 * @param sum La somma delle frequenze.
 * @param node_counter Il contatore dei nodi.
 */
void process_cell(HashMap *word_frequencies, wchar_t *string, unsigned int *hash_value, wchar_t *next_word, double *sum, int *node_counter);

/**
 * Restituisce una stringa casuale.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param strings Le stringhe.
 * @param size La dimensione.
 * @return La stringa casuale.
 */
wchar_t *get_random_string(HashMap *word_frequencies, wchar_t *strings[], size_t size);

/**
 * Scrive un testo casuale.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param words_to_generate Il numero di parole da generare.
 * @param previous_word La parola precedente.
 * @param output_file Il file di output.
 */
void write_random_text(HashMap *word_frequencies, int words_to_generate, wchar_t *previous_word, FILE *output_file);

/**
 * Legge una tabella.
 *
 * @param input_file Il file di input.
 * @param pipe_fd Il descrittore della pipe.
 */
void read_table(FILE *input_file, int pipe_fd[2]);

/**
 * Processa una tabella.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param pipe_fd Il descrittore della pipe.
 */
void process_table(HashMap *word_frequencies, int pipe_fd[2]);

/**
 * Genera un testo casuale a partire da una tabella di frequenze utilizzando un singolo processo.
 *
 * @param input_file Il file di input.
 * @param words_to_generate Il numero di parole da generare.
 * @param previous_word La parola precedente.
 * @param output_file Il file di output.
 */
void flatten_single_process(HashMap *word_frequencies, FILE *input_file, int words_to_generate, wchar_t *previous_word, FILE *output_file);

/**
 * Genera un testo casuale a partire da una tabella di frequenze.
 *
 * @param input_file Il file di input.
 * @param words_to_generate Il numero di parole da generare.
 * @param previous_word La parola precedente.
 * @param output_file Il file di output.
 * @param multiprocess_mode La modalità multiprocessore.
 */
void flatten(FILE *input_file, int words_to_generate, wchar_t *previous_word, FILE *output_file, bool multiprocess_mode) {
    // Creazione della hashmap
    HashMap *word_frequencies = hashmap_create();

    if (multiprocess_mode) {
        // Modalità multiprocess

        // Creazione della pipe (pipe_fd[0] per la lettura del file di input, pipe_fd[1] per la scrittura su file di output)
        int pipe_fd[2][2];
        if (pipe(pipe_fd[0]) == -1 || pipe(pipe_fd[1]) == -1) error_handler(ERR_PARALLELIZATION);

        // Creazione di un processo per la lettura della tabella
        pid_t read_pid = fork();
        if (read_pid == 0) {
            // Chiusura dei file descriptor della pipe di scrittura su file di output
            close(pipe_fd[1][0]);
            close(pipe_fd[1][1]);

            // Lettura della tabella e scrittura nella pipe
            read_table(input_file, pipe_fd[0]);

            // Fine del processo di lettura
            exit(EXIT_SUCCESS);
        } else if (read_pid == -1) {
            error_handler(ERR_PARALLELIZATION);
        }

        // Creazione di un processo per il processamento della tabella
        pid_t process_pid = fork();
        if (process_pid == 0) {
            // Chisura del lato di lettura della pipe di scrittura su file di output
            close(pipe_fd[1][0]);

            // Processamento della tabella
            process_table(word_frequencies, pipe_fd[0]);

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

            // Inizializza l'ambiente per i generatori di numeri casuali
            srand(time(NULL));

            // Se la parola precedente non è stata specificata, viene scelta casualmente tra i segni di punteggiatura; altrimenti verifica se è presente nella tabella delle frequenze
            if (wcscmp(previous_word, L"") == 0) {
                wchar_t *punctation_marks[] = { L".", L"?", L"!" };
                wcscpy(previous_word, get_random_string(word_frequencies, punctation_marks, 3));
            } else {
                if (!hashmap_get(word_frequencies, previous_word)) argument_error_handler(ERR_INVALID_OPTION_ARGUMENT, "-w");
            }

            // Scrittura del testo casuale
            write_random_text(word_frequencies, words_to_generate, previous_word, output_file);

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
        flatten_single_process(word_frequencies, input_file, words_to_generate, previous_word, output_file);
    }

    // Deallocazione della hashmap
    hashmap_destroy(word_frequencies);
}

/**
 * Parsa una stringa.
 *
 * @param string La stringa da parsare.
 */
void parse_string(wchar_t *string) {
    wchar_t *source = string;
    wchar_t *destination = string;

    // Scorrimento della stringa
    while (*source) {   
        // Se il carattere non è uno spazio, viene copiato nella stringa di destinazione
        if (*source != L' ') {
            *destination++ = *source;
        }

        // Carattere successivo
        source++;
    }

    // Terminazione della stringa di destinazione
    *destination = L'\0';

    // Se l'ultima parola termina con un a capo, viene sostituito con il terminatore di stringa
    if (destination > string && destination[-1] == L'\n') {
        destination[-1] = L'\0';
    }
}

/**
 * Processa una cella.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param string La stringa da processare.
 * @param hash_value Il valore hash.
 * @param next_word La prossima parola.
 * @param sum La somma delle frequenze.
 * @param node_counter Il contatore dei nodi.
 */
void process_cell(HashMap *word_frequencies, wchar_t *string, unsigned int *hash_value, wchar_t *next_word, double *sum, int *node_counter) {
    // Parsa la stringa
    parse_string(string);

    switch (*node_counter) {
        case 0:
            // Viene incrementato il numero di entry della hashmap
            word_frequencies->usage++;

            // Viene calcolato il fattore di carico
            double load_factor = (double)word_frequencies->usage / word_frequencies->size;

            // Se il fattore di carico supera il 75%, la hashmap viene ridimensionata
            if (load_factor > 0.75) hashmap_resize(word_frequencies);

            // Calcola l'hash
            *hash_value = hash(string, word_frequencies->size);

            // Inserisce la parola nella tabella
            word_frequencies->buckets[*hash_value] = hashmap_insert_entry(word_frequencies->buckets[*hash_value], string);
            break;

        case 1:
            // Copia la parola successiva
            wcscpy(next_word, string);
            break;

        case 2:
            // Resetta errno
            errno = 0;

            // Converte la stringa in double
            double frequency = wcstod(string, NULL);

            // Se la conversione non è andata a buon fine o la frequenza non è compresa tra 0 e 1, errore
            if (errno == ERANGE || frequency < 0 || frequency > 1) error_handler(ERR_INVALID_TABLE);

            // Inserisce la parola successiva nella lista delle parole successive e incrementa la dimensione della lista
            word_frequencies->buckets[*hash_value]->next_words = hashmap_insert_node(word_frequencies->buckets[*hash_value]->next_words, next_word, frequency);
            word_frequencies->buckets[*hash_value]->size++;

            // Incrementa la somma delle frequenze e resetta il contatore dei nodi
            *sum += frequency;
            *node_counter = 0;
            break;

        default:   
            // Errore
            error_handler(ERR_INTERNAL_ERROR);
    }
}

/**
 * Restituisce una stringa casuale.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param strings Le stringhe.
 * @param size La dimensione.
 * @return La stringa casuale.
 */
wchar_t *get_random_string(HashMap *word_frequencies, wchar_t *strings[], size_t size) {
    // Inizializza l'ambiente per i generatori di numeri casuali
    srand(time(NULL));

    // Scorre l'array delle stringhe
    while (size > 0) {
        // Genera un indice casuale
        int random = rand() % size;

        // Se la parola è presente nella tabella delle frequenze, la restituisce
        if (hashmap_get(word_frequencies, strings[random])) return strings[random];

        // Rimuove la parola dall'array
        for (int i = random; i < size - 1; i++) {
            strings[i] = strings[i + 1];
        }

        // Decrementa la dimensione dell'array
        size--;
    }

    // Se nessuna parola è presente nella tabella delle frequenze, restituisce una stringa vuota
    return L"";
}

/**
 * Scrive un testo casuale.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param words_to_generate Il numero di parole da generare.
 * @param previous_word La parola precedente.
 * @param output_file Il file di output.
 */
void write_random_text(HashMap *word_frequencies, int words_to_generate, wchar_t *previous_word, FILE *output_file) {
    // Inizializza l'ambiente per i generatori di numeri casuali
    gsl_rng_env_setup();

    // Inizializza il generatore di numeri casuali
    gsl_rng *r = gsl_rng_alloc(gsl_rng_default);
    gsl_rng_set(r, time(NULL));

    wchar_t word[MAX_WORD_LENGTH];
    bool first_iteration = true;

    for (int i = 0; i < words_to_generate; i++) {
        // Prende la entry della parola precedente
        Entry *entry = hashmap_get(word_frequencies, previous_word);

        // Array delle frequenze con dimensione uguale alla dimensione della lista delle parole successive
        double frequencies[entry->size];
        size_t size = 0;
        
        Node *next_words = entry->next_words;
        
        // Scorre la lista delle parole successive
        while (next_words) {
            // Popola l'array delle frequenze
            frequencies[size++] = next_words->frequency;
            next_words = next_words->next;
        }

        // Prende un indice casuale
        gsl_ran_discrete_t *discrete_distribution = gsl_ran_discrete_preproc(size, frequencies);
        unsigned int index = gsl_ran_discrete(r, discrete_distribution);

        next_words = entry->next_words;
        
        // Scorre la lista delle parole successive
        for (int i = 0; i < entry->size; i++) {
            // Se l'indice corrisponde alla parola, la copia ed esce dal ciclo
            if (i == index) {
                wcscpy(word, next_words->next_word);
                break;
            }

            // Passa alla parola successiva
            next_words = next_words->next;
        }
        
        // Se la parola precedente è un segno di punteggiatura, la parola successiva inizia con una lettera maiuscola
        if (wcscmp(previous_word, L".") == 0 || wcscmp(previous_word, L"?") == 0 || wcscmp(previous_word, L"!") == 0) {
            wcscpy(previous_word, word);
            word[0] = towupper(word[0]);
        } else {
            wcscpy(previous_word, word);
        }
        
        // Stampa uno spazio tra le parole, tranne che per i segni di punteggiatura
        if (wcscmp(word, L".") != 0 && wcscmp(word, L"?") != 0 && wcscmp(word, L"!") != 0 && !first_iteration) fwprintf(output_file, L" ");
        
        // Stampa la parola
        fwprintf(output_file, L"%ls", word);

        // Imposta la first_iterarion a false dopo la prima iterazione
        if (first_iteration) first_iteration = false;

        // Deallocazione della distribuzione discreta
        gsl_ran_discrete_free(discrete_distribution);
    }
    
    // Deallocazione del generatore di numeri casuali
    gsl_rng_free(r);
}

/**
 * Legge una tabella.
 *
 * @param input_file Il file di input.
 * @param pipe_fd Il descrittore della pipe.
 */
void read_table(FILE *input_file, int pipe_fd[2]) {
    // Chiusura del lato di lettura della pipe
    close(pipe_fd[0]);

    wchar_t character;

    // Lettura dal file e scrittura sulla pipe
    while ((character = fgetwc(input_file)) != WEOF) {
        write(pipe_fd[1], &character, sizeof(character));
    }

    // Chiusura del lato di scrittura della pipe
    close(pipe_fd[1]);
}

/**
 * Processa una tabella.
 *
 * @param word_frequencies La tabella delle frequenze.
 * @param pipe_fd Il descrittore della pipe.
 */
void process_table(HashMap *word_frequencies, int pipe_fd[2]) {
    // Chiusura del lato di scrittura della pipe
    close(pipe_fd[1]);

    wchar_t string[MAX_WORD_LENGTH];
    unsigned int hash_value;
    wchar_t next_word[MAX_WORD_LENGTH];

    double sum = 0;
    int node_counter = 0;

    wchar_t character;
    int index = 0;

    // Lettura dalla pipe e processamento del testo
    while (read(pipe_fd[0], &character, sizeof(character)) > 0) {
        switch (character) {
            case L',':
                // Termina la stringa
                string[index] = '\0';

                // Processa la cella
                process_cell(word_frequencies, string, &hash_value, next_word, &sum, &node_counter);

                // Incrementa il contatore dei nodi
                node_counter++;

                // Resetta l'indice
                index = 0;
                break;

            case L'\n':
                // Termina la stringa
                string[index] = '\0';

                // Processa la cella
                process_cell(word_frequencies, string, &hash_value, next_word, &sum, &node_counter);

                // Se la somma delle frequenze non è 1, errore
                if (round(sum) != 1) error_handler(ERR_INVALID_TABLE); 

                // Resetta la somma e il contatore dei nodi
                sum = 0;
                node_counter = 0;

                // Resetta l'indice
                index = 0;
                break;

            // Ignora gli spazi
            case L' ':
                break;

            default:
                // Aggiunge il carattere alla stringa
                if (character != L'\0') string[index++] = character;
        }
    }

    // Processamento dell'ultima parola
    if (index > 0) { 
        // Termina la stringa
        string[index] = '\0';

        // Processa la cella
        process_cell(word_frequencies, string, &hash_value, next_word, &sum, &node_counter);
        
        // Se la somma delle frequenze non è 1, errore
        if (round(sum) != 1) error_handler(ERR_INVALID_TABLE); 
    }

    // Chiusura del lato di lettura della pipe
    close(pipe_fd[0]);
}

/**
 * Genera un testo casuale a partire da una tabella di frequenze utilizzando un singolo processo.
 *
 * @param input_file Il file di input.
 * @param words_to_generate Il numero di parole da generare.
 * @param previous_word La parola precedente.
 * @param output_file Il file di output.
 */
void flatten_single_process(HashMap *word_frequencies, FILE *input_file, int words_to_generate, wchar_t *previous_word, FILE *output_file) {
    wchar_t string[MAX_WORD_LENGTH];
    unsigned int hash_value;
    wchar_t next_word[MAX_WORD_LENGTH];
    double sum = 0;
    int node_counter = 0;
    
    wchar_t character;
    int index = 0;

    // Lettura dal file e processamento del testo
    while((character = fgetwc(input_file)) != WEOF) {
        switch (character) {
            case L',':
                // Termina la stringa
                string[index] = '\0';

                // Processa la cella
                process_cell(word_frequencies, string, &hash_value, next_word, &sum, &node_counter);

                // Incrementa il contatore dei nodi
                node_counter++;

                // Resetta l'indice
                index = 0;
                break;

            case L'\n':
                // Termina la stringa
                string[index] = '\0';

                // Processa la cella
                process_cell(word_frequencies, string, &hash_value, next_word, &sum, &node_counter);

                // Se la somma delle frequenze non è 1, errore
                if (round(sum) != 1) error_handler(ERR_INVALID_TABLE); 

                // Resetta la somma e il contatore dei nodi
                sum = 0;
                node_counter = 0;

                // Resetta l'indice
                index = 0;
                break;

            // Ignora gli spazi
            case L' ':
                break;

            default:   
                // Aggiunge il carattere alla stringa
                string[index++] = character;
        }
    }

    // Processamento dell'ultima parola
    if (index > 0) {
        string[index] = '\0';
        process_cell(word_frequencies, string, &hash_value, next_word, &sum, &node_counter);
        
        // Se la somma delle frequenze non è 1, errore
        if (round(sum) != 1) error_handler(ERR_INVALID_TABLE); 
    }

    srand(time(NULL));

    // Se la parola precedente non è stata specificata, viene scelta casualmente tra i segni di punteggiatura; altrimenti verifica se è presente nella tabella delle frequenze
    if (wcscmp(previous_word, L"") == 0) {
        wchar_t *punctation_marks[] = { L".", L"?", L"!" };
        wcscpy(previous_word, get_random_string(word_frequencies, punctation_marks, 3));
    } else {
        if (!hashmap_get(word_frequencies, previous_word)) argument_error_handler(ERR_INVALID_OPTION_ARGUMENT, "-w");
    }

    // Scrittura del testo casuale
    write_random_text(word_frequencies, words_to_generate, previous_word, output_file);
}