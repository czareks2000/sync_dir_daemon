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
#include <limits.h>
#include <sys/mman.h>

// deklaracje funkcji (narazie będą tutaj (jeżeli fukncje nie są skończone), potem trzeba przerzucić do pliku nagłówkowego)
#include <main.h>

/**
 * @todo
 * Funkcja opisująca to, co robi funkcja.
 * 
 * @param param1 Opis pierwszego parametru.
 * @param param2 Opis drugiego parametru.
 */
void syncDirDemon();

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
 * 
 * @return 0 w przypadku poprawnych argumentów, -1 w przypadku błędów
 */
int checkParameters(int argc, char *argv[], char **source, char **destination, unsigned int *interval, int *recursive, unsigned long long *threshold);

/**
 * @todo
 * Funkcja kopiująca plik przy pomocy read/write.
 * 
 * @param source Ścieżka do pliku źródłowego
 * @param destination Ścieżka do pliku docelowego
 * @param dest_mode Uprawnienia ustawiane plikowi docelowemu
 * @param dest_access_time Czas ostatniego dostępu ustawiany plikowi docelowemu
 * @param dest_modification_time Czas ostatniej modyfikacji ustawiany plikowi docelowemu
 * 
 * @return 0 w przypadku poprawnego kopiowania, -1 w przypadku błędów
 */
int copySmallFile(const char *source, const char *destination, const mode_t dest_mode, const struct timespec *dest_access_time, const struct timespec *dest_modification_time);

/**
 * @todo
 * Funkcja kopiująca plik przy pomocy mmap/write (plik źródłowy zostaje zamapowany w całości w pamięci).
 * 
 * @param source Ścieżka do pliku źródłowego
 * @param destination Ścieżka do pliku docelowego
 * @param file_size Rozmiar kopiowanego pliku
 * @param dest_mode Uprawnienia ustawiane plikowi docelowemu
 * @param dest_access_time Czas ostatniego dostępu ustawiany plikowi docelowemu
 * @param dest_modification_time Czas ostatniej modyfikacji ustawiany plikowi docelowemu
 * 
 * @return 0 w przypadku poprawnego kopiowania, -1 w przypadku błędów
 */
int copyBigFile(const char *source, const char *destination, const unsigned long long file_size, const mode_t dest_mode, const struct timespec *dest_access_time, const struct timespec *dest_modification_time);


#define BUFFERSIZE 4096

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
    syncDirDemon();
   
    return 0;
}

// Definicje funkcji

void syncDirDemon()
{

}

int demon()
{
    pid_t pid;   
    int i;

    /* stwórz nowy proces */
    pid = fork ( );   
    if (pid == -1)      
        return -1;  
    else if (pid != 0)
        exit (EXIT_SUCCESS);

    /* stwórz nową sesję i grupę procesów */
    if (setsid ( ) == -1)
        return -1;

    /* ustaw katalog roboczy na katalog główny */
    if (chdir ("/") == -1)
        return -1;

    /* zamknij wszystkie pliki otwarte - użycie opcji NR_OPEN to przesada, lecz działa */
    for (i = 0; i < 1024; i++)
        close (i);

    /* przeadresuj deskryptory plików 0, 1, 2 na /dev/null */
    open ("/dev/null", O_RDWR);/* stdin */
    dup (0);/* stdout */
    dup (0); /* stderror */

    /* tu należy wykonać czynności demona... */ 
    
    return 0;
}

