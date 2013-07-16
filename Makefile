CFLAGS=-O2

TARG=battstat
OFILES=\
	battstat.o

$(TARG): $(OFILES)
	$(CC) -o $@ $(OFILES) $(LDFLAGS)
clean:
	rm -f $(TARG) *.o
