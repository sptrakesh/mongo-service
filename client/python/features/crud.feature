Feature: CRUD
  CRUD test suite.  Create, read, update and delete documents using mongo service.

  Scenario: CRUD test suite
    Given A mongo service client
    Then Create a document
    And Retrieve count of documents
    And Retrieve the document by id
    And Retrieve the document by property
    And Retrieve the version history document by id
    And Retrieve the version history document by entity id
    And Update the document by id
    And Update the document without version history
    And Delete the document
    And Retrieve count of documents after delete