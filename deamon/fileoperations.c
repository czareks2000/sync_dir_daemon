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
#include <stdarg.h>
#include <utime.h>

#include "../headers/checkdirs.h"
#include "../headers/fileoperations.h"

// rozmiar bufora
#define BUFFER 4096

void sendLog(const char *format, ...)
{   
    char message[1024];
    va_list args;
    va_start(args, format);

    // Tworzymy bufor formatowania.
    vsnprintf(message, sizeof(message), format, args);

    va_end(args);

    // Otwieramy połączenie z logiem
    openlog("sync_dir_deamon", LOG_ODELAY | LOG_PID, LOG_DAEMON);
    // Zapisujemy do logu informację.
    syslog(LOG_INFO, "%s", message);
    // Zamykamy połączenie z logiem.
    closelog();
}

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

    if(copyModificationDate(source, destination) < 0) {
        perror("Błąd podczas kopiowania daty modyfikacji");
        return -1;
    }

    return 0;
}

int cmpModificationDate(char *source, char *destination)
{   
    // struktury do przechowania artybutów plików
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

int copyModificationDate(char *source, char *destination)
{
    // uzyskanie informacji o pliku źródłowym
    struct stat sb;
    if (stat(source, &sb) == -1) {
        perror("Błąd podczas uzyskiwania informacji o pliku źródłowym");
        return -1;
    }

    // ustawienie takiej samej daty modyfikacji dla pliku docelowego
    struct utimbuf times;
    times.actime = sb.st_atime;
    times.modtime = sb.st_mtime;
    if (utime(destination, &times) == -1) {
        perror("Błąd podczas ustawiania daty modyfikacji dla pliku docelowego");
        return -1;
    }

    return 0;
}

off_t getFileSize(char *path)
{
    // struktura do przechowania artybutów pliku
    struct stat fileSB;

    // odczytanie artybutów
    if(stat(path, &fileSB) != 0)
        return -1;

    // zwrócenie rozmiaru pliku
    return fileSB.st_size;
}

int mmapCopy(char *source, char *destination)
{   
    // pobieramy rozmar pliku
    size_t size = getFileSize(source);

    // otwieramy pliki
    int srcFD = open(source, O_RDONLY);
    int dstFD = open(destination, O_RDWR | O_CREAT, 0666);

    // ustawiamy rozmar docelowego pliku na taki sam jak źródłowego
    ftruncate(dstFD, size);
    
    // mapujemy zawartość src i dst w pamięci podręcznej
    char *src = mmap(NULL, size, PROT_READ, MAP_PRIVATE, srcFD, 0);
    char *dst = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dstFD, 0);

    // kopjujemy zawartość pamięci src do dst
    memcpy(dst, src, size);

    // zwalniamy zaalokowaną pamięć
    munmap(src, size);
    munmap(dst, size);

    // zamykamy pliki
    close(srcFD);
    close(dstFD);

    // kopiujemy date modifikacji
    if(copyModificationDate(source, destination) < 0){
        perror("Błąd podczas kopiowania daty modyfikacji");
        return -1;
    }

    return 0;
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
            {   
                // aktualizujemy ścieżki (usuwamy aktualnie sprawdzany plik)  
                srcPath[strlen(srcPath) - strlen(srcEntry->d_name)] = '\0';
                dstPath[strlen(dstPath) - strlen(srcEntry->d_name)] = '\0';
                continue;
            }
            
            // kopiujemy do miejsca docelowego
            if(getFileSize(srcPath) >= filesize)
            {
                mmapCopy(srcPath, dstPath);
                sendLog("Demon skopiował plik: %s z użyciem mmap", srcPath);
            }
            else
            {
                copy(srcPath, dstPath);
                sendLog("Demon skopiował plik: %s", srcPath);
            }
        }
        // jeżeli plik jest katalogiem i jest wybrana opcja rekurencyjna
        else if(recursive == 1 && srcEntry->d_type == DT_DIR)
        {   
            // jeżeli katalog istnieje i ma tą samą datę modyfikacji co źródłowy to pomijamy
            if(checkExistence(dst,srcEntry->d_name) == 0 && cmpModificationDate(srcPath, dstPath) == 1)
            {   
                // aktualizujemy ścieżki (usuwamy aktualnie sprawdzany plik)  
                srcPath[strlen(srcPath) - strlen(srcEntry->d_name)] = '\0';
                dstPath[strlen(dstPath) - strlen(srcEntry->d_name)] = '\0';
                continue;
            }

            // jeżeli katalog nie stnieje
            if(checkExistence(dst,srcEntry->d_name) == 1)
            {
                // tworzymy katalog
                mkdir(dstPath, 0775);
                sendLog("Demon utworzył katalog: %s", dstPath);
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



