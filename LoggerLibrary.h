#pragma once

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef DEBUG_LOGGER_SIZE
    #define DEBUG_LOGGER_SIZE (256)
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__)
#include <iostream>
#include <cstring>
#endif

#ifdef HAL_UART_MODULE_ENABLED // STM32
    #warning Check if the hDebugUart is defined?
    extern UART_HandleTypeDef hDebugUart;
#elif defined(ESP32) // ESP32
    #warning 'Serial' is used for debugging, initialize by yourself!
#endif // HAL_UART_MODULE_ENABLED

enum logger_output_type_t
{
    LOG_OUT_TYPE_NONE = 0x00,
    LOG_OUT_TYPE_HEX = 0x01,
    LOG_OUT_TYPE_BYTES = 0x02,
};

template <uint16_t buffer_length>
class DebugLogger
{
public:
    const char *DebugTopic = "DEBUG";

    DebugLogger()
    {
        _ResetBuffer();
        // don't use any output here because UART isn't initialized yet for STM32
    }

    DebugLogger &PrintTopic(const char *topic)
    {
        if (topic == nullptr)
            return *this;

        _FillTopic(topic);
        _Print(_buffer, strlen(_buffer));

        return *this;
    }

    DebugLogger &Printf(const char *str, ...)
    {
        if (str == nullptr)
            return *this;

        va_list argptr;
        va_start(argptr, str);
        vsnprintf(_buffer_ptr, _buffer_size_left, str, argptr);
        va_end(argptr);

        _Print(_buffer, strlen(_buffer));

        return *this;
    }

    DebugLogger &Print(const void *data, uint16_t data_length, const char *topic, logger_output_type_t data_type)
    {
        if (data == nullptr || data_length == 0)
            return *this;

        if (topic != nullptr)
        {
            _FillTopic(topic);
        }

        switch (data_type)
        {
        case LOG_OUT_TYPE_HEX:
            _PrintHEX(data, data_length);
            break;
        
        case LOG_OUT_TYPE_BYTES:
            _PrintBytes(data, data_length);
            break;

        default:
            break;
        }

        return *this;
    }

    DebugLogger &Print(const char *str)
    {
        if (str == nullptr)
            return *this;
        
        _PrintBytes(str, strlen(str));
        
        return *this;
    }

    DebugLogger &Print(const char *str, const char *topic)
    {
        if (str == nullptr)
            return *this;
        
        return Print(str, strlen(str), topic, LOG_OUT_TYPE_BYTES);
    }

    DebugLogger &Print(const void *data, uint16_t data_length, logger_output_type_t data_type = LOG_OUT_TYPE_BYTES)
    {
        return Print(data, data_length, nullptr, data_type);
    }

    DebugLogger &Print(const void *data, uint16_t data_length, const char *topic)
    {
        return Print(data, data_length, topic, LOG_OUT_TYPE_BYTES);
    }

    DebugLogger &PrintNewLine()
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

    bool _FillHEX(uint8_t byte, bool last_item = false)
    {
        int result = snprintf(_buffer_ptr, _buffer_size_left, "0x%02X%s", byte, last_item ? "" : ", ");

        if (result <= 0 || result >= _buffer_size_left)
            return false;

        _buffer_ptr += result;
        _buffer_size_left -= result;

        return true;
    }

    void _PrintHEX(const void *data, uint16_t data_length)
    {
        static_assert(buffer_length > 6, "Can't print HEX with small buffer. String like '0xFF, \0' is 7 bytes length.");

        if (data == nullptr || data_length == 0)
            return;

        for (uint16_t i = 0; i < data_length; i++)
        {
            if ( !_FillHEX( ((uint8_t *)data)[i], (i == data_length - 1) ) )
            {
                _Print(_buffer, buffer_length - _buffer_size_left);
                _FillHEX( ((uint8_t *)data)[i], (i == data_length - 1) );
            }
        }

        _Print(_buffer, buffer_length - _buffer_size_left);
    }

    void _PrintBytes(const void *data, uint16_t data_length)
    {
        if (data == nullptr || data_length == 0)
            return;

        uint16_t data_left = data_length;

        uint8_t *data_pointer = (uint8_t *)data;
        uint16_t copied_length = 0;
        while (data_left > 0)
        {
            copied_length = (data_left <= _buffer_size_left) ? data_left : _buffer_size_left;
            memcpy(_buffer_ptr, data_pointer, copied_length);
            _buffer_size_left -= copied_length;
            data_left -= copied_length;
            data_pointer += copied_length;

            _Print(_buffer, buffer_length - _buffer_size_left);
        }
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

DebugLogger<DEBUG_LOGGER_SIZE> Logger;

#if defined(DEBUG) || defined(DETAILED_DEBUG)
    #define DEBUG_LOG_SIMPLE(fmt, ...) do { Logger.Printf(fmt, ##__VA_ARGS__); } while (0)
    #define DEBUG_LOG_TOPIC(topic, fmt, ...) do { Logger.PrintTopic(topic).Printf(fmt, ##__VA_ARGS__); } while (0)
    #define DEBUG_LOG_ARRAY_BIN(topic, data, data_length) do { Logger.Print(data, data_length, topic, LOG_OUT_TYPE_BYTES); } while (0)
    #define DEBUG_LOG_ARRAY_HEX(topic, data, data_length) do { Logger.Print(data, data_length, topic, LOG_OUT_TYPE_HEX); } while (0)
    #define DEBUG_LOG_STR(topic, null_terminated_string) do { Logger.Print(null_terminated_string, topic); } while (0)
	#define DEBUG_LOG_NEW_LINE() do { Logger.PrintNewLine(); } while (0)

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
    #define DEBUG_LOG_ARRAY_BIN(topic, data, data_length)
    #define DEBUG_LOG_ARRAY_HEX(topic, data, data_length)
    #define DEBUG_LOG_STR(topic, null_terminated_string)
	#define DEBUG_LOG_NEW_LINE()
#endif // DEBUG
