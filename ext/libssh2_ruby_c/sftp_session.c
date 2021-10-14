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

 /*
 * Helpers to return the LIBSSH2_SFTP for the given instance.
 * */
static inline LIBSSH2_SFTP *
get_sftp(VALUE self) {
    LibSSH2_Ruby_SftpSession *session_data;
    Data_Get_Struct(self, LibSSH2_Ruby_SftpSession, session_data);
    return session_data->sftp_session;
}

static inline LIBSSH2_SESSION *
get_session(VALUE self) {
    LibSSH2_Ruby_SftpSession *session_data;
    Data_Get_Struct(self, LibSSH2_Ruby_SftpSession, session_data);
    return session_data->ssh_session -> session;
}

// static VALUE
// open_file(VALUE self, VALUE path) {
//     LIBSSH2_SFTP_HANDLE *sftp_handle;

//     rb_check_type(path, T_STRING);

//     libssh2_session_set_blocking(get_session(self), 1);

//     sftp_handle = libssh2_sftp_open(get_sftp(self), RSTRING_PTR(path), LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC, LIBSSH2_SFTP_S_IRWXU);

//     if(!sftp_handle) {
//         int error = libssh2_session_last_error(get_session(self), NULL, NULL, 0);
//         if (error != NULL) {
//             rb_exc_raise(libssh2_ruby_wrap_error(error));
//             return Qnil;
//         }        
            
//         return Qnil;
//     }

//     return INT2NUM(sftp_handle);
// }

static VALUE
upload_file(VALUE self, VALUE local_path, VALUE remote_path) {
    LIBSSH2_SFTP_HANDLE *sftp_handle;
    FILE *local_file;
    char mem[1024*100];
    size_t nread;
    char *ptr;
    int rc;

    rb_check_type(remote_path, T_STRING);
    rb_check_type(local_path, T_STRING);

    libssh2_session_set_blocking(get_session(self), 1);

    sftp_handle = libssh2_sftp_open(get_sftp(self), 
                                    RSTRING_PTR(remote_path), 
                                    LIBSSH2_FXF_WRITE|LIBSSH2_FXF_CREAT|LIBSSH2_FXF_TRUNC, LIBSSH2_SFTP_S_IRWXU);

    if(!sftp_handle) {
        int error = libssh2_session_last_error(get_session(self), NULL, NULL, 0);
        if (error) {
            rb_exc_raise(libssh2_ruby_wrap_error(error));
        }                    
    }

    local_file = fopen(RSTRING_PTR(local_path), "rb");

    do {
        nread = fread(mem, 1, sizeof(mem), local_file);
        if(nread <= 0) {
            /* end of file */ 
            break;
        }
        ptr = mem;

        do {
            /* write data in a loop until we block */ 
            rc = libssh2_sftp_write(sftp_handle, ptr, nread);

            if(rc < 0)
                break;
            ptr += rc;
            nread -= rc;
        } while(nread);


    } while(rc > 0);

    return Qnil;
}

void init_libssh2_sftp_session() {
    VALUE cSftpSession = rb_cLibSSH2_Native_SftpSession;
    rb_define_alloc_func(cSftpSession, sftp_allocate);
    rb_define_method(cSftpSession, "initialize", sftp_initialize, 1);
    rb_define_method(cSftpSession, "upload_file", upload_file, 2);

}
