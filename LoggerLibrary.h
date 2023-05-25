#pragma once

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__GNUC__)
#include <iostream>
#include <cstring>
#endif

#ifdef HAL_UART_MODULE_ENABLED
#warning Check if the hDebugUart is defined?
extern UART_HandleTypeDef hDebugUart;
#endif // HAL_UART_MODULE_ENABLED

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

    void Print(const char *str, const char *topic = nullptr)
    {
        PrintTopic(topic);
        _HW_Print((uint8_t *)str, strlen(str));
    }

    void Print(const uint8_t *str, uint16_t length, const char *topic = nullptr)
    {
        PrintTopic(topic);
        _HW_Print((uint8_t *)str, length);
    }

    template <uint16_t length = 255>
    void Printf(const char *str, ...)
    {
        if (str == nullptr)
            return;

        char buffer[length];

        va_list argptr;
        va_start(argptr, str);
        vsnprintf(buffer, length, str, argptr);
        va_end(argptr);

        _HW_Print((uint8_t *)buffer, strlen(buffer));
    }

    void PrintNewLine()
    {
        Print("\n");
    }

    void PrintTopic(const char *topic)
    {
        Printf("+%s=", topic);
        //_HW_Print((uint8_t *)topic, strlen(topic));
    }

private:
#ifdef HAL_UART_MODULE_ENABLED
    UART_HandleTypeDef *_huart = nullptr;

    // HAL_StatusTypeDef _HW_Print(uint8_t *pData, uint16_t Size)
    bool _HW_Print(uint8_t *pData, uint16_t Size)
    {
        if (pData == nullptr)
            return false;

        HAL_StatusTypeDef result = HAL_UART_Transmit(_huart, pData, Size, 64);
        if (result != HAL_OK)
        {
            HAL_UART_AbortTransmit(_huart);
        }

        // return result;
        return (result == HAL_OK);
    }

#elif defined(_WIN32) || defined(_WIN64) || defined(__GNUC__) // HAL_UART_MODULE_ENABLED
    bool _HW_Print(uint8_t *pData, uint16_t Size)
    {
        if (pData == nullptr)
            return false;

        std::cout.write((char *)pData, (int)Size);
        std::cout << std::endl;

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

DebugSerial logger;

#if defined(DEBUG) || defined(DETAILED_DEBUG)
    #define DEBUG_LOG_SIMPLE(fmt, ...) do { logger.Printf(fmt, ##__VA_ARGS__); } while (0)

    #ifdef DETAILED_DEBUG
        #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
        #define DEBUG_LOG(fmt, ...) DEBUG_LOG_SIMPLE("+%s=[%s:%d] " fmt, logger.DebugTopic, __FILENAME__, __LINE__, ##__VA_ARGS__)
    #else // DETAILED_DEBUG
        #define DEBUG_LOG(fmt, ...) DEBUG_LOG_SIMPLE("+%s=" fmt, logger.DebugTopic, ##__VA_ARGS__)
    #endif // DETAILED_DEBUG
#else  // DEBUG
    #define DEBUG_LOG_SIMPLE(fmt, ...)
    #define DEBUG_LOG(fmt, ...)
#endif // DEBUG
