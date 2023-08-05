Feature: Rename
  Rename collection test suite.  Create document in collection, rename collection, drop collection.

  Scenario: Rename collection test suite
    Given A mongo service client
    Then Document created in a new collection
    And Cannot rename to existing collection
    And Rename collection
    And Retrieve document from renamed collection
    And Retrieve version history for renamed collection
    And Drop renamed collection
    And Revision history cleared for dropped collection
