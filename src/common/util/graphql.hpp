//
// Created by Rakesh on 08/04/2026.
//

#pragma once

#include "date.hpp"
#include "concept.hpp"
#if defined __has_include
#if __has_include("../../log/NanoLog.hpp")
#include "../../log/NanoLog.hpp"
#else
#include <log/NanoLog.hpp>
#endif

#if __has_include("../magic_enum/magic_enum.hpp")
#include "../magic_enum/magic_enum.hpp"
#else
#include <magic_enum/magic_enum.hpp>
#endif
#endif

#include <set>
#include <vector>

#include <boost/uuid/uuid.hpp>
#include <bsoncxx/oid.hpp>

namespace spt::util::graphql
{
  /**
   * Add non-visitable fields in the model to the response.  A callback function that library users can implement to
   * fully serialise partially visitable models.
   * @tparam M The partially visitable model type.
   * @param model The model instance to be fully serialised to GraphQL.
   * @param response The string response to append values to.
   */
  template <Visitable M>
    requires NotEnumeration<M>
  void populateFields( const M& model, std::string& response );

  /**
   * This is usually invoked from the {@xrefitem query(const M&)} function.  Can also be used if you wish a wrapped
   * GraphQL query representation of the model.
   * @tparam M The visitable struct.
   * @param model Instance of the visitable struct to convert to a GraphQL query representation.
   * @param response The string response to append values to.
   * @param filter Object containing fields to be omitted from the output query.
   * @return The input string response with the fields appended.
   */
  template <Visitable M>
    requires NotEnumeration<M>
  std::string& fields( const M& model, std::string& response, const boost::json::object& filter );

  /**
   * Serialise the specified enum type into a GraphQL query.  Does nothing since queries do not include values.
   * @tparam E The scoped enum type
   * @param response The string response to append values to.
   * @return The input string.
   */
  template <typename E>
    requires std::is_enum_v<E>
  std::string& fields( const E&, std::string& response, const boost::json::object& ) { return response; }

  /**
   * General implementation for converting a reference-wrapped model into a GraphQL query representation.
   * @tparam M The visitable model wrapped in a reference wrapper.
   * @param model The reference wrapper instance to serialise.
   * @param response The string response to append values to.
   * @param filter Object containing fields to be omitted from the output query.
   * @return The input string.
   */
  template <Visitable M>
    requires NotEnumeration<M>
  std::string& fields( const std::reference_wrapper<M>& model, std::string& response, const boost::json::object& filter );

  /**
   * General implementation for converting a reference wrapped `const` model into a GraphQL query representation.
   * @tparam M The visitable model wrapped in a reference wrapper.
   * @param model The reference wrapper instance to serialise.
   * @param response The string response to append values to.
   * @param filter Object containing fields to be omitted from the output query.
   * @return The input string.
   */
  template <Visitable M>
    requires NotEnumeration<M>
  std::string& fields( const std::reference_wrapper<const M>& model, std::string& response, const boost::json::object& filter );

  /**
   * Serialise the specified optional enum type into a GraphQL query representation. No action
   * needed since values are not serialised.
   * @tparam E The scoped enum type
   * @param response The string response to append values to.
   * @return The input string.
   */
  template <typename E>
    requires std::is_enum_v<E>
  std::string& fields( const std::optional<E>&, std::string& response, const boost::json::object& ) { return response; }

  /**
   * General implementation for serialising an optional type.  Delegates to the appropriate {@xrefitem fields(const M&)}
   * function with the optional value or a default constructed instance.
   * @tparam M The type wrapped in the optional.
   * @param opt The optional instance to be serialised.
   * @param response The string response to append values to.
   * @param filter Object containing fields to be omitted from the output query.
   * @return The input string.
   */
  template <typename M>
    requires NotEnumeration<M>
  std::string& fields( const std::optional<M>& opt, std::string& response, const boost::json::object& filter );

  /**
   * General implementation for converting a set into a GraphQL query representation.  Lists all the
   * fields of the contained type.
   * @tparam Model The type stored in the set.
   * @param items The set to serialise into a GraphQL representation
   * @param response The string response to append values to.
   * @param filter Object containing fields to be omitted from the output query.
   * @return The input string.
   */
  template<typename Model>
    requires NotEnumeration<Model>
  std::string& fields( const std::set<Model>& items, std::string& response, const boost::json::object& filter );

