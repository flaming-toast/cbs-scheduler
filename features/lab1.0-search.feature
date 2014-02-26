Feature: Basic browsing functions with Capybara

        Capybara is configured to be integrated with
        Cucumber and uses mechanize to perform remote
        accesses that will allow us testing arbitrary
        webb applications.

        Background:
                Given the web server is running
                Given the root path is "localhost:8088"

        Scenario: Check connection
                When I am on "index.html"
                Then I should see "THIS IS INDEX"

                #        Scenario: Perform a simple search
                #                Given I am on the root page
                #                And I search for "cs194-24"
                #                Then I should see "inst.eecs.berkeley.edu/~cs194-24/sp13/"
                #
                #        Scenario: Ensure a string is not present
                #                Given I am on the root page
                #                And I search for "cats"
                #                Then I should not see "berkeley"
