# Progetto - Sistemi Operativi II Modulo

Il programma gestisce la realizzazione dei due compiti tabulate e flatten, fornendo un'implementazione a singolo processo e multiprocesso.

## Funzionalità

Il programma realizza fondamentalmente due compiti

### Tabulate

Dato un testo, produce una tabella contentente per ogni parola del testo le parole immediatamente successive e la loro frequenza di occorrenza.

I parametri richiesti sono:

- un file di testo in codifica Unicode (UTF-8) contenente un testo, strutturato in frasi terminate dai caratteri ., ? o ! (gli altri caratteri di punteggiatura saranno ignorati eccetto gli apostrofi).

L'output è un file CSV contenente una tabella in cui ogni riga riporta una parola e le parole immediatamente successive con le loro frequenze.

### Flatten

Genera un testo in maniera casuale usando una tabella di frequenze, nella stessa forma calcolata da tabulate.

I parametri richiesti sono:

- un file di testo in formato CSV contenente una tabella in cui ogni riga riporta una parola e le parole immediatamente successive con le loro frequenze;
- il numero di parole da generare;
- una parola precedente da cui iniziare la generazione (opzionale).

L'output è un file di testo contenente il testo casuale generato.

## Requisiti di Sistema

Il programma richiede i seguenti requisiti di sistema:

- compilatore GCC (GNU Compiler Collection);
- libreria GNU Scientific Library (GSL).

## Installazione

L'installazione può essere completata con un semplice make

```bash
make
```

Se si desidera reinstallare il programma

```bash
make remake
```

Per eliminare i file oggetto

```bash
make clean
```

Per eliminare il file eseguibile

```bash
make cleaner
```

## Utilizzo

Per eseguire il comando tabulate inserire nel terminale

```bash
./bin/program tabulate input_file
```

Invece, per il compito flatten

```bash
./bin/program flatten input_file words_to_generate -w previous_word
```

Eseguire il programma con l'opzione di aiuto per ricevere maggiori informazioni

```bash
./bin/program -h
```


