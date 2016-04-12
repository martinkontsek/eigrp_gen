all:	eigrp_packet.o
	 gcc -Wall -pthread eigrp_gen.c eigrp_packet.o -o eigrp_gen


eigrp_packet.o: eigrp_packet.c eigrp_packet.h
	gcc -Wall -c eigrp_packet.c

clean:
	rm eigrp_packet.o
	rm eigrp_gen

run:
	./eigrp_gen

