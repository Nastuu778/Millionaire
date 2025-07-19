#include "question_manager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QDebug>

QuestionManager::QuestionManager(const QString &dbPath, QObject *parent)
    : QObject(parent)
{
    db_ = QSqlDatabase::addDatabase("QSQLITE");
    db_.setDatabaseName(dbPath);

    if (!db_.open())
    {
        emit databaseError(tr("Failed to open database: %1").arg(db_.lastError().text()));
    }
    else
    {
        initializeDatabase();
    }
}

void QuestionManager::initializeDatabase()
{
    QSqlQuery query;
    query.exec("PRAGMA foreign_keys = ON");

    const QStringList tables = {
        R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL
        ))",
        R"(
        CREATE TABLE IF NOT EXISTS questions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            category_id INTEGER REFERENCES categories(id) ON DELETE SET NULL,
            text TEXT NOT NULL,
            option1 TEXT NOT NULL,
            option2 TEXT NOT NULL,
            option3 TEXT NOT NULL,
            option4 TEXT NOT NULL,
            correct_answer INTEGER NOT NULL CHECK(correct_answer BETWEEN 1 AND 4),
            UNIQUE(text)
        ))"};

    for (const QString &sql : tables)
    {
        if (!query.exec(sql))
        {
            emit databaseError(tr("Failed to create table: %1").arg(query.lastError().text()));
        }
    }
}

