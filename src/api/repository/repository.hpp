//
// Created by Rakesh on 19/12/2024.
//

#pragma once

#include "../api.hpp"
#include "../model.hpp"
#include "../impl/settings.hpp"

#if defined __has_include
  #if __has_include("../../common/util/json.hpp")
    #include "../../common/util/json.hpp"
    #include "../../common/util/serialise.hpp"
  #else
    #include <mongo-service/common/util/json.hpp>
    #include <mongo-service/common/util/serialize.hpp>
  #endif
  #if __has_include("../../log/NanoLog.hpp")
    #include "../../log/NanoLog.hpp"
  #else
    #include <log/NanoLog.hpp>
  #endif
#endif

#include <concepts>
#include <expected>

namespace spt::mongoservice::api::repository
{
  template <typename T>
  concept Model = requires( T t )
  {
    std::is_default_constructible<T>{};
    visit_struct::traits::is_visitable<T>{};
    std::is_same<decltype(T::id), bsoncxx::oid>{};
  };

  /**
   * Simple structure used to communicate service interaction errors to caller.
   */
  struct Error
  {
    enum class Cause : uint_fast8_t { pool, command, empty, data, exception, invalid = 255 };

    Error( std::string_view message, Cause cause ) : message{ message }, cause{ cause } {}
    ~Error() = default;
    Error( Error&& ) = default;
    Error& operator=( Error&& ) = default;

    Error( const Error& ) = default;
    Error& operator=( const Error& ) = delete;

    std::string message;
    Cause cause{ Cause::invalid };
  };

