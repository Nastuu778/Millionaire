#include "game_session.h"
#include "json.hpp"
#include <fstream>
#include <random>
#include <algorithm>
#include <ctime>

using json = nlohmann::json;

GameSession::GameSession(std::shared_ptr<QuestionManager> questionManager)
    : questionManager_(questionManager), state_(State::NOT_STARTED)
{
    data_.usedLifelines.resize(static_cast<int>(Lifeline::COUNT), false);
}

bool GameSession::startSession(const std::string &playerName, int categoryId, int timeLimitSec)
{
    if (state_ != State::NOT_STARTED)
    {
        return false;
    }

    data_.playerName = playerName;
    data_.selectedCategoryId = categoryId;
    data_.startTime = time(nullptr);
    data_.timeLimitSec = timeLimitSec;
    data_.currentQuestionIndex = 0;
    data_.score = 0;
    data_.correctAnswers = 0;
    data_.wrongAnswers = 0;
    data_.secondChanceAvailable = false;
    std::fill(data_.usedLifelines.begin(), data_.usedLifelines.end(), false);

    int estimatedQuestions = timeLimitSec / 12;
    generateQuestions(categoryId, std::max(10, estimatedQuestions));

    if (data_.questions.empty())
    {
        return false;
    }

    state_ = State::IN_PROGRESS;
    return true;
}

void GameSession::update()
{
    if (state_ == State::IN_PROGRESS || state_ == State::ANSWER_SUBMITTED)
    {
        checkTimeExpiration();
    }
}

void GameSession::checkTimeExpiration()
{
    if (getRemainingTime() <= 0)
    {
        state_ = State::TIME_EXPIRED;
        finishGame();
    }
}

double GameSession::getRemainingTime() const
{
    if (state_ == State::NOT_STARTED)
    {
        return 0;
    }
    double elapsed = getElapsedTime();
    return std::max(0.0, data_.timeLimitSec - elapsed);
}

bool GameSession::submitAnswer(int answerIndex)
{
    if (state_ != State::IN_PROGRESS)
    {
        return false;
    }

    bool isCorrect = data_.questions[data_.currentQuestionIndex].isCorrect(answerIndex + 1);

    if (isCorrect)
    {
        data_.correctAnswers++;
        updateScore(true);
        state_ = State::ANSWER_SUBMITTED;
    }
    else if (hasSecondChance())
    {
        data_.secondChanceAvailable = false;
        state_ = State::IN_PROGRESS;
    }
    else
    {
        data_.wrongAnswers++;
        updateScore(false);
        finishGame();
    }

    return isCorrect;
}

bool GameSession::useLifeline(Lifeline lifeline)
{
    if (state_ != State::IN_PROGRESS || data_.usedLifelines[static_cast<int>(lifeline)])
    {
        return false;
    }

    data_.usedLifelines[static_cast<int>(lifeline)] = true;

    switch (lifeline)
    {
    case Lifeline::FIFTY_FIFTY:
        applyFiftyFifty();
        break;
    case Lifeline::AUDIENCE_HELP:
        applyAudienceHelp();
        break;
    case Lifeline::SECOND_CHANCE:
        applySecondChance();
        break;
    default:
        return false;
    }

    return true;
}

bool GameSession::nextQuestion()
{
    if (state_ != State::ANSWER_SUBMITTED)
    {
        return false;
    }

    data_.currentQuestionIndex++;
    fiftyFiftyRemovedOptions_.clear();
    audienceHelpDistribution_.clear();

    if (data_.currentQuestionIndex >= static_cast<int>(data_.questions.size()))
    {
        finishGame();
    }
    else
    {
        state_ = State::IN_PROGRESS;
    }

    return true;
}

void GameSession::generateQuestions(int categoryId, int estimatedCount)
{
    int count = std::min(estimatedCount + 5, 50);
    data_.questions = (categoryId == -1)
                          ? questionManager_->getRandomQuestions(count)
                          : questionManager_->getRandomQuestions(count, categoryId);
}

void GameSession::updateScore(bool correct)
{
    if (correct)
    {
        data_.score += (data_.currentQuestionIndex + 1) * 100;
    }
    else
    {
        data_.score = std::max(0, data_.score - 200);
    }
}

void GameSession::finishGame()
{
    state_ = State::FINISHED;
    data_.endTime = time(nullptr);
}

