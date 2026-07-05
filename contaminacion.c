#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contaminacion.h"

// Leer entero con validacion (evita que el programa se cuelgue con entradas invalidas)
int leerEntero() {
    int num;
    char c;
    while (scanf("%d", &num) != 1) {
        printf("Entrada invalida. Ingresa un numero entero: ");
        while ((c = getchar()) != '\n' && c != EOF);
    }
    while ((c = getchar()) != '\n' && c != EOF);
    return num;
}

// Leer flotante con validacion
float leerFloat() {
    float num;
    char c;
    while (scanf("%f", &num) != 1) {
        printf("Entrada invalida. Ingresa un numero decimal: ");
        while ((c = getchar()) != '\n' && c != EOF);
    }
    while ((c = getchar()) != '\n' && c != EOF);
    return num;
}

void menu() {
    printf("\n--- MENU ---\n");
    printf("1. Ingresar datos actuales\n");
    printf("2. Monitorear contaminacion actual\n");
    printf("3. Predecir contaminacion 24h\n");
    printf("4. Alertas preventivas\n");
    printf("5. Promedios historicos (30 dias)\n");
    printf("6. Recomendaciones\n");
    printf("7. Exportar reporte\n");
    printf("8. Guardar historico en archivo\n");
    printf("0. Salir\n");
    printf("Opcion: ");
}

// Inicializa nombres y valores por defecto. NO genera datos aleatorios:
// el historico se llena solo con datos reales ingresados por el usuario
// o cargados desde el archivo (evita que se pisen datos guardados).
void inicializar(Zona z[]) {
    const char *nombres[] = {
        "Zona Norte", "Zona Sur", "Zona Centro", "Zona Este", "Zona Oeste"
    };

    for (int i = 0; i < NUM_ZONAS; i++) {
        strcpy(z[i].nombre, nombres[i]);

        z[i].clima.temperatura = 20.0;
        z[i].clima.viento = 5.0;
        z[i].clima.humedad = 60.0;

        z[i].numRegistros = 0;

        z[i].actual.co2 = 0;
        z[i].actual.so2 = 0;
        z[i].actual.no2 = 0;
        z[i].actual.pm25 = 0;
    }
}

// Ingresar los datos actuales de contaminacion y clima de una zona,
// y agregarlos al historico (arreglo dentro del struct)
void ingresarDatos(Zona z[]) {
    int n;

    printf("\nSeleccione zona:\n");
    for (int i = 0; i < NUM_ZONAS; i++) {
        printf("%d. %s\n", i + 1, z[i].nombre);
    }

    printf("Opcion: ");
    n = leerEntero();

    if (n < 1 || n > NUM_ZONAS) {
        printf("Zona invalida.\n");
        return;
    }
    n--;

    printf("CO2 (ppm): ");   z[n].actual.co2  = leerFloat();
    printf("SO2 (ug/m3): "); z[n].actual.so2  = leerFloat();
    printf("NO2 (ug/m3): "); z[n].actual.no2  = leerFloat();
    printf("PM2.5 (ug/m3): "); z[n].actual.pm25 = leerFloat();

    printf("Temperatura (C): ");  z[n].clima.temperatura = leerFloat();
    printf("Viento (km/h): ");    z[n].clima.viento = leerFloat();
    printf("Humedad (%%): ");     z[n].clima.humedad = leerFloat();

    // Si el historico ya tiene 30 registros, se descarta el dia mas antiguo
    if (z[n].numRegistros >= DIAS_HIST) {
        for (int i = 0; i < DIAS_HIST - 1; i++) {
            z[n].historico[i] = z[n].historico[i + 1];
        }
        z[n].historico[DIAS_HIST - 1] = z[n].actual;
    } else {
        z[n].historico[z[n].numRegistros] = z[n].actual;
        z[n].numRegistros++;
    }

    printf("Datos ingresados correctamente.\n");
}