int checkParameters(int argc, char *argv[], char **source_dir, char **dest_dir, unsigned int *interval, int *recursive, unsigned long long *threshold)
{
    // Jeżeli podano za mało argumentów
    if (argc < 3)
        return -1;

    // Domyślne wartości
    *interval = 300; // 5min
    *recursive = 0; // bez rekurencyjnej synchronizacji
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

int isDir(const char *path)
{   
    // Próba otworzenia katalogu
    DIR *dir = opendir(path);

    // Sprawdzenie czy udało się otworzyć katalog
    if (dir == NULL)
        return -1;

    // Sprawdzenie czy udało się zamknąć katalog
    if(closedir(dir) < 0)
        return -2;

    return 0;
}

int copySmallFile(const char *source, const char *destination, const mode_t dest_mode, const struct timespec *dest_access_time, const struct timespec *dest_modification_time)
{
    int status = 0, source_fd = -1, dest_fd = -1;
    // Otwieramy plik źródłowy do odczytu
    if ((source_fd = open(source, O_RDONLY)) == -1)
        status = -1;
    // Otwieramy plik docelowy do zapisu. Jeżeli nie istnieje, to go tworzymy, nadając puste uprawnienia, a jeżeli już istnieje, to go czyścimy.
    else if ((dest_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0000)) == -1)
        status = -1;
    // Ustawiamy plikowi docelowemu uprawnienia dest_mode
    else if (fchmod(dest_fd, dest_mode) == -1)
        status = -1;
    else
    {
        char *buffer = NULL;
        // Rezerwujemy pamięć bufora
        if ((buffer = malloc(BUFFERSIZE)) == NULL)
        {
            status = -1;
        }      
        else
        {
            while (1)
            {
                // Pozycja w buforze.
                char *position = buffer;
                // Zapisujemy całkowitą liczbę bajtów pozostałych do odczytania.
                size_t remainingBytes = BUFFERSIZE;
                ssize_t bytesRead;
                // Dopóki liczby bajtów pozostałych do odczytania i bajtów odczytanych w aktualnej iteracji są niezerowe.
                while (remainingBytes != 0 && (bytesRead = read(source_fd, position, remainingBytes)) != 0)
                {
                    // Jeżeli wystąpił błąd w funkcji read.
                    if (bytesRead == -1)
                    {
                        // Jeżeli funkcja read została przerwana odebraniem sygnału. Blokujemy SIGUSR1 i SIGTERM na czas synchronizacji, więc te sygnały nie mogą spowodować tego błędu.
                        if (errno == EINTR)
                            // Ponawiamy próbę odczytu.
                            continue;
                            
                        // Jeżeli wystąpił inny błąd
                        // Ustawiamy kod błędu.
                        status = -1;
                        // BUFFERSIZE - BUFFERSIZE == 0, więc druga pętla się nie wykona
                        remainingBytes = BUFFERSIZE;
                        // Ustawiamy 0, aby warunek if(bytesRead == 0) przerwał zewnętrzną pętlę while(1).
                        bytesRead = 0;
                        // Przerywamy pętlę.
                        break;
                    }
                    // O liczbę bajtów odczytanych w aktualnej iteracji zmniejszamy liczbę pozostałych bajtów i
                    remainingBytes -= bytesRead;
                    // przesuwamy pozycję w buforze.
                    position += bytesRead;
                }

                position = buffer;  
                // Zapisujemy całkowitą liczbę odczytanych bajtów                        
                remainingBytes = BUFFERSIZE - remainingBytes; 

                ssize_t bytesWritten;
                // Dopóki liczby bajtów pozostałych do zapisania i bajtów zapisanych w aktualnej iteracji są niezerowe.
                while (remainingBytes != 0 && (bytesWritten = write(dest_fd, position, remainingBytes)) != 0)
                {
                    // Jeżeli wystąpił błąd w funkcji write.
                    if (bytesWritten == -1)
                    {
                        // Jeżeli funkcja write została przerwana odebraniem sygnału.
                        if (errno == EINTR)
                        // Ponawiamy próbę zapisu.
                        continue;
                        // Jeżeli wystąpił inny błąd
                        // Ustawiamy kod błędu.
                        status = -1;
                        // Ustawiamy 0, aby warunek if(bytesRead == 0) przerwał zewnętrzną pętlę while(1).
                        bytesRead = 0;
                        // Przerywamy pętlę.
                        break;
                    }
                    // Zmniejszamy liczbę pozostałych bajtów o liczbę bajtów zapisanych w aktualnej iteracji
                    remainingBytes -= bytesWritten;
                    // Przesuwamy pozycję w buforze.
                    position += bytesWritten;
                }
                // Jeżeli doszliśmy do końca pliku (EOF) lub wystąpił błąd
                if (bytesRead == 0)
                    break;
            }

            free(buffer);

            // Jeżeli nie wystąpił błąd podczas kopiowania
            if (status >= 0)
            {
                // Tworzymy strukturę zawierającą czasy ostatniego dostępu i modyfikacji.
                const struct timespec times[2] = {*dest_access_time, *dest_modification_time};

                // Ustawiamy czasy dostępu i modyifkacji
                if (futimens(dest_fd, times) == -1)
                    status = -1;
            }
        }
    }

    // Jeżeli plik źródłowy został otwarty, to go zamykamy
    if (source_fd != -1 && close(source_fd) == -1)
        return -1;
    // Jeżeli plik docelowy został otwarty, to go zamykamy
    if (dest_fd != -1 && close(dest_fd) == -1)
        return -1;

    return status;
}

