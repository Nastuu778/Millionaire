import http from 'http';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';
import { QuizClass } from './quze_core/lib/index.js';

// Получаем __dirname в ES modules
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);


// Создаем экземпляр класса QuizClass
const quiz = new QuizClass('questions.db');

// Функция для отправки JSON ответа
function sendResponse(res, statusCode, data) {
    res.writeHead(statusCode, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify(data));
}

const apiCall= function(req, res, methodName, queryParams){
    console.log('==> /api/' + methodName + '  metod:' + req.method + ', param:' + JSON.stringify(queryParams));
    if (methodName && typeof quiz[methodName] === 'function') {
        const result = quiz[methodName](queryParams);
        console.log('==> quiz.' + methodName + '(param)=' + JSON.stringify(result));
        sendResponse(res, 200, result);
    } else {
        sendResponse(res, 404, { success: false, error: 'Method not found' });
        console.warn('==> quiz.' + methodName + '"Предупреждение!!! не найден метод');
    }
}
const reqClient = function (req, res) { //Обработка запросов клиентов
    console.log('req.url= ' + req.url);
    // Обработка маршрута /api/info
    if (req.url.startsWith('/api')) {
        try {
            // Удаляем query параметры и получаем путь
            const path = req.url.split('?')[0];
            // Извлекаем имя метода (часть между /api/ и следующим / или концом строки)
            const methodMatch = path.match(/^\/api\/([^\/?]+)/);
            const methodName = methodMatch[1];
            const queryParams = {};

            const queryString = req.url.split('?')[1];
            let body=''
            if(req.method === 'POST'){ //Передача параметров методом POST
                // Собираем данные из потока
                req.on('data', (chunk) => {
                    body += chunk.toString();
                });

                      // Когда все данные получены
                req.on('end', () => {
                    try {
                        let params = {};
                        const contentType = req.headers['content-type'];
                        
                        // Парсим в зависимости от Content-Type
                        if (contentType === 'application/x-www-form-urlencoded') {
                            params = querystring.parse(body);
                        } else if (contentType === 'application/json') {
                            params = JSON.parse(body);
                        } else {
                            // Пробуем определить автоматически
                            try {
                                params = JSON.parse(body);
                            } catch (e) {
                                params = querystring.parse(body);
                            }
                        }
                        
                        apiCall(req, res, methodName, params);
                    } catch (error) {
                        res.writeHead(400, { 'Content-Type': 'application/json' });
                        res.end(JSON.stringify({ 
                            status: 'error', 
                            message: 'Invalid request data',
                            error: error.message 
                        }));
                    }
                });
            }
            if(req.method === 'GET'){ //Передача параметров методом GET
                if (queryString) {
                    queryString.split('&').forEach(pair => {
                        const [key, value] = pair.split('=');
                        if (key) {
                            queryParams[decodeURIComponent(key)] = value ? decodeURIComponent(value) : null;
                        }
                    });
                }
                apiCall(req, res, methodName, queryParams);
            }

        } catch (error) {
            sendResponse(res, 500, { success: false, error: error.message });
            console.warn('Предупреждение!!! Вызвано исключение error= "' + error + '"');
        }
        return;
    }

    // Обслуживание статических файлов из папки client
    if (req.method === 'GET') {
        if (req.url === '/')
            console.log('Подключение клиента');
        let filePath = path.join(__dirname, 'client', req.url === '/' ? 'index.html' : req.url);
        const ext = path.extname(filePath);

        // Проверяем существование файла
        fs.access(filePath, fs.constants.F_OK, (err) => {
            if (err) {
                // Если файл не найден, возвращаем index.html для SPA
                if (req.url.startsWith('/api')) {
                    sendResponse(res, 404, { success: false, error: 'Not Found' });
                } else {
                    filePath = path.join(__dirname, 'client', 'index.html');
                    serveStaticFile(res, filePath, getContentType(ext));
                }
            } else {
                serveStaticFile(res, filePath, getContentType(ext));
            }
        });
        return;
    }

    // Все остальные маршруты
    sendResponse(res, 404, { success: false, error: 'Not Found' });
}

// Создаем HTTP сервер
const server = http.createServer(reqClient);


// Функция для обслуживания статических файлов
function serveStaticFile(res, filePath, contentType) {
    fs.readFile(filePath, (err, content) => {
        if (err) {
            if (err.code === 'ENOENT') {
                sendResponse(res, 404, { success: false, error: 'File Not Found' });
            } else {
                sendResponse(res, 500, { success: false, error: 'Server Error' });
            }
        } else {
            res.writeHead(200, { 'Content-Type': contentType });
            res.end(content, 'utf-8');
        }
    });
}

// Функция для определения Content-Type
function getContentType(ext) {
    const map = {
        '.html': 'text/html',
        '.js': 'text/javascript',
        '.css': 'text/css',
        '.json': 'application/json',
        '.png': 'image/png',
        '.jpg': 'image/jpg',
        '.ico': 'image/x-icon'
    };
    return map[ext] || 'application/octet-stream';
}

// Запуск сервера
const PORT = 3000;
const listen = function () {
    console.log(`http:\\\\localhost:${PORT}`);
}

server.listen(PORT, listen);