  /**
   * General implementation for converting a set of scoped enums to GraphQL query representation.  No
   * action is needed since values are not serialised into the query.
   * @tparam E The scoped enumeration type to serialise.
   * @param response The string response to append values to.
   * @return The input string.
   */
  template<typename E>
    requires std::is_enum_v<E>
  std::string& fields( const std::set<E>&, std::string& response, const boost::json::object& ) { return response; }

  /**
   * General implementation for converting a vector into a GraphQL query representation.  Prints
   * the fields of the type contained in the vector.
   * @tparam Model The type stored in the vector.
   * @param vec The vector to serialise into a GraphQL query.
   * @param response The string response to append values to.
   * @param filter Object containing fields to be omitted from the output query.
   * @return The input string.
   */
  template <typename Model>
    requires NotEnumeration<Model>
  std::string& fields( const std::vector<Model>& vec, std::string& response, const boost::json::object& filter );

  /**
   * General implementation for converting a vector of scoped enums to a GraphQL query representation.
   * No action is needed since values are not serialised into the query.
   * @tparam E The scoped enumeration type to serialise.
   * @param response The string response to append values to.
   * @return The input string.
   */
  template<typename E>
    requires std::is_enum_v<E>
  std::string& fields( const std::vector<E>&, std::string& response, const boost::json::object& ) { return response; }

  /**
   * General implementation for serialising a shared pointer type.  Delegates to the appropriate {@xrefitem fields(const M&)}
   * function for the type of pointee.
   * @tparam Model The type wrapped in the shared pointer.
   * @param model The shared pointer instance to be serialised.
   * @param response The string response to append values to.
   * @param filter Object containing fields to be omitted from the output query.
   * @return The input string.
   */
  template <typename Model>
    requires NotEnumeration<Model>
  std::string& fields( const std::shared_ptr<Model>& model, std::string& response, const boost::json::object& filter );

  /**
   * General implementation for serialising a unique pointer type.  Delegates to the appropriate {@fields bson(const M&)}
   * function for the type of pointee.
   * @tparam Model The type wrapped in the unique pointer.
   * @param model The unique pointer instance to be serialised.
   * @param response The string response to append values to.
   * @param filter Object containing fields to be omitted from the output query.
   * @return The input string.
   */
  template <typename Model>
    requires NotEnumeration<Model>
  std::string& fields( const std::unique_ptr<Model>& model, std::string& response, const boost::json::object& filter );

  /**
   * General function for marshalling a class/struct to a GraphQL query representation.  Implementation
   * usually only required for *non-visitable* classes/structures.  Implement this function in your choice of
   * namespace for classes/structures that cannot be automatically serialised.
   * @tparam Model The type of class/structure.
   * @param model The instance to convert to a BSON value.
   * @param response The string response to append values to.
   * @param filter Object containing fields to be omitted from the output query.
   * @return The input string.
   */
  template <typename Model>
    requires NotEnumeration<Model>
  std::string& fields( const Model&, std::string& response, const boost::json::object& filter );

  /**
   * Concept that represents a serialisable entity.  An entity is serialisable if it is default constructable,
   * visitable and serialisable to string via the {@xrefitem fields(const M&)} function.
   * @tparam T The type of the model.
   */
  template <typename T>
  concept Model = requires( T t, std::string& response, const boost::json::object& filter )
  {
    std::is_default_constructible<T>{};
    visit_struct::traits::is_visitable<T>{};
    !std::is_enum<T>{};
    !std::is_arithmetic<T>{};
    { fields( t, response, filter ) } -> std::convertible_to<std::string>;
  };

  /**
   * Generate a string suitable for retrieving all fields of the model from a GraphQL service.  The output
   * is designed to be embedded in the full GraphQL query.
   * @tparam M The type of the model.
   * @param model The visitable and serialisable model.
   * @param filter Optional object containing fields to be omitted from the query.  When specifying the
   *   filter, use `null` values for fields you wish to omit.  For nested objects, create a nested filter
   *   object with the same name as the field in the model.  If you wish to omit an entire nested object,
   *   specify `null` for the field as for other fields.  Similar for arrays of objects.  Example:
   *   ```
   *   {
   *     "field1": null,
   *     "nested": {
   *       "field2": null
   *     },
   *     "nested2": null,
   *     "array1": null,
   *     "array2": {
   *       "field3": null
   *     },
   *   }
   *   ```
   * @param size The initial capacity of the response string.
   * @return A string containing the GraphQL query for the model.
   */
  template <Model M>
  std::string query( const M& model, const boost::json::object& filter = {}, std::size_t size = 128 )
  {
    auto response = std::string{};
    response.reserve( size );
    fields( model, response, filter );
    return response;
  }

