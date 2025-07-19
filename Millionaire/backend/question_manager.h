#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QVector>
#include <QPair>
#include "question.h"

class QuestionManager : public QObject
{
    Q_OBJECT
public:
    explicit QuestionManager(const QString &dbPath, QObject *parent = nullptr);
    ~QuestionManager();

    // Управление вопросами
    Q_INVOKABLE bool addQuestion(const Question &question, int categoryId = -1);
    Q_INVOKABLE bool removeQuestion(int id);
    Q_INVOKABLE bool updateQuestion(int id, const Question &question);
    Q_INVOKABLE bool questionExists(const QString &text) const;

    // Получение вопросов
    Q_INVOKABLE QVector<Question> getRandomQuestions(int count, int categoryId = -1) const;
    Q_INVOKABLE QVector<Question> getQuestionsByCategory(int categoryId, int limit = -1) const;
    Q_INVOKABLE Question getQuestionById(int id) const;

    // Управление категориями
    Q_INVOKABLE bool addCategory(const QString &name);
    Q_INVOKABLE bool removeCategory(int id);
    Q_INVOKABLE bool updateCategory(int id, const QString &newName);
    Q_INVOKABLE QVector<QPair<int, QString>> getAllCategories() const;
    Q_INVOKABLE QString getCategoryName(int id) const;

    // Статистика
    Q_INVOKABLE int getTotalQuestionsCount() const;
    Q_INVOKABLE int getCategoryQuestionsCount(int categoryId) const;
    Q_INVOKABLE int getLastInsertId() const;

    // Импорт/экспорт
    Q_INVOKABLE bool importFromJson(const QString &filePath);
    Q_INVOKABLE bool exportToJson(const QString &filePath) const;

signals:
    void databaseError(const QString &message);

private:
    void initializeDatabase();
    bool executeSQL(const QString &sql) const;
    Question createQuestionFromQuery(const QSqlQuery &query) const;
    bool beginTransaction() const;
    bool commitTransaction() const;
    bool rollbackTransaction() const;

    QSqlDatabase db_;
};