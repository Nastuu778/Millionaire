class Game {
    constructor() {
        this.currentLevel = 1;
        this.totalLevels = 15;
        this.hasSecondLife = false;
        this.usedSecondLife = false;
        this.hints = {
            '5050': { used: false, element: null },
            'life': { used: false, element: null },
            'audience': { used: false, element: null }
        };

        this.prizes = [
            100, 200, 300, 500, 1000, 
            2000, 4000, 8000, 16000, 32000, 
            64000, 125000, 250000, 500000, 1000000
        ];

        this.init();
    }

    init() {
        document.addEventListener('DOMContentLoaded', () => {
            this.canvas = document.getElementById('progress-canvas');
            this.ctx = this.canvas.getContext('2d');
            window.addEventListener('resize', () => {
                this.drawProgress();
            });
            
            this.modal = document.getElementById('modal');
            this.modalMessage = document.getElementById('modal-message');
            this.modalConfirm = document.getElementById('modal-confirm');
            this.modalConfirm.addEventListener('click', () => this.closeModal());

            this.initHints();
            this.initAnswerHandlers();
            this.loadQuestion();
        });
    }

    async loadQuestion() {
        try {
            const response = await fetch('/api/getQuestion');
            if (!response.ok) throw new Error('Network response was not ok');

            const data = await response.json();
            
            const decodedQuestion = {
                text: this.decodeText(data.question.text),
                A: this.decodeText('A: '+data.question.A),
                B: this.decodeText('B: '+data.question.B),
                C: this.decodeText('C: '+data.question.C),
                D: this.decodeText('D: '+data.question.D)
            };
            
            this.displayQuestion(decodedQuestion);
            this.currentLevel = data.level;
            this.drawProgress();
            
            if (data.help5050 !== undefined) {
                this.hints['5050'].used = !data.help5050;
                this.hints['5050'].element.classList.toggle('used', !data.help5050);
            }
            if (data.helpLife !== undefined) {
                this.hints['life'].used = !data.helpLife;
                this.hints['life'].element.classList.toggle('used', !data.helpLife);
                this.hasSecondLife = data.helpLife;
            }
            if (data.helpAudience !== undefined) {
                this.hints['audience'].used = !data.helpAudience;
                this.hints['audience'].element.classList.toggle('used', !data.helpAudience);
            }
            
        } catch (error) {
            console.error('Error loading question:', error);
            this.showModal('Произошла ошибка при загрузке вопроса. Пожалуйста, попробуйте снова.');
        }
    }

    decodeText(text) {
        try {
            return decodeURIComponent(escape(text));
        } catch (e) {
            return text;
        }
    }

    displayQuestion(question) {
        document.getElementById('question').textContent = question.text;

        const answers = document.querySelectorAll('.answer');
        answers.forEach((btn, index) => {
            const option = ['A', 'B', 'C', 'D'][index];
            btn.textContent = question[option];
            btn.style.display = 'flex';
            btn.disabled = false;
            btn.classList.remove('selected', 'correct', 'incorrect');
        });
    }

    drawProgress() {
        if (!this.canvas) return;
        
        const canvas = this.canvas;
        const ctx = this.ctx;
        const width = canvas.clientWidth;
        const height = canvas.clientHeight;
        
        canvas.width = width;
        canvas.height = height;
        
        ctx.clearRect(0, 0, width, height);
        
        // Гибридный расчет: сочетание линейной и логарифмической шкал
        const minPrize = Math.min(...this.prizes);
        const maxPrize = Math.max(...this.prizes);
        
        // Коэффициенты для гибридного расчета
        const linearWeight = 0.4;  // Вес линейной составляющей
        const logWeight = 0.6;      // Вес логарифмической составляющей
        
        // Рассчитываем коэффициенты для длин полос
        const minBarLength = width * 0.05;
        const maxBarLength = width * 0.95;
        
        // Рисуем все уровни
        const safeLevels = [5, 10, 15];
        const barHeight = height / this.totalLevels * 0.8;
        const gap = height / this.totalLevels * 0.2;
        
        // Предварительный расчет всех длин для правильного масштабирования
        const barLengths = [];
        let maxScaledLength = 0;
        
        for (let i = 0; i < this.totalLevels; i++) {
            const prize = this.prizes[i];
            
            // Линейная составляющая
            const linearPart = (prize - minPrize) / (maxPrize - minPrize);
            
            // Логарифмическая составляющая
            const logPart = Math.log10(prize / minPrize) / Math.log10(maxPrize / minPrize);
            
            // Гибридное значение (0-1)
            const hybridValue = linearPart * linearWeight + logPart * logWeight;
            
            // Длина полосы
            const barLength = minBarLength + hybridValue * (maxBarLength - minBarLength);
            barLengths.push(barLength);
            
            if (barLength > maxScaledLength) maxScaledLength = barLength;
        }
        
        // Нормализуем длины, чтобы максимальная соответствовала maxBarLength
        const scaleFactor = (maxBarLength - minBarLength) / maxScaledLength;
        
        // Рисуем все полосы, кроме текущей
        for (let i = 0; i < this.totalLevels; i++) {
            if (i + 1 === this.currentLevel) continue;
            
            const level = i + 1;
            const prize = this.prizes[i];
            const barLength = minBarLength + (barLengths[i] - minBarLength) * scaleFactor;
            const barX = (width - barLength) / 2;
            const barY = height - (i + 1) * (barHeight + gap);
            
            let color;
            if (safeLevels.includes(level)) {
                color = '#FF9800';
            } else if (level < this.currentLevel) {
                color = '#8BC34A';
            } else {
                color = '#3A3A6A';
            }
            
            this.drawRoundedBar(ctx, barX, barY, barLength, barHeight, 5, color);
            
            if (safeLevels.includes(level)) {
                ctx.fillStyle = '#FFFFFF';
                ctx.font = 'bold 10px Arial';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                const text = prize.toLocaleString('ru-RU') + ' ₽';
                ctx.fillText(text, width / 2, barY + barHeight / 2);
            }
        }
        
        // Рисуем текущий уровень
        if (this.currentLevel >= 1 && this.currentLevel <= 15) {
            const level = this.currentLevel;
            const prize = this.prizes[level - 1];
            const barLength = minBarLength + (barLengths[level - 1] - minBarLength) * scaleFactor;
            const barX = (width - barLength) / 2;
            const barY = height - level * (barHeight + gap);
            
            const highlightHeight = barHeight * 1.3;
            const highlightY = barY - (highlightHeight - barHeight) / 2;
            
            this.drawRoundedBar(ctx, 0, highlightY, width, highlightHeight, 8, '#FFFFFF80');
            this.drawRoundedBar(ctx, barX, barY, barLength, barHeight, 5, '#FFEB3B');
            
            ctx.fillStyle = '#000000';
            ctx.font = 'bold 14px Arial';
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            const text = prize.toLocaleString('ru-RU') + ' ₽';
            ctx.fillText(text, width / 2, highlightY + highlightHeight / 2);
        }
    }

    drawRoundedBar(ctx, x, y, width, height, radius, color) {
        ctx.fillStyle = color;
        ctx.beginPath();
        ctx.moveTo(x + radius, y);
        ctx.lineTo(x + width - radius, y);
        ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
        ctx.lineTo(x + width, y + height - radius);
        ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
        ctx.lineTo(x + radius, y + height);
        ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
        ctx.lineTo(x, y + radius);
        ctx.quadraticCurveTo(x, y, x + radius, y);
        ctx.closePath();
        ctx.fill();
    }

    getCurrentPrize() {
        return this.prizes[this.currentLevel - 2];
    }

    getSafePrize() {
        if (this.currentLevel >= 15) return 1000000;
        if (this.currentLevel >= 10) return 32000;
        if (this.currentLevel >= 5) return 1000;
        return 0;
    }

    initHints() {
        Object.keys(this.hints).forEach(key => {
            this.hints[key].element = document.getElementById(`hint-${key}`);
            this.hints[key].element.addEventListener('click', () => 
                
                this.useHint(key)
            );
        });
    }

    async useHint(type) {
        if (this.hints[type].used) return;

        try {
            const response = await fetch('/api/help', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ type: type })
            });

            if (!response.ok) throw new Error('Network response was not ok');

            const data = await response.json();
            this.hints[type].used = true;
            this.hints[type].element.classList.add('used');

            if (type === '5050') {
                this.apply5050Hint(data.question_enable);
            } else if (type === 'audience') {
                this.applyAudienceHint(data.question_postfix);
            } else if (type === 'life') {
                this.applyLifeHint();
            }
        } catch (error) {
            console.error('Error using hint:', error);
            this.showModal('Не удалось использовать подсказку. Попробуйте снова.');
        }
    }

    apply5050Hint(enableState) {
        const answers = document.querySelectorAll('.answer');
        answers.forEach((btn, index) => {
            const option = ['A', 'B', 'C', 'D'][index];
            if (!enableState[option]) {
                btn.style.display = 'none';
                btn.disabled = true;
            }
        });
    }

    applyAudienceHint(postfixes) {
        const answers = document.querySelectorAll('.answer');
        answers.forEach((btn, index) => {
            const option = ['A', 'B', 'C', 'D'][index];
            if (postfixes[option]) {
                btn.textContent = btn.textContent + ' ' + postfixes[option];
            }
        });
    }

    applyLifeHint() {
        this.hasSecondLife = true;
        this.showModal('Активирована вторая жизнь! При следующей ошибке игра продолжится.');
    }

    initAnswerHandlers() {
        document.querySelectorAll('.answer').forEach(btn => {
            btn.addEventListener('click', async () => {
                if (btn.classList.contains('selected')) return;

                const answerIndex = btn.dataset.index;
                const answerLetter = ['A', 'B', 'C', 'D'][answerIndex];

                document.querySelectorAll('.answer').forEach(a => a.disabled = true);
                btn.classList.add('selected');

                try {
                    const response = await fetch('/api/answer', {
                        method: 'POST',
                        headers: { 
                            'Content-Type': 'application/json',
                            'Accept': 'application/json'
                        },
                        body: JSON.stringify({ answer: answerLetter })
                    });

                    if (!response.ok) throw new Error('Network response was not ok');

                    const data = await response.json();

                    if (data.result === true) {
                        btn.classList.add('correct');
                        setTimeout(() => {
                            if (this.currentLevel === 15) {
                                this.handleGameEnd(true);
                            } else {
                                this.loadQuestion();
                            }
                        }, 1500);
                    } else {
                        btn.classList.add('incorrect');
                        if (data.answer) {
                            const correctIndex = ['A', 'B', 'C', 'D'].indexOf(data.answer);
                            document.querySelectorAll('.answer')[correctIndex].classList.add('correct');
                        }
                        if(!data.life){
                            setTimeout(() => {
                                this.handleGameEnd(false);
                            }, 1500);
                        }else{
                            document.querySelectorAll('.answer').forEach(a => a.disabled = false);
                        }
                    }
                } catch (error) {
                    console.error('Error submitting answer:', error);
                    this.handleGameEnd(false);
                }
            });
        });
    }

    showModal(message) {
        this.modalMessage.textContent = message;
        this.modal.style.display = 'flex';
    }

    closeModal() {
        this.modal.style.display = 'none';
    }

    showTimedModal(message, duration, callback) {
        this.showModal(message);
        setTimeout(() => {
            this.closeModal();
            if (callback) callback();
        }, duration);
    }

  
    handleGameEnd(isWin) {
        let message;
        //if (isWin) {
            message = `Поздравляем! Вы выиграли ${this.getCurrentPrize().toLocaleString('ru-RU')} рублей!`;
        //} else {
        //    const prize = this.getSafePrize();
        //    message = `Игра окончена. Ваш выигрыш: ${prize.toLocaleString('ru-RU')} рублей. Спасибо за участие!`;
        //}

        this.showTimedModal(message, 3000, () => {
            window.location.href = 'index.html';
        });
    }
}

// Запуск игры
new Game();