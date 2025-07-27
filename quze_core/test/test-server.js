// Проверка наличия модуля
try {
    const native = require('../build/bin/quze_core.node');
    console.log("Native module loaded successfully");
    console.log("Native exports:", Object.keys(native));
  } catch (e) {
    console.error("Error loading native module:", e);
  }
  
  // Основной тест
  const { MyClass } = require('../lib');
  
  console.log("MyClass type:", typeof MyClass);
  if (typeof MyClass !== 'function') {
    console.error("ERROR: MyClass is not a constructor function");
    process.exit(1);
  }
  
  try {
    const instance = new MyClass();
    console.log("Instance created successfully");
    
    const result = instance.getObject();
    console.log("Result:", result);
    
    // Валидация
    const assert = require('assert');
    assert.strictEqual(typeof result, 'object');
    assert.strictEqual(result.A, 42);
    assert.strictEqual(result.B, "Hello from C++");
    assert.strictEqual(result.C, true);
    
    console.log("✅ All tests passed!");
  } catch (e) {
    console.error("Test failed:", e);
  }