void GameSession::applyFiftyFifty()
{
    const auto &options = data_.questions[data_.currentQuestionIndex].getOptions();
    int correct = data_.questions[data_.currentQuestionIndex].getCorrectAnswer() - 1;

    std::vector<int> wrongOptions;
    for (int i = 0; i < static_cast<int>(options.size()); ++i)
    {
        if (i != correct)
        {
            wrongOptions.push_back(i);
        }
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(wrongOptions.begin(), wrongOptions.end(), g);

    fiftyFiftyRemovedOptions_.insert(wrongOptions[0]);
    fiftyFiftyRemovedOptions_.insert(wrongOptions[1]);
}

void GameSession::applyAudienceHelp()
{
    const int correct = data_.questions[data_.currentQuestionIndex].getCorrectAnswer() - 1;
    const int optionCount = static_cast<int>(
        data_.questions[data_.currentQuestionIndex].getOptions().size());

    audienceHelpDistribution_.resize(optionCount, 0);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> correctDist(70, 10);
    std::normal_distribution<> wrongDist(10, 5);

    int total = 0;
    for (int i = 0; i < optionCount; ++i)
    {
        if (i == correct)
        {
            audienceHelpDistribution_[i] = std::clamp(static_cast<int>(correctDist(gen)), 50, 90);
        }
        else
        {
            audienceHelpDistribution_[i] = std::clamp(static_cast<int>(wrongDist(gen)), 1, 30);
        }
        total += audienceHelpDistribution_[i];
    }

    for (int i = 0; i < optionCount; ++i)
    {
        audienceHelpDistribution_[i] = (audienceHelpDistribution_[i] * 100) / total;
    }
}

void GameSession::applySecondChance()
{
    data_.secondChanceAvailable = true;
}

bool GameSession::saveSession(const std::string &filename) const
{
    try
    {
        json j;

        j["playerName"] = data_.playerName;
        j["currentQuestionIndex"] = data_.currentQuestionIndex;
        j["score"] = data_.score;
        j["correctAnswers"] = data_.correctAnswers;
        j["wrongAnswers"] = data_.wrongAnswers;
        j["startTime"] = data_.startTime;
        j["endTime"] = data_.endTime;
        j["selectedCategoryId"] = data_.selectedCategoryId;
        j["secondChanceAvailable"] = data_.secondChanceAvailable;
        j["timeLimitSec"] = data_.timeLimitSec;

        j["usedLifelines"] = json::array();
        for (bool used : data_.usedLifelines)
        {
            j["usedLifelines"].push_back(used);
        }

        j["questions"] = json::array();
        for (const auto &question : data_.questions)
        {
            j["questions"].push_back(question.toJson());
        }

        if (!fiftyFiftyRemovedOptions_.empty())
        {
            j["fiftyFiftyRemoved"] = json::array();
            for (int option : fiftyFiftyRemovedOptions_)
            {
                j["fiftyFiftyRemoved"].push_back(option);
            }
        }

        if (!audienceHelpDistribution_.empty())
        {
            j["audienceHelp"] = audienceHelpDistribution_;
        }

        std::ofstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        file << j.dump(4);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool GameSession::loadSession(const std::string &filename)
{
    try
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return false;
        }

        json j;
        file >> j;

        data_.playerName = j["playerName"].get<std::string>();
        data_.currentQuestionIndex = j["currentQuestionIndex"].get<int>();
        data_.score = j["score"].get<int>();
        data_.correctAnswers = j["correctAnswers"].get<int>();
        data_.wrongAnswers = j["wrongAnswers"].get<int>();
        data_.startTime = j["startTime"].get<time_t>();
        data_.endTime = j["endTime"].get<time_t>();
        data_.selectedCategoryId = j["selectedCategoryId"].get<int>();
        data_.secondChanceAvailable = j["secondChanceAvailable"].get<bool>();
        data_.timeLimitSec = j["timeLimitSec"].get<int>();

        data_.usedLifelines.clear();
        for (const auto &item : j["usedLifelines"])
        {
            data_.usedLifelines.push_back(item.get<bool>());
        }

        data_.questions.clear();
        for (const auto &item : j["questions"])
        {
            data_.questions.push_back(Question::fromJson(item.get<std::string>()));
        }

        if (j.contains("fiftyFiftyRemoved"))
        {
            fiftyFiftyRemovedOptions_.clear();
            for (const auto &item : j["fiftyFiftyRemoved"])
            {
                fiftyFiftyRemovedOptions_.insert(item.get<int>());
            }
        }

        if (j.contains("audienceHelp"))
        {
            audienceHelpDistribution_ = j["audienceHelp"].get<std::vector<int>>();
        }

        if (j.contains("endTime"))
        {
            state_ = State::FINISHED;
        }
        else
        {
            state_ = (data_.currentQuestionIndex < static_cast<int>(data_.questions.size()))
                         ? State::IN_PROGRESS
                         : State::FINISHED;
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

const Question &GameSession::getCurrentQuestion() const
{
    if (data_.currentQuestionIndex >= static_cast<int>(data_.questions.size()))
    {
        throw std::out_of_range("No current question");
    }
    return data_.questions[data_.currentQuestionIndex];
}

double GameSession::getElapsedTime() const
{
    time_t end = (state_ == State::FINISHED || state_ == State::TIME_EXPIRED)
                     ? data_.endTime
                     : time(nullptr);
    return difftime(end, data_.startTime);
}

std::vector<int> GameSession::getFiftyFiftyOptions() const
{
    std::vector<int> result;
    const auto &options = getCurrentQuestion().getOptions();

    for (int i = 0; i < static_cast<int>(options.size()); ++i)
    {
        if (fiftyFiftyRemovedOptions_.count(i) == 0)
        {
            result.push_back(i);
        }
    }

    return result;
}

std::vector<int> GameSession::getAudienceHelpDistribution() const
{
    return audienceHelpDistribution_;
}