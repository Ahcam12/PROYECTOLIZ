/*
 * ============================================================
 *  SIMULADOR DE CARRERA F1 - Programacion I (Practico)
 *  Opcion de Proyecto 2
 *  Dra. Estela Lizbeth Munoz Andrade
 * ============================================================
 *
 *  Compilacion Linux/macOS:
 *    gcc f1_simulator.c -o f1_simulator
 *    ./f1_simulator
 *
 *  Compilacion Windows (MinGW):
 *    gcc f1_simulator.c -o f1_simulator.exe
 *    f1_simulator.exe
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- Portabilidad de sleep ---- */
#ifdef _WIN32
  #include <windows.h>
  #define CLEAR "cls"
  #define sleep_ms(ms) Sleep(ms)
#else
  #include <unistd.h>
  #define CLEAR "clear"
  #define sleep_ms(ms) usleep((ms) * 1000)
#endif

/* ---- Colores ANSI ---- */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN    "\033[36m"
#define C_WHITE   "\033[37m"
#define C_BGBLACK "\033[40m"

/* ---- Constantes ---- */
#define MAX_NOMBRE    40
#define MAX_AUTOS     10
#define ANCHO_PISTA   60   /* caracteres de pista */
#define CONFIG_FILE   "config_carrera.txt"
#define LOG_FILE      "resultado_carrera.txt"

/* ====================================================
 *  TIPOS DE DATOS ESTRUCTURADOS
 * ==================================================== */

/* Tipo de vehiculo (para la union) */
typedef enum {
    DEPORTIVO = 'D',
    TODOTERRENO = 'T'
} TipoVehiculo;

/* Estado del auto en carrera */
typedef enum {
    EN_CARRERA,
    ACCIDENTADO,
    FINALIZADO
} EstadoAuto;

/* Especificaciones especiales (UNION) */
typedef union {
    float turbo;      /* Solo para deportivos: factor de aceleracion extra */
    float traccion;   /* Solo para todoterreno: factor de control en lluvia */
} Especificaciones;

/* Estructura principal del Auto */
typedef struct {
    int            id;
    char           piloto[MAX_NOMBRE];
    char           equipo[MAX_NOMBRE];
    float          velocidadBase;   /* km/h */
    float          destreza;        /* 0.0 - 1.0: afecta probabilidad de accidente */
    float          distancia;       /* distancia recorrida en la simulacion */
    float          velocidadActual; /* km/h actual en este turno */
    EstadoAuto     estado;
    TipoVehiculo   tipoVehiculo;
    Especificaciones especificaciones;
    int            posicion;        /* posicion en la carrera (1er, 2do...) */
    char           color[16];       /* codigo de color ANSI */
    char           inicial;         /* caracter que representa al auto en pista */
} Auto;

/* Configuracion de la carrera leida del archivo */
typedef struct {
    float  longitudPista;     /* km */
    int    cantidadAutos;
    char   clima[20];         /* SECO, LLUVIA, NUBLADO */
    float  factorClima;       /* modificador de velocidad por clima */
    int    turnosMax;
    int    velocidadSimulacion; /* ms entre turnos */
} ConfigCarrera;

/* ====================================================
 *  PROTOTIPOS DE FUNCIONES
 * ==================================================== */
void   leerConfiguracion(ConfigCarrera *cfg, Auto **autos);
void   guardarConfiguracionDefault(void);
void   dibujarEncabezado(void);
void   dibujarPista(Auto *autos, int n, float longitud, int turno);
void   dibujarTablaEstadisticas(Auto *autos, int n);
void   actualizarPosiciones(Auto *autos, int n, ConfigCarrera *cfg);
int    determinarGanador(Auto *autos, int n);
void   ordenarPorPosicion(Auto *autos, int n);
void   guardarResultado(Auto *autos, int n, ConfigCarrera *cfg, int turno);
void   mostrarPodio(Auto *autos, int n, int turnosTotal);
void   limpiarPantalla(void);
float  randFloat(float min, float max);
void   pausar(int ms);
const char* nombreEstado(EstadoAuto e);
const char* colorAuto(int idx);
char   inicialAuto(int idx);
void   mostrarMenuInicio(ConfigCarrera *cfg);

