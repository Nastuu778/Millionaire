#include "question.h"
#include <algorithm>

using json = nlohmann::json;

void Question::validate() const
{
    if (options.size() != 4)
        throw std::invalid_argument("Должно быть 4 варианта ответа!");
    if (correctAnswer < 1 || correctAnswer > 4)
        throw std::invalid_argument("Правильный ответ должен быть от 1 до 4!");
}

Question::Question(const std::string &text,
                   const std::vector<std::string> &options,
                   int correctAnswer)
    : text(text), options(options), correctAnswer(correctAnswer)
{
    validate();
}

std::string Question::getText() const noexcept { return text; }

const std::vector<std::string> &Question::getOptions() const noexcept
{
    return options;
}

int Question::getCorrectAnswer() const noexcept { return correctAnswer; }

bool Question::isCorrect(int userAnswer) const noexcept
{
    return userAnswer == correctAnswer;
}

std::string Question::toJson() const
{
    return json{{"text", text}, {"options", options}, {"correctAnswer", correctAnswer}}.dump();
}

Question Question::fromJson(const std::string &jsonStr)
{
    auto j = json::parse(jsonStr);
    return Question(j["text"], j["options"], j["correctAnswer"]);
}