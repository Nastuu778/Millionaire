#include "question_manager.h"
#include <stdexcept>
#include <random>
#include <algorithm>
#include <iostream>

QuestionManager::QuestionManager(const std::string &dbPath)
{

    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK)
    {
        std::cerr << "Ошибка SQLite: " << sqlite3_errmsg(db) << std::endl;
        throw std::runtime_error("Не удалось открыть/создать БД!");
    }
    initializeDatabase();
}

QuestionManager::~QuestionManager()
{
    if (db)
    {
        sqlite3_close(db);
    }
}

void QuestionManager::initializeDatabase()
{
    const char *sql = R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL
        );
        
        CREATE TABLE IF NOT EXISTS questions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            category_id INTEGER,
            text TEXT NOT NULL,
            option1 TEXT NOT NULL,
            option2 TEXT NOT NULL,
            option3 TEXT NOT NULL,
            option4 TEXT NOT NULL,
            correct_answer INTEGER NOT NULL CHECK(correct_answer BETWEEN 1 AND 4),
            FOREIGN KEY (category_id) REFERENCES categories(id)
        );
    )";

    if (!executeSQL(sql))
    {
        throw std::runtime_error("Failed to initialize database");
    }
}

bool QuestionManager::addQuestion(const Question &question, int categoryId)
{
    const char *sql = R"(
        INSERT INTO questions 
        (category_id, text, option1, option2, option3, option4, correct_answer)
        VALUES (?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    const auto &options = question.getOptions();
    sqlite3_bind_int(stmt, 1, categoryId > 0 ? categoryId : question.getCategoryId());
    sqlite3_bind_text(stmt, 2, question.getText().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, options[0].c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, options[1].c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, options[2].c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, options[3].c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, question.getCorrectAnswer());

    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

bool QuestionManager::removeQuestion(int questionId)
{
    const std::string sql = "DELETE FROM questions WHERE id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, questionId);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool QuestionManager::updateQuestion(int questionId, const Question &updatedQuestion)
{
    const char *sql = R"(
        UPDATE questions SET
            text = ?,
            option1 = ?,
            option2 = ?,
            option3 = ?,
            option4 = ?,
            correct_answer = ?,
            category_id = ?
        WHERE id = ?
    )";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    const auto &options = updatedQuestion.getOptions();
    sqlite3_bind_text(stmt, 1, updatedQuestion.getText().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, options[0].c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, options[1].c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, options[2].c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, options[3].c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, updatedQuestion.getCorrectAnswer());
    sqlite3_bind_int(stmt, 7, updatedQuestion.getCategoryId());
    sqlite3_bind_int(stmt, 8, questionId);

    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Question> QuestionManager::getRandomQuestions(int count, int categoryId) const
{
    std::string sql = "SELECT id, text, option1, option2, option3, option4, correct_answer, category_id FROM questions";

    if (categoryId > 0)
    {
        sql += " WHERE category_id = " + std::to_string(categoryId);
    }

    sql += " ORDER BY RANDOM() LIMIT " + std::to_string(count);

    sqlite3_stmt *stmt;
    std::vector<Question> questions;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            questions.push_back(createQuestionFromStatement(stmt));
        }
        sqlite3_finalize(stmt);
    }

    return questions;
}

bool QuestionManager::addCategory(const std::string &name)
{
    const std::string sql = "INSERT INTO categories (name) VALUES (?)";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

bool QuestionManager::removeCategory(int categoryId)
{
    std::string deleteQuestionsSql = "DELETE FROM questions WHERE category_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, deleteQuestionsSql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, categoryId);
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!success)
        return false;

    std::string deleteCategorySql = "DELETE FROM categories WHERE id = ?";
    if (sqlite3_prepare_v2(db, deleteCategorySql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_int(stmt, 1, categoryId);
    success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool QuestionManager::updateCategory(int categoryId, const std::string &newName)
{
    const std::string sql = "UPDATE categories SET name = ? WHERE id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_text(stmt, 1, newName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, categoryId);
    bool result = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Question> QuestionManager::getQuestionsByCategory(int categoryId, int limit) const
{
    std::string sql = "SELECT id, text, option1, option2, option3, option4, correct_answer, category_id FROM questions WHERE category_id = ?";

    if (limit > 0)
    {
        sql += " LIMIT " + std::to_string(limit);
    }

    sqlite3_stmt *stmt;
    std::vector<Question> questions;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, categoryId);

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            questions.push_back(createQuestionFromStatement(stmt));
        }
        sqlite3_finalize(stmt);
    }

    return questions;
}

Question QuestionManager::createQuestionFromStatement(sqlite3_stmt *stmt) const
{
    std::string text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    std::vector<std::string> options = {
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)),
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)),
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5))};
    int correctAnswer = sqlite3_column_int(stmt, 6);
    int categoryId = sqlite3_column_int(stmt, 7);

    return Question(text, options, correctAnswer, categoryId);
}

bool QuestionManager::executeSQL(const std::string &sql) const
{
    char *errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        if (errMsg)
        {
            sqlite3_free(errMsg);
        }
        return false;
    }
    return true;
}

std::vector<std::pair<int, std::string>> QuestionManager::getAllCategories() const
{
    std::vector<std::pair<int, std::string>> categories;
    const std::string sql = "SELECT id, name FROM categories ORDER BY name";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            categories.emplace_back(
                sqlite3_column_int(stmt, 0),
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)));
        }
        sqlite3_finalize(stmt);
    }

    return categories;
}

int QuestionManager::getTotalQuestionsCount() const
{
    const std::string sql = "SELECT COUNT(*) FROM questions";
    sqlite3_stmt *stmt;
    int count = 0;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return count;
}

int QuestionManager::getCategoryQuestionsCount(int categoryId) const
{
    const std::string sql = "SELECT COUNT(*) FROM questions WHERE category_id = ?";
    sqlite3_stmt *stmt;
    int count = 0;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int(stmt, 1, categoryId);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    return count;
}