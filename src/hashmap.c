#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdbool.h>
#include <math.h>

#include "hashmap.h"
#include "error_handler.h"

/**
 * Dimensione iniziale della hashmap.
 */
#define INITIAL_SIZE 31

/**
 * Calcola l'hash di una stringa.
 *
 * @param string La stringa di cui calcolare l'hash.
 * @param size La dimensione della hashmap.
 * @return L'hash della stringa.
 */
unsigned int hash(wchar_t *string, int size) {
    unsigned int hash = 0;

    for (int i = 0; string[i] != '\0'; i++) {
        hash = (hash * 31) + string[i];
    }

    return hash % size;
}

/**
 * Crea un nodo.
 *
 * @param next_word La parola successiva.
 * @param frequency La frequenza della parola successiva.
 * @return Il nodo creato.
 */
Node *create_node(wchar_t *next_word, double frequency) {
    // Allocazione del nodo
    Node *node = (Node *)malloc(sizeof(Node));
    if (!node) error_handler(ERR_MEMORY_ALLOCATION);

    // Inizializzazione del nodo
    wcscpy(node->next_word, next_word);
    node->frequency = frequency;
    node->next = NULL;

    // Restituzione del nodo
    return node;
}

/**
 * Crea una entry.
 *
 * @param word La parola.
 * @return L'entry creata.
 */
Entry *crate_entry(wchar_t *word) {
    // Allocazione dell'entry
    Entry *entry = (Entry *)malloc(sizeof(Entry));
    if (!entry) error_handler(ERR_MEMORY_ALLOCATION);

    // Inizializzazione dell'entry
    wcscpy(entry->word, word);
    entry->next_words = NULL;
    entry->size = 0;
    entry->next = NULL;

    // Restituzione dell'entry
    return entry;
}

/**
 * Crea una nuova hashmap.
 *
 * @return La hashmap creata.
 */
HashMap *hashmap_create() {
    // Allocazione della hashmap
    HashMap *map = (HashMap *)malloc(sizeof(HashMap));
    if (!map) error_handler(ERR_MEMORY_ALLOCATION);

    // Allocazione dei bucket
    map->buckets = (Entry **)calloc(INITIAL_SIZE, sizeof(Entry *));
    if (!map->buckets) error_handler(ERR_MEMORY_ALLOCATION);

    // Dimensione iniziale
    map->size = INITIAL_SIZE;

    // Restituzione della hashmap
    return map;
}

/**
 * Ridimensiona una hashmap.
 *
 * @param map La hashmap da ridimensionare.
 */
void hashmap_resize(HashMap *map) {
    // Nuova dimensione
    size_t new_size = map->size * 2;

    // Nuovi bucket
    Entry **new_buckets = (Entry **)calloc(new_size, sizeof(Entry *));
    if (!new_buckets) error_handler(ERR_MEMORY_ALLOCATION);
    
    // Scorrimento dei bucket
    for (int i = 0; i < map->size; i++) {
        Entry *entry = map->buckets[i];

        // Scorrimento delle entry
        while (entry) {
            // Salvataggio dell'entry successiva
            Entry *next_entry = entry->next;

            // Calcolo del nuovo indice
            unsigned int index = hash(entry->word, new_size);

            // Inserimento dell'entry
            entry->next = new_buckets[index];
            new_buckets[index] = entry;

            // Entry successiva
            entry = next_entry;
        }
    }

    // Deallocazione dei vecchi bucket
    free(map->buckets);

    // Aggiornamento della hashmap
    map->buckets = new_buckets;
    map->size = new_size;
}

/**
 * Distrugge una hashmap.
 *
 * @param map La hashmap da distruggere.
 */
void hashmap_destroy(HashMap *map) {
    // Scorrimento dei bucket
    for (int i = 0; i < map->size; i++) {
        Entry *entry = map->buckets[i];

        // Scorrimento delle entry
        while (entry) {
            Node *node = entry->next_words;

            // Scorrimento dei nodi
            while (node) {
                Node *next_node = node->next;

                // Deallocazione del nodo
                free(node);

                node = next_node;
            }

            Entry *next_entry = entry->next;
            
            // Deallocazione dell'entry
            free(entry);

            entry = next_entry;
        }
    }

    // Deallocazione dei bucket
    free(map->buckets);

    // Deallocazione della hashmap
    free(map);
}

/**
 * Inserisce un nodo.
 *
 * @param head La testa della lista.
 * @param next_word La parola successiva.
 * @param frequency La frequenza della parola successiva.
 * @return La nuova testa della lista.
 */
Node *hashmap_insert_node(Node *head, wchar_t *next_word, double frequency) {
    // Viene creato il nodo
    Node *node = create_node(next_word, frequency);

    // Viene inserito il nodo
    node->next = head;
    head = node;

    // Viene restituita la nuova testa
    return head;
}

