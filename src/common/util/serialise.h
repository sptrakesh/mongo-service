//
// Created by Rakesh on 12/05/2023.
//

#pragma once

#include "bson.h"
#include "concept.h"
#if defined __has_include
#if __has_include("../../log/NanoLog.h")
#include "../../log/NanoLog.h"
#else
#include <log/NanoLog.h>
#endif
#endif

#include <memory>
#include <set>
#include <vector>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/types/bson_value/value.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>

#define FROM_BSON( type, property, bson ) { \
  auto s = spt::util::bsonValueIfExists<type>( #property, bson ); \
  if ( s ) (property) = *s; \
}

#define FROM_BSON_OPT( type, key, property, bson ) { \
  auto s = spt::util::bsonValueIfExists<type>( key, bson ); \
  if ( s ) (property) = *s; \
}

#define FROM_BSON_TYPE( type, property, bson, cls ) { \
  auto s = spt::util::bsonValueIfExists<type>( #property, bson ); \
  if ( s ) (property) = cls{ *s }; \
}

#define FROM_BSON_OPT_TYPE( type, key, property, bson, cls ) { \
  auto s = spt::util::bsonValueIfExists<type>( key, bson ); \
  if ( s ) (property) = cls{ *s }; \
}

namespace spt::util
{
  /**
   * Add non-visitable fields in the model to the builder.  A callback function that library users can implement to
   * fully serialise partially visitable models.
   * @tparam M The partially visitable model type.
   * @param model The model instance to be fully serialised to BSON.
   * @param builder The BSON stream builder to add non-visitable fields to.
   */
  template <Visitable M>
  void populate( const M& model, bsoncxx::builder::stream::document& builder );

  /**
   * This is usually invoked from the {@xrefitem marshall(const M&)} function.  Can also be used if you wish a wrapped
   * BSON value variant instead of a document.
   * @tparam M The visitable struct.
   * @param model Instance of the visitable struct to convert to a BSON document.
   * @return A BSON value encapsulating the BSON document.
   */
  template <Visitable M>
  bsoncxx::types::bson_value::value bson( const M& model );

  /**
   * General implementation for converting a set into a BSON array.  For each item in the set delegates
   * to the appropriate {@xrefitem bson(const M&)} function.
   * @tparam Model The type stored in the vector.
   * @param items The set to serialise into a BSON array.
   * @return The BSON array as a BSON value variant.
   */
  template<typename Model>
  bsoncxx::types::bson_value::value bson( const std::set<Model>& items );

  /**
   * General implementation for converting a vector into a BSON array.  For each item in the vector delegates
   * to the appropriate {@xrefitem bson(const M&)} function.
   * @tparam Model The type stored in the vector.
   * @param vec The vector to serialise into a BSON array.
   * @return The BSON array as a BSON value variant.
   */
  template <typename Model>
  bsoncxx::types::bson_value::value bson( const std::vector<Model>& vec );

  /**
   * General implementation for serialising an optional type.  Delegates to the appropriate {@xrefitem bson(const M&)} function
   * if the variant is set.
   * @tparam T The type wrapped in the optional.
   * @param model The optional instance to be serialised.
   * @return The BSON value variant.
   */
  template <typename T>
  inline bsoncxx::types::bson_value::value bson( const std::optional<T>& model )
  {
    return model ? bson( *model ) : bsoncxx::types::b_null{};
  }

  /**
   * General implementation for serialising a shared pointer type.  Delegates to the appropriate {@xrefitem bson(const M&)} function
   * if the shared pointer is valid.
   * @tparam T The type wrapped in the shared pointer.
   * @param model The shared pointer instance to be serialised.
   * @return The BSON value variant.
   */
  template <typename T>
  inline bsoncxx::types::bson_value::value bson( const std::shared_ptr<T>& model )
  {
    return model ? bson( *model.get() ) : bsoncxx::types::b_null{};
  }

  /**
   * General function for marshalling a class/struct to a BSON document.  Implementation
   * usually only required for *non-visitable* classes/structures.  Implement this function in your own
   * namespace for classes/structures that cannot be automatically serialised.
   * @tparam Model The type of class/structure.
   * @param model The instance to convert to a BSON value.
   * @return The value variant that represents the BSON document.
   */
  template <typename Model>
  bsoncxx::types::bson_value::value bson( const Model& model );

