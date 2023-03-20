#ifndef CHECKDIRS_H_
#define CHECKDIRS_H_

/**
 * @brief Sprawdza poprawność podanych ścieżek do katalogów.
 * 
 * @param argv Tablica napisów zawierających dwie ścieżki do katalogów (źródłowy i docelowy).
 * @return Wartość całkowita 0, jeśli obie ścieżki są poprawne, -1 w przeciwnym razie.
 * 
 */
int checkdirs(char *argv[]);

/**
 * @brief Funkcja liczy liczbę plików i katalogów (bez "." i "..") w podanym katalogu.
 * 
 * @param path Ścieżka do katalogu, w którym chcemy policzyć pliki.
 * @return Liczba plików (bez katalogów "." i "..") w podanym katalogu.
 * 
 */
int countFiles(char *path);

/**
 * @brief 
 * Funkcja porównuje zawartość katalogu źródłowego z zawartością katalogu docelowego
 * i usuwa pliki, które nie występują w źródłowym. Funkcja również usuwa puste katalogi.
 * 
 * @param source Ścieżka do katalogu źródłowego.
 * @param destination Ścieżka do katalogu docelowego.
 * @param recur Flaga określająca, czy rekurencyjnie usuwać pliki w podkatalogach.
 */
void deleteExcessiveFiles(char *source, char *destination, int recur);

/**
 * @brief Sprawdza, czy plik o określonej nazwie istnieje w danym strumieniu katalogów.
 *
 * @param dst Wskaźnik do strumienia katalogów.
 * @param filename Wskaźnik do łańcucha znaków zawierającego nazwę pliku do sprawdzenia.
 *
 * @return Zwraca 0, jeśli plik istnieje w strumieniu katalogów, a 1 w przeciwnym razie.
 * 
 */
int checkExistence(DIR *dst, char *filename);

#endif