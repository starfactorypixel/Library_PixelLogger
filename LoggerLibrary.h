#pragma once

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#include <iostream>
#include <cstring>
#endif

#ifdef HAL_UART_MODULE_ENABLED
#warning Check if the hDebugUart is defined?
extern UART_HandleTypeDef hDebugUart;
#endif // HAL_UART_MODULE_ENABLED

template <uint16_t buffer_length = 256>
class DebugSerial
{
public:
    const char *DebugTopic = "DEBUG";

    DebugSerial()
    {
#ifdef HAL_UART_MODULE_ENABLED
        _huart = &hDebugUart;
#endif // HAL_UART_MODULE_ENABLED
    }

    DebugSerial &Print(const char *str, const char *topic = nullptr)
    {
        PrintTopic(topic);
        _HW_Print((uint8_t *)str, strlen(str));
        
        return *this;
    }

    DebugSerial &Print(const uint8_t *str, uint16_t length, const char *topic = nullptr)
    {
        PrintTopic(topic);
        _HW_Print((uint8_t *)str, length);

        return *this;
    }

    DebugSerial &Printf(const char *str, ...)
    {
        if (str == nullptr)
            return *this;

        va_list argptr;
        va_start(argptr, str);
        vsnprintf(_buffer, buffer_length, str, argptr);
        va_end(argptr);

        _HW_Print((uint8_t *)_buffer, strlen(_buffer));

        return *this;
    }

    DebugSerial &PrintNewLine()
    {
        return Print("\n");
    }

    DebugSerial &PrintTopic(const char *topic)
    {
        if (topic == nullptr)
            return *this;

        return Printf("+%s\t", topic);
    }

private:
    char _buffer[buffer_length];

#ifdef HAL_UART_MODULE_ENABLED
    UART_HandleTypeDef *_huart = nullptr;

    bool _HW_Print(uint8_t *pData, uint16_t Size)
    {
        HAL_StatusTypeDef result = HAL_UART_Transmit(_huart, pData, Size, 64);
        if (result != HAL_OK)
        {
            HAL_UART_AbortTransmit(_huart);
        }

        return (result == HAL_OK);
    }

#elif defined(_WIN32) || defined(_WIN64) || defined(__linux__) // HAL_UART_MODULE_ENABLED
    bool _HW_Print(uint8_t *pData, uint16_t Size)
    {
        std::cout << _buffer << std::flush;

        return true;
    }

#else // HAL_UART_MODULE_ENABLED
#warning All logging will be ignored
    bool _HW_Print(uint8_t *pData, uint16_t Size)
    {
        return false;
    }
#endif // HAL_UART_MODULE_ENABLED
};

DebugSerial<256> Logger;

#if defined(DEBUG) || defined(DETAILED_DEBUG)
    #define DEBUG_LOG_SIMPLE(fmt, ...) do { Logger.Printf(fmt, ##__VA_ARGS__); } while (0)
    #define DEBUG_LOG_TOPIC(topic, fmt, ...) do { Logger.PrintTopic(topic).Printf(fmt, ##__VA_ARGS__); } while (0)

    #ifdef DETAILED_DEBUG
        #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
        #define DEBUG_LOG(fmt, ...) DEBUG_LOG_TOPIC(Logger.DebugTopic, "[%s:%d] " fmt "\n", __FILENAME__, __LINE__, ##__VA_ARGS__)
    #else // DETAILED_DEBUG
        #define DEBUG_LOG(fmt, ...) DEBUG_LOG_TOPIC(Logger.DebugTopic, fmt "\n", ##__VA_ARGS__)
    #endif // DETAILED_DEBUG
#else  // DEBUG
    #define DEBUG_LOG_SIMPLE(fmt, ...)
    #define DEBUG_LOG_TOPIC(topic, fmt, ...)
    #define DEBUG_LOG(fmt, ...)
#endif // DEBUG
