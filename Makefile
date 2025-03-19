main: main.c
	mpicc -o main main.c -Wall

run: main
	mpirun -np 8 --map-by :OVERSUBSCRIBE ./main 
