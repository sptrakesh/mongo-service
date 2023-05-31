//
// Created by Rakesh on 25/05/2023.
//

#pragma once

#include "../../src/common/util/bson.h"
#include "../../src/common/util/json.h"
#include "../../src/common/util/serialise.h"
#include "../../src/common/visit_struct/visit_struct_intrusive.hpp"

using std::operator""s;
using std::operator""sv;

namespace spt::util::test::serial
{
  struct NotVisitable
  {
    NotVisitable() = default;
    ~NotVisitable() = default;

    inline void set( bsoncxx::document::view bson )
    {
      FROM_BSON( std::string, identifier, bson )
      FROM_BSON( int64_t, integer, bson )
    }

    inline void set( simdjson::ondemand::object& obj )
    {
      FROM_JSON( identifier, obj )
      FROM_JSON( integer, obj )
    }

    std::string identifier;
    int64_t integer;
  };

  inline bsoncxx::types::bson_value::value bson( const NotVisitable& model )
  {
    return {
        bsoncxx::builder::stream::document{} <<
            "identifier" << model.identifier <<
            "integer" << model.integer <<
            bsoncxx::builder::stream::finalize
    };
  }

  inline boost::json::value json( const NotVisitable& model )
  {
    return boost::json::object{ { "identifier"sv, model.identifier }, { "integer"sv, model.integer } };
  }

  inline void set( NotVisitable& field, bsoncxx::types::bson_value::view value )
  {
    field.set( value.get_document().value );
  }

  inline void set( const char*, NotVisitable& field, simdjson::ondemand::value& value )
  {
    auto obj = value.get_object().value();
    field.set( obj );
  }

  struct CustomFields
  {
    inline void set( bsoncxx::document::view bson )
    {
      FROM_BSON_OPT( std::string, "identifier"sv, id, bson )
      FROM_BSON_OPT( bsoncxx::oid, "reference"sv, ref, bson )
    }

    inline void set( simdjson::ondemand::object& obj )
    {
      FROM_JSON_NAME( "identifier", id, obj )
      FROM_JSON_NAME( "reference", ref, obj )
    }

    BEGIN_VISITABLES(CustomFields);
    VISITABLE(std::string, id);
    VISITABLE(bsoncxx::oid, ref);
    END_VISITABLES;
  };

  inline bsoncxx::types::bson_value::value bson( const CustomFields& model )
  {
    return {
        bsoncxx::builder::stream::document{} <<
            "identifier" << model.id <<
            "reference" << model.ref <<
            bsoncxx::builder::stream::finalize
    };
  }

  inline boost::json::value json( const CustomFields& model )
  {
    return boost::json::object{ { "identifier"sv, model.id }, { "reference"sv, model.ref.to_string() } };
  }

  inline void set( CustomFields& field, bsoncxx::types::bson_value::view value )
  {
    field.set( value.get_document().value );
  }

  inline void set( const char*, CustomFields& field, simdjson::ondemand::value& value )
  {
    auto obj = value.get_object().value();
    field.set( obj );
  }

  struct Full
  {
    struct Nested
    {
      BEGIN_VISITABLES(Nested);
      VISITABLE(std::string, identifier);
      VISITABLE(int32_t, integer);
      VISITABLE(double, number);
      VISITABLE(std::chrono::time_point<std::chrono::system_clock>, date);
      VISITABLE(std::vector<double>, numbers);
      END_VISITABLES;
    };

    BEGIN_VISITABLES(Full);
    VISITABLE(NotVisitable, notVisitable);
    VISITABLE(CustomFields, customFields);
    VISITABLE(std::string, identifier);
    VISITABLE(std::optional<Nested>, nested);
    VISITABLE(std::vector<Nested>, nesteds);
    VISITABLE_DIRECT_INIT(std::shared_ptr<Nested>, nestedp, { nullptr });
    VISITABLE(std::vector<std::string>, strings);
    VISITABLE(std::optional<std::string>, ostring);
    VISITABLE(std::optional<bool>, obool);
    VISITABLE(std::chrono::time_point<std::chrono::system_clock>, time);
    VISITABLE(bsoncxx::oid, id);
    VISITABLE(bool, boolean);
    END_VISITABLES;
  };

  struct Partial
  {
    BEGIN_VISITABLES(Partial);
    VISITABLE(NotVisitable, notVisitable);
    VISITABLE(CustomFields, customFields);
    VISITABLE(std::string, identifier);
    std::string hidden;
    VISITABLE(bsoncxx::oid, id);
    END_VISITABLES;

    inline void populate( bsoncxx::document::view view )
    {
      FROM_BSON( std::string, hidden, view )
    }

    inline void populate( simdjson::ondemand::object& object )
    {
      FROM_JSON( hidden, object )
    }
  };

  inline void populate( const Partial& model, bsoncxx::builder::stream::document& doc )
  {
    if ( !model.hidden.empty() ) doc << "hidden" << model.hidden;
  }

  inline void populate( Partial& model, bsoncxx::document::view view )
  {
    model.populate( view );
  }

  inline void populate( const Partial& model, boost::json::object& object )
  {
    if ( !model.hidden.empty() ) object.emplace( "hidden"sv, model.hidden );
  }

  inline void populate( Partial& model, simdjson::ondemand::object& object )
  {
    model.populate( object );
  }
}