// Mostrar los niveles actuales de contaminacion de todas las zonas
void monitorear(Zona z[]) {
    printf("\n--- MONITOREO ACTUAL ---\n");
    for (int i = 0; i < NUM_ZONAS; i++) {
        printf("\n%s\n", z[i].nombre);
        printf("CO2: %.2f (limite %.2f)\n", z[i].actual.co2, LIMITE_CO2);
        printf("SO2: %.2f (limite %.2f)\n", z[i].actual.so2, LIMITE_SO2);
        printf("NO2: %.2f (limite %.2f)\n", z[i].actual.no2, LIMITE_NO2);
        printf("PM2.5: %.2f (limite %.2f)\n", z[i].actual.pm25, LIMITE_PM25);
    }
}

// Promedio ponderado: mas peso a los datos mas recientes (ultima posicion del arreglo)
float promedioPonderado(float *datos, int n) {
    if (n == 0) return 0.0;

    float suma = 0, pesos = 0;
    for (int i = 0; i < n; i++) {
        float p = (float)(i + 1);
        suma += datos[i] * p;
        pesos += p;
    }
    return suma / pesos;
}

// Predecir los niveles de contaminacion para las proximas 24 horas
// usando el promedio ponderado del historico, ajustado por el clima actual
void predecir(Zona z[]) {
    printf("\n--- PREDICCION 24 HORAS ---\n");

    for (int i = 0; i < NUM_ZONAS; i++) {
        if (z[i].numRegistros == 0) {
            printf("\n%s: sin datos historicos suficientes para predecir.\n", z[i].nombre);
            continue;
        }

        float co2[DIAS_HIST], so2[DIAS_HIST], no2[DIAS_HIST], pm[DIAS_HIST];

        for (int j = 0; j < z[i].numRegistros; j++) {
            co2[j] = z[i].historico[j].co2;
            so2[j] = z[i].historico[j].so2;
            no2[j] = z[i].historico[j].no2;
            pm[j]  = z[i].historico[j].pm25;
        }

        // Poco viento favorece la acumulacion de contaminantes (factor > 1)
        // Mucho viento ayuda a dispersarlos (factor < 1)
        float factorViento = 1.0 + (10.0 - z[i].clima.viento) / 50.0;
        if (factorViento < 0.5) factorViento = 0.5;

        // Alta humedad favorece levemente la retencion de particulas
        float factorHumedad = 1.0;
        if (z[i].clima.humedad > 70.0) factorHumedad = 1.05;

        float factor = factorViento * factorHumedad;

        z[i].prediccion.co2  = promedioPonderado(co2, z[i].numRegistros) * factor;
        z[i].prediccion.so2  = promedioPonderado(so2, z[i].numRegistros) * factor;
        z[i].prediccion.no2  = promedioPonderado(no2, z[i].numRegistros) * factor;
        z[i].prediccion.pm25 = promedioPonderado(pm, z[i].numRegistros)  * factor;

        printf("\n%s\n", z[i].nombre);
        printf("CO2: %.2f\n", z[i].prediccion.co2);
        printf("SO2: %.2f\n", z[i].prediccion.so2);
        printf("NO2: %.2f\n", z[i].prediccion.no2);
        printf("PM2.5: %.2f\n", z[i].prediccion.pm25);
    }
}

// Emitir alertas comparando nivel actual Y prediccion contra los limites
void alertas(Zona z[]) {
    printf("\n--- ALERTAS PREVENTIVAS ---\n");

    for (int i = 0; i < NUM_ZONAS; i++) {
        int alertaActual = 0;
        int alertaPrediccion = 0;

        if (z[i].actual.co2  > LIMITE_CO2)  alertaActual++;
        if (z[i].actual.so2  > LIMITE_SO2)  alertaActual++;
        if (z[i].actual.no2  > LIMITE_NO2)  alertaActual++;
        if (z[i].actual.pm25 > LIMITE_PM25) alertaActual++;

        if (z[i].prediccion.co2  > LIMITE_CO2)  alertaPrediccion++;
        if (z[i].prediccion.so2  > LIMITE_SO2)  alertaPrediccion++;
        if (z[i].prediccion.no2  > LIMITE_NO2)  alertaPrediccion++;
        if (z[i].prediccion.pm25 > LIMITE_PM25) alertaPrediccion++;

        if (alertaActual > 0) {
            printf("%s: ALERTA ACTUAL (%d contaminante(s) sobre el limite)\n",
                   z[i].nombre, alertaActual);
        }

        if (alertaPrediccion > 0) {
            printf("%s: ALERTA PREVENTIVA (se prevee que %d contaminante(s) superaran el limite en 24h)\n",
                   z[i].nombre, alertaPrediccion);
        }

        if (alertaActual == 0 && alertaPrediccion == 0) {
            printf("%s: Sin alertas.\n", z[i].nombre);
        }
    }
}

