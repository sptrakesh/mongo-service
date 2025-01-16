//
// Created by Rakesh on 22/12/2024.
//

#include "transaction.hpp"
#include "impl/settings.hpp"

using spt::mongoservice::api::model::request::TransactionBuilder;

bsoncxx::document::value TransactionBuilder::build()
{
  using bsoncxx::builder::stream::document;
  using bsoncxx::builder::stream::open_document;
  using bsoncxx::builder::stream::close_document;
  using bsoncxx::builder::stream::finalize;

  return document{} <<
    "database" << database <<
    "collection" << collection <<
    "application" << impl::ApiSettings::instance().application <<
    "action" << util::bson( Action::transaction ) <<
    "document" <<
      open_document <<
        "items" << (items << finalize) <<
      close_document <<
    finalize;
}

