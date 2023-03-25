#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <fcntl.h>

#include "../headers/checkdirs.h"
#include "../headers/fileoperations.h"
#include "../headers/demon.h"

int sigCheck = 0; // flaga aby nie wysyłać dwukrotnie tego samego logu(od sygnału i normalnie)
int term = 0; // flaga do obsługi SIGTERM
int recursive = 0; // flaga do opcji -R
int time = 300; // czas spania (s)
off_t filesize = 512; // próg dużego pliku (MB)

//obsługa SIGUSR1
void sigusr_handler(int sig) 
{
  syslog(LOG_INFO, "Demon obudzony przez SIGUSR1");
  sigCheck = 1;
}

//obsługa SIGTERM
void sigterm_handler(int sig) 
{
  syslog(LOG_INFO, "Proces demona zakończony.");
  term = 1;
}

int main(int argc, char *argv[])
{
  // wyswietlenie informacji o sposobie użycia programu
  if(argc == 2 && strcmp(argv[1], "--help") == 0) //help
  {
    printf("Użycie programu: ./demon <katalog źródłowy> <katalog docelowy> [opcje]\n");
    printf("Opcje:\n\n");
    printf("  -R: Uruchamia rekurencyjne kopiowanie katalogów.\n\n");
    printf("  -t <liczba>: Ustawia czas oczekiwania demona na liczba sekund, bazowa wartość wynosi 5 minut.\n\n");
    printf("  -s <liczba>: Ustawia próg wielkości pliku (w megabajtach).\n");
    printf("               Gdy plik przekroczy próg, kopiowany jest poprzez mmap.\n");
    printf("               Bazowy próg wynosi 512 megabajtów.\n");
    return -1;
  }
  else if(argc < 3)
  {
    printf("Użycie programu: ./demon <katalog źródłowy> <katalog docelowy> [opcje]\n");
    printf("Użycie: ./demon --help wypisze dostępne opcje.\n");
    return -1;
  }
  
  // sprawdzenie podanych parametrów
  if(setParameters(argc, argv) < 0)
    return -1;

  // sprawdzenie czy podane ścieżki są do katalogów
  if(checkdirs(argv) < 0)
    return -1;

  // przekształcenie procesu w demona
  if (demonize() < 0)
    return -1;

  // przypisanie fukncji do obsługi sygnałów SIGUSR1 i SIGTERM
  signal(SIGUSR1, sigusr_handler); 
  signal(SIGTERM, sigterm_handler);
  
  // główna pętla demona (można wyrzucić do oddzielnej fukncji)
  while(1)
  {
    // uspanie procesu
    syslog(LOG_INFO, "Demon idzie spać.");
    sleep(time); 

    // przesłanie informacji do logu
    if(sigCheck == 0)
      syslog(LOG_INFO, "Demon wybudzony naturalnie.");

    // jeżeli otrzymano sygnał SIGTERM
    if(term == 1)
      exit(0); //kończymy program
    
    // usuwamy pliki i katalogi z katalogu docelowego, których nie ma w źródłowym
    deleteExcessiveFiles(argv[1], argv[2], recursive);
    
    // kopiujemy pliki i katalogi których nie ma w katalogu docelowym a są w źródłowym
    synchronize(argv[1], argv[2], filesize, recursive);

    sigCheck = 0;
  }

  return 0;
}

int setParameters(int argc, char *argv[])
{
  if(argc > 3) 
  {
    for(int i = 3; i<argc; i++)
    {
      if(argv[i][0] == '-')
      {
        switch(argv[i][1])
        {
          case 't': // ustawiamy czas spania
            time = atoi(argv[i+1]);
            break;
          case 'R': // ustawiamy rekurencyjne sprawdzanie katalogów
            recursive = 1;
            break;
          case 's': // ustawiamy próg dużego pliku
            filesize = atol(argv[i+1])*1024*1024;
          default:
            break;
        }
      }
    }
  }
  else
    return -1;

  return 0;
}

int demonize()
{
  pid_t pid;
  int i;

  // utworzenie nowego procesu
  pid = fork ( );
  if (pid == -1)
    return -1;
  else if (pid != 0)
    exit (EXIT_SUCCESS);

  // utworzenie nowej sesji i grupy procesów
  if (setsid ( ) == -1)
    return -1;

  // ustawienie katalogu roboczego na katalog główny
  if (chdir ("/") == -1)
    return -1;

  // zamknięcie wszystkich otwartych plików - użycie opcji 1024 to przesada, lecz działa
  for (i = 0; i < 1024; i++)
    close (i);

  // przeadresowanie deskryptorów plików 0, 1, 2 na /dev/null 
  open ("/dev/null", O_RDWR); // stdin 
  dup (0); // stdout 
  dup (0); // stderror 

  syslog(LOG_INFO, "Demon zainicjalizowany.");

  return 0;
}