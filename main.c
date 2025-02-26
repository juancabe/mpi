#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include <mpi.h>


#define NUM_MAX 100000000
#define TAMANHO_LISTA 80
#define SETS_LISTA 3

#define SETS  3
#define TAMANHO  50

// Definición de tags para la comunicación
#define TAG_FOUND 100
#define TAG_STOP  101

int pID = 0;

unsigned int numeros[SETS_LISTA][TAMANHO_LISTA]= {
{8,66,748,1158,10231,497134,4608707,47820120,2,41,578,1423,52611,770895,2790721,34643109,
8,10,851,2209,69990,159627,3385804,15821867,5,13,692,1378,20023,820983,5989397,71761937,
6,62,127,8986,36126,971985,5739524,70900793,3,43,692,6066,60555,844783,3509650,93991716,
1,56,859,2407,41146,288994,9565675,70779739,9,47,876,2456,20591,920964,4403757,68563587,
2,21,355,7035,58257,956057,2244822,98602552,2,90,306,2971,95222,814239,7627087,54434251}
,
{4,77,886,7564,50085,993399,4874938,74030116,9,23,737,3708,67607,956008,6248183,43669844,
7,50,684,5893,69898,200014,2136757,91838319,8,75,877,2507,65557,559730,8465332,75839092,
1,94,144,2057,61882,864629,8394357,80773957,9,74,224,1417,21923,362955,8884601,49938426,
9,15,573,2464,66176,422314,9860085,88384300,8,55,153,2757,54958,912936,5326961,30264300,
5,69,299,6618,10463,556147,8711726,95397660,3,29,684,5303,58626,364864,8198828,80756457}
,
{1,44,763,6169,67524,919717,1473763,13587296,5,71,569,1163,11550,281387,8255291,93701450,
5,52,302,4630,61202,218275,7011910,75292541,1,91,505,2139,65462,978520,9179619,24758500,
9,41,901,5923,31806,660136,6392446,16262444,8,40,543,2759,57699,282161,6890176,39677629,
8,65,247,6476,88372,558802,7420916,28514707,5,62,103,2025,29201,719487,6806547,97055905,
1,11,717,6131,75383,936184,4064483,14741509,5,87,136,3076,60375,528071,8505013,67316720}
};

double mygettime(void) {
    struct timeval tv;
    if(gettimeofday(&tv, 0) < 0) {
        perror("oops");
    }
    return (double)tv.tv_sec + (0.000001 * (double)tv.tv_usec);
}

static unsigned int next = 1;

#define RAND_MAX 2147483647

int myrand(void){
    next = next * 1103515245 + 12345;
    return((unsigned)(next % RAND_MAX));
}

void mysrand(unsigned int seed){
    next = seed;
}
void busca_numero(unsigned int numero, unsigned long long * intentos, double * tpo, int encontrado[]){
    short bSalir=0;
    double t1,t2;
    *intentos=0;
    t1=mygettime();
    MPI_Status status;
    int i = 0;
    int flag = 0;
    int parar = 0;
    do{
        (*intentos)++;
        if ((myrand()%(NUM_MAX-1)+1)==numero){//Numero encontrado
            bSalir=1;
            if (pID != 0) {
                MPI_Send(&pID, 1, MPI_INT, 0, TAG_FOUND, MPI_COMM_WORLD);
            }
        }
        if (pID == 0) { //Comprobar si alguien lo a encontrado
            MPI_Iprobe(MPI_ANY_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &flag, &status);
            while (flag) {
                int mensajero;
                MPI_Recv(&mensajero, 1, MPI_INT, status.MPI_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &status);
                if (i == 0){
                    encontrado[mensajero] = 2;
                }else{
                    encontrado[mensajero]= 1;
                }
                bSalir = 1;
                i++;
                MPI_Iprobe(MPI_ANY_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &flag, &status);
            }
        }else{ //Comprobar si hay que para
            MPI_Iprobe(0, TAG_STOP, MPI_COMM_WORLD, &parar, &status);
            if (parar) {
                int dummy;
                MPI_Recv(&dummy, 1, MPI_INT, 0, TAG_STOP, MPI_COMM_WORLD, &status);
                bSalir = 1;
            }
        }
        
    } while (!bSalir);
    t2=mygettime();
    *tpo=t2-t1;
}


int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);

    int iNP, numero;
    unsigned long long intentos_totales = 0;
    double tpo_total = 0.0;
    int a;

    MPI_Comm_size(MPI_COMM_WORLD, &iNP);
    MPI_Comm_rank(MPI_COMM_WORLD, &pID); 

    int encontrado[iNP];
    for(int i = 0; i < iNP; i++){
        encontrado[i] = -1;
    }

    mysrand(pID);

    for (int i=0; i < SETS_LISTA; i++){
        if (pID == 0)
            printf("SET: %d\n", i);
        for (int j=0; j < TAMANHO; j++){
            if (pID == 0)
                numero=numeros[i][j];
                // Difunde el número a todos los procesos
                MPI_Bcast(&numero, 1, MPI_INT, 0, MPI_COMM_WORLD);

            unsigned long long intentos = 0;
            double tpo = 0.0;
            busca_numero(numero, &intentos, &tpo, encontrado);

            int enseñar=-1, tambien[iNP];
            tambien[0]=-1;
            if (pID == 0) {
                //Hay un fallo, el padre le envia al proceso que ha encontrado el numero tambien que pare, esto esta mal, ha que arreglarlo
                int b=0;
                for (int k = 1; k < iNP; k++) {
                    if (1 != encontrado[k] && 2 != encontrado[k]){
                        int siguiente = 0;
                        MPI_Send(&siguiente, 1, MPI_INT, k, TAG_STOP, MPI_COMM_WORLD);                        
                    }
                    if (1 == encontrado[k]){
                        tambien[b]=k;
                        encontrado[k]=-1;
                        tambien[b+1]=-1;
                        b++;
                    }
                    if (2 == encontrado[k]){
                        encontrado[k]=-1;
                        enseñar = k;
                    }
                }
            }

            unsigned long long sumar_intentos = 0;
            double maximo_tpo = 0.0;
            MPI_Reduce(&intentos, &sumar_intentos, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
            MPI_Reduce(&tpo, &maximo_tpo, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
            
            if (pID == 0) {
                intentos_totales += sumar_intentos;
                tpo_total += maximo_tpo;
                printf("S:%d/%d, N:%2d/%d) Num:%10d, Int:%10llu, Tpo: %.8f (ENCONTRADO POR %d)", i+1, SETS_LISTA, j+1, TAMANHO, numero, sumar_intentos, maximo_tpo, (enseñar == -1) ? 0 : enseñar);

                a=0;
                while (tambien[a] != -1){
                    if (a == 0){
                        printf(" tambien encontardo por %d", tambien[a]);
                    }else{
                        printf(", %d", tambien[a]);
                    }
                    tambien[a]=-1;
                    a++;
                } 
                printf("\n");
            }
            MPI_Barrier(MPI_COMM_WORLD);
            }
            if (pID == 0)
                printf("Resumen SET %d: Intentos Acumulados: %llu, Tiempo Acumulado: %.8f\n", i, intentos_totales, tpo_total);
            MPI_Barrier(MPI_COMM_WORLD);
        }

    MPI_Finalize();
    return 0;
}
