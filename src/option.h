#ifndef OPTION_H
#define OPTION_H
#include <QString>

///\module Option:
/// Плохой пример
/* MyClass * getMyClass() {
     if(expression)
        return new MyClass(args..);
     else
        return nullptr;
   }                                                     */
/// Хорощий пример
/// \code
/* Option<MyClass*> getMyClass() {
     if(expression)
        return Option<MyClass*>::Some(new MyClass(args..));
     else
        return Option<MyClass*>::None;
   }                                                   */
///
/// Варианты применения Option:
///\code
/*  // 1. Проверка перед использованием
    auto o_mc = getMyClass();
    if(o_mc.isSome())
        o_mc.unwrap()->myClassFoo(args..);

    // 2. Получение значения или выброс исключения
    MyClass * mc = getMyClass().unwrap();

    // 3. Получение значения или выброс исключения с сообщением об ошибке
    MyClass * mc = getMyClass().expect("Something is wrong. Exception std::logic_error is throwed.");

    // 4. Получение значения или выброс исключения MyCustomException
    MyClass * mc = getMyClass().unwrap<MyCustomException>();

    // 5. Получение значения или выброс исключения MyCustomException с сообщением об ошибке
    MyClass * mc = getMyClass().expect<MyCustomException>("Something wrong. Exception MyCustomException is throwed");    */

///\brief Контейнер для обязательной обработки ошибок возвращаемых результатов
#include <QDebug>
template <typename T>
class Option {
public:
    static Option<T> None(){
        return Option<T>();
    }
    static Option<T> Some(T & t) {
        return Option<T>(t);
    }

    static Option<T> Some(T && t) {
        return Option<T>(std::move(t));
    }

    bool operator==(const Option<T> & o){
        return isSome() == o.isSome() && ((isSome() && unwrap() == o.unwrap()) || isNone());
    }

    Option<T> & operator=(Option<T> & o){
        available = o.available;
        value = std::move(o.value);
        return *this;
    }

    ///\brief Внутри None ?
    bool isNone() { return !available; }

    ///\brief Внутри что-то есть Some
    bool isSome() { return available; }

    ///\brief Возвращает упакованное значение или бросает исключение в случае если значение None
    const T & unwrap() {
        return unwrap<std::logic_error>();
    }

    ///\brief Возвращает упакованное значение или бросает исключение в случае если значение None
    template< typename E = std::logic_error>
    const T & unwrap() {
       if(isSome())
           return value;
       throw E("Option is None value");
    }

    ///\brief Возвращает упакованное значение или бросает исключение с сообщением об ошибке в случае если значение None
    const T & expect (const QString & text) {
        return expect<std::logic_error>(text);
    }

    ///\brief Возвращает упакованное значение или бросает исключение с сообщением об ошибке в случае если значение None
    template <typename E = std::logic_error>
    const T & expect (const QString & text) {
        try {
            return unwrap<E>();
        } catch (E & e) { }

        throw E(text.toStdString().c_str());
    }

private:
    Option() {
        value = T();
        available = false;
    }

    Option(T & t) {
        value = t;
        available = true;
    }

    Option(T && t) {
        value = std::move(t);
        available = true;
    }

    ///\brief Статус валидности
    bool available {false};

    ///\brief Упакованное значение
    T value = T();
};

#endif // OPTION_H
