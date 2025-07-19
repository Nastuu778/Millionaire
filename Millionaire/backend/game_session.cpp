#include "game_session.h"
#include <QTimer>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QDebug>

GameSession::GameSession(QSharedPointer<QuestionManager> manager, QObject *parent)
    : QObject(parent), manager_(manager), timer_(new QTimer(this))
{
    // Инициализация таймера
    connect(timer_, &QTimer::timeout, this, &GameSession::updateTimer);

    // Инициализация звуков
    initSoundEffects();

    // Инициализация подсказок
    lifelinesUsed_.resize(static_cast<int>(Lifeline::COUNT));
    resetLifelines();
}

void GameSession::startGame(const QString &playerName, int categoryId, int timeLimitSec)
{
    if (state_ != State::NOT_STARTED)
    {
        emit errorOccurred(tr("Игра уже начата"));
        return;
    }

    // Установка базовых параметров
    playerName_ = playerName;
    timeLimitSec_ = timeLimitSec;
    remainingTime_ = timeLimitSec;
    currentQuestionIndex_ = 0;
    score_ = 0;
    correctAnswers_ = 0;
    wrongAnswers_ = 0;
    secondChanceUsed_ = false;

    // Загрузка вопросов
    questions_ = manager_->getRandomQuestions(15, categoryId);
    if (questions_.isEmpty())
    {
        emit errorOccurred(tr("Нет доступных вопросов"));
        return;
    }

    // Сброс состояния подсказок
    resetLifelines();

    // Запуск игры
    startTime_ = QDateTime::currentDateTime();
    timer_->start(1000);
    state_ = State::IN_PROGRESS;

    emit gameStarted();
    emit questionChanged();
    emit timerUpdated(remainingTime_);
}

void GameSession::submitAnswer(int answerIndex)
{
    if (state_ != State::IN_PROGRESS)
        return;

    const bool isCorrect = (answerIndex + 1) == currentQuestion().correctAnswer();

    if (isCorrect)
    {
        handleCorrectAnswer();
    }
    else
    {
        handleWrongAnswer();
    }
}

void GameSession::useLifeline(Lifeline lifeline)
{
    if (lifelinesUsed_.testBit(static_cast<int>(lifeline)))
    {
        emit errorOccurred(tr("Подсказка уже использована"));
        return;
    }

    lifelinesUsed_.setBit(static_cast<int>(lifeline), true);
    playSoundEffect(":/sounds/lifeline.wav");

    switch (lifeline)
    {
    case Lifeline::FIFTY_FIFTY:
        applyFiftyFifty();
        break;
    case Lifeline::AUDIENCE_HELP:
        applyAudienceHelp();
        break;
    case Lifeline::SECOND_CHANCE:
        hasSecondChance_ = true;
        break;
    }

    emit lifelineUsed(lifeline);
}

void GameSession::saveGame(const QString &filename) const
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly))
    {
        emit errorOccurred(tr("Ошибка сохранения"));
        return;
    }

    QJsonObject gameState;
    // Сохранение основных данных
    gameState["player"] = playerName_;
    gameState["score"] = score_;
    gameState["correct"] = correctAnswers_;
    gameState["wrong"] = wrongAnswers_;
    gameState["timeLeft"] = remainingTime_;
    gameState["current"] = currentQuestionIndex_;
    gameState["secondChance"] = hasSecondChance_ && !secondChanceUsed_;

    // Сохранение вопросов
    QJsonArray questionsArray;
    for (const auto &q : questions_)
    {
        questionsArray.append(q.toJson());
    }
    gameState["questions"] = questionsArray;

    // Сохранение подсказок
    QJsonArray lifelinesArray;
    for (int i = 0; i < lifelinesUsed_.size(); ++i)
    {
        lifelinesArray.append(lifelinesUsed_.testBit(i));
    }
    gameState["lifelines"] = lifelinesArray;

    file.write(QJsonDocument(gameState).toJson());
}

