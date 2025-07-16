#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "question.h"
#include "question_manager.h"
#include "game_session.h"

namespace py = pybind11;

PYBIND11_MODULE(quiz_core, m)
{
    // ===== Question =====
    py::class_<Question>(m, "Question")
        .def(py::init<>())
        .def(py::init<const std::string &, const std::vector<std::string> &, int, int>(),
             py::arg("text"), py::arg("options"), py::arg("correct_answer"), py::arg("category_id") = -1)
        .def("getText", &Question::getText)
        .def("getOptions", &Question::getOptions)
        .def("getCorrectAnswer", &Question::getCorrectAnswer)
        .def("getCategoryId", &Question::getCategoryId)
        .def("isCorrect", &Question::isCorrect)
        .def("toJson", &Question::toJson)
        .def_static("fromJson", &Question::fromJson);

    // ===== QuestionManager =====
    py::class_<QuestionManager>(m, "QuestionManager")
        .def(py::init<const std::string &>())
        .def("addQuestion", &QuestionManager::addQuestion, py::arg("question"), py::arg("category_id") = -1)
        .def("removeQuestion", &QuestionManager::removeQuestion)
        .def("updateQuestion", &QuestionManager::updateQuestion)
        .def("getRandomQuestions", &QuestionManager::getRandomQuestions,
             py::arg("count"), py::arg("category_id") = -1)
        .def("getQuestionsByCategory", &QuestionManager::getQuestionsByCategory,
             py::arg("category_id"), py::arg("limit") = -1)
        .def("addCategory", &QuestionManager::addCategory)
        .def("removeCategory", &QuestionManager::removeCategory)
        .def("updateCategory", &QuestionManager::updateCategory)
        .def("getAllCategories", &QuestionManager::getAllCategories)
        .def("getTotalQuestionsCount", &QuestionManager::getTotalQuestionsCount)
        .def("getCategoryQuestionsCount", &QuestionManager::getCategoryQuestionsCount)
        .def("questionExists", &QuestionManager::questionExists);

    // ===== GameSession =====
    py::enum_<GameSession::State>(m, "GameState")
        .value("NOT_STARTED", GameSession::State::NOT_STARTED)
        .value("IN_PROGRESS", GameSession::State::IN_PROGRESS)
        .value("ANSWER_SUBMITTED", GameSession::State::ANSWER_SUBMITTED)
        .value("FINISHED", GameSession::State::FINISHED)
        .value("TIME_EXPIRED", GameSession::State::TIME_EXPIRED)
        .export_values();

    py::enum_<GameSession::Lifeline>(m, "Lifeline")
        .value("FIFTY_FIFTY", GameSession::Lifeline::FIFTY_FIFTY)
        .value("AUDIENCE_HELP", GameSession::Lifeline::AUDIENCE_HELP)
        .value("SECOND_CHANCE", GameSession::Lifeline::SECOND_CHANCE)
        .value("COUNT", GameSession::Lifeline::COUNT)
        .export_values();

    py::class_<GameSession>(m, "GameSession")
        .def(py::init<std::shared_ptr<QuestionManager>>())
        .def("startSession", &GameSession::startSession,
             py::arg("player_name"), py::arg("category_id") = -1, py::arg("time_limit_sec") = 180)
        .def("submitAnswer", &GameSession::submitAnswer)
        .def("useLifeline", &GameSession::useLifeline)
        .def("nextQuestion", &GameSession::nextQuestion)
        .def("update", &GameSession::update)
        .def("saveSession", &GameSession::saveSession)
        .def("loadSession", &GameSession::loadSession)
        .def("getState", &GameSession::getState)
        .def("getCurrentQuestion", &GameSession::getCurrentQuestion)
        .def("getScore", &GameSession::getScore)
        .def("getPlayerName", &GameSession::getPlayerName)
        .def("getElapsedTime", &GameSession::getElapsedTime)
        .def("getRemainingTime", &GameSession::getRemainingTime)
        .def("getUsedLifelines", &GameSession::getUsedLifelines)
        .def("getQuestions", &GameSession::getQuestions)
        .def("getCurrentQuestionIndex", &GameSession::getCurrentQuestionIndex)
        .def("isFinished", &GameSession::isFinished)
        .def("getSelectedCategoryId", &GameSession::getSelectedCategoryId)
        .def("getCorrectAnswersCount", &GameSession::getCorrectAnswersCount)
        .def("getWrongAnswersCount", &GameSession::getWrongAnswersCount)
        .def("getTimeLimit", &GameSession::getTimeLimit)
        .def("getQuestionsCount", &GameSession::getQuestionsCount)
        .def("getFiftyFiftyOptions", &GameSession::getFiftyFiftyOptions)
        .def("getAudienceHelpDistribution", &GameSession::getAudienceHelpDistribution)
        .def("hasSecondChance", &GameSession::hasSecondChance);
}