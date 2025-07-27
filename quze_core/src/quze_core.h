#ifndef QUZE_CORE_H
#define QUZE_CORE_H

#include <napi.h>
#include <sqlite3.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <tuple>
#include <random>

class QuizClass : public Napi::ObjectWrap<QuizClass>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    QuizClass(const Napi::CallbackInfo &info);
    ~QuizClass();

private:
    // Внутренние методы
    int _getQuestionID();
    Napi::Array _getCategoryInfo(const Napi::Env &env);
    bool _prepareQuestionIds(const std::vector<std::string> &categoryNames, int level);
    Napi::Object _fetchQuestionData(int questionId, const Napi::Env &env);
    Napi::Object _buildQuestionObject(const char *text, const char *A, const char *B,
                                      const char *C, const char *D, const Napi::Env &env);

    // Методы, доступные из JavaScript
    Napi::Value Info(const Napi::CallbackInfo &info);
    Napi::Value Start(const Napi::CallbackInfo &info);
    Napi::Value help(const Napi::CallbackInfo &info);
    Napi::Value answer(const Napi::CallbackInfo &info);
    Napi::Value getQuestion(const Napi::CallbackInfo &info);

    // Поля класса
    sqlite3 *db;
    std::vector<int> questionIds;
    std::vector<std::string> categoryNames;
    int level;
    bool help5050;
    bool helpLife;
    bool helpAudience;
    bool helpLifeNext;
    std::string answCorrect;
};

// Инициализация модуля
Napi::Object Init(Napi::Env env, Napi::Object exports);

#endif // QUZE_CORE_H