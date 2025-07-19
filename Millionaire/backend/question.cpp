class Question
{
public:
    Question(const QString &text,
             const QStringList &options,
             int correctAnswer,
             int categoryId = -1);

    // Геттеры
    QString text() const { return text_; }
    QStringList options() const { return options_; }
    int correctAnswer() const { return correctAnswer_; }
    int categoryId() const { return categoryId_; }

    // Проверка ответа
    bool isCorrect(int userAnswer) const
    {
        return userAnswer == correctAnswer_;
    }

    // Валидация
    void validate() const
    {
        if (options_.size() != 4)
            throw std::invalid_argument("Должно быть 4 варианта ответа");
        if (correctAnswer_ < 1 || correctAnswer_ > 4)
            throw std::invalid_argument("Правильный ответ должен быть от 1 до 4");
    }

    // Сериализация
    QJsonObject toJson() const
    {
        return QJsonObject{
            {"text", text_},
            {"options", QJsonArray::fromStringList(options_)},
            {"correctAnswer", correctAnswer_},
            {"categoryId", categoryId_}};
    }

    static Question fromJson(const QJsonObject &json)
    {
        Question q(
            json["text"].toString(),
            json["options"].toVariant().toStringList(),
            json["correctAnswer"].toInt(),
            json["categoryId"].toInt(-1));
        q.validate();
        return q;
    }
};