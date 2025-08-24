/*
 * Примеры использования модернизированного LoRa Handler
 * ESP32 Gong/Ring System v2.0.0
 */

#include "lorahandler.h"

// Пример 1: Базовая отправка сообщения
void exampleBasicSend() {
    // Отправка простого сообщения без подтверждения
    const char* message = "Hello LoRa World!";
    sendLoRaMessage(message, MSG_TYPE_STATUS, false);
}

// Пример 2: Отправка сообщения с подтверждением и retry
void exampleReliableSend() {
    // Отправка важного сообщения с подтверждением доставки
    const char* message = "Critical system update";
    if (sendLoRaMessageWithRetry(message, MSG_TYPE_SCHEDULE, true)) {
        Serial.println("Message queued for reliable delivery");
    } else {
        Serial.println("Failed to queue message - LoRa busy");
    }
}

// Пример 3: Мониторинг состояния LoRa
void exampleStatusMonitoring() {
    if (isLoRaInitialized()) {
        Serial.printf("LoRa Status:\n");
        Serial.printf("  State: %d\n", getLoRaState());
        Serial.printf("  Errors: %d\n", getLoRaErrorCount());
        Serial.printf("  Success: %d\n", getLoRaSuccessCount());
        Serial.printf("  Memory Errors: %d\n", getLoRaMemoryErrors());
        Serial.printf("  Log Count: %d\n", getLoRaLogCount());
    } else {
        Serial.println("LoRa not initialized");
    }
}

// Пример 4: Настройка уровня логирования
void exampleLogLevelControl() {
    // Установка детального логирования для отладки
    setLoRaLogLevel(LORA_LOG_DEBUG);
    
    // Отправка тестового сообщения
    const char* message = "Debug test message";
    sendLoRaMessage(message, MSG_TYPE_STATUS, false);
    
    // Возврат к нормальному уровню логирования
    setLoRaLogLevel(LORA_LOG_INFO);
}

// Пример 5: Обработка ошибок и восстановление
void exampleErrorHandling() {
    // Проверка состояния LoRa
    if (getLoRaState() == LORA_ERROR) {
        Serial.println("LoRa in error state, attempting recovery...");
        
        // Сброс счетчиков ошибок
        resetLoRaErrors();
        
        // Проверка доступной памяти
        if (ESP.getFreeHeap() < 10240) {
            Serial.println("Low memory detected, cleaning up...");
            cleanupLoRaMemory();
        }
    }
}

// Пример 6: Отправка JSON сообщения
void exampleJsonMessage() {
    // Создание JSON сообщения
    DynamicJsonDocument doc(512);
    doc["type"] = "status";
    doc["timestamp"] = millis();
    doc["device"] = "ESP32_Gong";
    doc["status"] = "operational";
    doc["freeHeap"] = ESP.getFreeHeap();
    
    // Сериализация в буфер
    char messageBuffer[256];
    serializeJson(doc, messageBuffer, sizeof(messageBuffer));
    
    // Отправка с подтверждением
    sendLoRaMessageWithRetry(messageBuffer, MSG_TYPE_STATUS, true);
}

// Пример 7: Мониторинг памяти в реальном времени
void exampleMemoryMonitoring() {
    static unsigned long lastCheck = 0;
    const unsigned long CHECK_INTERVAL = 10000; // 10 секунд
    
    if (millis() - lastCheck >= CHECK_INTERVAL) {
        uint32_t freeHeap = ESP.getFreeHeap();
        
        if (freeHeap < 5120) {
            Serial.printf("Memory warning: %d bytes free\n", freeHeap);
            
            // Принудительная очистка при низкой памяти
            if (freeHeap < 2048) {
                Serial.println("Critical memory level, forcing cleanup");
                cleanupLoRaMemory();
            }
        }
        
        lastCheck = millis();
    }
}

// Пример 8: Обработка различных типов сообщений
void exampleMessageHandling() {
    // Отправка разных типов сообщений
    const char* gongMessage = "{\"type\":\"gong\",\"volume\":20}";
    const char* scheduleMessage = "{\"type\":\"schedule\",\"time\":\"12:00\"}";
    const char* statusMessage = "{\"type\":\"status\",\"uptime\":3600}";
    
    // Отправка с разными приоритетами
    sendLoRaMessageWithRetry(gongMessage, MSG_TYPE_GONG, true);      // Высокий приоритет
    sendLoRaMessageWithRetry(scheduleMessage, MSG_TYPE_SCHEDULE, false); // Средний приоритет
    sendLoRaMessage(statusMessage, MSG_TYPE_STATUS, false);           // Низкий приоритет
}

// Пример 9: Система мониторинга производительности
void examplePerformanceMonitoring() {
    static unsigned long lastReport = 0;
    static uint32_t lastErrorCount = 0;
    static uint32_t lastSuccessCount = 0;
    const unsigned long REPORT_INTERVAL = 60000; // 1 минута
    
    if (millis() - lastReport >= REPORT_INTERVAL) {
        uint32_t currentErrors = getLoRaErrorCount();
        uint32_t currentSuccess = getLoRaSuccessCount();
        
        // Расчет статистики
        uint32_t errorDelta = currentErrors - lastErrorCount;
        uint32_t successDelta = currentSuccess - lastSuccessCount;
        uint32_t totalDelta = errorDelta + successDelta;
        
        if (totalDelta > 0) {
            float successRate = (float)successDelta / totalDelta * 100.0;
            Serial.printf("LoRa Performance Report:\n");
            Serial.printf("  Success Rate: %.1f%%\n", successRate);
            Serial.printf("  Messages/min: %d\n", totalDelta);
            Serial.printf("  Errors/min: %d\n", errorDelta);
        }
        
        lastErrorCount = currentErrors;
        lastSuccessCount = currentSuccess;
        lastReport = millis();
    }
}

// Пример 10: Интеграция с основной системой
void exampleSystemIntegration() {
    // Проверка готовности LoRa перед отправкой
    if (!isLoRaInitialized()) {
        Serial.println("LoRa not ready, skipping message");
        return;
    }
    
    // Проверка состояния системы
    if (getLoRaState() != LORA_IDLE) {
        Serial.printf("LoRa busy in state %d, queuing message\n", getLoRaState());
        // Здесь можно добавить очередь сообщений
        return;
    }
    
    // Отправка системного сообщения
    const char* systemMessage = "System heartbeat";
    sendLoRaMessageWithRetry(systemMessage, MSG_TYPE_STATUS, false);
}

// Функция для демонстрации всех примеров
void runAllExamples() {
    Serial.println("=== LoRa Handler Examples ===\n");
    
    // Базовые примеры
    exampleBasicSend();
    delay(1000);
    
    exampleReliableSend();
    delay(1000);
    
    // Мониторинг
    exampleStatusMonitoring();
    delay(1000);
    
    // Логирование
    exampleLogLevelControl();
    delay(1000);
    
    // Обработка ошибок
    exampleErrorHandling();
    delay(1000);
    
    // JSON сообщения
    exampleJsonMessage();
    delay(1000);
    
    // Мониторинг памяти
    exampleMemoryMonitoring();
    delay(1000);
    
    // Обработка сообщений
    exampleMessageHandling();
    delay(1000);
    
    // Мониторинг производительности
    examplePerformanceMonitoring();
    delay(1000);
    
    // Интеграция с системой
    exampleSystemIntegration();
    
    Serial.println("\n=== Examples completed ===");
}
