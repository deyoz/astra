#ifndef __LEVENSHTEIN_H
#define __LEVENSHTEIN_H

#include <string>

/* Расстояние Левенштейна от a до b */
/* В эту ф-ю можно передавать ф-ю сравнения символов. *
 * Это удобно, в частности, для тестирования где      *
 * в качестве образца можно подставить 'x' означающий *
 * 'любой символ'                                     */
unsigned LevenshteinDistance(
        const std::string& a,
        const std::string& b,
        bool (*isEqual)(char a, char b) = NULL);
        
/* Расстояние Дамерау-Левенштейна от a до b                */
/* (это как у Левенштейна, но поменять местами две буквы - *
 * это одно исправление, а не два)                         */
unsigned DamerauLevenshteinDistance(
        const std::string& a,
        const std::string& b);

/* Функция аналогична предыдущей, но допускает не более max_distance исправлений между строками */
/* Если исправлеий между строками больше, то возвращается max_distance+1                        */
/************************************************************************************************/
/* Должна работать быстрее                                                                      */
unsigned DamerauLevenshteinDistance_lim(
        const std::string& a,
        const std::string& b,
        const unsigned max_distance);

#endif /* #ifndef __LEVENSHTEIN_H */
