use super::bindings;
use std::ffi::CStr;

/// Identifies the application that owns a window.
#[derive(Debug, Clone)]
pub struct ApplicationInfo {
    /// The name of the application.
    pub name: String,

    /// The internal id of the application
    pub internal: u64,
}

impl ApplicationInfo {
    #[doc(hidden)]
    pub unsafe fn from_c(value: &bindings::miracle_application_info_t) -> Self {
        let name = if value.application_name.is_null() {
            String::new()
        } else {
            unsafe {
                CStr::from_ptr(value.application_name)
                    .to_string_lossy()
                    .into_owned()
            }
        };
        Self {
            name,
            internal: value.internal,
        }
    }
}
