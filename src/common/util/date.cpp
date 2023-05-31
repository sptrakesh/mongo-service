//
// Created by Rakesh on 02/05/2020.
//

#include "date.h"
#if __has_include("../../log/NanoLog.h")
#include "../../log/NanoLog.h"
#else
#include <log/NanoLog.h>
#endif

#include <charconv>
#include <sstream>

namespace spt::util
{
  namespace pdate
  {
    bool isLeapYear( int16_t year )
    {
      bool result = false;

      if ( ( year % 400 ) == 0 ) result = true;
      else if ( ( year % 100 ) == 0 ) result = false;
      else if ( ( year % 4 ) == 0 ) result = true;

      return result;
    }
  }

  int64_t microSeconds( std::string_view date )
  {
    const auto var = parseISO8601( date );
    if ( std::holds_alternative<std::string>( var ) ) return 0;
    return std::get<DateTime>( var ).time_since_epoch().count();
  }

  std::variant<DateTime, std::string> parseISO8601( std::string_view date )
  {
    // 2021-02-11
    // 2021-02-11T11:17:43Z
    // 2021-02-11T11:17:43-0600
    // 2021-02-11T11:17:43+05:30
    // 2021-02-11T11:17:43.12Z
    // 2021-02-11T11:17:43.12-0600
    // 2021-02-11T11:17:43.12+05:30
    // 2021-02-11T11:17:43.123Z
    // 2021-02-11T11:17:43.123-0600
    // 2021-02-11T11:17:43.123+05:30
    // 2021-02-11T11:17:43.123456Z
    // 2021-02-11T11:17:43.123456-0600
    // 2021-02-11T11:17:43.123456+05:30
    if ( date.size() < 10 )
    {
      LOG_WARN << "Invalid date-time: " << date;
      return "Invalid date format";
    }
    if ( date.size() > 10 && date.size() < 20 )
    {
      LOG_WARN << "Invalid date-time: " << date;
      return "Invalid datetime format";
    }
    if ( date.size() > 10 && date[10] != 'T' )
    {
      LOG_WARN << "Invalid date-time: " << date;
      return "Invalid datetime separator";
    }

    static constexpr auto microSecondsPerHour = int64_t( 3600000000 );

    int16_t year{0};
    {
      auto sv = date.substr( 0, 4 );
      auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), year );
      if ( ec != std::errc() )
      {
        LOG_WARN << "Invalid date-time: " << date;
        return "Invalid datetime year";
      }
    }

    int16_t month{0};
    {
      auto sv = date.substr( 5, 2 );
      auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), month );
      if ( ec != std::errc() )
      {
        LOG_WARN << "Invalid date-time: " << date;
        return "Invalid datetime month";
      }
    }

    int16_t day{0};
    {
      auto sv = date.substr( 8, 2 );
      auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), day );
      if ( ec != std::errc() )
      {
        LOG_WARN << "Invalid date-time: " << date;
        return "Invalid datetime day";
      }
    }

    int16_t hour{0};
    if ( date.size() >= 13 && date[10] == 'T' )
    {
      auto sv = date.substr( 11, 2 );
      auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), hour );
      if ( ec != std::errc() )
      {
        LOG_WARN << "Invalid date-time: " << date;
        return "Invalid datetime hour";
      }
    }

    int16_t minute{0};
    if ( date.size() >= 16 && date[10] == 'T' && date[13] == ':' )
    {
      auto sv = date.substr( 14, 2 );
      auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), minute );
      if ( ec != std::errc() )
      {
        LOG_WARN << "Invalid date-time: " << date;
        return "Invalid datetime minute";
      }
    }

    int16_t second{0};
    if ( date.size() >= 19 && date[10] == 'T' && date[13] == ':' && date[16] == ':')
    {
      auto sv = date.substr( 17, 2 );
      auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), second );
      if ( ec != std::errc() )
      {
        LOG_WARN << "Invalid date-time: " << date;
        return "Invalid datetime second";
      }
    }

    const auto parseMillis = [date]() -> std::variant<int16_t, std::string>
    {
      if ( date[19] != '.' ) return int16_t( 0 );
      if ( date.size() < 22 )
      {
        LOG_WARN << "Invalid date-time: " << date;
        return "Invalid datetime millis";
      }

      int16_t m{0};
      auto sv = date.substr( 20, 3 );
      auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), m );
      if ( ec != std::errc() )
      {
        LOG_WARN << "Invalid date-time: " << date;
        return "Invalid datetime millis";
      }
      return m;
    };

    const auto parseMicros = [date]( int16_t millis ) -> std::variant<int16_t, std::string>
    {
      if ( date[19] != '.' ) return int16_t( 0 );
      if ( date.size() < 25 ) return int16_t( 0 );

      std::size_t idx = 23;
      if ( millis < 100 && !std::isdigit( date[22] ) )
      {
        idx = 22;
      }
      switch ( date[idx] )
      {
      case '+':
      case '-':
      case 'Z':
        return int16_t( 0 );
      }

      int16_t m{0};
      auto sv = date.substr( 23, 3 );
      auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), m );
      if ( ec != std::errc() )
      {
        LOG_WARN << "Invalid date-time: " << date;
        return "Invalid datetime micros";
      }
      return m;
    };

    using Tuple = std::tuple<int16_t, int16_t>;
    const auto parseZone = [&date]( int16_t millis ) -> std::variant<Tuple, std::string>
    {
      if ( date.size() < 20 ) return Tuple{ int16_t( 0 ), int16_t( 0 ) };

      const auto c19 = date[19];
      switch ( c19 )
      {
      case 'Z':
        return Tuple{ int16_t( 0 ), int16_t( 0 ) };
      case '+':
      case '-':
      {
        if ( date.size() < 24 )
        {
          LOG_WARN << "Invalid date-time: " << date;
          return "Invalid datetime zone";
        }
        const int16_t mult = date[19] == '+' ? 1 : -1;
        if ( date.size() == 24 )
        {
          int16_t h{0};
          {
            auto sv = date.substr( 20, 2 );
            auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), h );
            if ( ec != std::errc() )
            {
              LOG_WARN << "Invalid date-time: " << date;
              return "Invalid datetime zone hour";
            }
          }

          int16_t s{0};
          {
            auto sv = date.substr( 22, 2 );
            auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), s );
            if ( ec != std::errc() )
            {
              LOG_WARN << "Invalid date-time: " << date;
              return "Invalid datetime zone minute";
            }
          }
          return Tuple{ h * mult, s };
        }
        if ( date[22] != ':' )
        {
          LOG_WARN << "Invalid date-time: " << date << " at 22 " << date[22] << " for size " << int( date.size() );
          return "Invalid datetime zone";
        }

        int16_t h{0};
        {
          auto sv = date.substr( 20, 2 );
          auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), h );
          if ( ec != std::errc() )
          {
            LOG_WARN << "Invalid date-time: " << date;
            return "Invalid datetime zone hour";
          }
        }

        int16_t s{0};
        {
          auto sv = date.substr( 23, 2 );
          auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), s );
          if ( ec != std::errc() )
          {
            LOG_WARN << "Invalid date-time: " << date;
            return "Invalid datetime zone minute";
          }
        }
        return Tuple{ h * mult, s };
      }
      case '.':
        if ( millis < 100 && date.size() < 23 )
        {
          LOG_WARN << "Invalid date-time: " << date;
          return "Invalid datetime fraction";
        }
        if ( millis > 100 && date.size() < 24 )
        {
          LOG_WARN << "Invalid date-time: " << date;
          return "Invalid datetime fraction";
        }

        std::size_t idx = 23;
        if ( millis < 100 && !std::isdigit( date[22] ) )
        {
          idx = 22;
        }
        const auto c23 = date[idx];
        switch ( c23 )
        {
        case 'Z':
          return Tuple{ int16_t( 0 ), int16_t( 0 ) };
        case '+':
        case '-':
          if ( date.size() < ( idx + 5 ) )
          {
            LOG_WARN << "Invalid date-time: " << date;
            return "Invalid datetime zone";
          }
          const int16_t mult = date[idx] == '+' ? 1 : -1;
          if ( date.size() == ( idx + 5 ) )
          {
            int16_t h{0};
            {
              auto sv = date.substr( idx + 1, 2 );
              auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), h );
              if ( ec != std::errc() )
              {
                LOG_WARN << "Invalid date-time: " << date;
                return "Invalid datetime zone hour";
              }
            }

            int16_t s{0};
            {
              auto sv = date.substr( idx + 3, 2 );
              auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), s );
              if ( ec != std::errc() )
              {
                LOG_WARN << "Invalid date-time: " << date;
                return "Invalid datetime zone minute";
              }
            }
            return Tuple{ h * mult, s };
          }
          if ( date[idx + 3] != ':' )
          {
            LOG_WARN << "Invalid date-time: " << date << " at 26 " << date[26] << " for size " << int( date.size() );
            return "Invalid datetime zone";
          }

          int16_t h{0};
          {
            auto sv = date.substr( idx + 1, 2 );
            auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), h );
            if ( ec != std::errc() )
            {
              LOG_WARN << "Invalid date-time: " << date;
              return "Invalid datetime zone hour";
            }
          }

          int16_t s{0};
          {
            auto sv = date.substr( idx + 4, 2 );
            auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), s );
            if ( ec != std::errc() )
            {
              LOG_WARN << "Invalid date-time: " << date;
              return "Invalid datetime zone minute";
            }
          }
          return Tuple{ h * mult, s };
        }

        if ( date.size() < ( idx + 4 ) )
        {
          LOG_WARN << "Invalid date-time: " << date;
          return "Invalid datetime zone";
        }
        const auto c26 = date[idx + 3];
        switch ( c26 )
        {
        case 'Z':
          return Tuple{ int16_t( 0 ), int16_t( 0 ) };
        case '+':
        case '-':
          if ( date.size() < idx + 8 )
          {
            LOG_WARN << "Invalid date-time: " << date;
            return "Invalid datetime zone";
          }
          const int16_t mult = date[idx + 3] == '+' ? 1 : -1;
          if ( date.size() == ( idx + 8 ) )
          {
            int16_t h{0};
            {
              auto sv = date.substr( idx + 4, 2 );
              auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), h );
              if ( ec != std::errc() )
              {
                LOG_WARN << "Invalid date-time: " << date;
                return "Invalid datetime zone hour";
              }
            }

            int16_t s{0};
            {
              auto sv = date.substr( idx + 6, 2 );
              auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), s );
              if ( ec != std::errc() )
              {
                LOG_WARN << "Invalid date-time: " << date;
                return "Invalid datetime zone minute";
              }
            }
            return Tuple{ h * mult, s };
          }
          if ( date[idx + 6] != ':' )
          {
            LOG_WARN << "Invalid date-time: " << date << " at 29 " << date[idx + 6] << " for size " << int( date.size() );
            return "Invalid datetime zone";
          }

          int16_t h{0};
          {
            auto sv = date.substr( idx + 4, 2 );
            auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), h );
            if ( ec != std::errc() )
            {
              LOG_WARN << "Invalid date-time: " << date;
              return "Invalid datetime zone hour";
            }
          }

          int16_t s{0};
          {
            auto sv = date.substr( idx + 7, 2 );
            auto [p, ec] = std::from_chars( sv.data(), sv.data() + sv.size(), s );
            if ( ec != std::errc() )
            {
              LOG_WARN << "Invalid date-time: " << date;
              return "Invalid datetime zone minute";
            }
          }
          return Tuple{ h * mult, s };
        }
      }

      LOG_WARN << "Invalid date-time: " << date;
      return "Invalid datetime format zone";
    };

    const auto pm = parseMillis();
    if ( std::holds_alternative<std::string>( pm ) ) return std::get<std::string>( pm );
    int16_t millis = std::get<int16_t>( pm );

    const auto pmi = parseMicros( millis );
    if ( std::holds_alternative<std::string>( pmi ) ) return std::get<std::string>( pmi );
    const int16_t micros = std::get<int16_t>( pmi );

    const auto z = parseZone( millis );
    if ( std::holds_alternative<std::string>( z ) ) return std::get<std::string>( z );

    const auto [dsth, dstm] = std::get<Tuple>( z );
    if ( dsth > 23 || dstm > 59 )
    {
      LOG_WARN << "Invalid date-time: " << date;
      return "Invalid datetime zone";
    }

    if ( millis > 0 && !std::isdigit( date[22] ) ) millis *= 10;

    int64_t epoch = micros;
    epoch += millis * int64_t( 1000 );
    epoch += second * int64_t( 1000000 );
    epoch += minute * int64_t( 60000000 );
    epoch += hour * microSecondsPerHour;
    epoch += ( day - 1 ) * 24 * microSecondsPerHour;

    const int8_t isLeap = pdate::isLeapYear( year );

    for ( int i = 1; i < month; ++i )
    {
      switch ( i )
      {
      case 2:
        epoch += ( (isLeap) ? 29 : 28 ) * 24 * microSecondsPerHour;
        break;
      case 4:
      case 6:
      case 9:
      case 11:
        epoch += 30 * 24 * microSecondsPerHour;
        break;
      default:
        epoch += 31 * 24 * microSecondsPerHour;
      }
    }

    for ( int i = 1970; i < year; ++i )
    {
      if ( pdate::isLeapYear( i ) ) epoch += 366 * 24 * microSecondsPerHour;
      else epoch += 365 * 24 * microSecondsPerHour;
    }

    if ( dsth != 0 ) epoch += -1 * dsth * microSecondsPerHour;
    if ( dstm != 0 )
    {
      if ( dsth > 0 ) epoch -= dstm * int64_t( 60000000 );
      else epoch += dstm * int64_t( 60000000 );
    }
    return DateTime{ std::chrono::microseconds{ epoch } };
  }

  std::string isoDateMicros( int64_t epoch )
  {
    const int micros = epoch % int64_t( 1000 );
    epoch /= int64_t( 1000 );

    const int millis = epoch % int64_t( 1000 );
    epoch /= int64_t( 1000 );

    const int second = epoch % 60;

    epoch /= 60;
    const int minute = epoch % 60;

    epoch /= 60;
    const int hour = epoch % 24;
    epoch /= 24;
    int year = 1970;

    {
      int32_t days = 0;
      while ( ( days += ( pdate::isLeapYear( year ) ) ? 366 : 365 ) <= epoch ) ++year;

      days -= ( pdate::isLeapYear( year ) ) ? 366 : 365;
      epoch -= days;
    }

    uint8_t isLeap = pdate::isLeapYear( year );
    int month = 1;

    for ( ; month < 13; ++month )
    {
      int8_t length = 0;

      switch ( month )
      {
      case 2:
        length = isLeap ? 29 : 28;
        break;
      case 4:
      case 6:
      case 9:
      case 11:
        length = 30;
        break;
      default:
        length = 31;
      }

      if ( epoch >= length ) epoch -= length;
      else break;
    }

    const int day = epoch + 1;
    std::stringstream ss;
    ss << year << '-';

    if ( month < 10 ) ss << 0;
    ss << month << '-';

    if ( day < 10 ) ss << 0;
    ss << day << 'T';

    if ( hour < 10 ) ss << 0;
    ss << hour << ':';

    if ( minute < 10 ) ss << 0;
    ss << minute << ':';

    if ( second < 10 ) ss << 0;
    ss << second << '.';

    if ( millis < 10 ) ss << "00";
    else if ( millis < 100 ) ss << 0;
    ss << millis;

    if ( micros < 10 ) ss << "00";
    else if ( micros < 100 ) ss << 0;
    ss << micros << 'Z';

    return ss.str();
  }

  std::string isoDateMicros( std::chrono::microseconds epoch )
  {
    return isoDateMicros( epoch.count() );
  }

  std::string isoDateMicros( std::chrono::milliseconds epoch )
  {
    return isoDateMicros( std::chrono::duration_cast<std::chrono::microseconds>( epoch ).count() );
  }

  std::string isoDateMicros( const DateTime& epoch )
  {
    return isoDateMicros( std::chrono::duration_cast<std::chrono::microseconds>( epoch.time_since_epoch() ).count() );
  }

  std::string isoDateMillis( int64_t epoch )
  {
    epoch /= int64_t( 1000 );

    const int millis = epoch % int64_t( 1000 );
    epoch /= int64_t( 1000 );

    const int second = epoch % 60;

    epoch /= 60;
    const int minute = epoch % 60;

    epoch /= 60;
    const int hour = epoch % 24;
    epoch /= 24;
    int year = 1970;

    {
      int32_t days = 0;
      while ( ( days += ( pdate::isLeapYear( year ) ) ? 366 : 365 ) <= epoch ) ++year;

      days -= ( pdate::isLeapYear( year ) ) ? 366 : 365;
      epoch -= days;
    }

    uint8_t isLeap = pdate::isLeapYear( year );
    int month = 1;

    for ( ; month < 13; ++month )
    {
      int8_t length = 0;

      switch ( month )
      {
      case 2:
        length = isLeap ? 29 : 28;
        break;
      case 4:
      case 6:
      case 9:
      case 11:
        length = 30;
        break;
      default:
        length = 31;
      }

      if ( epoch >= length ) epoch -= length;
      else break;
    }

    const int day = epoch + 1;
    std::stringstream ss;
    ss << year << '-';

    if ( month < 10 ) ss << 0;
    ss << month << '-';

    if ( day < 10 ) ss << 0;
    ss << day << 'T';

    if ( hour < 10 ) ss << 0;
    ss << hour << ':';

    if ( minute < 10 ) ss << 0;
    ss << minute << ':';

    if ( second < 10 ) ss << 0;
    ss << second << '.';

    if ( millis < 10 ) ss << "00";
    else if ( millis < 100 ) ss << 0;
    ss << millis << 'Z';

    return ss.str();
  }

  std::string isoDateMillis( std::chrono::microseconds epoch )
  {
    return isoDateMillis( epoch.count() );
  }

  std::string isoDateMillis( std::chrono::milliseconds epoch )
  {
    return isoDateMillis( std::chrono::duration_cast<std::chrono::microseconds>( epoch ).count() );
  }

  std::string isoDateMillis( const DateTime& epoch )
  {
    return isoDateMillis( std::chrono::duration_cast<std::chrono::microseconds>( epoch.time_since_epoch() ).count() );
  }

}

