#ifndef QUESTION_H
#define QUESTION_H

#include <string>
#include <vector>
#include <stdexcept>
#include "json.hpp"

class Question
{
public:
    Question() = default;
    Question(const std::string &text, const std::vector<std::string> &options, int correctAnswer);

    // Геттеры
    std::string getText() const noexcept;
    const std::vector<std::string> &getOptions() const noexcept;
    int getCorrectAnswer() const noexcept;

    // Проверка ответа (принимает 1-4)
    bool isCorrect(int userAnswer) const noexcept;

    // Сериализация
    std::string toJson() const;
    static Question fromJson(const std::string &jsonStr);

private:
    std::string text;
    std::vector<std::string> options;
    int correctAnswer;

    void validate() const;
};

#endif // QUESTION_H