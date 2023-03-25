#ifndef DEMON_H_
#define DEMON_H_

/**
 * @brief Przekształca proces w demona
 *
 * @return Zwraca 0, w przypadku sukcesu, a -1 w przypadku błędu
 * 
 */
int demonize(); 

/**
 * @todo można zmień na lepszą fukncję (można dodać sprawdzanie poprawności argumentów)
 * 
 * @brief Ustawia parametry programu.
 * 
 * @param argc Liczba argumentów.
 * @param argv Tablica argumentów.
 * 
 * @return Wartość 0 w przypadku powodzenia, w przypadku błędu -1.
 */
int setParameters(int argc, char *argv[]);

#endif