/**
 * Inserisce una entry.
 *
 * @param head La testa della lista.
 * @param word La parola.
 * @return La nuova testa della lista.
 */
Entry *hashmap_insert_entry(Entry *head, wchar_t *word) {
    // Viene creata l'entry
    Entry *entry = crate_entry(word);

    // Viene inserita l'entry
    entry->next = head;
    head = entry;

    // Viene restituita la nuova testa
    return head;
}

/**
 * Inserisce un nodo in una hashmap.
 *
 * @param map La hashmap.
 * @param word La parola.
 * @param next_word La parola successiva.
 */
void hashmap_insert(HashMap *map, wchar_t *word, wchar_t *next_word) {
    // Viene calcolato l'hash della parola
    unsigned int index = hash(word, map->size);

    // Viene cercata l'entry della parola
    Entry *entry = map->buckets[index];
    
    // Scorrimento delle entry
    while (entry) {
        if (wcscmp(entry->word, word) == 0) {
            // Viene incrementato il numero di occorrenze della parola
            entry->size++;

            Node *node = entry->next_words;
            bool found = false;

            // Scorrimento dei nodi
            while (node) {
                // Viene calcolato il numero di occorrenze della parola successiva
                int frequency = (int)(round(node->frequency * (entry->size - 1)));
                
                // Se esiste un nodo per la parola successiva, viene incrementato il numero di occorrenze
                if (wcscmp(node->next_word, next_word) == 0) {
                    frequency++;
                    found = true;
                }

                // Viene aggiornata la frequenza del nodo
                node->frequency = (double)frequency / entry->size;
                
                // Nodo successivo
                node = node->next;
            }

            // Se non esiste un nodo per la parola successiva, viene creato e inserito
            if (!found) entry->next_words = hashmap_insert_node(entry->next_words, next_word, 1.0 / entry->size);

            // Esce
            return;
        }

        // Entry successiva
        entry = entry->next;
    }

    // Viene incrementato il numero di entry della hashmap
    map->usage++;

    // Viene calcolato il fattore di carico
    double load_factor = (double)map->usage / map->size;

    // Se il fattore di carico supera il 75%, la hashmap viene ridimensionata
    if (load_factor > 0.75) hashmap_resize(map);
    
    // Se non esiste un'entry per la parola, viene creata
    map->buckets[index] = hashmap_insert_entry(map->buckets[index], word);

    // Viene inserito il nodo per la parola successiva
    map->buckets[index]->next_words = hashmap_insert_node(map->buckets[index]->next_words, next_word, 1.0);
    map->buckets[index]->size++;
}

/**
 * Restituisce la entry di una parola.
 *
 * @param map La hashmap.
 * @param word La parola di cui cercare l'entry.
 * @return L'entry della parola.
 */
Entry *hashmap_get(HashMap *map, wchar_t *word) {
    // Viene calcolato l'indice della parola
    unsigned int index = hash(word, map->size);

    // Viene cercata l'entry della parola
    Entry *entry = map->buckets[index];

    // Scorrimento delle entry
    while (entry) {
        // Se l'entry è stata trovata, viene restituita
        if (wcscmp(entry->word, word) == 0) return entry;

        // Entry successiva
        entry = entry->next;
    }

    // Se l'entry non è stata trovata, viene restituita NULL
    return NULL;
}

/**
 * Converte una hashmap in un buffer.
 *
 * @param map La hashmap da convertire.
 * @return Il buffer convertito.
 */
