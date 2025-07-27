#include "quze_core.h"

QuizClass::QuizClass(const Napi::CallbackInfo &info) : Napi::ObjectWrap<QuizClass>(info), db(nullptr)
{
  Napi::Env env = info.Env();

  help5050 = true;
  helpLife = true;
  helpAudience = true;

  // Проверка аргументов
  if (info.Length() < 1 || !info[0].IsString())
  {
    Napi::TypeError::New(env, "Path to SQLite DB expected").ThrowAsJavaScriptException();
    return;
  }

  std::string dbPath = info[0].As<Napi::String>().Utf8Value();

  // Проверка существования файла
  if (!std::filesystem::exists(dbPath))
  {
    std::string err = "Database file does not exist: " + dbPath;
    Napi::Error::New(env, err).ThrowAsJavaScriptException();
    return;
  }

  // Открытие БД с дополнительными параметрами
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX;
  int rc = sqlite3_open_v2(dbPath.c_str(), &db, flags, nullptr);

  if (rc != SQLITE_OK)
  {
    std::string err = "Database connection error (" + std::to_string(rc) + "): ";
    if (db)
    {
      err += sqlite3_errmsg(db);
      sqlite3_close(db);
      db = nullptr;
    }
    else
    {
      err += "No error message available";
    }
    Napi::Error::New(env, err).ThrowAsJavaScriptException();
    return;
  }

  std::cout << "[C++] Successfully connected to database: " << dbPath << std::endl;
  std::cout << "[C++] SQLite version: " << sqlite3_libversion() << std::endl;
}

QuizClass::~QuizClass()
{
  if (db)
  {
    sqlite3_close(db);
    std::cout << "[C++] Database connection closed" << std::endl;
  }
}

// Инициализация класса (без изменений)
Napi::Object QuizClass::Init(Napi::Env env, Napi::Object exports)
{
  exports.Set("QuizClass",
              DefineClass(env, "QuizClass",
                          {InstanceMethod("start", &QuizClass::Start),
                           InstanceMethod("info", &QuizClass::Info),
                           InstanceMethod("getQuestion", &QuizClass::getQuestion),
                           InstanceMethod("answer", &QuizClass::answer),
                           InstanceMethod("help", &QuizClass::help)}));
  return exports;
}

int QuizClass::_getQuestionID()
{ // Получить ID очередного вопроса
  if (questionIds.empty())
    return -1;
  // Выбираем случайный вопрос
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, questionIds.size() - 1);
  int randomIndex = dist(gen);
  int selectedQuestion = questionIds[randomIndex];

  // Удаляем выбранный вопрос из массива
  questionIds.erase(questionIds.begin() + randomIndex);
  return selectedQuestion;
}

Napi::Array QuizClass::_getCategoryInfo(const Napi::Env &env)
{ // Добавить информацию о категориях
  Napi::Array jsArray = Napi::Array::New(env);

  const char *sql_ctg = "SELECT id, name FROM category;";
  const char *sql = "SELECT count(*) FROM questions WHERE id_category==?;";
  sqlite3_stmt *stm_ctg = nullptr, *stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql_ctg, -1, &stm_ctg, nullptr) != SQLITE_OK)
    return jsArray;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return jsArray;

  // Перебор категорий
  uint32_t index = 0;
  while (sqlite3_step(stm_ctg) == SQLITE_ROW)
  {
    int id = sqlite3_column_int(stm_ctg, 0);                          // Колонка 0 = id
    const char *name = (const char *)sqlite3_column_text(stm_ctg, 1); // Колонка 1 = name
    sqlite3_bind_int(stmt, 1, id);
    int cnt = (sqlite3_step(stmt) == SQLITE_ROW) ? sqlite3_column_int(stmt, 0) : -1; // Выполняем запрос
                                                                                     //   int cnt = sqlite3_column_int(stmt, 0);

    // Создаем объект категории
    Napi::Object category = Napi::Object::New(env);
    category.Set("name", Napi::String::New(env, name));
    category.Set("count", Napi::Number::New(env, cnt));

    jsArray[index++] = category; // Добавляем в массив

    // Сбрасываем statement
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    printf("id: %d, name: %s, count: %d\n", id, name, cnt);
  }

  sqlite3_finalize(stm_ctg); // Освобождаем ресурсы
  sqlite3_finalize(stmt);

  return jsArray;
}

bool QuizClass::_prepareQuestionIds(const std::vector<std::string> &categoryNames, int level)
{
  questionIds.clear();

  // Подготавливаем SQL запрос
  sqlite3_stmt *stmt;
  std::string sql = "SELECT q.id FROM questions q "
                    "JOIN category c ON q.id_category = c.id "
                    "JOIN level l ON q.id_level = l.id "
                    "WHERE c.name IN (";

  // Добавляем плейсхолдеры для каждой категории
  for (size_t i = 0; i < categoryNames.size(); i++)
  {
    sql += (i == 0) ? "?" : ", ?";
  }

  sql += ") AND l.id = ? ORDER BY RANDOM()";

  // Подготавливаем запрос
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
  {
    return false;
  }

  // Привязываем параметры категорий
  for (size_t i = 0; i < categoryNames.size(); i++)
  {
    sqlite3_bind_text(stmt, i + 1, categoryNames[i].c_str(), -1, SQLITE_TRANSIENT);
  }

  sqlite3_bind_int(stmt, 1 + categoryNames.size(), level);
  // Выполняем запрос и собираем результаты
  while (sqlite3_step(stmt) == SQLITE_ROW)
  {
    questionIds.push_back(sqlite3_column_int(stmt, 0));
  }

  // Сбрасываем statement
  sqlite3_finalize(stmt);
  return true;
}

