//
// Created by Rakesh on 24/05/2023.
//

#pragma once

#include "concept.hpp"
#include "bson.hpp"
#include "date.hpp"
#include "../magic_enum/magic_enum_all.hpp"
#include "parser.hpp"
#include "validate.hpp"
#include "../simdjson/simdjson.h"

#include <memory>
#include <ostream>
#include <set>
#include <vector>
#include <boost/json/value.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <bsoncxx/oid.hpp>

#define FROM_JSON( field, obj ) { \
  auto res = (obj).find_field_unordered( #field ); \
  if ( res.error() == simdjson::error_code::SUCCESS ) spt::util::json::set( (#field), (field), res.value() ); \
}

#define FROM_JSON_NS( field, obj, ns ) { \
  auto res = (obj).find_field_unordered( #field ); \
  if ( res.error() == simdjson::error_code::SUCCESS ) ns::set( (#field), (field), res.value() ); \
}

#define FROM_JSON_NAME( name, field, obj ) { \
  auto res = (obj).find_field_unordered( (name) ); \
  if ( res.error() == simdjson::error_code::SUCCESS ) spt::util::json::set( (name), (field), res.value() ); \
}

#define FROM_JSON_NAME_NS( name, field, obj, ns ) { \
  auto res = (obj).find_field_unordered( (name) ); \
  if ( res.error() == simdjson::error_code::SUCCESS ) ns::set( (name), (field), res.value() ); \
}

#define FROM_JSON_OBJ( field, obj, instance ) { \
  auto res = (obj).find_field_unordered( ( #field ) ); \
  if ( res.error() == simdjson::error_code::SUCCESS ) spt::util::json::set( (#field), (instance).(field), res.value() ); \
}

#define FROM_JSON_OBJ_NS( field, obj, instance, ns ) { \
  auto res = (obj).find_field_unordered( ( #field ) ); \
  if ( res.error() == simdjson::error_code::SUCCESS ) ns::set( (#field), (instance).(field), res.value() ); \
}

namespace spt::util::json
{
  /**
   * Add non-visitable fields in the model to the document.  A callback function that library users can implement to
   * fully serialise partially visitable models.
   * @tparam M The partially visitable model type.
   * @param model The model instance to be fully serialised to JSON.
   * @param object The JSON object to add non-visitable fields to.
   */
  template <Visitable M>
    requires NotEnumeration<M>
  void populate( const M& model, boost::json::object& object );

  /**
   * This is usually invoked from the {@xrefitem marshall(const M&)} function.  Can also be used if you wish a wrapped
   * JSON value variant instead of a document.
   * @tparam M The visitable struct.
   * @param model Instance of the visitable struct to convert to a JSON document.
   * @return A JSON value encapsulating the JSON document.
   */
  template <Visitable M>
    requires NotEnumeration<M>
  boost::json::value json( const M& model );

  /**
   *
   * @tparam E The scoped enum type to be converted to a JSON value.
   * @param model The scoped enum instance to be converted to a JSON value.  Outputs the name of the enumerated value.
   * @return The JSON representation of the enum.
   */
  template <typename E>
    requires std::is_enum_v<E>
  boost::json::value json( const E& model ) { return boost::json::value{ magic_enum::enum_name( model ) }; }

  /**
   * General implementation for converting a set into a JSON array.  For each item in the set delegates
   * to the appropriate {@xrefitem json(const M&)} function.
   * @tparam Model The type stored in the set.
   * @param items The set to serialise into a JSON array.
   * @return The JSON array as a JSON value variant.
   */
  template <typename Model>
  boost::json::value json( const std::set<Model>& items );

  /**
   * General implementation for converting a vector into a JSON array.  For each item in the vector delegates
   * to the appropriate {@xrefitem json(const M&)} function.
   * @tparam Model The type stored in the vector.
   * @param vec The vector to serialise into a JSON array.
   * @return The JSON array as a JSON value variant.
   */
  template <typename Model>
  boost::json::value json( const std::vector<Model>& vec );

  /**
   * General implementation for serialising an optional type.  Delegates to the appropriate {@xrefitem json(const M&)} function
   * if the variant is set.
   * @tparam T The type wrapped in the optional.
   * @param model The optional instance to be serialised.
   * @return The JSON value variant.
   */
  template <typename T>
  boost::json::value json( const std::optional<T>& model )
  {
    return model ? json( *model ) : boost::json::value{};
  }

  /**
   * General implementation for serialising a shared pointer to type.  Delegates to the appropriate {@xrefitem json(const M&)} function
   * if the shared pointer is valid.
   * @tparam T The type wrapped in the shared pointer.
   * @param model The shared pointer instance to be serialised.
   * @return The JSON value variant.
   */
  template <typename T>
  boost::json::value json( const std::shared_ptr<T>& model )
  {
    return model ? json( *model.get() ) : boost::json::value{};
  }

  /**
   * General function for marshalling a class/struct to a JSON document.  Implementation
   * usually only required for *non-visitable* classes/structures.  Implement this function in your own
   * namespace for classes/structures that cannot be automatically serialised.
   * @tparam Model The type of class/structure.
   * @param model The instance to convert to a JSON value.
   * @return The value variant that represents the JSON document.
   */
  template <typename Model>
  boost::json::value json( const Model& model );

  /**
   * Concept that represents a serialisable entity.  An entity is serialisable if it is default constructable,
   * visitable and serialisable to JSON via the {@xrefitem json(const M&)} function.
   * @tparam T The type of the model.
   */
  template <typename T>
  concept Model = requires( T t )
  {
    std::is_default_constructible<T>{};
    visit_struct::traits::is_visitable<T>{};
    { json( t ) } -> std::convertible_to<boost::json::value>;
  };

  /**
   * Serialise the visitable model into a JSON document.  Iterates over the visitable fields in the model and
   * adds to the output JSON document.  For partially visitable models, implement the {@xrefitem populate(const M&, boost::json::object&)} function
   * to add the non-visitable fields to the JSON document as appropriate.
   * @tparam M The type of the model.
   * @param model The visitable and serialisable model.
   * @return The JSON document instance representing the model.
   */
  template <Model M>
  boost::json::object marshall( const M& model )
  {
    auto root = boost::json::object{};
    visit_struct::for_each( model,
        [&root]( const char* name, const auto& value )
        {
          auto v = json( value );
          if ( !v.is_null() ) root.emplace( name, std::move( v ) );
        } );

    if constexpr ( visit_struct::traits::ext::is_fully_visitable<M>() == false ) populate( model, root );
    return root;
  }

  /**
   * Populate non-visitable fields in the specified model from the JSON object.  A call-back function that users
   * can implement to ensure full hydration of the model from the JSON document.
   * @tparam Model The partially visitable model
   * @param model The instance that is to be fully unmarshalled from JSON.
   * @param object The JSON object to unmarshall data from.
   */
  template <Visitable M>
  void populate( M& model, simdjson::ondemand::object& object );

  /**
   * Populate the fields in the visitable type from the JSON object.
   * @tparam M The visitable type
   * @param field The model instance that is to be populated from the JSON object.
   * @param object The JSON object to read data from.
   */
  template <Visitable M>
      requires NotEnumeration<M>
  void set( M& field, simdjson::ondemand::object& object );

  /**
   * Populate the fields in the custom type from the JSON object.  Implement this function for non-visitable
   * classes/structures as appropriate.
   * @tparam M The custom type
   * @param field The model instance that is to be populated from the JSON object.
   * @param object The JSON object to read data from.
   */
  template <typename M>
  void set( M& field, simdjson::ondemand::object& object );

  /**
   * General purpose function for populating a set of items from a JSON value variant which should be of type array.
   * @tparam Model The type of items stored in the set.
   * @param name The name of the field in the parent object. Used for logging.
   * @param field The set to populate from JSON.
   * @param value The JSON value variant of type array.
   */
  template <typename Model>
  void set( const char* name, std::set<Model>& field, simdjson::ondemand::value& value );

  /**
   * General purpose function for populating a vector of items from a JSON value variant which should be of type array.
   * @tparam Model The type of items stored in the vector.
   * @param name The name of the field in the parent object. Used for logging.
   * @param field The vector to populate from JSON.
   * @param value The JSON value variant of type array.
   */
  template <typename Model>
  void set( const char* name, std::vector<Model>& field, simdjson::ondemand::value& value );

  /**
   * Standard implementation for setting optional types.  If the JSON value is not `null` delegate to the appropriate
   * {@xrefitem set(const char*, M&, simdjson::ondemand::value&)} function.
   * @tparam M The type wrapped in the optional.
   * @param name The name of the field in the parent object. Used for logging.
   * @param field The field that is to be set.
   * @param value The JSON value variant.
   */
  template <typename M>
  void set( const char* name, std::optional<M>& field, simdjson::ondemand::value& value );

  /**
   * Standard implementation for setting shared pointer types.  If the JSON value is not `null` delegate to the appropriate
   * {@xrefitem set(const char*, M&, simdjson::ondemand::value&)} function.
   * @tparam M The type wrapped in the shared pointer.
   * @param name The name of the field in the parent object. Used for logging.
   * @param field The field that is to be set.
   * @param value The JSON value variant.
   */
  template <typename M>
  void set( const char* name, std::shared_ptr<M>& field, simdjson::ondemand::value& value );

  /**
   * Standard implementation for visitable types.
   * @tparam M The visitable type
   * @param name The name of the field in the parent object. Used for logging.
   * @param field The model instance that is to be populated from the JSON value variant.
   * @param value The JSON value variant. The value must be a document.
   */
  template <Visitable M>
      requires NotEnumeration<M>
  void set( const char* name, M& field, simdjson::ondemand::value& value );

  /**
   *
   * @tparam E The scoped enum type that is to be unmarshalled from JSON.
   * @param name The name of the field that is being unmarshalled.
   * @param field The scoped enum field to be unmarshalled.
   * @param value The JSON value to de-serialise into the enum.
   */
  template <typename E>
      requires std::is_enum_v<E>
  void set( const char* name, E& field, simdjson::ondemand::value& value )
  {
    if ( value.type().value() != simdjson::ondemand::json_type::string )
    {
      LOG_WARN << "Expected field " << name << " of type string, value of type " << magic_enum::enum_name( value.type().value() );
    }
    std::string_view v;
    value.get( v );
    if ( auto e = magic_enum::enum_cast<E>( v ); e ) field = *e;
    else LOG_WARN << "Value " << v << " is not a valid enumeration of type " << typeid( E ).name();
  }

  /**
   * General interface used by the {@refitem unmarshall(M&, bsoncxx::document::view)} function to unmarshall a model from a JSON value.
   * For non-visitable models, specialise this function as appropriate in your own namespace.  This is used when
   * visitable structures includes non-visitable fields.
   * @tparam M The type of the model.
   * @param name The name of the field in the parent object. Used for logging.
   * @param field The model instance whose fields are to be populated.
   * @param value The JSON value variant to read data from.
   */
  template <typename M>
  void set( const char* name, M& field, simdjson::ondemand::value& value );

  /**
   * Unmarshall the visitable fields in the specified model from the JSON document.  This is usually used for
   * visitable structs that have already been created.
   * @tparam M The type of the model.
   * @param model The model instance to unmarshall into.
   * @param view The JSON document to unmarshall struct fields from.
   */
  template <Model M>
  void unmarshall( M& model, std::string_view view )
  {
    auto& pool = parser::Pool::instance();
    auto proxy = pool.acquire();
    auto& parser = proxy.value().operator*();
    auto str = simdjson::padded_string{ view };
    auto doc = parser.parser().iterate( str );
    auto obj = doc.get_object().value();
    set( model, obj );
  }

  /**
   * Unmarshall a model instance from the specified JSON document.
   * @tparam M The type of the model.
   * @param view The JSON document to unmarshall the model from.
   * @return The unmarshalled model instance.
   */
  template <Model M>
  M unmarshall( std::string_view view )
  {
    M model{};
    unmarshall( model, view );
    return model;
  }

  /**
   * Unmarshall a JSON array into the specified vector of model objects.
   * @tparam M The type of the model.
   * @param container The vector into which model instances in the JSON array are to be unmarshalled into.
   * @param view The JSON string to parse.
   */
  template <Model M>
  void unmarshall( std::vector<M>& container, std::string_view view )
  {
    auto& pool = parser::Pool::instance();
    auto proxy = pool.acquire();
    auto& parser = proxy.value().operator*();
    auto str = simdjson::padded_string{ view };
    auto doc = parser.parser().iterate( str );

    auto arr = doc.get_array().value();
    container.reserve( arr.count_elements() );
    for ( simdjson::ondemand::object obj : arr )
    {
      container.emplace_back();
      set( container.back(), obj );
    }
  }

  /**
   * General purpose interface to unmarshall a model from a JSON document.  Users can specialise this function
   * for their non-visitable structures.
   * @tparam M The type of the model
   * @param model The instance that is to be unmarshalled from JSON.
   * @param view The JSON document to unmarshall into the model.
   */
  template <typename M>
  void unmarshall( M& model, std::string_view view );

  /**
   * General purpose interface to serialise a model to a string.  Uses `boost::json::serialize` to serialise the
   * value returned by the {@xrefitem json( const Model& )} function.
   * @tparam Model The type of the model.
   * @param model The instance that is to be converted to a string representation.
   * @return The JSON value serialised to a string.
   */
  template <typename Model>
  std::string str( const Model& model )
  {
    return boost::json::serialize( json( model ) );
  }

  /**
   * Serialise the model to the specified output stream.
   * @tparam Model The type of the model.
   * @param os The output stream to serialise the model into.
   * @param model The instance that is to be serialised into the output stream.
   * @return The output stream for chaining.
   */
  template <typename Model>
  std::ostream& operator<<( std::ostream& os, const Model& model )
  {
    os << json( model );
    return os;
  }
}

template <>
inline boost::json::value spt::util::json::json( const bool& model ) { return boost::json::value( model ); }

template <>
inline boost::json::value spt::util::json::json( const int8_t& model ) { return boost::json::value( static_cast<int16_t>( model ) ); }

template <>
inline boost::json::value spt::util::json::json( const uint8_t& model ) { return boost::json::value( static_cast<int16_t>( model ) ); }

template <>
inline boost::json::value spt::util::json::json( const int16_t& model ) { return boost::json::value( model ); }

template <>
inline boost::json::value spt::util::json::json( const uint16_t& model ) { return boost::json::value( model ); }

template <>
inline boost::json::value spt::util::json::json( const int32_t& model ) { return boost::json::value( model ); }

template <>
inline boost::json::value spt::util::json::json( const uint32_t& model ) { return boost::json::value( model ); }

template <>
inline boost::json::value spt::util::json::json( const int64_t& model ) { return boost::json::value( model ); }

template <>
inline boost::json::value spt::util::json::json( const float& model ) { return boost::json::value( model ); }

template <>
inline boost::json::value spt::util::json::json( const double& model ) { return boost::json::value( model ); }

template <>
inline boost::json::value spt::util::json::json( const bsoncxx::oid& model ) { return boost::json::value( model.to_string() ); }

template <>
inline boost::json::value spt::util::json::json( const std::string& model )
{
  return model.empty() ? boost::json::value{} : boost::json::value( model );
}

template <>
inline boost::json::value spt::util::json::json( const DateTime& model )
{
  const auto c = std::chrono::duration_cast<std::chrono::microseconds>( model.time_since_epoch() );
  return boost::json::value( spt::util::isoDateMillis( c ) );
}

template <>
inline boost::json::value spt::util::json::json( const DateTimeMs& model )
{
  const auto c = std::chrono::duration_cast<std::chrono::milliseconds>( model.time_since_epoch() );
  return boost::json::value( spt::util::isoDateMillis( c ) );
}

template <>
inline boost::json::value spt::util::json::json( const DateTimeNs& model )
{
  const auto c = std::chrono::duration_cast<std::chrono::microseconds>( model.time_since_epoch() );
  return boost::json::value( spt::util::isoDateMillis( c ) );
}

template <>
inline boost::json::value spt::util::json::json( const boost::json::array& model )
{
  return model.empty() ? boost::json::value{} : boost::json::value( model );
}

template <>
inline boost::json::value spt::util::json::json( const boost::json::object& model )
{
  return model.empty() ? boost::json::value{} : boost::json::value( model );
}

template <>
inline boost::json::value spt::util::json::json( const bsoncxx::array::value& model )
{
  return toJson( model );
}

template <>
inline boost::json::value spt::util::json::json( const bsoncxx::document::value& model )
{
  return toJson( model );
}

template <spt::util::Visitable M>
    requires spt::util::NotEnumeration<M>
inline boost::json::value spt::util::json::json( const M& model )
{
  auto root = boost::json::object{};
  visit_struct::for_each( model,
      [&root]( const char* name, const auto& value )
      {
        auto v = json( value );
        if ( !v.is_null() ) root.emplace( name, std::move( v ) );
      } );

  if constexpr ( visit_struct::traits::ext::is_fully_visitable<M>() == false ) populate( model, root );
  return root;
}

template <typename Model>
inline boost::json::value spt::util::json::json( const std::set<Model>& items )
{
  if ( items.empty() ) return boost::json::value{};
  auto arr = boost::json::array{};
  arr.reserve( items.size() );
  for ( const auto& item : items )
  {
    auto v = json( item );
    if ( !v.is_null() ) arr.push_back( std::move( v ) );
  }
  return arr.empty() ? boost::json::value{} : arr;
}

template <typename Model>
inline boost::json::value spt::util::json::json( const std::vector<Model>& vec )
{
  if ( vec.empty() ) return boost::json::value{};
  auto arr = boost::json::array{};
  arr.reserve( vec.size() );
  for ( const auto& item : vec )
  {
    auto v = json( item );
    if ( !v.is_null() ) arr.push_back( std::move( v ) );
  }
  return arr.empty() ? boost::json::value{} : arr;
}

template <spt::util::Visitable M>
  requires spt::util::NotEnumeration<M>
void spt::util::json::set( M& field, simdjson::ondemand::object& object )
{
  visit_struct::for_each( field,
      [&object]( const char* name, auto& member )
      {
        auto result = object.find_field_unordered( name );
        if ( result.error() == simdjson::error_code::SUCCESS && !result.is_null() )
        {
          set( name, member, result.value() );
        }
      } );

  if constexpr ( visit_struct::traits::ext::is_fully_visitable<M>() == false ) populate( field, object );
}

template <spt::util::Visitable M>
  requires spt::util::NotEnumeration<M>
inline void spt::util::json::set( const char*, M& field, simdjson::ondemand::value& value )
{
  auto obj = value.get_object().value();
  set( field, obj );
}

template <>
inline void spt::util::json::set( const char* name, bool& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::boolean )
  {
    LOG_WARN << "Expected field " << name << " of type bool, value of type " << magic_enum::enum_name( value.type().value() );
  }
  bool bv{ false };
  value.get( bv );

  if ( !validate( name, bv ) ) throw simdjson::simdjson_error{ simdjson::error_code::F_ATOM_ERROR };
  field = bv;
}

template <>
inline void spt::util::json::set( const char* name, int8_t& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::number )
  {
    LOG_WARN << "Expected field " << name << " of type int8_t, value of type " << magic_enum::enum_name( value.type().value() );
  }
  int64_t v{ 0 };
  value.get( v );
  const auto iv = static_cast<int8_t>( v );

  if ( !validate( name, iv ) ) throw simdjson::simdjson_error{ simdjson::error_code::F_ATOM_ERROR };
  field = iv;
}

template <>
inline void spt::util::json::set( const char* name, uint8_t& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::number )
  {
    LOG_WARN << "Expected field " << name << " of type int8_t, value of type " << magic_enum::enum_name( value.type().value() );
  }
  int64_t v{ 0 };
  value.get( v );
  auto iv = static_cast<uint8_t>( v );

  if ( !validate( name, iv ) ) throw simdjson::simdjson_error{ simdjson::error_code::F_ATOM_ERROR };
  field = iv;
}

template <>
inline void spt::util::json::set( const char* name, int16_t& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::number )
  {
    LOG_WARN << "Expected field " << name << " of type int16_t, value of type " << magic_enum::enum_name( value.type().value() );
  }
  int64_t v{ 0 };
  value.get( v );
  auto iv = static_cast<int16_t>( v );

  if ( !validate( name, iv ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
  field = iv;
}

template <>
inline void spt::util::json::set( const char* name, uint16_t& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::number )
  {
    LOG_WARN << "Expected field " << name << " of type int16_t, value of type " << magic_enum::enum_name( value.type().value() );
  }
  int64_t v{ 0 };
  value.get( v );
  auto iv = static_cast<uint16_t>( v );

  if ( !validate( name, iv ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
  field = iv;
}

template <>
inline void spt::util::json::set( const char* name, int32_t& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::number )
  {
    LOG_WARN << "Expected field " << name << " of type int32_t, value of type " << magic_enum::enum_name( value.type().value() );
  }
  int64_t v{ 0 };
  value.get( v );
  auto iv = static_cast<int32_t>( v );

  if ( !validate( name, iv ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
  field = iv;
}

template <>
inline void spt::util::json::set( const char* name, uint32_t& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::number )
  {
    LOG_WARN << "Expected field " << name << " of type int32_t, value of type " << magic_enum::enum_name( value.type().value() );
  }
  int64_t v{ 0 };
  value.get( v );
  auto iv = static_cast<uint32_t>( v );

  if ( !validate( name, iv ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
  field = iv;
}

template <>
inline void spt::util::json::set( const char* name, int64_t& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::number )
  {
    LOG_WARN << "Expected field " << name << " of type int64_t, value of type " << magic_enum::enum_name( value.type().value() );
  }
  int64_t iv{ 0 };
  value.get( iv );

  if ( !validate( name, iv ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
  field = iv;
}

template <>
inline void spt::util::json::set( const char* name, double& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::number )
  {
    LOG_WARN << "Expected field " << name << " of type double, value of type " << magic_enum::enum_name( value.type().value() );
  }
  double dv{ 0.0 };
  value.get( dv );

  if ( !validate( name, dv ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
  field = dv;
}

template <>
inline void spt::util::json::set( const char* name, bsoncxx::oid& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::string )
  {
    LOG_WARN << "Expected field " << name << " of type oid string, value of type " << magic_enum::enum_name( value.type().value() );
  }
  std::string_view v;
  value.get( v );

  bsoncxx::oid id;
  try
  {
    id = bsoncxx::oid{ v };
  }
  catch ( const std::exception& ex )
  {
    LOG_CRIT << "Invalid BSON object id " << v << " for field " << name << ". " << ex.what();
    throw;
  }

  if ( !validate( name, id ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
  field = id;
}

template <>
inline void spt::util::json::set( const char* name, std::string& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::string )
  {
    LOG_WARN << "Expected field " << name << " of type string, value of type " << magic_enum::enum_name( value.type().value() );
  }
  std::string_view v;
  value.get( v );
  std::string str;
  str.reserve( v.size() );
  str.append( v );

  if ( !validate( name, str ) ) throw simdjson::simdjson_error{ simdjson::error_code::STRING_ERROR };
  field = std::move( str );
}

template <>
inline void spt::util::json::set( const char* name, DateTime& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::string )
  {
    LOG_WARN << "Expected field " << name << " of type string, value of type " << magic_enum::enum_name( value.type().value() );
  }
  std::string_view v;
  value.get( v );
  auto date = util::parseISO8601( v );
  if ( date.has_value() )
  {
    auto dt = DateTime{ date->time_since_epoch() };
    if ( !validate( name, dt ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
    field = dt;
  }
  else LOG_WARN << "Error parsing ISO datetime from " << v << " for field " << name;
}

template <>
inline void spt::util::json::set( const char* name, DateTimeMs& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::string )
  {
    LOG_WARN << "Expected field " << name << " of type string, value of type " << magic_enum::enum_name( value.type().value() );
  }
  std::string_view v;
  value.get( v );
  auto date = util::parseISO8601( v );
  if ( date.has_value() )
  {
    auto dt = DateTime{ date->time_since_epoch() };
    if ( !validate( name, dt ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
    field = DateTimeMs{ std::chrono::duration_cast<std::chrono::milliseconds>( dt.time_since_epoch() ) };
  }
  else LOG_WARN << "Error parsing ISO datetime from " << v << " for field " << name;
}

template <>
inline void spt::util::json::set( const char* name, DateTimeNs& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::string )
  {
    LOG_WARN << "Expected field " << name << " of type string, value of type " << magic_enum::enum_name( value.type().value() );
  }
  std::string_view v;
  value.get( v );
  auto date = util::parseISO8601( v );
  if ( date.has_value() )
  {
    auto dt = DateTime{ date->time_since_epoch() };
    if ( !validate( name, dt ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
    field = DateTimeNs{ std::chrono::duration_cast<std::chrono::microseconds>( dt.time_since_epoch() ) };
  }
  else LOG_WARN << "Error parsing ISO datetime from " << v << " for field " << name;
}

template <>
inline void spt::util::json::set( const char* name, boost::json::array& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
    return;
  }

  auto str = value.raw_json();
  if ( str.error() == simdjson::error_code::SUCCESS )
  {
    auto ec = boost::system::error_code{};
    auto p = boost::json::parse( str.value(), ec );
    if ( ec ) LOG_WARN << "Error parsing from JSON. " << ec.message() << ". " << str.value();
    else field = p.as_array();
  }
  else LOG_WARN << "Error converting value to string. " << simdjson::error_message( str.error() );
}

template <>
inline void spt::util::json::set( const char* name, boost::json::object& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::object )
  {
    LOG_WARN << "Expected field " << name << " of type object, value of type " << magic_enum::enum_name( value.type().value() );
    return;
  }

  auto str = value.raw_json();
  if ( str.error() == simdjson::error_code::SUCCESS )
  {
    auto ec = boost::system::error_code{};
    auto p = boost::json::parse( str.value(), ec );
    if ( ec ) LOG_WARN << "Error parsing from JSON. " << ec.message() << ". " << str.value();
    else field = p.as_object();
  }
  else LOG_WARN << "Error converting value to string. " << simdjson::error_message( str.error() );
}

template <>
inline void spt::util::json::set( const char* name, bsoncxx::array::value& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
    return;
  }

  auto str = value.raw_json();
  if ( str.error() == simdjson::error_code::SUCCESS )
  {
    auto ec = boost::system::error_code{};
    auto p = boost::json::parse( str.value(), ec );
    if ( ec ) LOG_WARN << "Error parsing from JSON. " << ec.message() << ". " << str.value();
    else field = toBson( p.as_array() );
  }
  else LOG_WARN << "Error converting value to string. " << simdjson::error_message( str.error() );
}

template <>
inline void spt::util::json::set( const char* name, bsoncxx::document::value& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::object )
  {
    LOG_WARN << "Expected field " << name << " of type object, value of type " << magic_enum::enum_name( value.type().value() );
    return;
  }

  auto str = value.raw_json();
  if ( str.error() == simdjson::error_code::SUCCESS )
  {
    auto ec = boost::system::error_code{};
    auto p = boost::json::parse( str.value(), ec );
    if ( ec ) LOG_WARN << "Error parsing from JSON. " << ec.message() << ". " << str.value();
    else field = toBson( p.as_object() );
  }
  else LOG_WARN << "Error converting value to string. " << simdjson::error_message( str.error() );
}

template <>
inline void spt::util::json::set( const char* name, std::set<bool>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  for ( bool x: arr )
  {
    if ( !validate( name, x ) ) throw simdjson::simdjson_error{ simdjson::error_code::F_ATOM_ERROR };
    field.insert( x );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::vector<bool>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  field.reserve( arr.count_elements() );
  for ( bool x: arr )
  {
    if ( !validate( name, x ) ) throw simdjson::simdjson_error{ simdjson::error_code::F_ATOM_ERROR };
    field.push_back( x );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::set<int32_t>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  for ( int64_t x: arr )
  {
    const auto v = static_cast<int32_t>( x );
    if ( !validate( name, v ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
    field.insert( v );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::vector<int32_t>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  field.reserve( 8 );
  for ( int64_t x: arr )
  {
    const auto v = static_cast<int32_t>( x );
    if ( !validate( name, v ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
    field.push_back( v );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::set<int64_t>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  for ( int64_t x: arr )
  {
    if ( !validate( name, x ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
    field.insert( x );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::vector<int64_t>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  field.reserve( 8 );
  for ( int64_t x: arr )
  {
    if ( !validate( name, x ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
    field.push_back( x );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::set<double>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  for ( double x: arr )
  {
    if ( !validate( name, x ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
    field.insert( x );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::vector<double>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  field.reserve( 8 );
  for ( double x: arr )
  {
    if ( !validate( name, x ) ) throw simdjson::simdjson_error{ simdjson::error_code::NUMBER_OUT_OF_RANGE };
    field.push_back( x );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::set<std::string, std::less<>>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  for ( std::string_view x: arr )
  {
    if ( !validate( name, x ) ) throw simdjson::simdjson_error{ simdjson::error_code::STRING_ERROR };
    field.emplace( x );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::vector<std::string>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  field.reserve( 8 );
  for ( std::string_view x: arr )
  {
    if ( !validate( name, x ) ) throw simdjson::simdjson_error{ simdjson::error_code::STRING_ERROR };
    field.emplace_back( x );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::set<bsoncxx::oid>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  for ( std::string_view x: arr )
  {
    auto v = bsoncxx::oid{ x };
    if ( !validate( name, v ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
    field.insert( v );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::vector<bsoncxx::oid>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  field.reserve( 8 );
  for ( std::string_view x: arr )
  {
    auto v = bsoncxx::oid{ x };
    if ( !validate( name, v ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
    field.push_back( v );
  }
}

template <>
inline void spt::util::json::set( const char* name, std::set<DateTime>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  for ( std::string_view x: arr )
  {
    if ( const auto date = util::parseISO8601( x ); date.has_value() )
    {
      if ( !validate( name, date.value() ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
      field.emplace( date->time_since_epoch() );
    }
    else LOG_WARN << "Error parsing ISO datetime from " << x << " for field " << name;
  }
}

template <>
inline void spt::util::json::set( const char* name, std::set<DateTimeMs>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  for ( std::string_view x: arr )
  {
    if ( const auto date = util::parseISO8601( x ); date.has_value() )
    {
      if ( !validate( name, date.value() ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
      field.emplace( std::chrono::duration_cast<std::chrono::milliseconds>( date->time_since_epoch() ) );
    }
    else LOG_WARN << "Error parsing ISO datetime from " << x << " for field " << name;
  }
}

template <>
inline void spt::util::json::set( const char* name, std::set<DateTimeNs>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  for ( std::string_view x: arr )
  {
    if ( const auto date = util::parseISO8601( x ); date.has_value() )
    {
      if ( !validate( name, date.value() ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
      field.emplace( date->time_since_epoch() );
    }
    else LOG_WARN << "Error parsing ISO datetime from " << x << " for field " << name;
  }
}

template <>
inline void spt::util::json::set( const char* name, std::vector<DateTime>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  field.reserve( 8 );
  for ( std::string_view x: arr )
  {
    if ( const auto date = util::parseISO8601( x ); date.has_value() )
    {
      if ( !validate( name, date ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
      field.emplace_back( date->time_since_epoch() );
    }
    else LOG_WARN << "Error parsing ISO datetime from " << x << " for field " << name;
  }
}

template <>
inline void spt::util::json::set( const char* name, std::vector<DateTimeMs>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  field.reserve( 8 );
  for ( std::string_view x: arr )
  {
    if ( const auto date = util::parseISO8601( x ); date.has_value() )
    {
      if ( !validate( name, date ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
      field.emplace_back( std::chrono::duration_cast<std::chrono::milliseconds>( date->time_since_epoch() ) );
    }
    else LOG_WARN << "Error parsing ISO datetime from " << x << " for field " << name;
  }
}

template <>
inline void spt::util::json::set( const char* name, std::vector<DateTimeNs>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  field.reserve( 8 );
  for ( std::string_view x: arr )
  {
    if ( const auto date = util::parseISO8601( x ); date.has_value() )
    {
      if ( !validate( name, date ) ) throw simdjson::simdjson_error{ simdjson::error_code::UTF8_ERROR };
      field.emplace_back( date->time_since_epoch() );
    }
    else LOG_WARN << "Error parsing ISO datetime from " << x << " for field " << name;
  }
}

template <typename M>
inline void spt::util::json::set( const char* name, std::set<M>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  for ( simdjson::ondemand::object obj : arr )
  {
    auto m = M{};
    set( m, obj );
    field.insert( std::move( m ) );
  }
}

template <typename M>
inline void spt::util::json::set( const char* name, std::vector<M>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() != simdjson::ondemand::json_type::array )
  {
    LOG_WARN << "Expected field " << name << " of type array, value of type " << magic_enum::enum_name( value.type().value() );
  }
  auto arr = value.get_array();
  field.reserve( arr.count_elements() );
  for ( simdjson::ondemand::object obj : arr )
  {
    field.emplace_back();
    set( field.back(), obj );
  }
}

template <typename M>
inline void spt::util::json::set( const char* name, std::optional<M>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() == simdjson::ondemand::json_type::null ) return;
  auto m = M{};
  set( name, m, value );
  field = std::move( m );
}

template <typename M>
inline void spt::util::json::set( const char* name, std::shared_ptr<M>& field, simdjson::ondemand::value& value )
{
  if ( value.type().value() == simdjson::ondemand::json_type::null ) return;
  auto m = std::make_shared<M>();
  set( name, *m.get(), value );
  field = m;
}
