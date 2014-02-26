Given /^the root path is "(.*?)"$/ do |url|
        Capybara.app = 'blah'
        Capybara.configure do |config|
                config.run_server = false
                config.default_driver = :mechanize
                config.app_host = url
        end
end

Given /^the web server is running$/ do
        `ps aux | grep httpd/http[d]`.strip.should_not be_empty
end

When /^I am on the index page$/ do
        visit '/index.html'
end

When /^I visit "(.*?)"$/ do |url|
        visit url
end

Then /^I should not see "(.*?)"$/ do |text|
        page.should have_no_content text
end

Then /^I should see "(.*?)"$/ do |text|
        page.should have_content text
end

Then /^I (should|should not) see (the current year|last year)$/ do |should, year|
        expected_year = Time.now.year
        expected_year -= 1 if year == 'last year'
        if should == 'should'
                page.should have_content expected_year
        else
                page.should_not have_content expected_year
        end
end

When /^I search for "(.*?)"$/ do |keyword|
        fill_in 'gbqfq', :with => keyword
        click_button 'gbqfba'
end

Then /^show me the page$/ do
        save_and_open_page
end