// Calcular y mostrar el promedio historico de los 4 contaminantes (ultimos 30 dias)
void promedios(Zona z[]) {
    printf("\n--- PROMEDIOS HISTORICOS (hasta 30 dias) ---\n");

    for (int i = 0; i < NUM_ZONAS; i++) {
        if (z[i].numRegistros == 0) {
            printf("\n%s: sin registros historicos.\n", z[i].nombre);
            continue;
        }

        float co2 = 0, so2 = 0, no2 = 0, pm = 0;

        for (int j = 0; j < z[i].numRegistros; j++) {
            co2 += z[i].historico[j].co2;
            so2 += z[i].historico[j].so2;
            no2 += z[i].historico[j].no2;
            pm  += z[i].historico[j].pm25;
        }

        int n = z[i].numRegistros;
        printf("\n%s (%d registros)\n", z[i].nombre, n);
        printf("CO2 promedio: %.2f\n", co2 / n);
        printf("SO2 promedio: %.2f\n", so2 / n);
        printf("NO2 promedio: %.2f\n", no2 / n);
        printf("PM2.5 promedio: %.2f\n", pm / n);
    }
}

// Generar recomendaciones de mitigacion revisando los 4 contaminantes
void recomendaciones(Zona z[]) {
    printf("\n--- RECOMENDACIONES ---\n");

    for (int i = 0; i < NUM_ZONAS; i++) {
        int requiereAccion = 0;

        if (z[i].actual.co2 > LIMITE_CO2 || z[i].prediccion.co2 > LIMITE_CO2) {
            printf("%s: Reducir el trafico vehicular en horas pico (CO2 elevado).\n", z[i].nombre);
            requiereAccion = 1;
        }
        if (z[i].actual.so2 > LIMITE_SO2 || z[i].prediccion.so2 > LIMITE_SO2) {
            printf("%s: Suspender temporalmente actividades industriales (SO2 elevado).\n", z[i].nombre);
            requiereAccion = 1;
        }
        if (z[i].actual.no2 > LIMITE_NO2 || z[i].prediccion.no2 > LIMITE_NO2) {
            printf("%s: Restringir circulacion vehicular pesada (NO2 elevado).\n", z[i].nombre);
            requiereAccion = 1;
        }
        if (z[i].actual.pm25 > LIMITE_PM25 || z[i].prediccion.pm25 > LIMITE_PM25) {
            printf("%s: Evitar actividades al aire libre, especialmente grupos vulnerables (PM2.5 elevado).\n", z[i].nombre);
            requiereAccion = 1;
        }

        if (!requiereAccion) {
            printf("%s: No se requieren medidas de mitigacion.\n", z[i].nombre);
        }
    }
}

