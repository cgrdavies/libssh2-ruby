require 'pry'
require File.expand_path("../setup", __FILE__)

describe "writing a file via SFTP" do
  include_context "acceptance"

  let(:local_path) { File.open('spec/support/fixtures/test.txt').path }
  let(:remote_path) { '/root/temp.txt' }

  it "should execute and block a single command" do
    session.auth_by_password(acceptance_config["user"],
                             acceptance_config["password"])

    sftp = LibSSH2::Native::SftpSession.new(session.native_session)
    sftp.upload_file(local_path, remote_path)
  end
end