  /**
   * Serialise non-visitable fields in the specified model into the output GraphQL mutation.  This
   * function is used as a `callback` to allow partially visitable structs to append additional
   * information as appropriate.
   * @tparam M The partially visitable model
   * @param model The instance that is to be fully unmarshalled from BSON.
   * @param view The string containing the GraphQL mutation.
   */
  template <Visitable M>
    requires NotEnumeration<M>
  void populate( const M& model, std::string& view );

  /**
   * General purpose function for populating the fields of a type into the GraphQL mutation query.
   * @tparam M The type to serialise into the GraphQL mutation.
   * @param model The model to serialise into the GraphQL mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template <Visitable M>
    requires NotEnumeration<M>
  std::string& gql( const M& model, std::string& response );

  /**
   * General purpose function for populating a GraphQL mutation query with the name of an enumeration.
   * @tparam E The type to serialise into the GraphQL mutation.
   * @param value The enum value to serialise into the GraphQL mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template <typename E>
    requires std::is_enum_v<E>
  std::string& gql( const E& value, std::string& response )
  {
    return response.append( magic_enum::enum_name( value ) );
  }

  /**
   * General purpose function for populating the fields of a reference-wrapped type into the GraphQL mutation query.
   * @tparam M The type to serialise into the GraphQL mutation.
   * @param model The reference wrapped model to serialise into the GraphQL mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template <Visitable M>
    requires NotEnumeration<M>
  std::string& gql( const std::reference_wrapper<M>& model, std::string& response );

  /**
   * General purpose function for populating the fields of a constant reference-wrapped type into the GraphQL mutation query.
   * @tparam M The type to serialise into the GraphQL mutation.
   * @param model The reference wrapped model to serialise into the GraphQL mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template <Visitable M>
    requires NotEnumeration<M>
  std::string& gql( const std::reference_wrapper<const M>& model, std::string& response );

  /**
   * General purpose function for populating an optional scoped enum into the GraphQL mutation query.
   * @tparam E The scoped enum type to serialise into the GraphQL mutation.
   * @param model The optional scoped enum to serialise into the GraphQL mutation.  If `nullopt` adds `null` to the mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template <typename E>
    requires std::is_enum_v<E>
  std::string& gql( const std::optional<E>& model, std::string& response )
  {
    return response.append( model ? magic_enum::enum_name( *model ) : "null" );
  }

  /**
   * General purpose function for populating the fields of an optional type into the GraphQL mutation query.
   * @tparam M The type to serialise into the GraphQL mutation.
   * @param model The optional model to serialise into the GraphQL mutation.  If `nullopt` adds `null` to the mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template <typename M>
    requires NotEnumeration<M>
  std::string& gql( const std::optional<M>& opt, std::string& response );

  /**
   * General purpose function for serialising a set of items into the GraphQL mutation query.
   * @tparam Model The type to serialise into the GraphQL mutation.
   * @param items The set of items to serialise into the GraphQL mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template<typename Model>
    requires NotEnumeration<Model>
  std::string& gql( const std::set<Model>& items, std::string& response );

  /**
   * General purpose function for serialising a set of scoped enums into the GraphQL mutation query.
   * @tparam E The scoped enum type to serialise into the GraphQL mutation.
   * @param items The set of enums to serialise into the GraphQL mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template<typename E>
    requires std::is_enum_v<E>
  std::string& gql( const std::set<E>& items, std::string& response );

  /**
   * General purpose function for serialising a vector of items into the GraphQL mutation query.
   * @tparam Model The type to serialise into the GraphQL mutation.
   * @param items The vector of items to serialise into the GraphQL mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template <typename Model>
    requires NotEnumeration<Model>
  std::string& gql( const std::vector<Model>& items, std::string& response );

  /**
   * General purpose function for serialising a vector of scoped enums into the GraphQL mutation query.
   * @tparam E The scoped enum type to serialise into the GraphQL mutation.
   * @param items The vector of enums to serialise into the GraphQL mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template<typename E>
    requires std::is_enum_v<E>
  std::string& gql( const std::vector<E>& items, std::string& response );

  /**
   * General purpose function for serialising a shared pointer into a GraphQL mutation.
   * @tparam Model The type to serialise into the GraphQL mutation.
   * @param model The pointer to serialise into the GraphQL mutation.  If not set, will output `null`.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template <typename Model>
    requires NotEnumeration<Model>
  std::string& gql( const std::shared_ptr<Model>& model, std::string& response );

  /**
   * General purpose function for serialising a unique pointer into a GraphQL mutation.
   * @tparam Model The type to serialise into the GraphQL mutation.
   * @param model The pointer to serialise into the GraphQL mutation.  If not set, will output `null`.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template <typename Model>
    requires NotEnumeration<Model>
  std::string& gql( const std::unique_ptr<Model>& model, std::string& response );

  /**
   * Template function for serialising standard types into a GraphQL mutation.
   * @tparam Model The type to serialise into the GraphQL mutation.
   * @param model The value to serialise into the GraphQL mutation.
   * @param response The string containing the GraphQL mutation.
   * @return The input string.
   */
  template <typename Model>
    requires NotEnumeration<Model>
  std::string& gql( const Model& model, std::string& response );

