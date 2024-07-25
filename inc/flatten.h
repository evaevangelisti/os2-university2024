#ifndef FLATTEN_H
#define FLATTEN_H

#include <stdbool.h>

#include "hashmap.h"

/**
 * Genera un testo casuale a partire da una tabella di frequenze.
 *
 * @param input_file Il file di input.
 * @param words_to_generate Il numero di parole da generare.
 * @param previous_word La parola precedente.
 * @param output_file Il file di output.
 * @param multiprocess_mode La modalità multiprocessore.
 */
void flatten(FILE *input_file, int words_to_generate, wchar_t *previous_word, FILE *output_file, bool multiprocess_mode);

#endif