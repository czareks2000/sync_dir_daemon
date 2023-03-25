#ifndef FILEOPERATIONS_H_
#define FILEOPERATIONS_H_


/**
 * @brief Wysyła informacje do logu
 * 
 * @param format Treść informacji
 * @param ... dodatkowe argumenty do formatowania
 * 
 * @return void
 */
void sendLog(const char *format, ...);

/**
 * @todo dodać kopiowanie daty moyfikacji po utworzeniu kopii
 * 
 * @brief Kopiuje plik z jednej lokalizacji do drugiej.
 * 
 * @param source Ścieżka do pliku źródłowego.
 * @param destination Ścieżka do pliku docelowego.
 * 
 * @return Wartość 0 w przypadku powodzenia, w przypadku błędu -1.
 */
int copy(char *source, char *destination);

/**
 * @brief Porównuje daty modyfikacji dwóch plików.
 * 
 * @param source Ścieżka do pliku źródłowego.
 * @param destination Ścieżka do pliku docelowego.
 * 
 * @return Zwraca 
 *   0 - gdy data modyfikacji pliku źródłowego jest późniejsza od daty modyfikacji pliku docelowego, 
 *   1 - gdy daty modyfikacji obu plików są równe,
 *   2 - jeśli data modyfikacji pliku źródłowego jest wcześniejsza od daty modyfikacji modyfikacji pliku docelowego,
 *  -1 - w przypadku błędu. 
 */
int cmpModificationDate(char *source, char *destination);

/**
 * @brief Pobiera rozmiar pliku o podanej ścieżce.
 * 
 * @param path Ścieżka do pliku.
 * 
 * @return Zwraca rozmiar pliku w bajtach lub -1, gdy nie uda się odczytać informacji o pliku
 */
off_t getFileSize(char *path);

/**
 * @brief Kopiuje plik o podanej ścieżce do nowego pliku przy użyciu mapowania pamięci.
 * 
 * @param source Ścieżka do pliku, który ma zostać skopiowany.
 * @param destination Ścieżka do nowego pliku, do którego zostanie skopiowany plik źródłowy.
 * 
 * @return void
 */
void mmapCopy(char *source, char *destination);

/**
 * @brief Funkcja kopiuje pliki i katalogi z jednej lokalizacji do drugiej, wykorzystując rekurencję.
 * 
 * @param source Ścieżka lokalizacji źródłowej
 * @param destination Ścieżka lokalizacji docelowej
 * @param filesize Rozmiar pliku, powyżej którego zostanie zastosowane kopiowanie za pomocą mmap
 * @param recursive Czy katalagi mają być kopiowane rekurencyjnie (1) czy nie (0)
 * 
 * @return void
 */
void synchronize(char *source, char* destination, off_t filesize, int recursive);

#endif