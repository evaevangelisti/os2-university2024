# compiler e linker
CC := gcc

# eseguibile
EXECUTABLE := program

# cartelle
SRCDIR := src
INCDIR := inc
OBJDIR := obj
BINDIR := bin

# flags
CPPFLAGS := -I./$(INCDIR)
CFLAGS := -Wall
LDFLAGS	:= 
LDLIBS := -lgsl -lgslcblas -lm

# file sorgente
SRC := $(wildcard $(SRCDIR)/*.c)

# file oggetto
OBJ := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))

# default
all: $(EXECUTABLE)

# remake
remake: cleaner all

# link
$(EXECUTABLE): $(OBJ) | $(BINDIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $(BINDIR)/$@

# compile
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# crea cartelle
$(BINDIR) $(OBJDIR):
	@mkdir -p $@

# rimuove solo oggetti
clean:
	@$(RM) -rv $(OBJDIR)

# rimuove tutto
cleaner: clean
	@$(RM) -rv $(BINDIR)

# non-file targets
.PHONY: all remake clean cleaner