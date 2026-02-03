/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __LOGGING_H__
#define __LOGGING_H__

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/initializer.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif

#include <boost/units/detail/utility.hpp>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#define TYPE_NAME(_type) boost::units::detail::demangle(typeid(_type).name())

#define LOG(_Level, _Msg)                                        \
    do {                                                         \
        if (LoggingDisabled()) break;                            \
        log4cplus::Logger logger = log4cplus::Logger::getRoot(); \
        LOG4CPLUS_##_Level(logger, _Msg);                        \
    } while (0)

/// @brief A class providing basic control over logging capabilities
/// in OpenSDN control plane.
class Logging {

    /// @brief A log4cplus object to maintain multi- and singlethreaded
    /// execution of the logging library.
    log4cplus::Initializer initializer_;

public:

    /// @brief Prepares log4cplus library for execution. Uses RAII
    /// to free resources after the completion of the program.
    Logging();

    /// @brief Destroys the object and shutdowns the logging
    /// system.
    ~Logging();

    /// @brief Performs basic initialization of the logging system (
    /// log4cplus).
    void Init();

    /// @brief Performs customized initialization of the logging system (
    /// log4cplus) using settings specified as the arguments of the
    /// function.
    void Init(const std::string &filename,
              long maxFileSize,
              int maxBackupIndex,
              bool useSyslog,
              const std::string &syslogFacility,
              const std::string &ident,
              log4cplus::LogLevel logLevel);

    /// @brief Performs customized initialization of the logging system (
    /// log4cplus) using settings specified in the provided file.
    void Init(const std::string &propertyFile);
};

void LoggingInit();
void LoggingInit(const std::string &filename,
                 long maxFileSize,
                 int maxBackupIndex,
                 bool useSyslog,
                 const std::string &syslogFacility,
                 const std::string &ident,
                 log4cplus::LogLevel logLevel);

void LoggingInit(const std::string &propertyFile);
void SetLoggingLevel(log4cplus::LogLevel logLevel);
bool LoggingUseSyslog();

//
// Disable logging - For testing purposes only
//
bool LoggingDisabled();
void SetLoggingDisabled(bool flag);
void SetUseSysLog(bool); // To be used only for testing
#endif /* __LOGGING_H__ */
