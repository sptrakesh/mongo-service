//
// Created by Rakesh on 19/12/2024.
//

#pragma once

#include "../../options/find.hpp"
#include "action.hpp"

namespace spt::mongoservice::api::model::request
{
  struct Pipeline
  {
    struct Document
    {
      struct Stage
      {
        Stage( std::string_view command, bsoncxx::types::bson_value::value&& value ) :
          command{ command }, value{ std::forward<bsoncxx::types::bson_value::value>( value ) } {}
        Stage( std::string_view command, bsoncxx::document::value&& value ) :
          command{ command }, value{ std::forward<bsoncxx::document::value>( value ) } {}
        Stage( std::string_view command, bsoncxx::document::view value ) :
          command{ command }, value{ value } {}
        Stage() = default;
        ~Stage() = default;
        Stage(Stage&&) = default;
        Stage& operator=(Stage&&) = default;

        Stage(const Stage&) = delete;
        Stage& operator=(const Stage&) = delete;

        BEGIN_VISITABLES(Stage);
        std::string command;
        std::optional<bsoncxx::types::bson_value::value> value;
        END_VISITABLES;
      };

      Document() = default;
      ~Document() = default;
      Document(Document&&) = default;
      Document& operator=(Document&&) = default;

      Document(const Document&) = delete;
      Document& operator=(const Document&) = delete;

      BEGIN_VISITABLES(Document);
      VISITABLE(std::vector<Stage>, specification);
      END_VISITABLES;
    };

    Pipeline() = default;
    ~Pipeline() = default;
    Pipeline(Pipeline&&) = default;
    Pipeline& operator=(Pipeline&&) = default;

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

    void addStage( std::string_view command, bsoncxx::types::bson_value::value&& value )
    {
      document.specification.emplace_back( command, std::forward<bsoncxx::types::bson_value::value>( value ) );
    }

    void addStage( std::string_view command, bsoncxx::document::value&& value )
    {
      document.specification.emplace_back( command, std::forward<bsoncxx::document::value>( value ) );
    }

    template <util::Visitable Model>
    void addStage( std::string_view command, const Model& model )
    {
      document.specification.emplace_back( command, util::bson( model ) );
    }

    BEGIN_VISITABLES(Pipeline);
    VISITABLE(Document, document);
    VISITABLE(std::optional<options::Find>, options);
    VISITABLE(std::string, database);
    VISITABLE(std::string, collection);
    VISITABLE(std::string, application);
    VISITABLE(std::string, correlationId);
    VISITABLE_DIRECT_INIT(Action, action, {Action::pipeline});
    VISITABLE_DIRECT_INIT(bool, skipMetric, {false});
    END_VISITABLES;
  };

  void populate( const Pipeline::Document::Stage& model, bsoncxx::builder::stream::document& builder );
  void populate( const Pipeline::Document::Stage& model, boost::json::object& object );
}
