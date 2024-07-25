#ifndef HASHMAP_H
#define HASHMAP_H

#include "constants.h"

/*
 * Struttura che rappresenta un nodo.
 */
typedef struct Node {
    wchar_t next_word[MAX_WORD_LENGTH];
    double frequency;
    struct Node *next;
} Node;

/**
 * Struttura che rappresenta una entry.
 */
typedef struct Entry {
    wchar_t word[MAX_WORD_LENGTH];
    Node *next_words;
    size_t size;
    struct Entry *next;
} Entry;

/**
 * Struttura che rappresenta una hashmap.
 */
typedef struct {
    Entry **buckets;
    size_t usage;
    size_t size;
} HashMap;

/**
 * Calcola l'hash di una stringa.
 *
 * @param string La stringa di cui calcolare l'hash.
 * @param size La dimensione della hashmap.
 * @return L'hash della stringa.
 */
unsigned int hash(wchar_t *string, int size);

/**
 * Crea una nuova hashmap.
 *
 * @return La hashmap creata.
 */
HashMap *hashmap_create();

/**
 * Ridimensiona una hashmap.
 *
 * @param map La hashmap da ridimensionare.
 */
void hashmap_resize(HashMap *map);

/**
 * Distrugge una hashmap.
 *
 * @param map La hashmap da distruggere.
 */
void hashmap_destroy(HashMap *map);

/**
 * Inserisce un nodo.
 *
 * @param head La testa della lista.
 * @param next_word La parola successiva.
 * @param frequency La frequenza della parola successiva.
 * @return La nuova testa della lista.
 */
Node *hashmap_insert_node(Node *head, wchar_t *next_word, double frequency);

/**
 * Inserisce una entry.
 *
 * @param head La testa della lista.
 * @param word La parola.
 * @return La nuova testa della lista.
 */
Entry *hashmap_insert_entry(Entry *head, wchar_t *word);

/**
 * Inserisce un nodo in una hashmap.
 *
 * @param map La hashmap.
 * @param word La parola.
 * @param next_word La parola successiva.
 */
void hashmap_insert(HashMap *map, wchar_t *word, wchar_t *next_word);

/**
 * Restituisce la entry di una parola.
 *
 * @param map La hashmap.
 * @param word La parola di cui cercare l'entry.
 * @return L'entry della parola.
 */
Entry *hashmap_get(HashMap *map, wchar_t *word);

/**
 * Converte una hashmap in un buffer.
 *
 * @param map La hashmap da convertire.
 * @return Il buffer convertito.
 */
wchar_t *hashmap_serialize(HashMap *map);

/**
 * Carica una hashmap da un buffer.
 *
 * @param map La hashmap da caricare.
 * @param buffer Il buffer da cui caricare la hashmap.
 */
 void hashmap_deserialize(HashMap *map, wchar_t *buffer);

#endif