/*
 * Системные тесты для модернизированного ESP32 Gong/Ring System
 * v2.0.0
 */

#include "lorahandler.h"
#include "config.h"

// Тестовые константы
#define TEST_TIMEOUT 30000  // 30 секунд на тест
#define TEST_RETRY_COUNT 3
#define TEST_MESSAGE_COUNT 10

// Результаты тестов
struct TestResults {
    bool passed;
    uint32_t duration;
    uint32_t errors;
    String details;
};

// Глобальные переменные для тестов
static unsigned long testStartTime;
static uint32_t initialErrorCount;
static uint32_t initialSuccessCount;
static uint32_t initialMemoryErrors;

// Вспомогательные функции для тестов
void resetTestState() {
    testStartTime = millis();
    initialErrorCount = getLoRaErrorCount();
    initialSuccessCount = getLoRaSuccessCount();
    initialMemoryErrors = getLoRaMemoryErrors();
}

bool isTestTimeout() {
    return (millis() - testStartTime) > TEST_TIMEOUT;
}

void logTestResult(const char* testName, bool passed, const char* details = nullptr) {
    Serial.printf("[TEST] %s: %s\n", testName, passed ? "PASSED" : "FAILED");
    if (details) {
        Serial.printf("       Details: %s\n", details);
    }
}

// Тест 1: Инициализация LoRa
TestResults testLoRaInitialization() {
    TestResults results = {false, 0, 0, ""};
    resetTestState();
    
    Serial.println("\n=== Test 1: LoRa Initialization ===");
    
    // Проверка инициализации
    if (!isLoRaInitialized()) {
        results.details = "LoRa not initialized";
        results.duration = millis() - testStartTime;
        logTestResult("LoRa Initialization", false, results.details.c_str());
        return results;
    }
    
    // Проверка состояния
    if (getLoRaState() != LORA_IDLE) {
        results.details = "LoRa not in IDLE state";
        results.duration = millis() - testStartTime;
        logTestResult("LoRa Initialization", false, results.details.c_str());
        return results;
    }
    
    results.passed = true;
    results.duration = millis() - testStartTime;
    logTestResult("LoRa Initialization", true);
    return results;
}

// Тест 2: Отправка сообщений
TestResults testMessageSending() {
    TestResults results = {false, 0, 0, ""};
    resetTestState();
    
    Serial.println("\n=== Test 2: Message Sending ===");
    
    const char* testMessage = "Test message for LoRa";
    uint32_t messagesSent = 0;
    uint32_t messagesFailed = 0;
    
    // Отправка нескольких тестовых сообщений
    for (int i = 0; i < TEST_MESSAGE_COUNT; i++) {
        if (sendLoRaMessageWithRetry(testMessage, MSG_TYPE_STATUS, false)) {
            messagesSent++;
        } else {
            messagesFailed++;
        }
        delay(100); // Небольшая задержка между сообщениями
    }
    
    // Ожидание завершения отправки
    unsigned long waitStart = millis();
    while (getLoRaState() != LORA_IDLE && (millis() - waitStart) < 10000) {
        delay(100);
    }
    
    // Проверка результатов
    uint32_t finalSuccessCount = getLoRaSuccessCount();
    uint32_t finalErrorCount = getLoRaErrorCount();
    
    if (finalSuccessCount > initialSuccessCount && finalErrorCount == initialErrorCount) {
        results.passed = true;
        results.details = String("Sent: ") + messagesSent + ", Failed: " + messagesFailed;
    } else {
        results.details = String("Success count: ") + (finalSuccessCount - initialSuccessCount) + 
                         ", Error count: " + (finalErrorCount - initialErrorCount);
    }
    
    results.duration = millis() - testStartTime;
    logTestResult("Message Sending", results.passed, results.details.c_str());
    return results;
}

// Тест 3: Retry логика
TestResults testRetryLogic() {
    TestResults results = {false, 0, 0, ""};
    resetTestState();
    
    Serial.println("\n=== Test 3: Retry Logic ===");
    
    // Отправка сообщения с подтверждением (ACK)
    const char* testMessage = "Retry test message";
    if (!sendLoRaMessageWithRetry(testMessage, MSG_TYPE_GONG, true)) {
        results.details = "Failed to queue message for retry";
        results.duration = millis() - testStartTime;
        logTestResult("Retry Logic", false, results.details.c_str());
        return results;
    }
    
    // Ожидание завершения retry процесса
    unsigned long waitStart = millis();
    while (getLoRaState() != LORA_IDLE && (millis() - waitStart) < 15000) {
        delay(100);
    }
    
    // Проверка, что система вернулась в IDLE состояние
    if (getLoRaState() == LORA_IDLE) {
        results.passed = true;
        results.details = "Retry process completed successfully";
    } else {
        results.details = String("System stuck in state: ") + getLoRaState();
    }
    
    results.duration = millis() - testStartTime;
    logTestResult("Retry Logic", results.passed, results.details.c_str());
    return results;
}

