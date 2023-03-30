Feature: Index
  Index test suite.  Create and drop indices.

  Scenario: Index test suite
    Given A mongo service client for indexing
    Then Create an index
    And Create the index again
    And Drop the index
    And Create a unique index
    And Create a unique index again
    And Drop the unique index