  /**
   * Concept that represents a serialisable entity.  An entity is serialisable if it is default constructable,
   * visitable and serialisable to BSON via the {@xrefitem bson(const M&)} function.
   * @tparam T The type of the model.
   */
  template <typename T>
  concept Model = requires( T t )
  {
    std::is_default_constructible<T>{};
    visit_struct::traits::is_visitable<T>{};
    { bson( t ) } -> std::convertible_to<bsoncxx::types::bson_value::value>;
  };

  /**
   * Serialise the visitable model into a BSON document.  Iterates over the visitable fields in the model and
   * adds to the output BSON document.  For partially visitable models, implement the {@xrefitem populate(const M&, bsoncxx::builder::stream::document&)} function
   * to add the non-visitable fields to the BSON stream builder as appropriate.
   * @tparam M The type of the model.
   * @param model The visitable and serialisable model.
   * @return The BSON document instance representing the model.
   */
  template <Model M>
  bsoncxx::document::value marshall( const M& model )
  {
    using std::operator ""sv;
    auto root = bsoncxx::builder::stream::document{};
    visit_struct::for_each( model,
        [&root]( const char* name, const auto& value )
        {
          auto n = std::string_view{ name };
          auto v = bson( value );
          if ( n == "id" && v.view().type() == bsoncxx::type::k_oid ) root << "_id"sv << std::move( v );
          else if ( v.view().type() != bsoncxx::type::k_null ) root << n << std::move( v );
        } );

    if constexpr ( visit_struct::traits::ext::is_fully_visitable<M>() == false ) populate( model, root );
    return root << bsoncxx::builder::stream::finalize;
  }

  /**
   * Populate non-visitable fields in the specified model from the BSON value variant.  A call-back function that users
   * can implement to ensure full hydration of the model from the BSON document.
   * @tparam Model The partially visitable model
   * @param model The instance that is to be fully unmarshalled from BSON.
   * @param view The BSON document to unmarshall data from.
   */
  template <Visitable M>
  void populate( M& model, bsoncxx::document::view view );

  /**
   * General purpose function for populating a set of items from a BSON value variant which should be of type array.
   * @tparam Model The type of items stored in the set.
   * @param field The set to populate from BSON.
   * @param value The BSON value variant of type array.
   */
  template<typename Model>
  void set( std::set<Model>& field, bsoncxx::types::bson_value::view value );

  /**
   * General purpose function for populating a vector of items from a BSON value variant which should be of type array.
   * @tparam Model The type of items stored in the vector.
   * @param field The vector to populate from BSON.
   * @param value The BSON value variant of type array.
   */
  template <typename Model>
  void set( std::vector<Model>& field, bsoncxx::types::bson_value::view value );

  /**
   * Standard implementation for setting optional types.  If the BSON value is not `null` delegate to the appropriate
   * {@xrefitem set(M&, bsoncxx::types::bson_value::view)} function.
   * @tparam M The type wrapped in the optional.
   * @param field The field that is to be set.
   * @param value The BSON value variant.
   */
  template <typename M>
  void set( std::optional<M>& field, bsoncxx::types::bson_value::view value );

  /**
   * Standard implementation for setting shared pointer types.  If the BSON value is not `null` delegate to the appropriate
   * {@xrefitem set(M&, bsoncxx::types::bson_value::view)} function.
   * @tparam M The type wrapped in the shared pointer.
   * @param field The field that is to be set.
   * @param value The BSON value variant.
   */
  template <typename M>
  void set( std::shared_ptr<M>& field, bsoncxx::types::bson_value::view value );

  /**
   * Standard implementation for visitable types.
   * @tparam M The visitable type
   * @param field The model instance that is to be populated from the BSON value variant.
   * @param value The BSON value variant. The value must be a document.
   */
  template <Visitable M>
  void set( M& field, bsoncxx::types::bson_value::view value );

  /**
   * General interface used by the {@refitem unmarshall(M&, bsoncxx::document::view)} function to unmarshall a model from a BSON value.
   * For non-visitable models, specialise this function as appropriate in your own namespace.  This is used when
   * visitable structures includes non-visitable fields.
   * @tparam M The type of the model.
   * @param field The model instance whose fields are to be populated.
   * @param value The BSON value variant to read data from.
   */
  template <typename M>
  void set( M& field, bsoncxx::types::bson_value::view value );

