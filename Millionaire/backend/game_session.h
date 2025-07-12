#ifndef GAME_SESSION_H
#define GAME_SESSION_H

#include <vector>
#include <string>
#include <memory>
#include <ctime>
#include <unordered_set>
#include "question.h"
#include "question_manager.h"

class GameSession
{
public:
    enum class State
    {
        NOT_STARTED,
        IN_PROGRESS,
        ANSWER_SUBMITTED,
        FINISHED,
        TIME_EXPIRED
    };

    enum class Lifeline
    {
        FIFTY_FIFTY,
        AUDIENCE_HELP,
        SECOND_CHANCE,
        COUNT
    };

    struct SessionData
    {
        std::string playerName;
        int currentQuestionIndex = 0;
        int score = 0;
        int correctAnswers = 0;
        int wrongAnswers = 0;
        std::vector<bool> usedLifelines;
        std::vector<Question> questions;
        time_t startTime = 0;
        time_t endTime = 0;
        int selectedCategoryId = -1;
        bool secondChanceAvailable = false;
        int timeLimitSec = 180;
    };

    explicit GameSession(std::shared_ptr<QuestionManager> questionManager);

    bool startSession(const std::string &playerName, int categoryId = -1, int timeLimitSec = 180);
    bool submitAnswer(int answerIndex);
    bool useLifeline(Lifeline lifeline);
    bool nextQuestion();
    void update();

    bool saveSession(const std::string &filename) const;
    bool loadSession(const std::string &filename);

    State getState() const { return state_; }
    const Question &getCurrentQuestion() const;
    int getScore() const { return data_.score; }
    const std::string &getPlayerName() const { return data_.playerName; }
    double getElapsedTime() const;
    double getRemainingTime() const;
    const std::vector<bool> &getUsedLifelines() const { return data_.usedLifelines; }
    const std::vector<Question> &getQuestions() const { return data_.questions; }
    int getCurrentQuestionIndex() const { return data_.currentQuestionIndex; }
    bool isFinished() const { return state_ == State::FINISHED || state_ == State::TIME_EXPIRED; }
    int getSelectedCategoryId() const { return data_.selectedCategoryId; }
    int getCorrectAnswersCount() const { return data_.correctAnswers; }
    int getWrongAnswersCount() const { return data_.wrongAnswers; }
    int getTimeLimit() const { return data_.timeLimitSec; }
    int getQuestionsCount() const { return static_cast<int>(data_.questions.size()); }

    std::vector<int> getFiftyFiftyOptions() const;
    std::vector<int> getAudienceHelpDistribution() const;
    bool hasSecondChance() const { return data_.secondChanceAvailable; }

private:
    void generateQuestions(int categoryId, int estimatedCount);
    void updateScore(bool correct);
    void finishGame();
    void checkTimeExpiration();
    void applyFiftyFifty();
    void applyAudienceHelp();
    void applySecondChance();

    std::shared_ptr<QuestionManager> questionManager_;
    SessionData data_;
    State state_ = State::NOT_STARTED;
    std::unordered_set<int> fiftyFiftyRemovedOptions_;
    std::vector<int> audienceHelpDistribution_;
};

#endif // GAME_SESSION_H