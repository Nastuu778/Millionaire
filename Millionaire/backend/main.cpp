#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include "core/question_manager.h"
#include "core/game_session.h"

int main(int argc, char *argv[])
{
    // Настройка приложения
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/images/logo.png"));
    app.setApplicationName("Кто хочет стать миллионером");
    app.setApplicationVersion("1.0");

    // Инициализация менеджера вопросов
    const QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/questions.db";
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    auto questionManager = QSharedPointer<QuestionManager>::create(dbPath);

    // Проверка и создание тестовых данных при первом запуске
    if (questionManager->getTotalQuestionsCount() == 0)
    {
        questionManager->importFromJson(":/data/default_questions.json");
    }

    // Создание игровой сессии
    auto gameSession = new GameSession(questionManager);

    // Настройка QML движка
    QQmlApplicationEngine engine;

    // Регистрация типов для QML
    qmlRegisterType<GameSession>("Millionaire.Core", 1, 0, "GameSession");
    qmlRegisterType<Question>("Millionaire.Core", 1, 0, "Question");

    // Экспорт объектов в QML
    engine.rootContext()->setContextProperty("questionManager", questionManager.get());
    engine.rootContext()->setContextProperty("gameSession", gameSession);

    // Загрузка главного QML-файла
    engine.load("qrc:/qml/main.qml");

    if (engine.rootObjects().isEmpty())
    {
        qCritical() << "Failed to load QML interface";
        return -1;
    }

    return app.exec();
}