/* ====================================================
 *  FUNCION PRINCIPAL
 * ==================================================== */
int main(void) {
    srand((unsigned int)time(NULL));

    ConfigCarrera cfg;
    Auto *autos = NULL;

    dibujarEncabezado();

    /* Verificar si existe archivo de configuracion, si no, crearlo */
    FILE *test = fopen(CONFIG_FILE, "r");
    if (!test) {
        printf(C_YELLOW "\n  [!] No se encontro '%s'. Creando configuracion por defecto...\n" C_RESET, CONFIG_FILE);
        guardarConfiguracionDefault();
        sleep_ms(1000);
    } else {
        fclose(test);
    }

    /* Leer configuracion desde archivo */
    leerConfiguracion(&cfg, &autos);

    mostrarMenuInicio(&cfg);

    printf(C_GREEN "\n  Iniciando simulacion...\n" C_RESET);
    sleep_ms(1500);

    int turno = 0;
    int autosActivos = cfg.cantidadAutos;

    /* ---- BUCLE PRINCIPAL DE SIMULACION ---- */
    while (autosActivos > 0 && turno < cfg.turnosMax) {
        turno++;

        /* Actualizar posiciones */
        actualizarPosiciones(autos, cfg.cantidadAutos, &cfg);

        /* Contar autos activos */
        autosActivos = 0;


        /* Contar cuantos terminaron esta ronda */
        for (int i = 0; i < cfg.cantidadAutos; i++) {
            if (autos[i].estado == EN_CARRERA) {
                autosActivos++;
            }
        }

        /* Dibujar pantalla completa */
        limpiarPantalla();
        dibujarPista(autos, cfg.cantidadAutos, cfg.longitudPista, turno);
        dibujarTablaEstadisticas(autos, cfg.cantidadAutos);

        /* Si todos terminaron o se accidentaron, terminar */
        if (autosActivos == 0) break;

        pausar(cfg.velocidadSimulacion);
    }

    /* Mostrar podio final */
    mostrarPodio(autos, cfg.cantidadAutos, turno);

    /* Guardar resultados en archivo */
    guardarResultado(autos, cfg.cantidadAutos, &cfg, turno);

    /* Liberar memoria dinamica */
    free(autos);
    autos = NULL;

    printf(C_CYAN "\n  Resultados guardados en '%s'\n" C_RESET, LOG_FILE);
    printf(C_WHITE "  Presione ENTER para salir...\n" C_RESET);
    getchar();
    getchar();

    return 0;
}

/* ====================================================
 *  IMPLEMENTACION DE FUNCIONES
 * ==================================================== */

