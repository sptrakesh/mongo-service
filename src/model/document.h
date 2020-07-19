//
// Created by Rakesh on 19/07/2020.
//

#pragma once

#include <optional>

#include <boost/asio/streambuf.hpp>
#include <bsoncxx/document/view.hpp>

namespace spt::model
{
  struct Document
  {
    explicit Document( const boost::asio::streambuf& buffer, std::size_t length );
    ~Document() = default;
    Document(Document&&) = default;
    Document& operator=(Document&&) = default;

    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;

    [[nodiscard]] std::optional<bsoncxx::document::view> bson() const;

  private:
    std::optional<bsoncxx::document::view> view;
  };
}
