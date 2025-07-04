#include "question.h"
#include <iostream>

int main()
{
    Question q("Столица Франции?", {"Лондон", "Париж", "Берлин", "Мадрид"}, 2);

    std::cout << "Вопрос: " << q.getText() << "\n";
    for (size_t i = 0; i < q.getOptions().size(); ++i)
        std::cout << i + 1 << ") " << q.getOptions()[i] << "\n";

    std::cout << "Правильный ответ: " << q.getCorrectAnswer() << "\n";

    return 0;
}