/* Genera un float aleatorio en [min, max] */
float randFloat(float min, float max) {
    return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

/* Pausa en milisegundos */
void pausar(int ms) {
    sleep_ms(ms);
}

/* Limpia la pantalla */
void limpiarPantalla(void) {
    system(CLEAR);
}

/* Retorna color ANSI por indice */
const char* colorAuto(int idx) {
    const char *colores[] = {
        C_RED, C_GREEN, C_YELLOW, C_BLUE, C_MAGENTA,
        C_CYAN, C_WHITE, C_RED, C_GREEN, C_YELLOW
    };
    return colores[idx % 10];
}

/* Retorna inicial del auto por indice */
char inicialAuto(int idx) {
    return (char)('A' + idx);
}

/* Nombre legible del estado */
const char* nombreEstado(EstadoAuto e) {
    switch (e) {
        case EN_CARRERA:   return "En carrera";
        case ACCIDENTADO:  return "Accidentado";
        case FINALIZADO:   return "Finalizo!  ";
        default:           return "Desconocido";
    }
}

/* ---- Dibuja el encabezado del programa ---- */
void dibujarEncabezado(void) {
    limpiarPantalla();
    printf(C_RED C_BOLD);
    printf("  +==========================================================+\n");
    printf("  |   ____  _                              _                  |\n");
    printf("  |  / ___|(_)_ __ ___  _   _  |  _____ _ __  F1             |\n");
    printf("  |  \\___ \\| | '_ ` _ \\| | | | | |/ / __| '_ \\              |\n");
    printf("  |   ___) | | | | | | | |_| | |   <\\__ \\ | | |             |\n");
    printf("  |  |____/|_|_| |_| |_|\\__,_| |_|\\_\\___/_| |_|             |\n");
    printf("  |                                                            |\n");
    printf("  |       SIMULADOR DE CARRERA - Programacion I               |\n");
    printf("  +==========================================================+\n");
    printf(C_RESET "\n");
}

/* ---- Crea el archivo de configuracion por defecto ---- */
void guardarConfiguracionDefault(void) {
    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) {
        printf(C_RED "  [ERROR] No se pudo crear el archivo de configuracion.\n" C_RESET);
        return;
    }

    /* Formato:
       LONGITUD_PISTA_KM
       CLIMA (SECO/LLUVIA/NUBLADO)
       VELOCIDAD_SIMULACION_MS
       CANTIDAD_AUTOS
       PILOTO EQUIPO VELOCIDAD_BASE DESTREZA TIPO(D/T)
       ...
    */
    fprintf(f, "# Configuracion de la carrera F1\n");
    fprintf(f, "# LONGITUD_PISTA_KM\n");
    fprintf(f, "5.0\n");
    fprintf(f, "# CLIMA: SECO LLUVIA NUBLADO\n");
    fprintf(f, "SECO\n");
    fprintf(f, "# VELOCIDAD_SIMULACION_MS (pausa entre turnos)\n");
    fprintf(f, "600\n");
    fprintf(f, "# CANTIDAD_AUTOS\n");
    fprintf(f, "6\n");
    fprintf(f, "# PILOTO EQUIPO VELOCIDAD_BASE DESTREZA TIPO(D=Deportivo T=Todoterreno)\n");
    fprintf(f, "Verstappen Red_Bull      320.0 0.95 D\n");
    fprintf(f, "Hamilton   Mercedes      315.0 0.93 D\n");
    fprintf(f, "Leclerc    Ferrari       318.0 0.90 D\n");
    fprintf(f, "Norris     McLaren       312.0 0.88 D\n");
    fprintf(f, "Alonso     Aston_Martin  308.0 0.85 T\n");
    fprintf(f, "Sainz      Williams      305.0 0.82 T\n");

    fclose(f);
    printf(C_GREEN "  [OK] Archivo '%s' creado.\n" C_RESET, CONFIG_FILE);
}

