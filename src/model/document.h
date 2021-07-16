//
// Created by Rakesh on 19/07/2020.
//

#pragma once

#include <optional>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/streambuf.hpp>
#include <bsoncxx/document/view.hpp>

namespace spt::model
{
  struct Document
  {
    explicit Document( const boost::asio::streambuf& buffer, std::size_t length );
    explicit Document( const uint8_t* buffer, std::size_t length );
    explicit Document( bsoncxx::document::view view );
    ~Document() = default;
    Document(Document&&) = default;
    Document& operator=(Document&&) = default;

    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;

    [[nodiscard]] bool valid() const;
    [[nodiscard]] std::optional<bsoncxx::document::view> bson() const;

    [[nodiscard]] std::string action() const;
    [[nodiscard]] std::string database() const;
    [[nodiscard]] std::string collection() const;
    [[nodiscard]] bsoncxx::document::view document() const;
    [[nodiscard]] std::optional<bsoncxx::document::view> options() const;
    [[nodiscard]] std::optional<bsoncxx::document::view> metadata() const;
    [[nodiscard]] std::optional<std::string>application() const;
    [[nodiscard]] std::optional<std::string>correlationId() const;
    [[nodiscard]] std::optional<bool>skipVersion() const;

    [[nodiscard]] std::string json() const;

  private:
    std::optional<bsoncxx::document::view> view;
  };

  boost::asio::awaitable<Document> parseDocument( const uint8_t* buffer, std::size_t length );
}
