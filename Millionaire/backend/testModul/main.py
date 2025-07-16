import sys
import os

# Добавляем текущую папку в путь
sys.path.append(os.path.dirname(__file__))

try:
    import quiz_core
    print("Модуль успешно загружен!")
    print(f"Путь к модулю: {quiz_core.__file__}")
    
    # Простой тест
    q = quiz_core.Question()
    q.text = "Тестовый вопрос"
    print(f"Создан вопрос: {q.text}")
    
except Exception as e:
    print(f"Ошибка загрузки модуля: {e}")