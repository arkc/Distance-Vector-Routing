#all title to indicate all main files in a program
all: Dist_Vect
Load: Dist_Vect.c
	gcc -o Dist_Vect Dist_Vect.c
Clean: 
	rm -f Dist_Vect *~core
