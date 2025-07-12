#include "game_session.h"
#include "question_manager.h"
#include <iostream>
#include <memory>

int main()
{
    try
    {
        // 1. Инициализация менеджера вопросов
        std::cout << "Initializing database...\n";
        auto questionManager = std::make_shared<QuestionManager>("C:/Dev/NASTYA/Millionaire/backend/questions.db");

        // 2. Вывод информации о доступных категориях и вопросах
        std::cout << "\nChecking database content:\n";
        auto categories = questionManager->getAllCategories();
        if (categories.empty())
        {
            std::cerr << "No categories found in database!\n";
            return 1;
        }

        for (const auto &[id, name] : categories)
        {
            int count = questionManager->getCategoryQuestionsCount(id);
            std::cout << "Category " << id << " (" << name << "): "
                      << count << " questions\n";
        }

        // 3. Выбор категории (возьмем первую доступную)
        int categoryId = categories[0].first;
        std::string categoryName = categories[0].second;
        int questionsInCategory = questionManager->getCategoryQuestionsCount(categoryId);

        std::cout << "\nSelected category: " << categoryName
                  << " (ID: " << categoryId << ", Questions: "
                  << questionsInCategory << ")\n";

        // 4. Создаем игровую сессию
        int requestedQuestions = 1; // Хотим 1 вопросов
        int actualQuestions = std::min(requestedQuestions, questionsInCategory);

        std::cout << "Starting session with " << actualQuestions << " questions...\n";
        GameSession session(questionManager);
        if (!session.startSession("TestPlayer", categoryId, 20))
        { // 20 секунд
            std::cerr << "\nFailed to start session! Possible reasons:\n";
            std::cerr << "1. Not enough questions in category (need at least 1)\n";
            std::cerr << "2. Database connection issues\n";
            std::cerr << "3. Session already started\n";
            return 1;
        }

        // 5. Игровой цикл
        std::cout << "\nGame started successfully!\n";
        while (!session.isFinished())
        {
            session.update();

            // Вывод информации о текущем вопросе
            const auto &question = session.getCurrentQuestion();
            std::cout << "\n--- Question " << (session.getCurrentQuestionIndex() + 1)
                      << " ---\n";
            std::cout << "Score: " << session.getScore() << " | "
                      << "Time left: " << session.getRemainingTime() << "s\n";
            std::cout << question.getText() << "\n";

            // Вывод вариантов ответа
            const auto &options = question.getOptions();
            for (size_t i = 0; i < options.size(); ++i)
            {
                std::cout << (i + 1) << ") " << options[i] << "\n";
            }

            // Ввод ответа
            int answer;
            std::cout << "Your choice (1-4): ";
            std::cin >> answer;

            // Проверка ответа
            if (!session.submitAnswer(answer - 1))
            {
                std::cout << "Wrong answer!\n";
            }
            else
            {
                std::cout << "Correct! Current score: " << session.getScore() << "\n";
            }

            // Переход к следующему вопросу
            if (session.getState() == GameSession::State::ANSWER_SUBMITTED)
            {
                session.nextQuestion();
            }
        }

        // 6. Итоги игры
        std::cout << "\n--- Game over! ---\n";
        std::cout << "Final score: " << session.getScore() << "\n";
        std::cout << "Correct answers: " << session.getCorrectAnswersCount() << "/"
                  << session.getQuestionsCount() << "\n";
        std::cout << "Time spent: " << session.getElapsedTime() << " seconds\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}