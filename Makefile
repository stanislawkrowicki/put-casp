CFLAGS = -Wall

ifdef DEBUG
CFLAGS += -DDEBUG
endif

TYPESFILE = inf160133_160232_types.c

all: producer client dispatcher

producer:
	gcc $(CFLAGS) inf160133_160232_p.c $(TYPESFILE) -o p.out

client:
	gcc $(CFLAGS) inf160133_160232_c.c $(TYPESFILE) -o c.out

dispatcher:
	gcc $(CFLAGS) inf160133_160232_d.c $(TYPESFILE) -o d.out


clean:
	@rm -f [pcd].out

rm_ipcs:
	ipcrm -q 0
	ipcrm -q 1
	ipcrm -q 2