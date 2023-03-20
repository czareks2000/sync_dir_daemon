#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <syslog.h>

#include "../headers/checkdirs.h"

#define BUFFER 4096

int copy(char *source, char *destination)
{
    int plik_zrodlowy = open(source, O_RDONLY);

    if(plik_zrodlowy < 0)
    {
        fprintf(stderr, "Błąd: nie udało się otworzyć pliku %s\n", source);
        return -1;
    }

    int plik_docelowy = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0666);
     
    if(plik_docelowy < 0)
    {
        fprintf(stderr, "Błąd: nie udało się otworzyć pliku %s\n", destination);
        close(plik_zrodlowy);
        return -1;
    }
    
    char *bufor = malloc(BUFFER);

    if (bufor == NULL) {
        perror("Nie udało się zarezerować pamięci dla bufora");
        close(plik_zrodlowy);
        close(plik_docelowy);
        return -1;
    }

    ssize_t liczba_odczytanych_bajtow = 0;
    ssize_t liczba_zapisanych_bajtow = 0;

    while (1)
    {
        liczba_odczytanych_bajtow = read(plik_zrodlowy, bufor, BUFFER);
        if(liczba_odczytanych_bajtow == 0)
            break;

        if(liczba_odczytanych_bajtow < 0)
        {
            perror("Błąd podczas odczytu danych");
            close(plik_zrodlowy);
            close(plik_docelowy);
            free(bufor);
            return -1;
        }

        liczba_zapisanych_bajtow = write(plik_docelowy, bufor, liczba_odczytanych_bajtow);
        if(liczba_zapisanych_bajtow < liczba_odczytanych_bajtow)
        {
            perror("Błąd podczas zapisywania danych");
            close(plik_zrodlowy);
            close(plik_docelowy);
            free(bufor);
            return -1;
        }
    }

    free(bufor); 

    if (close(plik_zrodlowy) == -1) {
        perror("Błąd podczas zamykania pliku źródłowego");
        return -1;
    }

    if (close(plik_docelowy) == -1) {
        perror("Błąd podczas zamykania pliku docelowego");
        return -1;
    }

    return 0;
}

int cmpModificationDate(char *source, char *destination)
{
    struct stat sbSrc;
    struct stat sbDst;

    // Odczytujemy dane o plikach
    if(stat(source, &sbSrc) != 0 || stat(destination, &sbDst) != 0)
        return -1;

    if(sbSrc.st_mtime > sbDst.st_mtime)
        return 0; // data modyfikacji pliku źródłowego jest późniejsza
    else if(sbSrc.st_mtime == sbDst.st_mtime)
        return 1; // daty modyfikacji obu plików są równe
    else if(sbSrc.st_mtime < sbDst.st_mtime)
        return 2; // data modyfikacji pliku źródłowego jest wcześniejsza
}

off_t getFileSize(char *path)
{
    struct stat fileSB;

    if(stat(path, &fileSB) != 0)
        return -1;

    return fileSB.st_size;
}

void mmapCopy(char *source, char *destination)
{
    size_t size = getFileSize(source);
    int srcFD = open(source, O_RDONLY);
    int dstFD = open(destination, O_RDWR | O_CREAT, 0666);
    
    char *src = mmap(NULL, size, PROT_READ, MAP_PRIVATE, srcFD, 0);
    ftruncate(dstFD, size);
    char *dst = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dstFD, 0);

    memcpy(dst, src, size);

    munmap(src, size);
    munmap(dst, size);

    close(srcFD);
    close(dstFD);
}

void synchronize(char *source, char* destination, off_t filesize, int recursive)
{   
    // otwieramy katalogi
    DIR *src = opendir(source);
    DIR *dst = opendir(destination);

    // struktura reprezentująca plik
    struct dirent *srcEntry;

    // tablice przechowujące zbudowane ścieżki
    char srcPath[512];
    char dstPath[512];

    // tworzymy ścieżki
    strcpy(srcPath, source);
    strcpy(dstPath, destination);
    strcat(srcPath, "/");
    strcat(dstPath, "/");
    
    // ten while można do oddzielnej funkcji wyrzucić , w main() tez jest on używany

    // sprawdzamy zawartość katalogu
    while((srcEntry = readdir(src)) != NULL)
    {   
        // pomijamy "." i ".."
        if(strcmp(srcEntry->d_name,".") == 0 || strcmp(srcEntry->d_name,"..") == 0)
            continue;

        // aktualizujemy ścieżki (dodajemy aktualnie sprawdzany plik)    
        strcat(srcPath, srcEntry->d_name);
        strcat(dstPath, srcEntry->d_name);

        // jeżeli plik nie jest katalogiem
        if(srcEntry->d_type != DT_DIR)
        {   
            // jeżeli plik istnieje i ma tą samą datę modyfikacji to pomijamy
            if(checkExistence(dst,srcEntry->d_name) == 0 && cmpModificationDate(srcPath, dstPath) == 1)
                continue;
            
            // kopiujemy do miejsca docelowego
            if(getFileSize(srcPath) >= filesize)
            {
                mmapCopy(srcPath, dstPath);
                syslog(LOG_INFO, "Demon kopiuje plik z użyciem mmap.");
            }
            else
            {
                copy(srcPath, dstPath);
                syslog(LOG_INFO, "Demon kopiuje plik.");
            }
        }
        // jeżeli plik jest katalogiem i jest wybrana opcja rekurencyjna
        else if(recursive == 1 && srcEntry->d_type == DT_DIR)
        {   
            // jeżeli katalog istnieje i ma tą samą datę modyfikacji co źródłowy to pomijamy
                if(checkExistence(dst,srcEntry->d_name) == 0 && cmpModificationDate(srcPath, dstPath) == 1)
            continue;

            // jeżeli katalog nie stnieje
            if(checkExistence(dst,srcEntry->d_name) == 1)
            {
                // tworzymy katalog
                mkdir(dstPath, 0775);
                syslog(LOG_INFO, "Demon skopiował katalog.");
            }

            // kopiujemy pliki rekurencyjnie
            synchronize(srcPath, dstPath, filesize, 1);
        }

        // aktualizujemy ścieżki (usuwamy aktualnie sprawdzany plik)  
        srcPath[strlen(srcPath) - strlen(srcEntry->d_name)] = '\0';
        dstPath[strlen(dstPath) - strlen(srcEntry->d_name)] = '\0';
    }

    // zamykamy katalogi
    closedir(src);
    closedir(dst);
}