/* ---- Lee la configuracion del archivo de texto ---- */
void leerConfiguracion(ConfigCarrera *cfg, Auto **autos) {
    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) {
        printf(C_RED "  [ERROR] No se puede abrir '%s'.\n" C_RESET, CONFIG_FILE);
        exit(1);
    }

    char linea[256];
    int paso = 0; /* 0=longitud, 1=clima, 2=velocidad, 3=cantidad, 4=autos */
    int autoIdx = 0;

    cfg->cantidadAutos = 0;
    cfg->turnosMax = 200;

    while (fgets(linea, sizeof(linea), f)) {
        /* Ignorar comentarios y lineas vacias */
        if (linea[0] == '#' || linea[0] == '\n' || linea[0] == '\r') continue;

        switch (paso) {
            case 0:
                sscanf(linea, "%f", &cfg->longitudPista);
                paso++;
                break;
            case 1:
                sscanf(linea, "%s", cfg->clima);
                /* Factor de clima */
                if (strcmp(cfg->clima, "LLUVIA") == 0)       cfg->factorClima = 0.75f;
                else if (strcmp(cfg->clima, "NUBLADO") == 0) cfg->factorClima = 0.90f;
                else                                          cfg->factorClima = 1.00f;
                paso++;
                break;
            case 2:
                sscanf(linea, "%d", &cfg->velocidadSimulacion);
                paso++;
                break;
            case 3:
                sscanf(linea, "%d", &cfg->cantidadAutos);
                if (cfg->cantidadAutos > MAX_AUTOS) cfg->cantidadAutos = MAX_AUTOS;
                /* Asignacion dinamica del arreglo de autos (malloc) */
                *autos = (Auto *)malloc(cfg->cantidadAutos * sizeof(Auto));
                if (!(*autos)) {
                    printf(C_RED "  [ERROR] Memoria insuficiente.\n" C_RESET);
                    fclose(f);
                    exit(1);
                }
                /* Inicializar todos a cero */
                memset(*autos, 0, cfg->cantidadAutos * sizeof(Auto));
                paso++;
                break;
            case 4:
                if (autoIdx < cfg->cantidadAutos) {
                    Auto *a = &((*autos)[autoIdx]);
                    char tipoChar;
                    sscanf(linea, "%s %s %f %f %c",
                           a->piloto, a->equipo,
                           &a->velocidadBase, &a->destreza,
                           &tipoChar);

                    a->id              = autoIdx + 1;
                    a->distancia       = 0.0f;
                    a->velocidadActual = 0.0f;
                    a->estado          = EN_CARRERA;
                    a->posicion        = 0;
                    a->tipoVehiculo    = (tipoChar == 'T') ? TODOTERRENO : DEPORTIVO;
                    a->inicial         = inicialAuto(autoIdx);

                    /* Copiar color */
                    strncpy(a->color, colorAuto(autoIdx), sizeof(a->color) - 1);

                    /* Inicializar union segun tipo */
                    if (a->tipoVehiculo == DEPORTIVO) {
                        a->especificaciones.turbo = randFloat(1.05f, 1.20f);
                    } else {
                        a->especificaciones.traccion = randFloat(1.10f, 1.30f);
                    }

                    autoIdx++;
                }
                break;
        }
    }

    fclose(f);
    printf(C_GREEN "  [OK] Configuracion cargada: %d autos | Pista %.1f km | Clima: %s\n" C_RESET,
           cfg->cantidadAutos, cfg->longitudPista, cfg->clima);
}

/* ---- Actualiza la posicion de cada auto en un turno ---- */
/*      Paso por referencia: modifica el arreglo de autos  */
void actualizarPosiciones(Auto *autos, int n, ConfigCarrera *cfg) {
    static int posFinalizados = 0; /* posicion para el proximo que termine */
    static int iniciado = 0;
    if (!iniciado) { posFinalizados = 1; iniciado = 1; }

    for (int i = 0; i < n; i++) {
        Auto *a = &autos[i]; /* paso por referencia */
        if (a->estado != EN_CARRERA) continue;

        /* Velocidad base modificada por clima y factor aleatorio */
        float factorAleatorio = randFloat(0.85f, 1.10f);
        float vel = a->velocidadBase * cfg->factorClima * factorAleatorio;

        /* Bonus segun tipo de vehiculo */
        if (a->tipoVehiculo == DEPORTIVO) {
            vel *= a->especificaciones.turbo;
        } else {
            /* Todoterreno gana en lluvia */
            if (strcmp(cfg->clima, "LLUVIA") == 0) {
                vel *= a->especificaciones.traccion;
            }
        }

        a->velocidadActual = vel;

        /* Avanzar distancia: vel en km/h, cada turno simula ~5 segundos */
        float deltaDist = (vel / 3600.0f) * 5.0f; /* km por turno */
        a->distancia += deltaDist;

        /* Verificar si llego a la meta */
        if (a->distancia >= cfg->longitudPista) {
            a->distancia = cfg->longitudPista;
            a->estado    = FINALIZADO;
            a->posicion  = posFinalizados++;
        }

        /* Probabilidad de accidente: (1 - destreza) * 2% por turno */
        float probAccidente = (1.0f - a->destreza) * 0.02f;
        if (randFloat(0.0f, 1.0f) < probAccidente) {
            a->estado   = ACCIDENTADO;
            a->posicion = n; /* ultimo lugar */
        }
    }
}

