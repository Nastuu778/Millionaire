// Проверка поддержки localStorage
const isLocalStorageAvailable = () => {
    try {
        const testKey = '__audio_test__';
        localStorage.setItem(testKey, testKey);
        localStorage.removeItem(testKey);
        return true;
    } catch {
        return false;
    }
};

// Основная функция управления аудио
const initAudio = () => {
    // Инициализация фоновой музыки
    const music = document.getElementById('backgroundMusic');
    if (music) {
        music.volume = 0.3;

        // Восстановление состояния
        if (isLocalStorageAvailable()) {
            const savedTime = localStorage.getItem('musicTime');
            const isPlaying = localStorage.getItem('musicPlaying') === 'true';

            if (savedTime) {
                music.currentTime = parseFloat(savedTime);
            }

            if (isPlaying) {
                const tryPlay = () => {
                    music.play()
                        .then(() => {
                            clearInterval(playAttempt);
                            document.removeEventListener('click', tryPlay);
                        })
                        .catch(e => console.debug("Ожидание взаимодействия..."));
                };

                const playAttempt = setInterval(tryPlay, 1000);
                document.addEventListener('click', tryPlay);
            }
        }

        // Сохранение состояния
        music.addEventListener('timeupdate', () => {
            if (isLocalStorageAvailable()) {
                localStorage.setItem('musicTime', music.currentTime);
            }
        });

        music.addEventListener('play', () => {
            if (isLocalStorageAvailable()) {
                localStorage.setItem('musicPlaying', 'true');
            }
        });

        music.addEventListener('pause', () => {
            if (isLocalStorageAvailable()) {
                localStorage.setItem('musicPlaying', 'false');
            }
        });
    }

    // Инициализация звуков клика
    const clickSound = document.getElementById('clickSound');
    if (clickSound) {
        clickSound.volume = 0.5;

        // Обработчик для всех кликабельных элементов
        const playClickSound = (e) => {
            if (e.target.closest('button, .hint, [data-click-sound]')) {
                clickSound.currentTime = 0;
                clickSound.play().catch(e => console.debug("Звук клика не воспроизведен"));
            }
        };

        document.addEventListener('click', playClickSound);

        // Специальная обработка для важных кнопок
        const specialButtons = [
            'start-game', 'modal-confirm',
            'hint-5050', 'hint-life', 'hint-audience'
        ];

        specialButtons.forEach(id => {
            const btn = document.getElementById(id);
            if (btn) {
                btn.addEventListener('click', () => {
                    clickSound.currentTime = 0;
                    clickSound.play().catch(e => console.debug("Звук кнопки не воспроизведен"));
                });
            }
        });
    }
};

// Инициализация при загрузке и после динамической загрузки контента
document.addEventListener('DOMContentLoaded', initAudio);
document.addEventListener('ajaxComplete', initAudio); // Для динамических страниц