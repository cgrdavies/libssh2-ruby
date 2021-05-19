require 'pry'
require File.expand_path("../setup", __FILE__)

describe "writing a file via SFTP" do
  include_context "acceptance"

  it "should execute and block a single command" do
    session.auth_by_password(acceptance_config["user"],
                             acceptance_config["password"])

    
                             binding.pry
    puts 'stuff'
  end
end