int copyBigFile(const char *source, const char *destination, const unsigned long long file_size,const mode_t dest_mode, const struct timespec *dest_access_time, const struct timespec *dest_modification_time)
{
    // Podstawowo ustawiony jest status oznaczający brak wystąpienia błędu.
    int status = 0, source_fd = -1, desc_fd = -1;
    // Otwieramy plik źródłowy do odczytu i przypisujemy mu deskryptor
    if ((source_fd = open(source, O_RDONLY)) == -1)
        status = -1;
    // Otwieramy plik docelowy do zapisu. Wprzypadku gdy istnieje to go czyścimy, a gdy nie istnieje to tworzymy go nadając puste uprawnienia.
    else if ((desc_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0000)) == -1)
        status = -2;
    // Ustawiamy plikowi docelowemu uprawnienia dstMode.
    else if (fchmod(desc_fd, dest_mode) == -1)
        status = -3;
    else
    {
        char *map;
        // Odwzorowujemy (mapujemy) w pamięci plik źródłowy w trybie do odczytu.
        if ((map = mmap(0, file_size, PROT_READ, MAP_SHARED, source_fd, 0)) == MAP_FAILED)
            status = -4;
        else
        {
            char *buffer = NULL;
            // Używamy odgórnie ustalonego rozmiaru bufora.
            // Rezerwujemy pamięć bufora.
            if ((buffer = malloc(sizeof(char) * BUFFERSIZE)) == NULL)
                status = -5;
            else
            {
                // Numer bajtu w pliku źródłowym.
                unsigned long long byte_number;
                // Aktualna pozycja w buforze.
                char *current_position;
                size_t remaining_bytes;
                ssize_t written_bytes;
                // Nie może być (byte_number < fileSize - BUFFERSIZE), bo byte_number i fil_size są typu unsigned, więc jeżeli file_size < BUFFERSIZE i odejmiemy, to mamy przepeł
                //nienie.
                for (byte_number = 0; byte_number + BUFFERSIZE < file_size; byte_number += BUFFERSIZE)
                {
                    // Kopiujemy rozmiar bufora bajtów ze zmapowanej pamięci do bufora.
                    memcpy(buffer, map + byte_number, BUFFERSIZE);
                    current_position = buffer;
                    // Zapisujemy całkowitą liczbę bajtów pozostałych (zawsze jest równa rozmiarowi bufora).
                    remaining_bytes = BUFFERSIZE;
                    // Pętla działą do momentu gdy liczby bajtów pozostałych do zapisania i bajtów zapisanych w aktualnej iteracji są różne od zera.
                    while (remaining_bytes != 0 && (written_bytes = write(desc_fd, current_position, remaining_bytes)) != 0)
                    {
                        // W wypadku gdy wystąpił błąd w funkcji write.
                        if (written_bytes == -1)
                        {
                            // Jeżeli funkcja write została przerwana odebraniem sygnału.
                            if (errno == EINTR)
                            // Próbujemy zapisać ponownie.
                            continue;
                            // W wypadku wystąpienia błędu
                            status = -6;
                            // Ustawiamy byte_number aby przerwać wykonywanie powyższej pętli for i ją przerywamy.
                            byte_number = ULLONG_MAX - BUFFERSIZE;
                            break;
                        }
                        // O liczbę bajtów zapisanych w obecnej iteracji zmniejszamy liczbę reszty bajtów
                        remaining_bytes -= written_bytes;
                        // przesuwamy obecną pozycję w buforze.
                        current_position += written_bytes;
                    }
                }
                // Jeżeli wciaż nie wystąpił błąd podczas kopiowania
                if (status >= 0)
                {
                    // Zapisujemy liczbę bajtów z końca pliku, które nie zmieściły się w całym pojedyńczym buforze.
                    remaining_bytes = file_size - byte_number;
                    // Kopiujemy te bajty z pamięci do bufora.
                    memcpy(buffer, map + byte_number, remaining_bytes);
                    // Zapisujemy pozycję pierwszego bajtu bufora.
                    current_position = buffer;
                    // Dopóki liczby bajtów pozostałych do zapisania i bajtów zapisanych w aktualnej iteracji są niezerowe (podobnie jak powyzej).
                    while (remaining_bytes != 0 && (written_bytes = write(desc_fd, current_position, remaining_bytes)) != 0)
                    {
                        // Jeżeli wystąpił błąd w funkcji write.
                        if (written_bytes == -1)
                        {
                            // Jeżeli funkcja write została przerwana odebraniem sygnału.
                            if (errno == EINTR)
                            // Próbujemy zapisać ponownie.
                            continue;
                            // W wypadku wystąpienia błędu ustawiamy kod i przerywamy pętlę
                            status = -7;
                            break;
                        }
                        // O liczbę bajtów zapisanych w obecnej iteracji zmniejszamy liczbę reszty bajtów
                        remaining_bytes -= written_bytes;
                        // przesuwamy obecną pozycję w buforze.
                        current_position += written_bytes;
                    }
                }
                // Zwalniamy pamięć bufora.
                free(buffer);
                // Jeśli przy kopiowaniu nie wystąpił żaden błąd to tworzymy strukturę zawierającą ostatni czasy ostatniego i modyfikacji
                if (status >= 0)
                {
                    const struct timespec times[2] = {*dest_access_time, *dest_modification_time};
                    // Zapisy ustawiają czas modyfikacji na aktualny czas systemowy.
                    if (futimens(desc_fd, times) == -1)
                        status = -8;
                }
            }
                // Usuwamy odwzorowanie pliku źródłowego.
                if (munmap(map, file_size) == -1)
                    status = -9;
        }
    }
    // Jeżeli plik źródłowy został otwarty to podejmujemy próbę zamknięcia go.
    if (source_fd != -1 && close(source_fd) == -1)
    // Każdy niezamknięty plik zajmuje nam cenny deskryptor, a proces może mieć jedynie ograniczoną ich liczbę.
        status = -10;
    // Jeżeli plik docelowy został otwarty to podejmujemy próbę zamknięcia go.
    if (desc_fd != -1 && close(desc_fd) == -1)
        status = -11;
    // Funkcja zwraca status z jakim skończyła kopiowanie plików.
    return status;
}