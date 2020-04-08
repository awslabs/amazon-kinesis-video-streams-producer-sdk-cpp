#pragma once

#include <log4cplus/configurator.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/logger.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/version.h>
#include <sstream>
#include <stdexcept>

namespace com { namespace amazonaws { namespace kinesis { namespace video {

// configure the logger by loading configuration from specific properties file.
// generally, it should be called only once in your main() function.
#define LOG_CONFIGURE(filename) \
  try { \
    log4cplus::PropertyConfigurator::doConfigure(filename); \
  } catch(...) { \
    LOG4CPLUS_ERROR(log4cplus::Logger::getRoot(), "Exception occured while opening " << filename); \
  }

#if LOG4CPLUS_VERSION < LOG4CPLUS_MAKE_VERSION(2,0,0)
  #define _LOG_CONFIGURE_CONSOLE(level, logToStdErr) \
    log4cplus::helpers::SharedObjectPtr<log4cplus::Appender> _appender(new log4cplus::ConsoleAppender()); \
    std::auto_ptr<log4cplus::Layout> _layout(new log4cplus::PatternLayout("%D [%t] ")); \
    _appender->setLayout(_layout); \
    log4cplus::BasicConfigurator::doConfigure(log4cplus::Logger::getDefaultHierarchy(), logToStdErr); \
    log4cplus::Logger::getRoot().addAppender(_appender); \
    log4cplus::Logger::getRoot().setLogLevel(log4cplus::getLogLevelManager().fromString(level));
#else
  #define _LOG_CONFIGURE_CONSOLE(level, logToStdErr) \
    log4cplus::helpers::SharedObjectPtr<log4cplus::Appender> _appender(new log4cplus::ConsoleAppender()); \
    _appender->setLayout(move(std::unique_ptr<log4cplus::PatternLayout>(new log4cplus::PatternLayout("%D [%t] ")))); \
    log4cplus::BasicConfigurator::doConfigure(log4cplus::Logger::getDefaultHierarchy(), logToStdErr); \
    log4cplus::Logger::getRoot().addAppender(_appender); \
    log4cplus::Logger::getRoot().setLogLevel(log4cplus::getLogLevelManager().fromString(level));
#endif

#define LOG_CONFIGURE_STDOUT(level) _LOG_CONFIGURE_CONSOLE(level, false)

#define LOG_CONFIGURE_STDERR(level) _LOG_CONFIGURE_CONSOLE(level, true)

// runtime queries for enabled log level. useful if message construction is expensive.
#define LOG_IS_TRACE_ENABLED (KinesisVideoLogger::getInstance().isEnabledFor(log4cplus::TRACE_LOG_LEVEL))
#define LOG_IS_DEBUG_ENABLED (KinesisVideoLogger::getInstance().isEnabledFor(log4cplus::DEBUG_LOG_LEVEL))
#define LOG_IS_INFO_ENABLED  (KinesisVideoLogger::getInstance().isEnabledFor(log4cplus::INFO_LOG_LEVEL))
#define LOG_IS_WARN_ENABLED  (KinesisVideoLogger::getInstance().isEnabledFor(log4cplus::WARN_LOG_LEVEL))
#define LOG_IS_ERROR_ENABLED (KinesisVideoLogger::getInstance().isEnabledFor(log4cplus::ERROR_LOG_LEVEL))
#define LOG_IS_FATAL_ENABLED (KinesisVideoLogger::getInstance().isEnabledFor(log4cplus::FATAL_LOG_LEVEL))

// logging macros - any usage must be preceded by a LOGGER_TAG definition visible at the current scope.
// failure to use the LOGGER_TAG macro will result in "error: 'KinesisVideoLogger' has not been declared"
#define LOG_TRACE(msg)   LOG4CPLUS_TRACE(KinesisVideoLogger::getInstance(), msg);
#define LOG_DEBUG(msg)   LOG4CPLUS_DEBUG(KinesisVideoLogger::getInstance(), msg);
#define LOG_INFO(msg)    LOG4CPLUS_INFO(KinesisVideoLogger::getInstance(), msg);
#define LOG_WARN(msg)    LOG4CPLUS_WARN(KinesisVideoLogger::getInstance(), msg);
#define LOG_ERROR(msg)   LOG4CPLUS_ERROR(KinesisVideoLogger::getInstance(), msg);
#define LOG_FATAL(msg)   LOG4CPLUS_FATAL(KinesisVideoLogger::getInstance(), msg);

#define LOG_AND_THROW(msg) \
  do { \
    std::ostringstream __oss; \
    __oss << msg; \
    LOG_ERROR(__oss.str()); \
    throw std::runtime_error(__oss.str()); \
  } while (0)

#define LOG_AND_THROW_IF(cond, msg) \
  if (cond) { \
    LOG_AND_THROW(msg); \
  }

#define ASSERT_MSG(cond, msg) \
  LOG_AND_THROW_IF(!(cond), \
      __FILE__ << ":" << __LINE__ << ": " << msg << ": " << #cond)

#define ASSERT(cond) \
  ASSERT_MSG(cond, "Assertion failed");


// defines a class which contains a logger instance with the given tag
#define LOGGER_TAG(tag) \
  struct KinesisVideoLogger { \
    static log4cplus::Logger& getInstance() { \
      static log4cplus::Logger s_logger = log4cplus::Logger::getInstance(tag); \
      return s_logger; \
    } \
  };

} // namespace video
} // namespace kinesis
} // namespace amazonaws
} // namespace com