bool QuestionManager::addQuestion(const Question &question, int categoryId)
{
    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO questions 
        (category_id, text, option1, option2, option3, option4, correct_answer)
        VALUES (?, ?, ?, ?, ?, ?, ?)
    )");

    const QStringList &options = question.options();
    query.addBindValue(categoryId > 0 ? categoryId : QVariant());
    query.addBindValue(question.text());
    for (int i = 0; i < 4; ++i)
    {
        query.addBindValue(options.value(i));
    }
    query.addBindValue(question.correctAnswer());

    if (!query.exec())
    {
        emit databaseError(tr("Failed to add question: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool QuestionManager::removeQuestion(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM questions WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec())
    {
        emit databaseError(tr("Failed to remove question: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool QuestionManager::updateQuestion(int id, const Question &question)
{
    QSqlQuery query;
    query.prepare(R"(
        UPDATE questions SET
            text = ?,
            option1 = ?,
            option2 = ?,
            option3 = ?,
            option4 = ?,
            correct_answer = ?,
            category_id = ?
        WHERE id = ?
    )");

    const QStringList &options = question.options();
    query.addBindValue(question.text());
    for (int i = 0; i < 4; ++i)
    {
        query.addBindValue(options.at(i));
    }
    query.addBindValue(question.correctAnswer());
    query.addBindValue(question.categoryId() > 0 ? question.categoryId() : QVariant());
    query.addBindValue(id);

    if (!query.exec())
    {
        emit databaseError(tr("Failed to update question: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool QuestionManager::addCategory(const QString &name)
{
    QSqlQuery query;
    query.prepare("INSERT INTO categories (name) VALUES (?)");
    query.addBindValue(name);

    if (!query.exec())
    {
        emit databaseError(tr("Failed to add category: %1").arg(query.lastError().text()));
        return false;
    }
    return true;
}

bool QuestionManager::removeCategory(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM categories WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec())
    {
        emit databaseError(tr("Failed to remove category: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

bool QuestionManager::updateCategory(int id, const QString &newName)
{
    QSqlQuery query;
    query.prepare("UPDATE categories SET name = ? WHERE id = ?");
    query.addBindValue(newName);
    query.addBindValue(id);

    if (!query.exec())
    {
        emit databaseError(tr("Failed to update category: %1").arg(query.lastError().text()));
        return false;
    }
    return query.numRowsAffected() > 0;
}

QVector<Question> QuestionManager::getRandomQuestions(int count, int categoryId) const
{
    QVector<Question> questions;
    QSqlQuery query;

    QString sql = "SELECT * FROM questions";
    if (categoryId > 0)
    {
        sql += " WHERE category_id = ?";
    }
    sql += " ORDER BY RANDOM() LIMIT ?";

    query.prepare(sql);
    if (categoryId > 0)
        query.addBindValue(categoryId);
    query.addBindValue(count);

    if (query.exec())
    {
        while (query.next())
        {
            questions.append(createQuestionFromQuery(query));
        }
    }
    else
    {
        emit databaseError(tr("Failed to get questions: %1").arg(query.lastError().text()));
    }

    return questions;
}

QVector<Question> QuestionManager::getQuestionsByCategory(int categoryId, int limit) const
{
    QVector<Question> questions;
    QSqlQuery query;

    QString sql = "SELECT * FROM questions WHERE category_id = ?";
    if (limit > 0)
    {
        sql += " LIMIT ?";
    }

    query.prepare(sql);
    query.addBindValue(categoryId);
    if (limit > 0)
        query.addBindValue(limit);

    if (query.exec())
    {
        while (query.next())
        {
            questions.append(createQuestionFromQuery(query));
        }
    }

    return questions;
}

Question QuestionManager::getQuestionById(int id) const
{
    QSqlQuery query;
    query.prepare("SELECT * FROM questions WHERE id = ?");
    query.addBindValue(id);

    if (query.exec() && query.next())
    {
        return createQuestionFromQuery(query);
    }
    return Question();
}

QVector<QPair<int, QString>> QuestionManager::getAllCategories() const
{
    QVector<QPair<int, QString>> categories;
    QSqlQuery query("SELECT id, name FROM categories ORDER BY name");

    while (query.next())
    {
        categories.append(qMakePair(
            query.value(0).toInt(),
            query.value(1).toString()));
    }

    return categories;
}

QString QuestionManager::getCategoryName(int id) const
{
    QSqlQuery query;
    query.prepare("SELECT name FROM categories WHERE id = ?");
    query.addBindValue(id);

    if (query.exec() && query.next())
    {
        return query.value(0).toString();
    }
    return QString();
}

int QuestionManager::getTotalQuestionsCount() const
{
    QSqlQuery query("SELECT COUNT(*) FROM questions");
    return query.exec() && query.next() ? query.value(0).toInt() : 0;
}

int QuestionManager::getCategoryQuestionsCount(int categoryId) const
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM questions WHERE category_id = ?");
    query.addBindValue(categoryId);
    return query.exec() && query.next() ? query.value(0).toInt() : 0;
}

int QuestionManager::getLastInsertId() const
{
    return db_.lastInsertId().toInt();
}

bool QuestionManager::questionExists(const QString &text) const
{
    QSqlQuery query;
    query.prepare("SELECT 1 FROM questions WHERE text = ? LIMIT 1");
    query.addBindValue(text);
    return query.exec() && query.next();
}

bool QuestionManager::importFromJson(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        emit databaseError(tr("Failed to open file"));
        return false;
    }

    QJsonArray questionsArray = QJsonDocument::fromJson(file.readAll()).array();
    if (!beginTransaction())
        return false;

    try
    {
        for (const QJsonValue &val : questionsArray)
        {
            Question q = Question::fromJson(val.toObject());
            if (!addQuestion(q, q.categoryId()))
            {
                throw std::runtime_error("Failed to add question");
            }
        }
        return commitTransaction();
    }
    catch (...)
    {
        rollbackTransaction();
        emit databaseError(tr("Import failed"));
        return false;
    }
}

bool QuestionManager::exportToJson(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        emit databaseError(tr("Failed to create file"));
        return false;
    }

    QJsonArray questionsArray;
    QSqlQuery query("SELECT * FROM questions");

    while (query.next())
    {
        questionsArray.append(createQuestionFromQuery(query).toJson());
    }

    file.write(QJsonDocument(questionsArray).toJson());
    return true;
}

Question QuestionManager::createQuestionFromQuery(const QSqlQuery &query) const
{
    return Question(
        query.value("text").toString(),
        {query.value("option1").toString(),
         query.value("option2").toString(),
         query.value("option3").toString(),
         query.value("option4").toString()},
        query.value("correct_answer").toInt(),
        query.value("category_id").isNull() ? -1 : query.value("category_id").toInt());
}

bool QuestionManager::beginTransaction() const
{
    return db_.transaction();
}

bool QuestionManager::commitTransaction() const
{
    return db_.commit();
}

bool QuestionManager::rollbackTransaction() const
{
    return db_.rollback();
}

QuestionManager::~QuestionManager()
{
    if (db_.isOpen())
    {
        db_.close();
    }
}