  /**
   * Serialise the specified model into a GraphQL structure representation for a `mutation` operation.
   * @tparam M The type of the model.
   * @param model The visitable and serialisable model.
   * @param size The initial capacity of the response string.
   * @return The string representation of the model in GraphQL format.
   */
  template <Model M>
  std::string mutation( const M& model, std::size_t size = 512 )
  {
    auto response = std::string{};
    response.reserve( size );
    gql( model, response );
    return response;
  }
}

template <>
inline std::string& spt::util::graphql::fields( const bool&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const int8_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const uint8_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const int16_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const uint16_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const int32_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const uint32_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const int64_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const uint64_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const float16_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const float32_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const float64_t&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const bsoncxx::oid&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const std::string&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const DateTime&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const DateTimeMs&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const DateTimeNs&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const std::chrono::seconds&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const std::chrono::milliseconds&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const std::chrono::microseconds&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const std::chrono::nanoseconds&, std::string& response, const boost::json::object& ) { return response; }

template <>
inline std::string& spt::util::graphql::fields( const boost::uuids::uuid&, std::string& response, const boost::json::object& ) { return response; }

template<spt::util::Visitable Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::fields( const std::reference_wrapper<Model>& model, std::string& response, const boost::json::object& filter )
{
  return fields( model.get(), response, filter );
}

template<spt::util::Visitable Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::fields( const std::reference_wrapper<const Model>& model, std::string& response, const boost::json::object& filter )
{
  return fields( model.get(), response, filter );
}

template <typename M>
  requires spt::util::NotEnumeration<M>
std::string& spt::util::graphql::fields( const std::optional<M>& opt, std::string& response, const boost::json::object& filter )
{
  return fields( opt.value_or( M{} ), response, filter );
}

template<typename Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::fields( const std::set<Model>& items, std::string& response, const boost::json::object& filter )
{
  return fields( items.empty() ? Model{} : *items.begin(), response, filter );
}

template <typename Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::fields( const std::vector<Model>& vec, std::string& response, const boost::json::object& filter )
{
  return fields( vec.empty() ? Model{} : vec.front(), response, filter );
}

template <typename Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::fields( const std::shared_ptr<Model>& model, std::string& response, const boost::json::object& filter )
{
  return fields( model ? *model.get() : Model{}, response, filter );
}

template <typename Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::fields( const std::unique_ptr<Model>& model, std::string& response, const boost::json::object& filter )
{
  return fields( model ? *model.get() : Model{}, response, filter );
}

template <spt::util::Visitable M>
  requires spt::util::NotEnumeration<M>
