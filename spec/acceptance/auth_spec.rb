require File.expand_path("../setup", __FILE__)

describe "authentication" do
  include_context "acceptance"

  it "should not be authenticated by default" do
    expect(session).not_to be_authenticated
  end

  describe "by password" do
    it "should authenticate" do
      session.auth_by_password(acceptance_config["user"],
                             acceptance_config["password"])

      expect(session).to be_authenticated
    end

    it "should be able to fail" do
      expect {
        session.auth_by_password(acceptance_config["user"] + "_wrong", "wrong")
      }.to raise_error(LibSSH2::Native::Error::ERROR_PUBLICKEY_UNRECOGNIZED)

      expect(session).not_to be_authenticated
    end
  end

  describe "by keypair" do
    it "should authenticate" do
      session.auth_by_publickey_fromfile(acceptance_config["user"],
                                         acceptance_config["public_key_path"],
                                         acceptance_config["private_key_path"])

      expect(session).to be_authenticated
    end

    let(:fake_public_key) { 'spec/support/fixtures/fake_key.pub' }
    let(:fake_private_key) { 'spec/support/fixtures/fake_key' }

    it "should be able to fail" do
      expect {
        session.auth_by_publickey_fromfile(acceptance_config['user'],
        fake_public_key,
        fake_private_key)
      }.to raise_error(LibSSH2::Native::Error::ERROR_PUBLICKEY_UNVERIFIED)      
    end
  end
end