// Тест 4: Управление памятью
TestResults testMemoryManagement() {
    TestResults results = {false, 0, 0, ""};
    resetTestState();
    
    Serial.println("\n=== Test 4: Memory Management ===");
    
    uint32_t initialHeap = ESP.getFreeHeap();
    
    // Проверка функции мониторинга памяти
    bool memoryCheck = checkLoRaMemory();
    if (!memoryCheck) {
        results.details = "Memory check failed";
        results.duration = millis() - testStartTime;
        logTestResult("Memory Management", false, results.details.c_str());
        return results;
    }
    
    // Проверка, что память не уменьшилась критически
    uint32_t currentHeap = ESP.getFreeHeap();
    if (currentHeap < initialHeap * 0.8) { // Не менее 80% от начального
        results.details = String("Memory usage increased significantly: ") + 
                         initialHeap + " -> " + currentHeap;
        results.duration = millis() - testStartTime;
        logTestResult("Memory Management", false, results.details.c_str());
        return results;
    }
    
    results.passed = true;
    results.details = String("Memory stable: ") + initialHeap + " -> " + currentHeap;
    results.duration = millis() - testStartTime;
    logTestResult("Memory Management", true, results.details.c_str());
    return results;
}

// Тест 5: Логирование
TestResults testLogging() {
    TestResults results = {false, 0, 0, ""};
    resetTestState();
    
    Serial.println("\n=== Test 5: Logging System ===");
    
    uint32_t initialLogCount = getLoRaLogCount();
    
    // Тестирование разных уровней логирования
    setLoRaLogLevel(LORA_LOG_DEBUG);
    logLoRa(LORA_LOG_DEBUG, "Debug test message");
    logLoRa(LORA_LOG_INFO, "Info test message");
    logLoRa(LORA_LOG_WARN, "Warning test message");
    logLoRa(LORA_LOG_ERROR, "Error test message");
    
    // Проверка, что логи записались
    uint32_t finalLogCount = getLoRaLogCount();
    if (finalLogCount > initialLogCount) {
        results.passed = true;
        results.details = String("Logs recorded: ") + (finalLogCount - initialLogCount);
    } else {
        results.details = "No logs recorded";
    }
    
    // Возврат к нормальному уровню логирования
    setLoRaLogLevel(LORA_LOG_INFO);
    
    results.duration = millis() - testStartTime;
    logTestResult("Logging System", results.passed, results.details.c_str());
    return results;
}

// Тест 6: Обработка ошибок
TestResults testErrorHandling() {
    TestResults results = {false, 0, 0, ""};
    resetTestState();
    
    Serial.println("\n=== Test 6: Error Handling ===");
    
    // Проверка текущего состояния ошибок
    uint32_t currentErrors = getLoRaErrorCount();
    uint32_t currentMemoryErrors = getLoRaMemoryErrors();
    
    // Сброс счетчиков ошибок
    resetLoRaErrors();
    
    // Проверка, что счетчики сбросились
    if (getLoRaErrorCount() == 0 && getLoRaMemoryErrors() == 0) {
        results.passed = true;
        results.details = "Error counters reset successfully";
    } else {
        results.details = "Failed to reset error counters";
    }
    
    results.duration = millis() - testStartTime;
    logTestResult("Error Handling", results.passed, results.details.c_str());
    return results;
}