Napi::Object QuizClass::_fetchQuestionData(int questionId, const Napi::Env &env)
{
  sqlite3_stmt *stmt;
  std::string sql = "SELECT q.text, q.A, q.B, q.C, q.D, q.answer, q.id_category, q.id_level FROM questions q WHERE q.id=?";

  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
  {
    Napi::Error::New(env, "Failed to prepare SQL statement").ThrowAsJavaScriptException();
    return Napi::Object::New(env);
  }

  sqlite3_bind_int(stmt, 1, questionId);
  if (sqlite3_step(stmt) != SQLITE_ROW)
  {
    sqlite3_finalize(stmt);
    Napi::Error::New(env, "Question not found").ThrowAsJavaScriptException();
    return Napi::Object::New(env);
  }

  const char *text = (const char *)sqlite3_column_text(stmt, 0);
  std::cout << "[C++] " << text << std::endl;
  const char *A = (const char *)sqlite3_column_text(stmt, 1);
  const char *B = (const char *)sqlite3_column_text(stmt, 2);
  const char *C = (const char *)sqlite3_column_text(stmt, 3);
  const char *D = (const char *)sqlite3_column_text(stmt, 4);
  answCorrect = (const char *)sqlite3_column_text(stmt, 5);

  Napi::Object result = Napi::Object::New(env);
  result.Set("text", Napi::String::New(env, text));
  result.Set("A", Napi::String::New(env, A));
  result.Set("B", Napi::String::New(env, B));
  result.Set("C", Napi::String::New(env, C));
  result.Set("D", Napi::String::New(env, D));

  sqlite3_finalize(stmt);
  return result;
}

Napi::Object QuizClass::_buildQuestionObject(const char *text, const char *A, const char *B,
                                             const char *C, const char *D, const Napi::Env &env)
{
  Napi::Object result = Napi::Object::New(env);
  result.Set("text", Napi::String::New(env, text));
  result.Set("A", Napi::String::New(env, A));
  result.Set("B", Napi::String::New(env, B));
  result.Set("C", Napi::String::New(env, C));
  result.Set("D", Napi::String::New(env, D));

  return result;
}

Napi::Value QuizClass::Info(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  Napi::Object obj = Napi::Object::New(env);
  obj.Set("category", _getCategoryInfo(env));
  return obj;
}

Napi::Value QuizClass::Start(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  level = 1;
  help5050 = true;
  helpLife = true;
  helpAudience = true;
  helpLifeNext = false;

  // Проверка входных аргументов
  if (info.Length() < 1 || !info[0].IsObject() ||
      !info[0].As<Napi::Object>().Has("category") ||
      !info[0].As<Napi::Object>().Get("category").IsArray())
  {
    Napi::TypeError::New(env, "Expected object with 'category' array").ThrowAsJavaScriptException();
    return env.Null();
  }

  // Получаем массив категорий
  Napi::Array categories = info[0].As<Napi::Object>().Get("category").As<Napi::Array>();
  categoryNames.clear();

  // Собираем имена категорий
  for (uint32_t i = 0; i < categories.Length(); i++)
  {
    Napi::Value val = categories[i];
    if (val.IsString())
    {
      categoryNames.push_back(val.As<Napi::String>().Utf8Value());
    }
  }

  // Подготавливаем ID вопросов
  if (!_prepareQuestionIds(categoryNames, 1))
  {
    Napi::Error::New(env, "Failed to prepare question IDs").ThrowAsJavaScriptException();
    return env.Null();
  }

  // Получаем ID вопроса, случайный
  int questionId = _getQuestionID();
  if (questionId == -1)
  {
    Napi::Error::New(env, "No questions available").ThrowAsJavaScriptException();
    return env.Null();
  }

  // Получаем данные вопроса и формируем результат
  return _fetchQuestionData(questionId, env);
}
Napi::Value QuizClass::getQuestion(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  Napi::Object result = Napi::Object::New(env);

  // 1. Получаем ID вопроса
  int questionId = _getQuestionID();
  if (questionId == -1)
  {
    Napi::Error::New(env, "No more questions available").ThrowAsJavaScriptException();
    return env.Null();
  }

  // 2. Получаем данные вопроса (уже содержит текст и варианты ответов)
  Napi::Object questionData = _fetchQuestionData(questionId, env);

  // 3. Добавляем текущие состояния подсказок и уровня
  result.Set("question", questionData);
  result.Set("help5050", Napi::Boolean::New(env, help5050));
  result.Set("helpLife", Napi::Boolean::New(env, helpLife));
  result.Set("helpAudience", Napi::Boolean::New(env, helpAudience));
  result.Set("level", Napi::Number::New(env, level));

  return result;
}