wchar_t *hashmap_serialize(HashMap *map) {
    // Dimensione del buffer (inizializzata a 1 per il carattere di fine stringa)
    size_t buffer_size = sizeof(wchar_t);

    // Scorrimento dei bucket
    for (int i = 0; i < map->size; i++) {
        Entry *entry = map->buckets[i];

        // Scorrimento delle entry
        while (entry) {
            // Dimensione dell'entry
            size_t entry_size = sizeof(wchar_t) * wcslen(entry->word);

            // Aggiornamento della dimensione del buffer
            buffer_size += entry_size;

            Node *node = entry->next_words;

            while (node) {
                // Dimensione della parola successiva
                size_t next_word_size = sizeof(wchar_t) * wcslen(node->next_word);

                // Dimensione della frequenza
                size_t frequency_size = sizeof(wchar_t) * 8;

                // Aggiornamento della dimensione del buffer con la parola successiva, la frequenza e due spazi
                buffer_size += next_word_size + frequency_size + sizeof(wchar_t) * 2;

                // Nodo successivo
                node = node->next;
            }

            // Carattere di a capo
            buffer_size += sizeof(wchar_t);

            // Entry successiva
            entry = entry->next;
        }
    }

    // Allocazione del buffer
    wchar_t *buffer = (wchar_t *)malloc(buffer_size);
    if (!buffer) error_handler(ERR_MEMORY_ALLOCATION);
    
    // Offset
    size_t offset = 0;

    // Scorrimento dei bucket
    for (int i = 0; i < map->size; i++) {
        Entry *entry = map->buckets[i];

        // Scorrimento delle entry
        while (entry) {
            // Copia la parola nell'entry
            size_t entry_size = wcslen(entry->word);
            wmemcpy(buffer + offset, entry->word, entry_size);
        
            // Incrementa l'offset della dimensione dell'entry
            offset += entry_size;

            Node *node = entry->next_words;

            // Scorrimento dei nodi
            while (node) {
                // Carattere di spazio
                buffer[offset++] = L' ';

                // Copia la parola successiva
                size_t next_word_size = wcslen(node->next_word);
                wmemcpy(buffer + offset, node->next_word, next_word_size);

                // Incrementa l'offset della dimensione della parola successiva
                offset += next_word_size;

                // Carattere di spazio
                buffer[offset++] = L' ';

                // Buffer per la frequenza
                wchar_t frequency[9];

                // Conversione della frequenza in stringa
                swprintf(frequency, 9, L"%f", node->frequency);

                // Copia la frequenza
                size_t frequency_size = wcslen(frequency);
                wmemcpy(buffer + offset, frequency, frequency_size);
                
                // Incrementa l'offset della dimensione della frequenza
                offset += frequency_size;

                // Nodo successivo
                node = node->next;
            }

            // Carattere di a capo
            buffer[offset++] = L'\n';
            
            // Entry successiva
            entry = entry->next;
        }
    }

    // Carattere di fine stringa
    buffer[offset] = L'\0';

    // Viene restituito il buffer
    return buffer;
}

/**
 * Carica una hashmap da un buffer.
 *
 * @param map La hashmap da caricare.
 * @param buffer Il buffer da cui caricare la hashmap.
 */
 void hashmap_deserialize(HashMap *map, wchar_t *buffer) {
    // Offset
    size_t offset = 0;

    // Scorrimento del buffer
    while (buffer[offset] != '\0') {
        // Indice delle stringhe
        size_t index = 0;

        // Viene incrementato il numero di entry della hashmap
        map->usage++;

        // Viene calcolato il fattore di carico
        double load_factor = (double)map->usage / map->size;

        // Se il fattore di carico supera il 75%, la hashmap viene ridimensionata
        if (load_factor > 0.75) hashmap_resize(map);
        
        // Parola
        wchar_t word[MAX_WORD_LENGTH];

        // Legge la parola
        while (buffer[offset] != L' ') {
            word[index] = buffer[offset];

            // Viene incrementato l'indice e l'offset
            index++;
            offset++;
        }

        // Viene aggiunto il carattere di fine stringa
        word[index] = L'\0';

        // Viene incrementato l'offset del carattere di spazio
        offset++;

        // Viene calcolato l'hash della parola
        unsigned int hash_value = hash(word, map->size);

        // Viene inserita l'entry
        map->buckets[hash_value] = hashmap_insert_entry(map->buckets[hash_value], word);

        while (buffer[offset] != '\n') {
            // Reset dell'indice
            index = 0;

            // Parola successiva
            wchar_t next_word[MAX_WORD_LENGTH];

            // Legge la parola successiva
            while (buffer[offset] != L' ') {
                next_word[index] = buffer[offset];

                // Viene incrementato l'indice e l'offset
                index++;
                offset++;
            }

            // Viene aggiunto il carattere di fine stringa
            next_word[index] = L'\0';

            // Reset dell'indice
            index = 0;

            // Viene incrementato l'offset del carattere di spazio
            offset++;

            // Frequenza
            wchar_t frequency_string[9];

            // Legge la frequenza
            while (buffer[offset] != L' ' && buffer[offset] != L'\n') {
                frequency_string[index] = buffer[offset];

                // Viene incrementato l'indice e l'offset
                index++;
                offset++;
            }  

            // Viene aggiunto il carattere di fine stringa
            frequency_string[index] = L'\0';

            // Viene incrementato l'offset del carattere di spazio
            if (buffer[offset] == ' ') offset++;

            // Viene convertita la frequenza in double
            double frequency_value = wcstod(frequency_string, NULL);

            // Viene inserito il nodo e incrementata la dimensione della lista
            map->buckets[hash_value]->next_words = hashmap_insert_node(map->buckets[hash_value]->next_words, next_word, frequency_value);
            map->buckets[hash_value]->size++;
        }

        // Viene aggiornato l'offset del carattere di a capo
        offset++;
    }
}