  /**
   * Create a new document.  Can create a regular document or a `timeseries` document.
   * @tparam Document A constrained type for the input document.
   * @tparam Metadata Optional metadata type to associate with the version history document created as part of this action.
   *   Not relevant for `timeseries` documents which do not create version history.
   * @tparam CreateModel The constrained type for the input model.
   * @param request The data that is to be sent to the service to create the document.
   * @return Model representing the result of the `create` action, or an error.
   */
  template <Model Document, util::Visitable... Metadata, template <typename...> typename CreateModel>
  requires std::is_same_v<CreateModel<Document, Metadata...>, model::request::Create<Document, Metadata...>> ||
    std::is_same_v<CreateModel<Document, Metadata...>, model::request::CreateWithReference<Document, Metadata...>> ||
    std::is_same_v<CreateModel<Document>, model::request::CreateTimeseries<Document>>
  std::expected<model::response::Create, Error> create( CreateModel<Document, Metadata...>& request )
  {
    using O = std::expected<model::response::Create, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted creating document. " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while creating document. " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while creating document. " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while creating document. " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error creating document " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error creating document. " << util::json::str( request );
      return O{ std::unexpect, "Unknown error creating document."sv, Error::Cause::exception };
    }
  }

  /**
   * Merge the input data into an existing document.
   * @tparam Document The constrained type for the document that is being updated.
   * @tparam Metadata The metadata to associate with the version history document that will be created as part of this action.
   * @param request The request data that is transmitted to the service.
   * @return The merged document as a result of applying the update, or an error.
   */
  template <Model Document, util::Visitable Metadata, template <typename...> typename MergeModel>
  requires std::is_same_v<MergeModel<Document, Metadata>, model::request::MergeForId<Document, Metadata>> ||
    std::is_same_v<MergeModel<Document, Metadata>, model::request::MergeForIdWithReference<Document, Metadata>>
  std::expected<model::response::Update<Document>, Error> update( MergeModel<Document, Metadata>& request )
  {
    using O = std::expected<model::response::Update<Document>, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted updating document. " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while updating document. " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while updating document. " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while updating document. " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error updating document " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error updating document. " << util::json::str( request );
      return O{ std::unexpect, "Unknown error updating document."sv, Error::Cause::exception };
    }
  }

  /**
   * Update or replace an existing document.
   * @tparam Document The constrained type for the document being updated.
   * @tparam Metadata The metadata to associate with the version history document that will be created as part of this action.
   * @tparam Filter The filter to use to `uniquely` identify the document to be updated.
   * @tparam UpdateModel The constrained model for replacing or updating a document.
   * @param request The update model and filter.
   * @return A updated model or an error.
   */
  template <Model Document, util::Visitable Metadata, util::Visitable Filter, template<typename, typename, typename> typename UpdateModel>
  requires std::is_same_v<UpdateModel<Document, Metadata, Filter>, model::request::Replace<Document, Metadata, Filter>> ||
    std::is_same_v<UpdateModel<Document, Metadata, Filter>, model::request::ReplaceWithReference<Document, Metadata, Filter>> ||
    std::is_same_v<UpdateModel<Document, Metadata, Filter>, model::request::Update<Document, Metadata, Filter>> ||
    std::is_same_v<UpdateModel<Document, Metadata, Filter>, model::request::UpdateWithReference<Document, Metadata, Filter>>
  std::expected<model::response::Update<Document>, Error> update( UpdateModel<Document, Metadata, Filter>& request )
  {
    using O = std::expected<model::response::Update<Document>, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted updating document. " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while updating document. " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while updating document. " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while updating document. " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error updating document " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error updating document. " << util::json::str( request );
      return O{ std::unexpect, "Unknown error updating document."sv, Error::Cause::exception };
    }
  }

  /**
   * Apply the same change to multiple documents.  Note that this operation can take a long time to complete on
   * large collections.  This can cause client socket timeouts.  Use appropriate filters to restrict the set of
   * documents being modified in each invocation of this function.
   * @tparam Document The constrained type for the documents being updated.
   * @tparam Metadata The metadata to associate with the version history document that will be created as part of this action.
   * @tparam Filter The filter to use to identify the documents to be updated.
   * @param request The update model and filter to use.
   * @return The result of the update action or error.
   */
  template <Model Document, util::Visitable Metadata, util::Visitable Filter, template <typename...> typename UpdateModel>
  requires std::is_same_v<UpdateModel<Document, Metadata, Filter>, model::request::Update<Document, Metadata, Filter>> ||
    std::is_same_v<UpdateModel<Document, Metadata, Filter>, model::request::UpdateWithReference<Document, Metadata, Filter>>
  std::expected<model::response::UpdateMany, Error> updateMany( UpdateModel<Document, Metadata, Filter>& request )
  {
    using O = std::expected<model::response::UpdateMany, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted updating documents. " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while updating documents. " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while updating documents. " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while updating documents. " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error updating documents " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error updating documents. " << util::json::str( request );
      return O{ std::unexpect, "Unknown error updating documents."sv, Error::Cause::exception };
    }
  }

  /**
   * Count the number of documents matching a filter.  Note that this operation can take a long time on large
   * collections.  Use the appropriate options to restrict the total documents that will be counted as well
   * as the maximum time the database should spend on the count operation.
   * @tparam Document The filter representing the subset of documents to count.
   * @param request The request comprising the filter and options.
   * @return The result of the count operation or an error.
   */
  template <util::Visitable Document>
  std::expected<model::response::Count, Error> count( model::request::Count<Document>& request )
  {
    using O = std::expected<model::response::Count, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted counting documents for " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while counting documents for " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while counting documents for " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while counting documents for " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error counting documents " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error counting documents for " << util::json::str( request );
      return O{ std::unexpect, "Unknown error counting documents."sv, Error::Cause::exception };
    }
  }

  /**
   * Retrieve documents matching a filter.  Note that the filter alone may not be sufficient to limit the
   * number of matching documents.  Use appropriate options to restrict the number of documents returned.
   * @tparam Document The constrained type of documents being retrieved.
   * @tparam Filter The filter to use to identify the matching documents to retrieve.
   * @param request The model representing the filter and options.
   * @return The matching document(s) or an error.
   */
  template <Model Document, util::Visitable Filter>
  std::expected<model::response::Retrieve<Document>, Error> retrieve( model::request::Retrieve<Filter>& request )
  {
    using O = std::expected<model::response::Retrieve<Document>, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted retrieving documents for " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while retrieving documents for " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while retrieving documents for " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while retrieving documents " << *err <<
          ". " << util::json::str( request.document ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error retrieving documents " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error retrieving documents for " << util::json::str( request );
      return O{ std::unexpect, "Unknown error retrieving documents."sv, Error::Cause::exception };
    }
  }

  /**
   * Retrieve documents using an aggregation pipeline.  As with other retrieval operations, ensure
   * proper limits are specified to limit the number of matching documents.
   * @tparam Document The constrained type of documents being retrieved using aggregation pipeline
   * @param request The request with the pipeline specification.
   * @return The resulting documents or an error
   */
  template <Model Document>
  std::expected<model::response::Retrieve<Document>, Error> pipeline( model::request::Pipeline& request )
  {
    using O = std::expected<model::response::Retrieve<Document>, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted executing pipeline for " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while executing pipeline for " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while executing pipeline for " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while executing pipeline for " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error executing pipeline " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error executing pipeline for " << util::json::str( request );
      return O{ std::unexpect, "Unknown error executing pipeline."sv, Error::Cause::exception };
    }
  }

  /**
   * Retrieve distinct values for a field.
   * @tparam Filter The filter used to constrain the matching documents as appropriate.
   * @param request The request with filter and options.
   * @return The distinct values or an error.
   */
  template <util::Visitable Filter>
  std::expected<model::response::Distinct, Error> distinct( model::request::Distinct<Filter>& request )
  {
    using O = std::expected<model::response::Distinct, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted retrieving distinct values for " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while retrieving distinct values for " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while retrieving distinct values for " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while retrieving distinct values for " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error retrieving distinct values " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error retrieving distinct values for " << util::json::str( request );
      return O{ std::unexpect, "Unknown error retrieving distinct values."sv, Error::Cause::exception };
    }
  }

  /**
   * Insert or delete documents in bulk.  Note that the operations are executed sequentially and are not `transactional`.
   * @tparam Document The constrained type of documents being created in bulk.
   * @tparam Metadata The metadata to associate with the version history document that will be created as part of this action.
   * @tparam Delete The type representing the filter or model used to delete documents.
   * @param request The request with the create and/or delete specifications.
   * @return The results of executing the bulk operations or an error.
   */
  template <Model Document, util::Visitable Metadata, util::Visitable Delete>
  requires std::constructible_from<Delete, bsoncxx::document::view>
  std::expected<model::response::Bulk, Error> bulk( model::request::Bulk<Document, Metadata, Delete>& request )
  {
    using O = std::expected<model::response::Bulk, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted executing bulk statements " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while executing bulk statements " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while executing bulk statements " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while executing bulk statements " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error executing bulk statements " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error executing bulk statements " << util::json::str( request );
      return O{ std::unexpect, "Unknown error executing bulk statements."sv, Error::Cause::exception };
    }
  }

  /**
   * Delete document(s).
   * @tparam Document Type that represents the documents that are to be deleted.
   * @tparam Metadata The metadata to associate with the version history document that will be created as part of this action.
   * @param request The request with the specification for the documents to be deleted.
   * @return The results of the delete operation or an error.
   */
  template <util::Visitable Document, util::Visitable Metadata>
  requires std::constructible_from<Metadata, bsoncxx::document::view>
  std::expected<model::response::Delete, Error> remove( model::request::Delete<Document, Metadata>& request )
  {
    using O = std::expected<model::response::Delete, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted while deleting documents " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while deleting documents " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while deleting documents " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while deleting documents " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error deleting documents " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error deleting documents " << util::json::str( request );
      return O{ std::unexpect, "Unknown error deleting documents."sv, Error::Cause::exception };
    }
  }

  /**
   * Ensure an index matching the specification.
   * @tparam Document Type that represents the index to be created.
   * @param request The request with the index specification and options.
   * @return The result of ensuring the index or an error.
   */
  template <util::Visitable Document>
  std::expected<model::response::Index, Error> index( model::request::Index<Document>& request )
  {
    using O = std::expected<model::response::Index, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted ensuring index " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while ensuring index " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while ensuring index " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while ensuring index " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error ensuring index " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error ensuring index for " << util::json::str( request );
      return O{ std::unexpect, "Unknown error ensuring index."sv, Error::Cause::exception };
    }
  }

  /**
   * Remove an index.
   * @tparam Document The type representing the index specification.
   * @param request The request with the index specification and options.
   * @return The result of dropping the index or an error.
   */
  template <util::Visitable Document>
  std::expected<model::response::DropIndex, Error> dropIndex( model::request::DropIndex<Document>& request )
  {
    using O = std::expected<model::response::DropIndex, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted dropping index " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while dropping index " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while dropping index " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while dropping index " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error dropping index " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error dropping index for " << util::json::str( request );
      return O{ std::unexpect, "Unknown error dropping index."sv, Error::Cause::exception };
    }
  }

  /**
   * Create or rename a collection.  Note tha renaming a collection can be a very heavyweight
   * operation, depending upon the size of the existing collection. The rename operation enques the
   * updates to the version history documents to avoid hanging up the client.
   * @tparam CollectionModel Constrained type representing the collection to be created or renamed.
   * @param request The request with the collection name and options.
   * @return The result of creating or renaming the collection, or an error.
   */
  template <typename CollectionModel>
  requires std::is_same_v<CollectionModel, model::request::CreateCollection> ||
    std::is_same_v<CollectionModel, model::request::RenameCollection>
  std::expected<model::response::CreateCollection, Error> collection( CollectionModel& request )
  {
    using O = std::expected<model::response::CreateCollection, Error>;
    using std::operator ""sv;

    try
    {
      request.application = impl::ApiSettings::instance().application;
      auto bson = util::marshall( request );
      const auto [type, opt] = execute( bson );
      if ( type == ResultType::poolFailure )
      {
        LOG_WARN << "Connection pool exhausted creating or renaming collection " << util::json::str( request );
        return O{ std::unexpect, "Connection pool exhausted"sv, Error::Cause::pool };
      }

      if ( type == ResultType::commandFailure )
      {
        LOG_WARN << "Command returned no data while creating or renaming collection " << util::json::str( request );
        return O{ std::unexpect, "Command returned no response"sv, Error::Cause::command };
      }

      if ( !opt )
      {
        LOG_WARN << "API returned no data while creating or renaming collection " << util::json::str( request );
        return O{ std::unexpect, "API returned no response"sv, Error::Cause::empty };
      }

      if ( auto err = util::bsonValueIfExists<std::string>( "error", opt->view() ); err )
      {
        LOG_WARN << "API returned error while creating or renaming collection " << *err <<
          ". " << util::json::str( request ) <<
          ". " << boost::json::serialize( util::toJson( opt->view() ) );
        return O{ std::unexpect, *err, Error::Cause::data };
      }

      return O{ std::in_place, opt->view() };
    }
    catch( const std::exception& ex )
    {
      LOG_WARN << "Error creating or renaming collection " << ex.what() << ". " << util::json::str( request );
      return O{ std::unexpect, ex.what(), Error::Cause::exception };
    }
    catch ( ... )
    {
      LOG_WARN << "Unknown error creating or renaming collection for " << util::json::str( request );
      return O{ std::unexpect, "Unknown error creating or renaming collection."sv, Error::Cause::exception };
    }
  }

  /**
   * Send the transaction specification to the service for execution.  See `model::request::TransactionBuilder`
   * for a utility to compose the transaction specification.
   * @param request The transaction request that is to be executed.
   * @return The results of executing the transaction, or an error.
   */
  std::expected<model::response::Transaction, Error> transaction( bsoncxx::document::value request );

  /**
   * Drop the specified collection.  Associated version history documents are deleted asynchronously if specified.
   * @param request The request with the specification and options for dropping the collection.
   * @return The result of dropping the collection or an error.
   */
  std::expected<model::response::DropCollection, Error> dropCollection( model::request::DropCollection& request );
}
