#include <libssh2_ruby.h>

/*
 * Deallocates the memory associated with the channel data
 * structure. This also properly lowers the reference count
 * on the session structure.
 * */
static void
sftp_dealloc(LibSSH2_Ruby_SftpSession *session_data) {
    if (session_data->sftp_session != NULL) {
        BLOCK(libssh2_sftp_shutdown(session_data->sftp_session));
    }

    if (session_data->ssh_session != NULL) {
        libssh2_ruby_session_release(session_data->ssh_session);
    }

    free(session_data);
}

/*
 * Called to allocate the memory associated with channels. We allocate
 * some space to store pointers to libssh2 structs in here.
 * */
static VALUE
sftp_allocate(VALUE self) {
    LibSSH2_Ruby_SftpSession *sftp_session = malloc(sizeof(LibSSH2_Ruby_SftpSession));
    sftp_session->sftp_session = NULL;
    sftp_session->ssh_session = NULL;

    return Data_Wrap_Struct(self, 0, sftp_dealloc, sftp_session);
}

/*
 * call-seq:
 *     SftpSession.new(session)
 *
 * Creates a new channel for the given session. This will open
 * a channel on the session so the session must be ready for that
 * or an exception will be raised.
 *
 * */
static VALUE
sftp_initialize(VALUE self, VALUE rb_session) {
    LIBSSH2_SESSION *ssh_session;
    LibSSH2_Ruby_SftpSession *sftp_session;

    // Verify we have a valid session object
    CHECK_SESSION(rb_session);

    // Get the internal data from the instance.
    Data_Get_Struct(self, LibSSH2_Ruby_SftpSession, sftp_session);

    // Read the interal data from the session
    Data_Get_Struct(rb_session, LibSSH2_Ruby_Session, sftp_session->ssh_session);
    ssh_session = sftp_session->ssh_session->session;

    // Create the channel, which we always do in a blocking
    // fashion since there is no other opportunity.
    do {
        sftp_session->sftp_session = libssh2_sftp_init(ssh_session);

        // If we don't have an sftp session and don't have a EAGAIN
        // error, then we raise an exception.
        if (sftp_session->sftp_session == NULL) {
            int error = libssh2_session_last_error(ssh_session, NULL, NULL, 0);
            if (error != LIBSSH2_ERROR_EAGAIN) {
                rb_exc_raise(libssh2_ruby_wrap_error(error));
                return Qnil;
            }
        }
    } while(sftp_session->sftp_session == NULL);

    // Increase the refcount of the session data for us now that
    // we have a channel.
    libssh2_ruby_session_retain(sftp_session->ssh_session);

    return self;
}

void init_libssh2_sftp_session() {
    VALUE cSftpSession = rb_cLibSSH2_Native_SftpSession;
    rb_define_alloc_func(cSftpSession, sftp_allocate);
    rb_define_method(cSftpSession, "initialize", sftp_initialize, 1);
}
