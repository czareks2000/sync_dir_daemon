#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <stdbool.h>

// deklaracje funkcji (narazie będą tutaj, potem trzeba przerzucić do pliku nagłówkowego)

/**
 * @todo
 * Funkcja opisująca to, co robi funkcja.
 * 
 * @param param1 Opis pierwszego parametru.
 * @param param2 Opis drugiego parametru.
 */
void sync_dir_demon();

/**
 * @todo
 * Fukncja sprawdzająca poprawność argumentów i przypisująca wartości do zmiennych.
 * 
 * @param argc liczba argumentów wywołania programu
 * @param argv tablica argumentów wywołania programu
 * @param source_dir adres zmiennej, do której zostanie przypisana wartość ścieżki źródłowej
 * @param dest_dir adres zmiennej, do której zostanie przypisana wartość ścieżki docelowej
 * @param interval adres zmiennej, do której zostanie przypisana wartość czasu spania
 * @param recursive adres zmiennej, do której zostanie przypisana wartość opcji rekurencyjnej
 * @param threshold adres zmiennej, do której zostanie przypisana wartość progowa rozmiaru pliku
 * @return 0 w przypadku poprawnych argumentów, -1 w przypadku błędów
 */
int checkParameters(int argc, char *argv[], char **source, char **destination, unsigned int *interval, int *recursive, unsigned long long *threshold);

/**
 * Fukncja sprawdzająca czy podana ścieżka jest katalogiem.
 * 
 * @param path Ścieżka do katalogu
 * @return 0 jeżeli podana ścieżka jest katalogiem, -1 w przypadku wystąpienia błędu
 */
int is_dir(const char *path);

/*
argumenty:
sciezka_zrodlowa - ścieżka do katalogu, z którego kopiujemy
sciezka_docelowa - ścieżka do katalogu, do którego kopiujemy
dodatkowe opcje:
-i <czas_spania> - czas spania (w sekundach)
-R - rekurencyjna synchronizacja katalogów
-t <prog_duzego_pliku> - minimalny rozmiar pliku, żeby był on potraktowany jako duży (w bajtach)
*/
int main(int argc, char *argv[]) 
{   
    // Ścieżka źródłowa
    char *source_dir;
    // Ścieżka docelowa
    char *dest_dir;
    // Czas spania
    unsigned int interval;
    // Czy synchronizacja ma być rekurencyjna
    int recursive;
    // Próg dzielący pliki małe od dużych
    unsigned long long threshold;

    // Sprawdzenie poprawności podanych argumentów i przypisanie wartości dla zmiennych
    if (checkParameters(argc, argv, &source_dir, &dest_dir, &interval, &recursive, &threshold) < 0) 
    {
        fprintf(stderr, "Uzycie: %s sciezka_zrodlowa sciezka_docelowa [-i <czas_spania>] [-R] [-t <prog_duzego_pliku>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Sprawdzenie czy ścieżka źródłowa jest katalogiem
    if(is_dir(source_dir) < 0)
    {
        perror(source_dir);
        exit(EXIT_FAILURE);
    }

    // Sprawdzenie czy ścieżka docelowa jest katalogiem
    if(is_dir(dest_dir) < 0)
    {
        perror(dest_dir);
        exit(EXIT_FAILURE);
    }

    // Wywołanie demona
    sync_dir_demon();
   
    return 0;
}

// Definicje funkcji

void sync_dir_demon()
{

}

int checkParameters(int argc, char *argv[], char **source_dir, char **dest_dir, unsigned int *interval, int *recursive, unsigned long long *threshold)
{
    // Jeżeli podano za mało argumentów
    if (argc < 3)
        return -1;

    // Domyślne wartości
    *interval = 300;
    *recursive = 0;
    *threshold = 1024 * 1024 * 500; // 500 MB

    // Sprawdzenie poprawności argumentów i przypisanie wartości do zmiennych
    for (int i = 1; i < argc; i++) 
    {
        if (strcmp(argv[i], "-i") == 0) 
        {
            i++;
            if (i >= argc) 
            {
                fprintf(stderr, "Brak argumentu dla opcji -i\n");
                return -1;
            } 
            else 
                *interval = atoi(argv[i]);

        } 
        else if (strcmp(argv[i], "-R") == 0) 
        {
            *recursive = 1;
        } 
        else if (strcmp(argv[i], "-t") == 0) 
        {
            i++;
            if (i >= argc) 
            {
                fprintf(stderr, "Brak argumentu dla opcji -t\n");
                return -1;
            } 
            else 
                *threshold = atoi(argv[i]);
        } 
        else if (*source_dir == NULL) 
        {
            *source_dir = argv[i];
        } 
        else if (*dest_dir == NULL) 
        {
            *dest_dir = argv[i];
        } 
        else 
        {
            fprintf(stderr, "Nieznana opcja lub za duzo argumentow\n");
            return -1;
        }
    }

    // Sprawdzenie czy podane są wymagane argumenty
    if (*source_dir == NULL || *dest_dir == NULL) 
    {
        fprintf(stderr, "Nie podano wymaganych argumentow\n");
        return -1;
    }

    return 0;
}

int is_dir(const char *path)
{   
    DIR *dir = opendir(path);

    if (dir == NULL)
        return -1;

    if(closedir(dir) < 0)
        return -1;

    return 0;
}