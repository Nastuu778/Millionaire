#ifndef QUESTION_H
#define QUESTION_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <stdexcept>

class Question
{
public:
    Question() = default;
    Question(const QString &text,
             const QStringList &options,
             int correctAnswer,
             int categoryId = -1);

    // Геттеры
    QString getText() const noexcept { return text; }
    QStringList getOptions() const noexcept { return options; }
    int getCorrectAnswer() const noexcept { return correctAnswer; }
    int getCategoryId() const noexcept { return categoryId; }

    // Проверка ответа
    bool isCorrect(int userAnswer) const noexcept
    {
        return userAnswer == correctAnswer;
    }

    // Сериализация
    QJsonObject toJson() const;
    static Question fromJson(const QJsonObject &json);

private:
    void validate() const;

    QString text;
    QStringList options;
    int correctAnswer;
    int categoryId;
};

#endif // QUESTION_H