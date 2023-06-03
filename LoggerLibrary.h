#pragma once

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#include <iostream>
#include <cstring>
#endif

#ifdef HAL_UART_MODULE_ENABLED // STM32
    #warning Check if the hDebugUart is defined?
    extern UART_HandleTypeDef hDebugUart;
#elif defined(ESP32) // ESP32
    #warning 'Serial' is used for debugging!
#endif // HAL_UART_MODULE_ENABLED

enum logger_output_type_t
{
    LOG_OUT_TYPE_NONE = 0x00,
    LOG_OUT_TYPE_HEX = 0x01,
    LOG_OUT_TYPE_BYTES = 0x02,
};

template <uint16_t buffer_length = 256>
class DebugSerial
{
public:
    const char *DebugTopic = "DEBUG";

    DebugSerial()
    {
        _ResetBuffer();

        PrintTopic(DebugTopic).Printf("Logger config: %d bytes buffer size\n\n", buffer_length);
    }

    DebugSerial &PrintTopic(const char *topic)
    {
        if (topic == nullptr)
            return *this;

        _FillTopic(topic);
        _Print(_buffer, strlen(_buffer));

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

        _Print(_buffer, strlen(_buffer));

        return *this;
    }

    DebugSerial &Print(const void *data, uint16_t data_length, const char *topic, logger_output_type_t data_type)
    {
        if (data == nullptr || data_length == 0 || data_length > _buffer_size_left)
            return *this;

        if (topic != nullptr)
        {
            _FillTopic(topic);
        }

        switch (data_type)
        {
        case LOG_OUT_TYPE_HEX:
        {
            int result = 0;
            for (uint8_t i = 0; i < data_length; i++)
            {
                result = snprintf(_buffer_ptr, _buffer_size_left, "0x%02X%s", ((uint8_t *)data)[i], (i == data_length - 1) ? "" : ", ");
                if (result < 0)
                    break;
                
                _buffer_ptr += result;
                _buffer_size_left -= result;
            }
            break;
        }
        
        case LOG_OUT_TYPE_BYTES:
        {
            memcpy(_buffer_ptr, data, data_length);
            _buffer_size_left -= data_length;
            break;
        }

        default:
            return *this;
        }

        _Print(_buffer, buffer_length - _buffer_size_left + 1);

        return *this;
    }

    DebugSerial &Print(const char *str)
    {
        if (str == nullptr)
            return *this;
        
        return Print(str, strlen(str), nullptr, LOG_OUT_TYPE_BYTES);
    }

    DebugSerial &Print(const char *str, const char *topic)
    {
        if (str == nullptr)
            return *this;
        
        return Print(str, strlen(str), topic, LOG_OUT_TYPE_BYTES);
    }

    DebugSerial &Print(const void *data, uint16_t data_length, logger_output_type_t data_type = LOG_OUT_TYPE_BYTES)
    {
        return Print(data, data_length, nullptr, data_type);
    }

    DebugSerial &Print(const void *data, uint16_t data_length, const char *topic)
    {
        return Print(data, data_length, topic, LOG_OUT_TYPE_BYTES);
    }

    DebugSerial &PrintNewLine()
    {
        return Print("\n");
    }

private:
    char _buffer[buffer_length];
    char *_buffer_ptr = _buffer;
    uint16_t _buffer_size_left = buffer_length;

    void _ResetBuffer()
    {
        memset(_buffer, 0, buffer_length);
        _buffer_ptr = _buffer;
        _buffer_size_left = buffer_length;
    }

    int _FillTopic(const char *topic)
    {
        int result = snprintf(_buffer_ptr, _buffer_size_left, "+%s\t", topic);

        if (result > 0)
        {
            _buffer_ptr += result;
            _buffer_size_left -= result;
        }

        return result;
    }

    void _Print(const void *pData, uint16_t Size)
    {
        _HW_Print(pData, Size);
        _ResetBuffer();
    }

#ifdef HAL_UART_MODULE_ENABLED
    bool _HW_Print(const void *pData, uint16_t Size)
    {
        HAL_StatusTypeDef result = HAL_UART_Transmit(&hDebugUart, (uint8_t *)pData, Size, 64);
        if (result != HAL_OK)
        {
            HAL_UART_AbortTransmit(&hDebugUart);
        }

        return (result == HAL_OK);
    }

#elif defined(ESP32) // HAL_UART_MODULE_ENABLED
    bool _HW_Print(const void *pData, uint16_t Size)
    {
        Serial.write((uint8_t *)pData, Size);

        return true;
    }

#elif defined(_WIN32) || defined(_WIN64) || defined(__linux__) // ESP32
    bool _HW_Print(const void *pData, uint16_t Size)
    {
        std::cout << std::string((char *)pData, Size) << std::flush;

        return true;
    }

#else // HAL_UART_MODULE_ENABLED
#warning All logging will be ignored
    bool _HW_Print(const void *pData, uint16_t Size)
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