// Exportar un reporte completo con nivel actual, promedio historico y prediccion
void exportar(Zona z[]) {
    FILE *f = fopen("reporte.txt", "w");
    if (!f) {
        printf("No se pudo generar el reporte.\n");
        return;
    }

    fprintf(f, "REPORTE DE CALIDAD DEL AIRE\n");
    fprintf(f, "===========================\n");

    for (int i = 0; i < NUM_ZONAS; i++) {
        fprintf(f, "\nZona: %s\n", z[i].nombre);
        fprintf(f, "Registros historicos: %d\n", z[i].numRegistros);

        fprintf(f, "\n-- Nivel actual --\n");
        fprintf(f, "CO2: %.2f | SO2: %.2f | NO2: %.2f | PM2.5: %.2f\n",
                z[i].actual.co2, z[i].actual.so2, z[i].actual.no2, z[i].actual.pm25);

        if (z[i].numRegistros > 0) {
            float co2 = 0, so2 = 0, no2 = 0, pm = 0;
            for (int j = 0; j < z[i].numRegistros; j++) {
                co2 += z[i].historico[j].co2;
                so2 += z[i].historico[j].so2;
                no2 += z[i].historico[j].no2;
                pm  += z[i].historico[j].pm25;
            }
            int n = z[i].numRegistros;

            fprintf(f, "\n-- Promedio historico (%d dias) --\n", n);
            fprintf(f, "CO2: %.2f | SO2: %.2f | NO2: %.2f | PM2.5: %.2f\n",
                    co2 / n, so2 / n, no2 / n, pm / n);
        }

        fprintf(f, "\n-- Prediccion 24h --\n");
        fprintf(f, "CO2: %.2f | SO2: %.2f | NO2: %.2f | PM2.5: %.2f\n",
                z[i].prediccion.co2, z[i].prediccion.so2,
                z[i].prediccion.no2, z[i].prediccion.pm25);

        int alerta = (z[i].actual.co2 > LIMITE_CO2 || z[i].prediccion.co2 > LIMITE_CO2 ||
                      z[i].actual.so2 > LIMITE_SO2 || z[i].prediccion.so2 > LIMITE_SO2 ||
                      z[i].actual.no2 > LIMITE_NO2 || z[i].prediccion.no2 > LIMITE_NO2 ||
                      z[i].actual.pm25 > LIMITE_PM25 || z[i].prediccion.pm25 > LIMITE_PM25);

        fprintf(f, "\nEstado: %s\n", alerta ? "ALERTA - se requieren medidas de mitigacion" : "ACEPTABLE");
        fprintf(f, "----------------------------------------\n");
    }

    fclose(f);
    printf("Reporte generado en reporte.txt\n");
}

// Cargar el historico previamente guardado en un archivo de texto
void cargar(Zona z[]) {
    FILE *f = fopen("historico.txt", "r");
    if (!f) {
        // No hay historico previo; no es un error critico
        return;
    }

    for (int i = 0; i < NUM_ZONAS; i++) {
        int n;
        char nombre[NOMBRE_MAX];
        float temp, viento, humedad;

        if (fscanf(f, "%s %d %f %f %f", nombre, &n, &temp, &viento, &humedad) != 5) {
            break;
        }

        strcpy(z[i].nombre, nombre);
        z[i].numRegistros = n;
        z[i].clima.temperatura = temp;
        z[i].clima.viento = viento;
        z[i].clima.humedad = humedad;

        for (int j = 0; j < n; j++) {
            fscanf(f, "%f %f %f %f",
                   &z[i].historico[j].co2, &z[i].historico[j].so2,
                   &z[i].historico[j].no2, &z[i].historico[j].pm25);
        }

        if (n > 0) {
            z[i].actual = z[i].historico[n - 1];
        }
    }

    fclose(f);
    printf("Historico cargado desde historico.txt\n");
}

// Guardar el historico completo de todas las zonas en un archivo de texto
void guardar(Zona z[]) {
    FILE *f = fopen("historico.txt", "w");
    if (!f) {
        printf("No se pudo guardar el historico.\n");
        return;
    }

    for (int i = 0; i < NUM_ZONAS; i++) {
        fprintf(f, "%s %d %.2f %.2f %.2f\n",
                z[i].nombre, z[i].numRegistros,
                z[i].clima.temperatura, z[i].clima.viento, z[i].clima.humedad);

        for (int j = 0; j < z[i].numRegistros; j++) {
            fprintf(f, "%.2f %.2f %.2f %.2f\n",
                    z[i].historico[j].co2, z[i].historico[j].so2,
                    z[i].historico[j].no2, z[i].historico[j].pm25);
        }
    }

    fclose(f);
    printf("Historico guardado en historico.txt\n");
}