/* ---- Dibuja la pista en consola ---- */
void dibujarPista(Auto *autos, int n, float longitud, int turno) {
    printf(C_BOLD C_WHITE);
    printf("  +----------------------------------------------------------+\n");
    printf("  |   GRAN PREMIO F1  -  VUELTA / TURNO: %-5d               |\n", turno);
    printf("  |   Clima: %-10s                                       |\n",
           turno > 0 ? "Activo" : "Esperando");
    printf("  +----------------------------------------------------------+\n" C_RESET);

    printf("  " C_YELLOW "[Inicio]" C_RESET);
    for (int k = 0; k < ANCHO_PISTA - 2; k++) printf("=");
    printf(C_YELLOW "[Meta]" C_RESET "\n");

    /* Una fila por auto */
    for (int i = 0; i < n; i++) {
        Auto *a = &autos[i];

        /* Calcular posicion en la pista (0 a ANCHO_PISTA) */
        int pos = (int)((a->distancia / longitud) * (ANCHO_PISTA - 1));
        if (pos < 0) pos = 0;
        if (pos >= ANCHO_PISTA) pos = ANCHO_PISTA - 1;

        printf("  |");
        for (int k = 0; k < ANCHO_PISTA; k++) {
            if (k == pos) {
                if (a->estado == ACCIDENTADO) {
                    printf(C_RED "X" C_RESET);
                } else if (a->estado == FINALIZADO) {
                    printf(C_GREEN "F" C_RESET);
                } else {
                    printf("%s%c%s", a->color, a->inicial, C_RESET);
                }
            } else {
                printf(".");
            }
        }
        printf("|\n");
    }

    printf("  ");
    for (int k = 0; k < ANCHO_PISTA + 2; k++) printf("-");
    printf("\n");

    /* Leyenda */
    printf("  " C_BOLD "Leyenda: " C_RESET);
    for (int i = 0; i < n; i++) {
        printf("%s%c%s=%-12s  ", autos[i].color, autos[i].inicial, C_RESET, autos[i].piloto);
        if ((i + 1) % 3 == 0) printf("\n          ");
    }
    printf("\n");
}

/* ---- Dibuja la tabla de estadisticas ---- */
void dibujarTablaEstadisticas(Auto *autos, int n) {
    printf("\n  " C_BOLD C_WHITE);
    printf("+----+----------------+----------------+----------+-----------+------------+\n");
    printf("| ## |   Piloto       |   Equipo       | Dist(km) | Vel(km/h) | Estado     |\n");
    printf("+----+----------------+----------------+----------+-----------+------------+\n");
    printf(C_RESET);

    for (int i = 0; i < n; i++) {
        Auto *a = &autos[i];
        const char *colorEstado = C_WHITE;
        if (a->estado == FINALIZADO)  colorEstado = C_GREEN;
        if (a->estado == ACCIDENTADO) colorEstado = C_RED;
        if (a->estado == EN_CARRERA)  colorEstado = C_CYAN;

        printf("  | %s%c%s  | %-14s | %-14s | %8.3f | %9.1f | %s%-10s%s |\n",
               a->color, a->inicial, C_RESET,
               a->piloto, a->equipo,
               a->distancia, a->velocidadActual,
               colorEstado, nombreEstado(a->estado), C_RESET);
    }
    printf("  +----+----------------+----------------+----------+-----------+------------+\n");
}

/* ---- Determina y retorna el indice del ganador ---- */
int determinarGanador(Auto *autos, int n) {
    for (int i = 0; i < n; i++) {
        if (autos[i].posicion == 1) return i;
    }
    /* Si nadie termino, el que mas avanzo */
    int mejor = 0;
    for (int i = 1; i < n; i++) {
        if (autos[i].distancia > autos[mejor].distancia) mejor = i;
    }
    return mejor;
}

