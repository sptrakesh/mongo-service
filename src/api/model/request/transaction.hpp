//
// Created by Rakesh on 22/12/2024.
//

#pragma once

#include "create.hpp"
#include "update.hpp"
#include "delete.hpp"

namespace spt::mongoservice::api::model::request
{
  struct TransactionBuilder
  {
    TransactionBuilder( std::string_view database, std::string_view collection ) : database{ database }, collection{ collection } {}
    ~TransactionBuilder() = default;

    TransactionBuilder( const TransactionBuilder& ) = delete;
    TransactionBuilder& operator=( const TransactionBuilder& ) = delete;
    TransactionBuilder( TransactionBuilder&& ) = delete;
    TransactionBuilder& operator=( TransactionBuilder&& ) = delete;

    template <util::Visitable Document, util::Visitable Metadata>
    requires std::is_same_v<decltype(Document::id), bsoncxx::oid>
    void addCreate( const Create<Document, Metadata>& request )
    {
      items << util::marshall( request );
    }

    template <util::Visitable Document, util::Visitable Metadata>
    requires std::is_same_v<decltype(Document::id), bsoncxx::oid>
    void addUpdate( const MergeForId<Document, Metadata>& request )
    {
      items << util::marshall( request );
    }

    template <util::Visitable Document, util::Visitable Metadata, util::Visitable Filter, template<typename, typename, typename> typename Model>
    requires std::is_same_v<Model<Document, Metadata, Filter>, Replace<Document, Metadata, Filter>> ||
      std::is_same_v<Model<Document, Metadata, Filter>, Update<Document, Metadata, Filter>>
    void addUpdate( const Model<Document, Metadata, Filter>& request )
    {
      items << util::marshall( request );
    }

    template <util::Visitable Document, util::Visitable Metadata>
    requires std::constructible_from<Metadata, bsoncxx::document::view>
    void addRemove( const Delete<Document, Metadata>& request )
    {
      items << util::marshall( request );
    }

    [[nodiscard]] bsoncxx::document::value build();

  private:
    std::string database;
    std::string collection;
    bsoncxx::builder::stream::array items;
  };
}