std::string& spt::util::graphql::fields( const M& model, std::string& response, const boost::json::object& filter )
{
  response.append( "{\n" );
  visit_struct::for_each( model,
      [&response, &filter]( const char* name, const auto& value )
      {
        if ( const auto iter = filter.find( name ); iter == filter.end() )
        {
          response.append( name ).append( " " );
          fields( value, response, filter );
        }
        else
        {
          if ( iter->value().is_object() )
          {
            response.append( name ).append( " " );
            fields( value, response, iter->value().as_object() );
          }
          else
          {
            /* Omit field as it is included in the filter */
          }
        }
      } );

  if constexpr ( visit_struct::traits::ext::is_fully_visitable<M>() == false ) populateFields( model, response );
  response.append( "\n}\n" );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const bool& model, std::string& response )
{
  response.append( model ? "true" : "false" );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const int8_t& model, std::string& response )
{
  response.append( std::to_string( model ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const uint8_t& model, std::string& response )
{
  response.append( std::to_string( model ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const int16_t& model, std::string& response )
{
  response.append( std::to_string( model ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const uint16_t& model, std::string& response )
{
  response.append( std::to_string( model ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const int32_t& model, std::string& response )
{
  response.append( std::to_string( model ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const uint32_t& model, std::string& response )
{
  response.append( std::to_string( model ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const int64_t& model, std::string& response )
{
  response.append( std::to_string( model ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const uint64_t& model, std::string& response )
{
  response.append( std::to_string( model ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const float32_t& model, std::string& response )
{
  response.append( std::to_string( model ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const float64_t& model, std::string& response )
{
  response.append( std::to_string( model ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const bsoncxx::oid& model, std::string& response )
{
  response.append( "\"" ).append( model.to_string() ).append( "\"" );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const std::string& model, std::string& response )
{
  response.append( "\"" ).append( model ).append( "\"" );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const DateTime& model, std::string& response )
{
  response.append( "\"" ).append( isoDateMicros( model ) ).append( "\"" );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const DateTimeMs& model, std::string& response )
{
  response.append( "\"" ).append( isoDateMicros( model ) ).append( "\"" );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const DateTimeNs& model, std::string& response )
{
  response.append( "\"" ).append( isoDateMicros( model ) ).append( "\"" );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const std::chrono::seconds& model, std::string& response )
{
  response.append( std::to_string( model.count() ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const std::chrono::milliseconds& model, std::string& response )
{
  response.append( std::to_string( model.count() ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const std::chrono::microseconds& model, std::string& response )
{
  response.append( std::to_string( model.count() ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const std::chrono::nanoseconds& model, std::string& response )
{
  response.append( std::to_string( model.count() ) );
  return response;
}

template <>
inline std::string& spt::util::graphql::gql( const boost::uuids::uuid& model, std::string& response )
{
  response.append( "\"" ).append( boost::uuids::to_string( model ) ).append( "\"" );
  return response;
}

template<spt::util::Visitable Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::gql( const std::reference_wrapper<Model>& model, std::string& response )
{
  return gql( model.get(), response );
}

template<spt::util::Visitable Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::gql( const std::reference_wrapper<const Model>& model, std::string& response )
{
  return gql( model.get(), response );
}

template <typename M>
  requires spt::util::NotEnumeration<M>
std::string& spt::util::graphql::gql( const std::optional<M>& opt, std::string& response )
{
  return opt ? gql( opt.value(), response ) : response.append( "null" );
}

template<typename Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::gql( const std::set<Model>& items, std::string& response )
{
  if ( items.empty() ) return response.append( "[] " );
  response.append( "[\n" );

  for ( const auto& item : items )
  {
    response.append( " " );
    gql( item, response );
    response.append( "\n" );
  }

  return response.append( "]\n" );
}

template<typename E>
  requires std::is_enum_v<E>
std::string& spt::util::graphql::gql( const std::set<E>& items, std::string& response )
{
  if ( items.empty() ) return response.append( "[] " );
  response.append( "[\n" );

  for ( const auto& item : items )
  {
    response.append( " " ).append( magic_enum::enum_name( item ) ).append( "\n");
  }

  return response.append( "]\n" );
}

template <typename Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::gql( const std::vector<Model>& items, std::string& response )
{
  if ( items.empty() ) return response.append( "[] " );
  response.append( "[\n" );

  for ( const auto& item : items )
  {
    response.append( " " );
    gql( item, response );
    response.append( "\n" );
  }

  return response.append( "]\n" );
}

template<typename E>
  requires std::is_enum_v<E>
std::string& spt::util::graphql::gql( const std::vector<E>& items, std::string& response )
{
  if ( items.empty() ) return response.append( "[] " );
  response.append( "[\n" );

  for ( const auto& item : items )
  {
    response.append( " " ).append( magic_enum::enum_name( item ) ).append( "\n" );
  }

  return response.append( "]\n" );
}

template <typename Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::gql( const std::shared_ptr<Model>& model, std::string& response )
{
  return model ? gql( *model.get(), response ) : response.append( "null" );
}

template <typename Model>
  requires spt::util::NotEnumeration<Model>
std::string& spt::util::graphql::gql( const std::unique_ptr<Model>& model, std::string& response )
{
  return model ? gql( *model.get(), response ) : response.append( "null" );
}

template <spt::util::Visitable M>
  requires spt::util::NotEnumeration<M>
std::string& spt::util::graphql::gql( const M& model, std::string& response )
{
  response.append( "{\n" );
  visit_struct::for_each( model,
      [&response]( const char* name, const auto& value )
      {
        response.append( name ).append( ": " );
        gql( value, response );
        response.append( "\n" );
      } );

  if constexpr ( visit_struct::traits::ext::is_fully_visitable<M>() == false ) populate( model, response );
  return response.append( "}\n" );
}