bool GameSession::loadGame(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        emit errorOccurred(tr("Ошибка загрузки"));
        return false;
    }

    const QJsonObject gameState = QJsonDocument::fromJson(file.readAll()).object();

    // Загрузка основных данных
    playerName_ = gameState["player"].toString();
    score_ = gameState["score"].toInt();
    correctAnswers_ = gameState["correct"].toInt();
    wrongAnswers_ = gameState["wrong"].toInt();
    remainingTime_ = gameState["timeLeft"].toInt();
    currentQuestionIndex_ = gameState["current"].toInt();
    hasSecondChance_ = gameState["secondChance"].toBool();
    secondChanceUsed_ = false;

    // Загрузка вопросов
    questions_.clear();
    for (const auto &q : gameState["questions"].toArray())
    {
        questions_.append(Question::fromJson(q.toObject()));
    }

    // Загрузка подсказок
    const auto lifelines = gameState["lifelines"].toArray();
    for (int i = 0; i < lifelines.size(); ++i)
    {
        lifelinesUsed_.setBit(i, lifelines[i].toBool());
    }

    // Перезапуск игры
    state_ = State::IN_PROGRESS;
    startTime_ = QDateTime::currentDateTime();
    timer_->start(1000);

    emit gameLoaded();
    emit questionChanged();
    return true;
}

// Приватные методы реализации
void GameSession::handleCorrectAnswer()
{
    correctAnswers_++;
    score_ += (currentQuestionIndex_ + 1) * 100;
    playSoundEffect(":/sounds/correct.wav");

    emit answerSubmitted(true);
    moveToNextQuestion();
}

void GameSession::handleWrongAnswer()
{
    if (hasSecondChance_ && !secondChanceUsed_)
    {
        secondChanceUsed_ = true;
        playSoundEffect(":/sounds/lifeline.wav");
        emit secondChanceUsed();
        return;
    }

    wrongAnswers_++;
    score_ = qMax(0, score_ - 200);
    playSoundEffect(":/sounds/wrong.wav");

    emit answerSubmitted(false);
    endGame();
}

void GameSession::applyFiftyFifty()
{
    const int correct = currentQuestion().correctAnswer() - 1;
    QList<int> wrongOptions;

    for (int i = 0; i < 4; ++i)
    {
        if (i != correct)
            wrongOptions << i;
    }

    std::shuffle(wrongOptions.begin(), wrongOptions.end(), *QRandomGenerator::global());
    fiftyFiftyRemovedOptions_.insert(wrongOptions.constFirst());
    fiftyFiftyRemovedOptions_.insert(wrongOptions.constLast());

    emit questionChanged(); // Обновляем отображение
}

void GameSession::applyAudienceHelp()
{
    const int correct = currentQuestion().correctAnswer() - 1;
    audienceDistribution_.resize(4);

    // Генерация правдоподобного распределения
    audienceDistribution_[correct] = QRandomGenerator::global()->bounded(70, 90);
    for (int i = 0; i < 4; ++i)
    {
        if (i != correct)
        {
            audienceDistribution_[i] = QRandomGenerator::global()->bounded(1, 30);
        }
    }

    // Нормализация до 100%
    int total = 0;
    for (int val : audienceDistribution_)
    {
        total += val;
    }
    for (int &val : audienceDistribution_)
    {
        val = (val * 100) / total;
    }

    emit audienceHelpGenerated(audienceDistribution_);
}

void GameSession::moveToNextQuestion()
{
    if (++currentQuestionIndex_ >= questions_.size())
    {
        endGame();
        return;
    }

    // Сброс состояния для нового вопроса
    fiftyFiftyRemovedOptions_.clear();
    audienceDistribution_.clear();
    secondChanceUsed_ = false;

    emit questionChanged();
}

void GameSession::endGame()
{
    state_ = State::FINISHED;
    timer_->stop();
    emit gameFinished(score_);
}

void GameSession::updateTimer()
{
    remainingTime_ = timeLimitSec_ - startTime_.secsTo(QDateTime::currentDateTime());

    if (remainingTime_ <= 0)
    {
        remainingTime_ = 0;
        state_ = State::TIME_EXPIRED;
        timer_->stop();
        emit timeExpired();
    }

    emit timerUpdated(remainingTime_);
}

void GameSession::initSoundEffects()
{
    correctSound_.setSource(QUrl("qrc:/sounds/correct.wav"));
    wrongSound_.setSource(QUrl("qrc:/sounds/wrong.wav"));
    lifelineSound_.setSource(QUrl("qrc:/sounds/lifeline.wav"));
}

void GameSession::playSoundEffect(const QString &resourcePath)
{
    QSoundEffect *effect = nullptr;

    if (resourcePath.contains("correct"))
        effect = &correctSound_;
    else if (resourcePath.contains("wrong"))
        effect = &wrongSound_;
    else if (resourcePath.contains("lifeline"))
        effect = &lifelineSound_;

    if (effect)
    {
        effect->play();
    }
}

void GameSession::resetLifelines()
{
    lifelinesUsed_.fill(false);
    hasSecondChance_ = false;
    secondChanceUsed_ = false;
    fiftyFiftyRemovedOptions_.clear();
    audienceDistribution_.clear();
}