Napi::Value QuizClass::answer(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();
  // Одним условием проверяем все требования:
  if (info.Length() == 0 ||
      !info[0].IsObject() ||
      !info[0].As<Napi::Object>().Has("answer") ||
      !info[0].As<Napi::Object>().Get("answer").IsString())
  {
    Napi::TypeError::New(env, "Требуется объект с строковым свойством 'answer'").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string userAnswer = info[0].As<Napi::Object>().Get("answer").As<Napi::String>();
  Napi::Object result = Napi::Object::New(env);
  result.Set("result", Napi::Boolean::New(env, userAnswer == answCorrect));
  result.Set("life", Napi::Boolean::New(env, helpLifeNext));
  if (userAnswer == answCorrect)
  { //
    level++;
    if (level == 6)
    {
      _prepareQuestionIds(categoryNames, 2);
    }
    if (level == 11)
    {
      _prepareQuestionIds(categoryNames, 3);
    }
  }
  else
  {
    if (helpLifeNext)
    {
      helpLifeNext = false;
    }
    else
    {
      result.Set("answer", Napi::String::New(env, answCorrect));
    }
  }

  return result;
}

Napi::Value QuizClass::help(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  // 1. Проверка аргументов
  if (info.Length() < 1 || !info[0].IsObject())
  {
    Napi::TypeError::New(env, "Expected object with 'type' property").ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Object params = info[0].As<Napi::Object>();
  if (!params.Has("type") || !params.Get("type").IsString())
  {
    Napi::TypeError::New(env, "Missing or invalid 'type' parameter").ThrowAsJavaScriptException();
    return env.Null();
  }

  std::string helpType = params.Get("type").As<Napi::String>();
  Napi::Object result = Napi::Object::New(env);

  // 2. Обработка разных типов подсказок
  if (helpType == "5050")
  {
    if (!help5050)
    {
      Napi::Error::New(env, "Подсказка 50/50 уже использована").ThrowAsJavaScriptException();
      return env.Null();
    }
    help5050 = false;

    // Формируем структуру для 50/50
    Napi::Object questionEnable = Napi::Object::New(env);
    // Оставляем правильный ответ и один случайный неправильный
    std::vector<std::string> options = {"A", "B", "C", "D"};
    options.erase(std::remove(options.begin(), options.end(), answCorrect), options.end());
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(options.begin(), options.end(), gen);
    std::string wrongAnswer = options[0];

    for (const auto &opt : {"A", "B", "C", "D"})
    {
      questionEnable.Set(opt, Napi::Boolean::New(env, opt == answCorrect || opt == wrongAnswer));
    }

    result.Set("question_enable", questionEnable);
  }
  else if (helpType == "audience")
  {
    if (!helpAudience)
    {
      Napi::Error::New(env, "Подсказка 'Зал' уже использована").ThrowAsJavaScriptException();
      return env.Null();
    }
    helpAudience = false;

    // Генерация случайных процентов голосования
    Napi::Object questionPostfix = Napi::Object::New(env);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> correctDist(50, 80); // Правильный ответ получает больше

    int correctPercent = correctDist(gen);
    questionPostfix.Set(answCorrect, Napi::String::New(env, "(" + std::to_string(correctPercent) + "%)"));

    // Распределяем оставшиеся проценты
    int remaining = 100 - correctPercent;
    std::vector<std::string> wrongOptions = {"A", "B", "C", "D"};
    wrongOptions.erase(std::remove(wrongOptions.begin(), wrongOptions.end(), answCorrect), wrongOptions.end());

    for (size_t i = 0; i < wrongOptions.size(); i++)
    {
      int value = (i == wrongOptions.size() - 1) ? remaining : std::min(remaining, static_cast<int>((gen() % 30) + 5));
      questionPostfix.Set(wrongOptions[i], Napi::String::New(env, "(" + std::to_string(value) + "%)"));
      remaining -= value;
    }

    result.Set("question_postfix", questionPostfix);
  }
  else if (helpType == "life")
  {
    if (!helpLife)
    {
      Napi::Error::New(env, "Подсказка 'Жизнь' уже использована").ThrowAsJavaScriptException();
      return env.Null();
    }
    helpLife = false;
    helpLifeNext = true;
    // Для подсказки "жизнь" возвращаем пустой объект
    result.Set("success", Napi::Boolean::New(env, true));
  }
  else
  {
    Napi::Error::New(env, "Неизвестный тип подсказки").ThrowAsJavaScriptException();
    return env.Null();
  }

  // 3. Возвращаем обновлённые состояния подсказок
  result.Set("help5050", Napi::Boolean::New(env, help5050));
  result.Set("helpLife", Napi::Boolean::New(env, helpLife));
  result.Set("helpAudience", Napi::Boolean::New(env, helpAudience));

  return result;
}

// Получить случайный вопрос по выбранным категориям
// Инициализация модуля (без изменений)

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
  QuizClass::Init(env, exports);
  return exports;
}

NODE_API_MODULE(quze_core, Init)