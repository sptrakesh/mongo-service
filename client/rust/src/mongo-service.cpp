//
// Created by Rakesh on 21/10/2025.
//
#include "mongo-service/include/mongo-service.hpp"

#include <string_view>
#include <bsoncxx/validate.hpp>
#include <bsoncxx/document/view.hpp>
#include <mongo-service/api/api.hpp>
#include <log/NanoLog.hpp>

void init_logger( Logger conf )
{
  switch ( conf.level )
  {
  case LogLevel::DEBUG: nanolog::set_log_level( nanolog::LogLevel::DEBUG ); break;
  case LogLevel::INFO: nanolog::set_log_level( nanolog::LogLevel::INFO ); break;
  case LogLevel::WARN: nanolog::set_log_level( nanolog::LogLevel::WARN ); break;
  case LogLevel::CRIT: nanolog::set_log_level( nanolog::LogLevel::CRIT ); break;
  }

  auto dir = std::string( conf.path.data(), conf.path.size() );
  auto name = std::string( conf.name.data(), conf.name.size() );
  nanolog::initialize( nanolog::GuaranteedLogger(), dir, name, conf.console );
}

void init( Configuration conf )
{
  auto pool = spt::mongoservice::pool::Configuration{};
  pool.initialSize = conf.pool.initialSize;
  pool.maxPoolSize = conf.pool.maxPoolSize;
  pool.maxConnections = conf.pool.maxConnections;
  pool.maxIdleTime = std::chrono::seconds{ conf.pool.maxIdleTimeSeconds };
  auto app = std::string_view{ conf.application.begin(), conf.application.end() };
  auto sh = std::string_view{ conf.host.begin(), conf.host.end() };
  auto sp = std::to_string( conf.port );
  spt::mongoservice::api::init( sh, sp, app, pool );
}

rust::Vec<uint8_t> execute( rust::Vec<uint8_t> data )
{
  const auto view = bsoncxx::validate( data.data(), data.size() );
  if ( !view )
  {
    LOG_WARN << "Invalid BSON document";
    throw std::runtime_error( "Invalid BSON document" );
  }

  const auto [type, response] = spt::mongoservice::api::execute( *view );
  if ( type == spt::mongoservice::api::ResultType::poolFailure ) throw std::runtime_error( "Pool exhausted" );
  if ( type == spt::mongoservice::api::ResultType::commandFailure ) throw std::runtime_error( "Command failed" );
  if ( !response ) throw std::runtime_error( "Empty response" );

  auto resp = rust::Vec<uint8_t>{};
  resp.reserve( response->length() );
  auto buf = response->data();
  for ( std::size_t i = 0; i < response->length(); ++i ) resp.push_back( buf[i] );
  return resp;
}