// Тест 7: Производительность
TestResults testPerformance() {
    TestResults results = {false, 0, 0, ""};
    resetTestState();
    
    Serial.println("\n=== Test 7: Performance Test ===");
    
    const int PERFORMANCE_ITERATIONS = 50;
    unsigned long startTime = micros();
    
    // Выполнение множественных операций
    for (int i = 0; i < PERFORMANCE_ITERATIONS; i++) {
        // Проверка состояния
        isLoRaInitialized();
        getLoRaState();
        getLoRaErrorCount();
        getLoRaSuccessCount();
        
        // Логирование
        logLoRa(LORA_LOG_DEBUG, "Performance test iteration");
    }
    
    unsigned long endTime = micros();
    unsigned long totalTime = endTime - startTime;
    float avgTime = (float)totalTime / PERFORMANCE_ITERATIONS;
    
    // Проверка производительности (операции должны выполняться быстро)
    if (avgTime < 1000) { // Менее 1ms на операцию
        results.passed = true;
        results.details = String("Avg time per operation: ") + avgTime + " microseconds";
    } else {
        results.details = String("Performance degraded: ") + avgTime + " microseconds per operation";
    }
    
    results.duration = millis() - testStartTime;
    logTestResult("Performance Test", results.passed, results.details.c_str());
    return results;
}

// Тест 8: Стабильность
TestResults testStability() {
    TestResults results = {false, 0, 0, ""};
    resetTestState();
    
    Serial.println("\n=== Test 8: Stability Test ===");
    
    const int STABILITY_ITERATIONS = 100;
    uint32_t stateChanges = 0;
    LoRaState lastState = getLoRaState();
    
    // Выполнение множественных операций для проверки стабильности
    for (int i = 0; i < STABILITY_ITERATIONS; i++) {
        // Отправка сообщения
        const char* message = "Stability test message";
        sendLoRaMessage(message, MSG_TYPE_STATUS, false);
        
        // Проверка изменения состояния
        LoRaState currentState = getLoRaState();
        if (currentState != lastState) {
            stateChanges++;
            lastState = currentState;
        }
        
        // Небольшая задержка
        delay(10);
    }
    
    // Ожидание завершения всех операций
    unsigned long waitStart = millis();
    while (getLoRaState() != LORA_IDLE && (millis() - waitStart) < 10000) {
        delay(100);
    }
    
    // Проверка стабильности
    if (getLoRaState() == LORA_IDLE && stateChanges > 0) {
        results.passed = true;
        results.details = String("Stable operation with ") + stateChanges + " state changes";
    } else {
        results.details = "System not stable or stuck";
    }
    
    results.duration = millis() - testStartTime;
    logTestResult("Stability Test", results.passed, results.details.c_str());
    return results;
}

// Главная функция запуска всех тестов
void runAllSystemTests() {
    Serial.println("\n==========================================");
    Serial.println("ESP32 Gong/Ring System - System Tests v2.0.0");
    Serial.println("==========================================\n");
    
    // Массив тестов
    TestResults (*testFunctions[])() = {
        testLoRaInitialization,
        testMessageSending,
        testRetryLogic,
        testMemoryManagement,
        testLogging,
        testErrorHandling,
        testPerformance,
        testStability
    };
    
    const char* testNames[] = {
        "LoRa Initialization",
        "Message Sending",
        "Retry Logic",
        "Memory Management",
        "Logging System",
        "Error Handling",
        "Performance Test",
        "Stability Test"
    };
    
    int totalTests = sizeof(testFunctions) / sizeof(testFunctions[0]);
    int passedTests = 0;
    int failedTests = 0;
    unsigned long totalTestTime = 0;
    
    // Запуск всех тестов
    for (int i = 0; i < totalTests; i++) {
        Serial.printf("Running test %d/%d: %s\n", i + 1, totalTests, testNames[i]);
        
        TestResults result = testFunctions[i]();
        totalTestTime += result.duration;
        
        if (result.passed) {
            passedTests++;
        } else {
            failedTests++;
        }
        
        delay(1000); // Пауза между тестами
    }
    
    // Итоговый отчет
    Serial.println("\n==========================================");
    Serial.println("TEST RESULTS SUMMARY");
    Serial.println("==========================================");
    Serial.printf("Total Tests: %d\n", totalTests);
    Serial.printf("Passed: %d\n", passedTests);
    Serial.printf("Failed: %d\n", failedTests);
    Serial.printf("Success Rate: %.1f%%\n", (float)passedTests / totalTests * 100.0);
    Serial.printf("Total Test Time: %lu ms\n", totalTestTime);
    Serial.printf("Average Test Time: %.1f ms\n", (float)totalTestTime / totalTests);
    
    if (failedTests == 0) {
        Serial.println("\n🎉 ALL TESTS PASSED! System is working correctly.");
    } else {
        Serial.printf("\n⚠️  %d tests failed. Please review the results above.\n", failedTests);
    }
    
    Serial.println("==========================================\n");
}
