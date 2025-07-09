#ifndef QUESTION_MANAGER_H
#define QUESTION_MANAGER_H

#include "question.h"
#include <vector>
#include <string>
#include <sqlite3.h>

class QuestionManager
{
public:
    explicit QuestionManager(const std::string &dbPath);
    ~QuestionManager();

    // Управление вопросами
    bool addQuestion(const Question &question, int categoryId = -1);
    bool removeQuestion(int questionId);
    bool updateQuestion(int questionId, const Question &updatedQuestion);

    // Получение вопросов
    std::vector<Question> getRandomQuestions(int count, int categoryId = -1) const;
    std::vector<Question> getQuestionsByCategory(int categoryId, int limit = -1) const;

    // Управление категориями
    bool addCategory(const std::string &name);
    bool removeCategory(int categoryId);
    bool updateCategory(int categoryId, const std::string &newName);
    std::vector<std::pair<int, std::string>> getAllCategories() const;

    // Статистика
    int getTotalQuestionsCount() const;
    int getCategoryQuestionsCount(int categoryId) const;

    // Проверка дубликатов
    bool questionExists(const std::string &questionText) const;

private:
    sqlite3 *db;

    void initializeDatabase();
    bool executeSQL(const std::string &sql) const;
    int getLastInsertId() const;
    Question createQuestionFromStatement(sqlite3_stmt *stmt) const;
};

#endif // QUESTION_MANAGER_H