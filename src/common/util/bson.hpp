//
// Created by Rakesh on 08/01/2020.
//

#pragma once

#include <optional>
#include <ostream>

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <bsoncxx/array/value.hpp>
#include <bsoncxx/array/view.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/oid.hpp>

namespace spt::util
{
  /**
   * Extract the value of a property from the BSON document.  The property must exist in the document.
   *
   * @tparam DataType The type of data to extract from the BSON document.
   * @param key The name of the property to extract from the BSON document.
   * @param view The BSON document from which the value is to be extracted.
   * @return The value of specified type.
  */
 template<typename DataType>
  DataType bsonValue( std::string_view key, const bsoncxx::document::view& view );

  /**
   * Extract the value of a property from the BSON document if the property exists.
   *
   * @tparam DataType The type of data to extract from the BSON document.
   * @param key The name of the property to extract from the BSON document.
   * @param view The BSON document from which the value is to be extracted.
   * @return The value of specified type if the property exists.
   */
  template<typename DataType>
  std::optional<DataType> bsonValueIfExists( std::string_view key, const bsoncxx::document::view& view );

  /**
   * Gwnerate a string representation of the value of the property in a BSON document.
   *
   * @param key The field/property within the BSON document to extract.
   * @param view The BSON document from which the property is to be extracted.
   * @return The string representation of the property
   */
  std::string toString( std::string_view key, const bsoncxx::document::view& view );

  /**
   * Generate a JSON representation of a BSON array.
   *
   * @param view The BSON array to convert to JSON.
   * @return The JSON representation of the BSON array.
   */
  boost::json::array toJson( const bsoncxx::array::view& view );

  /**
   * Write a JSON representation of a BSON array to an output stream.
   *
   * @param os The output stream to write the JSON representation to.
   * @param view The BSON array to be written as JSON.
   * @return The output stream for chaining.
   */
  std::ostream& toJson( std::ostream& os, const bsoncxx::array::view& view );

  /**
   * Generate a JSON representation of a BSON document.
   *
   * @param view The BSON document to convert to JSON.
   * @return The JSON representation of the BSON document.
   */
  boost::json::object toJson( const bsoncxx::document::view& view );

  /**
   * Write a JSON representation of a BSON document to an output stream.
   *
   * @param os The output stream to write the JSON representation to.
   * @param view The BSON document to be written as JSON.
   * @return The output stream for chaining.
   */
  std::ostream& toJson( std::ostream& os, const bsoncxx::document::view& view );

  /**
   * Generate a BSON array representation of a JSON array.
   *
   * @param array The JSON array to convert to BSON.
   * @return The BSON array representation of the JSON array.
   */
  bsoncxx::array::value toBson( const boost::json::array& array );

  /**
   * Generate a BSON representation of a JSON object.
   *
   * @param object The JSON object to convert to BSON.
   * @return The BSON document representation of the JSON object.
   */
  bsoncxx::document::value toBson( const boost::json::object& object );

  /**
   * Convert the BSON array into its equivalent JSON.
   *
   * @param array The BSON array to convert to JSON
   * @return The JSON representation of the array
   */
  boost::json::array fromBson( bsoncxx::array::view array );

  /**
   * Convert the BSON document into its equivalent JSON.
   *
   * @param object The BSON document to convert to JSON.
   * @return The JSON representation of the BSON document.
   */
  boost::json::object fromBson( bsoncxx::document::view object );

  /**
   * Parse the string representation of a BSON object id to an instance.
   *
   * @param id The BSON object id string representation to parse
   * @return The parsed object id if successful.
   */
  std::optional<bsoncxx::oid> parseId( std::string_view id );

  /**
   * Generate a BSON object id at the specified timestamp.
   *
   * @param timestamp The timestamp to use as the first 4 bytes in the generated object id.
   * @param id The id from which the last 8 bytes of the generated object id are set.
   * @return The generated object id.
   */
  bsoncxx::oid generateId( std::chrono::system_clock::time_point timestamp, bsoncxx::oid id = {} );
}
