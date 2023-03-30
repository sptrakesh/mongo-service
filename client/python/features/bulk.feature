Feature: Bulk operations
  Bulk operation test suite.  Create and delete documents in bulk.

  Scenario: Bulk operation test suite
    Given Connection for bulk operations
    Then Create documents in bulk
    And Retrieve count of documents after bulk insert
    And Delete documents in bulk
    And Retrieve count of documents after bulk delete
    And Create a large batch of documents
    And Delete a large batch of documents
