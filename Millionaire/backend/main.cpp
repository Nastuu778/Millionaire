#include "question_manager.h"
#include <iostream>

int main()
{
    try
    {
        // 1. Создаём менеджер вопросов (указываем путь к БД)
        QuestionManager qm("C:\\Dev\\NASTYA\\Millionaire\\backend\\questions.db");
        std::cout << "База данных успешно открыта!\n";

        // 2. Добавляем категорию
        qm.addCategory("История");
        qm.addCategory("Наука");
        std::cout << "Категории добавлены.\n";

        // 3. Получаем список всех категорий
        auto categories = qm.getAllCategories();
        std::cout << "Список категорий:\n";
        for (const auto &cat : categories)
        {
            std::cout << "ID: " << cat.first << ", Название: " << cat.second << "\n";
        }

        Question q1(
            "В каком году началась Вторая мировая война?",
            {"1939", "1941", "1914", "1945"},
            1, // Правильный ответ
            1  // ID категории "История"
        );
        qm.addQuestion(q1, 1); // category_id = 1

        Question q2(
            "Какой газ преобладает в атмосфере Земли?",
            {"Кислород", "Азот", "Углекислый газ", "Водород"},
            2, // Правильный ответ
            2  // ID категории "Наука"
        );
        qm.addQuestion(q2, 2);

        std::cout << "Вопросы добавлены.\n";

        // 5. Получаем 5 случайных вопросов
        auto randomQuestions = qm.getRandomQuestions(5, 0); // 0 = все категории
        std::cout << "\nRandom вопросы:\n";
        for (const auto &q : randomQuestions)
        {
            std::cout << "Текст: " << q.getText() << "\n";
            std::cout << "Правильный ответ: " << q.getCorrectAnswer() << "\n\n";
        }

        // 6. Удаляем вопрос (пример для ID=1)
        // qm.removeQuestion(1);
        // std::cout << "Вопрос с ID=1 удалён.\n";

        // 7. Обновляем категорию (пример для ID=1)
        // qm.updateCategory(1, "Новая История");
        // std::cout << "Категория обновлена.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}