  /**
   * Unmarshall the visitable fields in the specified model from the BSON document.  This is usually used for
   * visitable structs which have already been created.
   * @tparam M The type of the model.
   * @param model The model instance to unmarshall into.
   * @param view The BSON document to unmarshall struct fields from.
   */
  template <Model M>
  inline void unmarshall( M& model, bsoncxx::document::view view )
  {
    auto value = bsoncxx::types::bson_value::value{ view };
    set( model, value.view() );
  }

  /**
   * Unmarshall a model instance from the specified BSON document.
   * @tparam M The type of the model.
   * @param view The BSON document to unmarshall the model from.
   * @return The unmarshalled model instance.
   */
  template <Model M>
  inline M unmarshall( bsoncxx::document::view view )
  {
    M model{};
    unmarshall( model, view );
    return model;
  }

  /**
   * General purpose interface to unmarshall a model from a BSON document.  Users can specialise this function
   * for their non-visitable structures.
   * @tparam M The type of the model
   * @param model The instance that is to be unmarshalled from BSON.
   * @param view The BSON document to unmarshall into the model.
   */
  template <typename M>
  void unmarshall( M& model, bsoncxx::document::view view );
}

template <>
inline bsoncxx::types::bson_value::value spt::util::bson( const bool& model ) { return { model }; }

template <>
inline bsoncxx::types::bson_value::value spt::util::bson( const int32_t& model ) { return { model }; }

template <>
inline bsoncxx::types::bson_value::value spt::util::bson( const int64_t& model ) { return { model }; }

template <>
inline bsoncxx::types::bson_value::value spt::util::bson( const float& model ) { return { model }; }

template <>
inline bsoncxx::types::bson_value::value spt::util::bson( const double& model ) { return { model }; }

template <>
inline bsoncxx::types::bson_value::value spt::util::bson( const std::string& model )
{
  return model.empty() ? bsoncxx::types::b_null{} : bsoncxx::types::bson_value::value{ model };
}

template <>
inline bsoncxx::types::bson_value::value spt::util::bson( const bsoncxx::oid& model ) { return { model }; }

template <>
inline bsoncxx::types::bson_value::value spt::util::bson( const std::chrono::time_point<std::chrono::system_clock>& model )
{
  return { bsoncxx::types::b_date{ model } };
}

template <spt::util::Visitable M>
inline bsoncxx::types::bson_value::value spt::util::bson( const M& model )
{
  using std::operator ""sv;
  auto root = bsoncxx::builder::stream::document{};
  visit_struct::for_each( model,
      [&root]( const char* name, const auto& value )
      {
        auto n = std::string_view{ name };
        auto v = bson( value );
        if ( n == "id"sv && v.view().type() == bsoncxx::type::k_oid )
        {
          root << "_id"sv << std::move( v );
        } else if ( v.view().type() != bsoncxx::type::k_null ) root << n << std::move( v );
      } );

  if constexpr ( visit_struct::traits::ext::is_fully_visitable<M>() == false ) populate( model, root );
  return { root << bsoncxx::builder::stream::finalize };
}

template<typename Model>
inline bsoncxx::types::bson_value::value spt::util::bson( const std::set<Model> &items )
{
  if ( items.empty()) return bsoncxx::types::b_null{};

  auto arr = bsoncxx::builder::stream::array{};
  for ( const auto &item: items )
  {
    auto v = bson( item );
    if ( v.view().type() != bsoncxx::type::k_null ) arr << std::move( v );
  }
  return { arr << bsoncxx::builder::stream::finalize };
}

template <typename Model>
inline bsoncxx::types::bson_value::value spt::util::bson( const std::vector<Model>& vec )
{
  if ( vec.empty() ) return bsoncxx::types::b_null{};

  auto arr = bsoncxx::builder::stream::array{};
  for ( const auto& item: vec )
  {
    auto v = bson( item );
    if ( v.view().type() != bsoncxx::type::k_null ) arr << std::move( v );
  }
  return { arr << bsoncxx::builder::stream::finalize };
}

