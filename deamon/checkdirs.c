#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <syslog.h>
#include <ftw.h>

#include "../headers/checkdirs.h"
#include "../headers/fileoperations.h"

int checkdirs(char *argv[]) 
{
    struct stat sb1;
    struct stat sb2;

    // Sprawdzamy czy ścieżka źródłowa jest poprawna
    if(stat(argv[1], &sb1) == 0)
    {
        // Sprawdzamy czy ścieżka docelowa jest poprawna
        if(stat(argv[2], &sb2) != 0)
        {
            printf("Nieprawidłowa ścieżka do katalogu docelowego!\n");
            return -1;
        }
    }
    else
    {
        printf("Nieprawidłowa ścieżka do katalogu źródłowego!\n");
        return -1;
    }

    // Sprawdzamy czy podane ścieżki prowadzą do katalogów
    if(S_ISDIR(sb1.st_mode))
    {
        if(!S_ISDIR(sb2.st_mode))
        {
            printf("Ścieżka nie prowadzi do katalogu docelowego!\n");
            return -1;
        }
    }
    else
    {
        printf("Ścieżka nie prowadzi do katalogu źródłowego!\n");
        return -1;
    }

    return 0;
}

int countFiles(char *path)
{   
    // Otwieramy katalog
    DIR *src = opendir(path);
    struct dirent *entry;
    int count = 0;

    // Odczytujemy zawartość
    while((entry = readdir(src)) != NULL)
    {   
        // Zwiększamy licznik pilków, pomijając "." i ".."
        if(strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0)
            continue;
        count++;
    }

    // Zamykamy katalog
    closedir(src);

    // Zawracamy liczbę plików
    return count;
}

void deleteExcessiveFiles(char *source, char *destination, int recur)
{
    DIR *src;
    struct dirent *sEnt;
    int flag = 0; // flaga oznaczająca czy katalog źródłowy istnieje (0), lub nie istnieje (1)
    char srcPath[512];

    // Sprawdzamy czy ścieżka źródłowa istnieje
    if((src = opendir(source)) != NULL)
    {
        strcpy(srcPath, source);
        strcat(srcPath, "/");
    }
    else
        flag = 1; // Katalog źródłowy nie istnieje

    // Otwieramy katalog docelowy
    DIR *dst = opendir(destination);
    struct dirent *dEnt;
    char dstPath[512];
    strcpy(dstPath, destination);
    strcat(dstPath, "/");

    int found; // Flaga oznaczająca czy wpis z katalogu docelowego znajduje się w źródłowym (1), jeżeli nie to (0)

    // Przeszukujemy katalog docelowy
    while((dEnt = readdir(dst)) != NULL)
    {   
        // Pomijamy "." i ".."
        if(strcmp(dEnt->d_name,".") == 0 || strcmp(dEnt->d_name,"..") == 0)
            continue;

        found = 0; // Wstępnie ustawiamy flagę 

        // Dołączamy nazwę bieżącego wpisu katalogu do ścieżki do katalogu źródłowego
        strcat(srcPath, dEnt->d_name);
        // Dołączamy nazwę bieżącego wpisu katalogu do ścieżki do katalogu docelowego
        strcat(dstPath, dEnt->d_name);

        // Jeżeli katalog źródłowy istnieje
        if(flag == 0)
        {
            rewinddir(src);

            // Przeszukujemy katalog źródłowy
            while((sEnt = readdir(src)) != NULL)
            {   
                // Pomijamy "." i ".."
                if(strcmp(sEnt->d_name,".") == 0 || strcmp(sEnt->d_name,"..") == 0)
                    continue;

                // Jeżeli sprawdzane elementy mają taką samą nazwę
                if(strcmp(dEnt->d_name, sEnt->d_name) == 0)
                {
                    found = 1; // ustawiamy flagę

                    // Jeśli elementy to katalogi i mają takie same daty modyfikacji
                    // oraz rekurencja jest włączona, usuwamy zbędne pliki
                    if(sEnt->d_type == DT_DIR && cmpModificationDate(srcPath, dstPath) == 0 && recur == 1)
                    {
                        deleteExcessiveFiles(srcPath, dstPath, recur);
                    }
                }
            }
        }

        // Jeżeli wpis nie znajduje się w katalogu źródłowym
        if(found == 0)
        {   
            // Jeżeli jest katalogiem
            if(dEnt->d_type == DT_DIR)
            {   
                // Jeżeli posiada katalogi lub pliki
                if(countFiles(dstPath) > 0)
                {
                    printf("%d\n", countFiles(dstPath));
                    printf("%s\n", dstPath);
                    // Rekurencyjne usuwanie podkatalogów i plików
                    deleteExcessiveFiles(srcPath, dstPath, recur);
                }
                // Usuwamy katalog
                syslog(LOG_INFO, "Daemon deleted directory: %s", dstPath);
                remove(dstPath);
            }
            else // Jeżeli nie jest katalogiem
            {   
                // Usuwamy plik
                syslog(LOG_INFO, "Daemon deleted file: %s", dstPath);
                remove(dstPath);
            }
        }

        // Usunięcie ostatnich elementów ze ścieżek
        srcPath[strlen(srcPath) - strlen(dEnt->d_name)] = '\0';
        dstPath[strlen(dstPath) - strlen(dEnt->d_name)] = '\0';
    }

    // Jeżeli katalog źródłowy został otwarty to go zamykamy
    if(flag == 0)
        closedir(src);

    // Zamknięcie katalogu docelowego
    closedir(dst);
}

int checkExistence(DIR *dst, char *filename)
{
    struct dirent *dEnt;

    // Odczytujemy zawartość katalogu
    while((dEnt = readdir(dst)) != NULL)
    {
        // Sprawzamy czy nazwa się zgadza
        if(strcmp(filename, dEnt->d_name) == 0)
        {
            rewinddir(dst);
            return 0; // Plik istnieje
        }
    }

    rewinddir(dst);
    return 1; // Plik nie istnieje
}
