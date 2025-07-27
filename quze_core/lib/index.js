const native = require('../build/bin/quze_core.node');

// Экспортируем нативный класс напрямую
module.exports = {
  QuizClass: native.QuizClass
};