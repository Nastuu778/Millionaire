document.addEventListener('DOMContentLoaded', async () => {
    try {
        // Загружаем категории с сервера
        const response = await fetch('/api/info');
        const data = await response.json();
        
        const categoriesContainer = document.getElementById('categories');
        data.category.forEach(category => {
            const categoryItem = document.createElement('label');
            categoryItem.className = 'category-item';
            categoryItem.innerHTML = `
                <input type="checkbox" name="category" value="${category.name}">
                ${category.name} (${category.count} вопросов)
            `;
            categoriesContainer.appendChild(categoryItem);
        });

        // Обработчик начала игры
        document.getElementById('startGame').addEventListener('click', () => {
            const selectedCategories = Array.from(document.querySelectorAll('input[name="category"]:checked'))
                .map(checkbox => checkbox.value);
            
            if (selectedCategories.length === 0) {
                alert('Пожалуйста, выберите хотя бы одну категорию');
                return;
            }

            // Сохраняем выбранные категории в sessionStorage и переходим на страницу игры
            sessionStorage.setItem('selectedCategories', JSON.stringify(selectedCategories));
            window.location.href = 'game.html';
        });
    } catch (error) {
        console.error('Ошибка при загрузке категорий:', error);
        alert('Не удалось загрузить категории. Пожалуйста, попробуйте позже.');
    }
});