template <spt::util::Visitable M>
inline void spt::util::set( M& field, bsoncxx::types::bson_value::view value )
{
  if ( value.type() != bsoncxx::type::k_document ) LOG_CRIT << "Value not document type but " << bsoncxx::to_string( value.type() );
  auto view = value.get_document().value;
  visit_struct::for_each( field,
      [&view]( const char* name, auto& member )
      {
        using std::operator ""sv;
        auto n = std::string_view{ name };
        if ( auto it = view.find( n ); it != std::cend( view ) )
        {
          if ( it->type() != bsoncxx::type::k_null ) set( member, it->get_value() );
        }
        else if ( n == "id"sv )
        {
          if ( auto iter = view.find( "_id"sv ); iter != std::cend( view ) && iter->type() == bsoncxx::type::k_oid )
          {
            set( member, iter->get_value() );
          }
        }
      } );

  if constexpr ( visit_struct::traits::ext::is_fully_visitable<M>() == false ) populate( field, view );
}

template <>
inline void spt::util::set( bool& field, bsoncxx::types::bson_value::view value ) { field = value.get_bool().value; }

template <>
inline void spt::util::set( int32_t& field, bsoncxx::types::bson_value::view value )
{
  switch ( value.type() )
  {
  case bsoncxx::type::k_int64:
    field = static_cast<int32_t>( value.get_int64().value );
    break;
  case bsoncxx::type::k_double:
    field = static_cast<int32_t>( value.get_double().value );
    break;
  default:
    field = value.get_int32().value;
    break;
  }
}

template <>
inline void spt::util::set( int64_t& field, bsoncxx::types::bson_value::view value )
{
  switch ( value.type() )
  {
  case bsoncxx::type::k_int32:
    field = static_cast<int64_t>( value.get_int32().value );
    break;
  case bsoncxx::type::k_double:
    field = static_cast<int64_t>( value.get_double().value );
    break;
  default:
    field = value.get_int64().value;
    break;
  }
}

template <>
inline void spt::util::set( double& field, bsoncxx::types::bson_value::view value )
{
  switch ( value.type() )
  {
  case bsoncxx::type::k_int32:
    field = static_cast<double>( value.get_int32().value );
    break;
  case bsoncxx::type::k_int64:
    field = static_cast<double>( value.get_int64().value );
    break;
  default:
    field = value.get_double().value;
    break;
  }
}

template <>
inline void spt::util::set( bsoncxx::oid& field, bsoncxx::types::bson_value::view value ) { field = value.get_oid().value; }

template <>
inline void spt::util::set( std::string& field, bsoncxx::types::bson_value::view value ) { field = value.get_string().value; }

template <>
inline void spt::util::set( std::chrono::time_point<std::chrono::system_clock>& field, bsoncxx::types::bson_value::view value )
{
  if ( bsoncxx::type::k_int32 == value.type() ) field = std::chrono::time_point<std::chrono::system_clock>{ std::chrono::microseconds{ value.get_int32() } };
  else if ( bsoncxx::type::k_int64 == value.type() ) field = std::chrono::time_point<std::chrono::system_clock>{ std::chrono::microseconds{ value.get_int64() } };
  else field = std::chrono::time_point<std::chrono::system_clock>{ value.get_date().value };
}

template <>
inline void spt::util::set( std::chrono::milliseconds& field, bsoncxx::types::bson_value::view value )
{
  if ( bsoncxx::type::k_int32 == value.type() ) field = std::chrono::milliseconds{ value.get_int32() };
  else if ( bsoncxx::type::k_int64 == value.type() ) field = std::chrono::milliseconds{ value.get_int64() };
  else field = value.get_date().value;
}

template <>
inline void spt::util::set( std::chrono::microseconds& field, bsoncxx::types::bson_value::view value )
{
  if ( bsoncxx::type::k_int32 == value.type() ) field = std::chrono::microseconds{ value.get_int32() };
  else if ( bsoncxx::type::k_int64 == value.type() ) field = std::chrono::microseconds{ value.get_int64() };
  else field = std::chrono::duration_cast<std::chrono::microseconds>( value.get_date().value );
}

template <>
inline void spt::util::set( std::chrono::nanoseconds& field, bsoncxx::types::bson_value::view value )
{
  if ( bsoncxx::type::k_int32 == value.type() ) field = std::chrono::microseconds{ value.get_int32() };
  else if ( bsoncxx::type::k_int64 == value.type() ) field = std::chrono::microseconds{ value.get_int64() };
  else field = std::chrono::duration_cast<std::chrono::nanoseconds>( value.get_date().value );
}

template <>
inline void spt::util::set( std::set<bool>& field, bsoncxx::types::bson_value::view value )
{
  for ( const auto& item: value.get_array().value ) field.insert( item.get_bool().value );
}

