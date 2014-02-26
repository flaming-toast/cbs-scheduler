Feature: Basic browsing functions with Capybara

        Capybara is configured to be integrated with
        Cucumber and uses mechanize to perform remote
        accesses that will allow us testing arbitrary
        web applications.

        Background:
                Given the web server is running
                Given the root path is "http://127.0.0.1:8088"

        Scenario: Check connection
                When I am on the index page
                Then I should see "THIS IS INDEX"

        Scenario: Ensure a string is not present
                Given I am on the index page
                And I visit "/index.html"
                Then I should not see "THIS IS NOT INDEX"

        Scenario: Test non-index page
                When I visit "/not_index.html"
                Then I should see "THIS IS NOT INDEX"
                And I should not see "THIS IS INDEX"

        Scenario: Test cgi script for current year
                When I visit "/year.html"
                Then I should see the current year
                Then I should not see last year

        Scenario: Test cgi script for last year
                When I visit "/last_year.html"
                Then I should see last year
                Then I should not see the current year
