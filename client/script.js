document.addEventListener('DOMContentLoaded', async () => {
    if (document.querySelector('.welcome-container')) {
        // Это приветственная страница
        try {
            const response = await fetch('/api/info');
            const data = await response.json();
            renderCategories(data.category);
            
            document.getElementById('start-game').addEventListener('click', startGame);
        } catch (error) {
            console.error('Error loading categories:', error);
        }
    }
});

function renderCategories(categories) {
    const container = document.getElementById('categories');
    container.innerHTML = '';
    
    categories.forEach(category => {
        const categoryItem = document.createElement('label');
        categoryItem.className = 'category-item';
        categoryItem.innerHTML = `
            <input type="checkbox" name="category" value="${category.name}">
            ${category.name} (${category.count})
        `;
        container.appendChild(categoryItem);
    });
}

async function startGame() { //Нажатие кнопки Старт игры
    const selectedCategories = Array.from(document.querySelectorAll('input[name="category"]:checked'))
        .map(checkbox => checkbox.value);
    
    if (selectedCategories.length === 0) {
        alert('Пожалуйста, выберите хотя бы одну категорию');
        return;
    }
    
    try {
        await fetch('/api/start', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ category: selectedCategories })
        });
        
        // Переходим на страницу игры
        window.location.href = 'game.html';
    } catch (error) {
        console.error('Error starting game:', error);
    }
}