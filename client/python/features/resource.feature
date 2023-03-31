Feature: Resource
  Test illustrating use of the client as a auto-closing resource.

  Scenario: Auto closing resource
    Given Mongo service running on localhost on port 2000
    Then Client can be used as an auto-closing resource