/* ---- Muestra el podio final ---- */
void mostrarPodio(Auto *autos, int n, int turnosTotal) {
    limpiarPantalla();
    printf(C_BOLD C_YELLOW);
    printf("\n  +=========================================================+\n");
    printf("  |                   RESULTADO FINAL                       |\n");
    printf("  |          Gran Premio F1 - Turnos: %-5d                 |\n", turnosTotal);
    printf("  +=========================================================+\n\n");
    printf(C_RESET);

    /* Ordenar por posicion para mostrar podio */
    /* Crear arreglo auxiliar de indices para ordenar */
    int idx[MAX_AUTOS];
    for (int i = 0; i < n; i++) idx[i] = i;

    /* Bubble sort por posicion (0 = no termino, se va al final) */
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            Auto *a = &autos[idx[j]];
            Auto *b = &autos[idx[j+1]];
            int pa = (a->posicion == 0) ? 999 : a->posicion;
            int pb = (b->posicion == 0) ? 999 : b->posicion;
            if (pa > pb) { int tmp = idx[j]; idx[j] = idx[j+1]; idx[j+1] = tmp; }
        }
    }

    const char *medallas[] = { C_YELLOW"[ORO]   ", C_WHITE"[PLATA] ", C_RED"[BRONCE]" };

    for (int rank = 0; rank < n; rank++) {
        Auto *a = &autos[idx[rank]];
        const char *med = (rank < 3) ? medallas[rank] : "        ";
        if (rank < 3) printf("  %s", med);
        else          printf("  [%2d]   ", rank + 1);

        printf("%s%-14s%s | %-14s | %.3f km | %s\n" C_RESET,
               a->color, a->piloto, C_RESET,
               a->equipo, a->distancia,
               nombreEstado(a->estado));
    }

    printf(C_BOLD C_GREEN "\n  GANADOR: %s (%s)\n" C_RESET,
           autos[idx[0]].piloto, autos[idx[0]].equipo);
}

/* ---- Guarda el resultado en archivo de texto ---- */
void guardarResultado(Auto *autos, int n, ConfigCarrera *cfg, int turno) {
    FILE *f = fopen(LOG_FILE, "w");
    if (!f) {
        printf(C_RED "  [ERROR] No se pudo guardar el resultado.\n" C_RESET);
        return;
    }

    fprintf(f, "=== RESULTADO FINAL DE CARRERA F1 ===\n");
    fprintf(f, "Longitud pista: %.1f km\n", cfg->longitudPista);
    fprintf(f, "Clima: %s (factor: %.2f)\n", cfg->clima, cfg->factorClima);
    fprintf(f, "Turnos simulados: %d\n\n", turno);

    fprintf(f, "%-4s %-16s %-16s %-10s %-10s %-12s\n",
            "Pos", "Piloto", "Equipo", "Dist(km)", "Vel(km/h)", "Estado");
    fprintf(f, "----------------------------------------------------------------------\n");

    /* Indices para ordenar */
    int idx[MAX_AUTOS];
    for (int i = 0; i < n; i++) idx[i] = i;
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            int pa = (autos[idx[j]].posicion == 0)   ? 999 : autos[idx[j]].posicion;
            int pb = (autos[idx[j+1]].posicion == 0) ? 999 : autos[idx[j+1]].posicion;
            if (pa > pb) { int tmp = idx[j]; idx[j] = idx[j+1]; idx[j+1] = tmp; }
        }
    }

    for (int r = 0; r < n; r++) {
        Auto *a = &autos[idx[r]];
        fprintf(f, "%-4d %-16s %-16s %-10.3f %-10.1f %-12s\n",
                r + 1, a->piloto, a->equipo,
                a->distancia, a->velocidadActual,
                nombreEstado(a->estado));
    }

    fclose(f);
}

/* ---- Menu de inicio ---- */
void mostrarMenuInicio(ConfigCarrera *cfg) {
    printf("\n  " C_BOLD C_CYAN "CONFIGURACION CARGADA:" C_RESET "\n");
    printf("  Pista     : %.1f km\n", cfg->longitudPista);
    printf("  Clima     : %s (factor velocidad: %.0f%%)\n",
           cfg->clima, cfg->factorClima * 100.0f);
    printf("  Autos     : %d\n", cfg->cantidadAutos);
    printf("  Velocidad : %d ms por turno\n\n", cfg->velocidadSimulacion);

    printf("  " C_BOLD "Opciones:" C_RESET "\n");
    printf("  [1] Iniciar carrera con configuracion actual\n");
    printf("  [2] Usar configuracion mas rapida (200ms)\n");
    printf("  [3] Salir\n\n");
    printf("  Seleccione: ");

    int op = 0;
    scanf("%d", &op);
    if (op == 2) cfg->velocidadSimulacion = 200;
    if (op == 3) { free(NULL); exit(0); }
}
