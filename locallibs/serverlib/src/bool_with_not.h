#ifndef SERVERLIB_BOOL_WITH_NOT_H
#define SERVERLIB_BOOL_WITH_NOT_H

/**
 *  Класс BoolWithNot предназначен для работы с валидирующими ф-циями.
 *  Пример:
 *    BoolWithNot isValidCity(const std::string& );
 *    if (!isValidCity(s)) {
 *      return Error();
 *    }
 *    nsi::City c(s);
 *  Почему не использовать boost::optional?
 *  Почему явно выписан operator! и нет приведения к bool?
 *
 *  Такой дизайн позволяет навязать стиль разбора параметров, который исключает следующую ошибку.
 *  Пример с ошибкой:
 *    boost::optional<City> parseCity(const std::string&);
 *    boost::optional<City> c(parseCity(s));
 *    if (c) {          // <= ошибка
 *      // work with c
 *    }
 *    wotk without c
 *  Пример без ошибки:
 *    boost::optional<City> parseCity(const std::string&);
 *    boost::optional<City> c(parseCity(s));
 *    if (!s.empty() && !c) {
 *      return Error();
 *    }
 *    if (c) {
 *      // work with c
 *    }
 *    wotk without c
 *  Очевидно, что в случае подачи на вход неверно заполненной строки s код
 *  должен вернуть ошибку, что сделано во втором примере, а в первом примере код работает
 *  как-будто на вход не пришел город.
 *
 *  PS1: Особо одаренные знатоки C++ безусловно могут написать !!. Этого делать не стоит.
 *  Если возникла ситуация, когда ф-ция возвращает BoolWithNot, а вы считаете, что необходима
 *  проверка на true, приходите с примерами.
 *  
 *  PS2: Таких примеров нет.
 * */

class BoolWithNot
{
public:
    BoolWithNot(bool v) : v_(v)
    {
    }
    bool operator!() const
    {
        return v_ == false;
    }
private:
    bool v_;
};

#endif /* SERVERLIB_BOOL_WITH_NOT_H */

