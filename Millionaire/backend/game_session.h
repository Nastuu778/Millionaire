#pragma once
#include "question_manager.h"
#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QSoundEffect>

class GameSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int remainingTime READ remainingTime NOTIFY timerUpdated)

public:
    enum class State
    {
        NOT_STARTED,
        IN_PROGRESS,
        ANSWER_PENDING,
        FINISHED,
        TIME_EXPIRED
    };
    Q_ENUM(State)

    enum class Lifeline
    {
        FIFTY_FIFTY,
        AUDIENCE_HELP,
        SECOND_CHANCE
    };
    Q_ENUM(Lifeline)

    explicit GameSession(QSharedPointer<QuestionManager> manager, QObject *parent = nullptr);

    // Основное API
    Q_INVOKABLE bool startGame(const QString &playerName, int categoryId = -1, int timeLimitSec = 180);
    Q_INVOKABLE bool submitAnswer(int answerIndex);
    Q_INVOKABLE void useLifeline(Lifeline lifeline);
    Q_INVOKABLE void saveGame(const QString &filename) const;
    Q_INVOKABLE bool loadGame(const QString &filename);

    // Геттеры
    Question currentQuestion() const;
    int score() const;
    State state() const;
    int remainingTime() const;
    Q_INVOKABLE QList<int> fiftyFiftyOptions() const;
    Q_INVOKABLE QList<int> audienceHelpDistribution() const;
    Q_INVOKABLE bool hasSecondChance() const;

signals:
    void stateChanged(State newState);
    void questionChanged();
    void answerResult(bool isCorrect);
    void lifelineUsed(Lifeline lifeline);
    void timerUpdated(int remainingSeconds);
    void gameFinished(int finalScore);
    void errorOccurred(const QString &message);

private slots:
    void updateTimer();

private:
    // Внутренние методы
    void endGame();
    void moveToNextQuestion();
    void applyFiftyFifty();
    void applyAudienceHelp();
    void updateScore(bool isCorrect);
    void playSoundEffect(const QString &filename);

    // Состояние игры
    QSharedPointer<QuestionManager> manager_;
    QVector<Question> questions_;
    QString playerName_;
    int currentQuestionIndex_ = 0;
    int score_ = 0;
    int correctAnswers_ = 0;
    int wrongAnswers_ = 0;
    State state_ = State::NOT_STARTED;

    // Таймер и время
    QTimer *timer_;
    QDateTime startTime_;
    int timeLimitSec_ = 180;
    int remainingTime_ = 0;

    // Подсказки
    QBitArray lifelinesUsed_;
    bool hasSecondChance_ = false;
    bool secondChanceUsed_ = false;
    QSet<int> fiftyFiftyRemovedOptions_;
    QList<int> audienceDistribution_;

    // Звуковые эффекты
    QSoundEffect correctSound_;
    QSoundEffect wrongSound_;
    QSoundEffect lifelineSound_;
};