template <>
inline void spt::util::set( std::vector<bool>& field, bsoncxx::types::bson_value::view value )
{
  field.reserve( 8 );
  for ( const auto& item: value.get_array().value ) field.emplace_back( item.get_bool().value );
}

template <>
inline void spt::util::set( std::set<int32_t>& field, bsoncxx::types::bson_value::view value )
{
  for ( const auto& item: value.get_array().value ) field.insert( item.get_int32().value );
}

template <>
inline void spt::util::set( std::vector<int32_t>& field, bsoncxx::types::bson_value::view value )
{
  field.reserve( 8 );
  for ( const auto& item: value.get_array().value ) field.emplace_back( item.get_int32().value );
}

template <>
inline void spt::util::set( std::set<int64_t>& field, bsoncxx::types::bson_value::view value )
{
  for ( const auto& item: value.get_array().value ) field.insert( item.get_int64().value );
}

template <>
inline void spt::util::set( std::vector<int64_t>& field, bsoncxx::types::bson_value::view value )
{
  field.reserve( 8 );
  for ( const auto& item: value.get_array().value ) field.emplace_back( item.get_int64().value );
}

template <>
inline void spt::util::set( std::set<double>& field, bsoncxx::types::bson_value::view value )
{
  for ( const auto& item: value.get_array().value ) field.insert( item.get_double().value );
}

template <>
inline void spt::util::set( std::vector<double>& field, bsoncxx::types::bson_value::view value )
{
  field.reserve( 8 );
  for ( const auto& item: value.get_array().value ) field.emplace_back( item.get_double().value );
}

template <>
inline void spt::util::set( std::set<std::string>& field, bsoncxx::types::bson_value::view value )
{
  for ( const auto& item: value.get_array().value ) field.emplace( item.get_string().value );
}

template <>
inline void spt::util::set( std::vector<std::string>& field, bsoncxx::types::bson_value::view value )
{
  field.reserve( 8 );
  for ( const auto& item: value.get_array().value ) field.emplace_back( item.get_string().value );
}

template <>
inline void spt::util::set( std::set<bsoncxx::oid>& field, bsoncxx::types::bson_value::view value )
{
  for ( const auto& item: value.get_array().value ) field.insert( item.get_oid().value );
}

template <>
inline void spt::util::set( std::vector<bsoncxx::oid>& field, bsoncxx::types::bson_value::view value )
{
  field.reserve( 8 );
  for ( const auto& item: value.get_array().value ) field.emplace_back( item.get_oid().value );
}

template <>
inline void spt::util::set( std::set<std::chrono::time_point<std::chrono::system_clock>>& field, bsoncxx::types::bson_value::view value )
{
  for ( const auto& item: value.get_array().value ) field.emplace( item.get_date().value );
}

template <>
inline void spt::util::set( std::vector<std::chrono::time_point<std::chrono::system_clock>>& field, bsoncxx::types::bson_value::view value )
{
  field.reserve( 8 );
  for ( const auto& item: value.get_array().value ) field.emplace_back( item.get_date().value );
}

template <typename M>
inline void spt::util::set( std::optional<M>& field, bsoncxx::types::bson_value::view value )
{
  if ( value.type() == bsoncxx::type::k_null ) return;
  auto m = M{};
  set( m, value );
  field = std::move( m );
}

template <typename M>
inline void spt::util::set( std::shared_ptr<M>& field, bsoncxx::types::bson_value::view value )
{
  if ( value.type() == bsoncxx::type::k_null ) return;
  auto m = std::make_shared<M>();
  set( *m.get(), value );
  field = m;
}

template <typename Model>
inline void spt::util::set( std::set<Model>& field, bsoncxx::types::bson_value::view value )
{
  if ( value.type() != bsoncxx::type::k_array ) return;
  for ( const auto& item : value.get_array().value )
  {
    auto m = Model{};
    set( m, bsoncxx::types::bson_value::value{ item.get_document().value } );
    field.insert( std::move( m ) );
  }
}

template <typename Model>
inline void spt::util::set( std::vector<Model>& field, bsoncxx::types::bson_value::view value )
{
  if ( value.type() != bsoncxx::type::k_array ) return;
  field.reserve( 8 );
  for ( const auto& item : value.get_array().value )
  {
    auto m = Model{};
    set( m, bsoncxx::types::bson_value::value{ item.get_document().value } );
    field.push_back( std::move( m ) );
  }
}
