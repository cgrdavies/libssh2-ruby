#include <libssh2_ruby.h>

VALUE libssh2_ruby_wrap_error(int error) {
    VALUE instance = rb_class_new_instance(0, 0, rb_eLibSSH2_Native_Error);
    rb_iv_set(instance, "@error_code", INT2NUM(error));
    return instance;
}

void init_libssh2_error() {
    rb_define_attr(rb_eLibSSH2_Native_Error, "error_code", 1, 0);
}
