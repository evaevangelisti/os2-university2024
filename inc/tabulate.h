#ifndef TABULATE_H
#define TABULATE_H

#include <stdbool.h>

#include "hashmap.h"

/**
 * Converte un file di testo in una tabella di frequenze.
 *
 * @param input_file Il file di input.
 * @param output_file Il file di output.
 * @param multiprocess_mode La modalità multiprocessore.
 */
void tabulate(FILE *input_file, FILE *output_file, bool multiprocess_mode);

#endif