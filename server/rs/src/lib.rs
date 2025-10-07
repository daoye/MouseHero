mod c;

use self::c::*;
// use copypasta::{ClipboardContext, ClipboardProvider};
use enigo::{Enigo, Key, Keyboard, Mouse, Settings};
use std::ffi::{c_char, CStr, CString};

use arboard::Clipboard;

use std::cell::RefCell;
use std::sync::Once;
use log::{debug, info, error};

static INIT_LOGGER: Once = Once::new();

fn init_logger() {
    INIT_LOGGER.call_once(|| {
        let _ = env_logger::Builder::from_env(
            env_logger::Env::default().default_filter_or("enigo=error,rs=error")
        )
        .format_timestamp_millis()
        .try_init();
    });
}

/// Error codes returned by the library functions.
/// These values match the C enum definition.
#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq)]
pub enum YAError {
    /// Operation completed successfully
    Success = 0,
    /// Platform-specific error occurred
    PlatformError = -1,
    /// Clipboard operation failed
    ClipboardError = -2,
    /// Invalid input provided
    InvalidInput = -3,
}

#[no_mangle]
pub extern "C" fn key_unicode_action(ch: u32, direction: CDirection) -> YAError {
    // Try to convert to a valid char scalar
    match char::from_u32(ch) {
        Some(c) => match with_enigo_mut(|en| en.key(Key::Unicode(c), direction.cast()).map_err(|_| ())) {
            Ok(_) => YAError::Success,
            Err(_) => YAError::PlatformError,
        },
        None => YAError::InvalidInput,
    }
}

#[no_mangle]
pub extern "C" fn key_ascii_action(ch: u32, direction: CDirection) -> YAError {
    // Only handle printable ASCII 0x20..0x7E here
    if ch < 0x20 || ch > 0x7E { return YAError::InvalidInput; }

    #[cfg(target_os = "windows")]
    {
        use enigo::Key::*;
        let ya = with_enigo_mut(|en| {
            let res = match ch as u8 as char {
                'A' | 'a' => en.key(A, direction.cast()),
                'B' | 'b' => en.key(B, direction.cast()),
                'C' | 'c' => en.key(C, direction.cast()),
                'D' | 'd' => en.key(D, direction.cast()),
                'E' | 'e' => en.key(E, direction.cast()),
                'F' | 'f' => en.key(F, direction.cast()),
                'G' | 'g' => en.key(G, direction.cast()),
                'H' | 'h' => en.key(H, direction.cast()),
                'I' | 'i' => en.key(I, direction.cast()),
                'J' | 'j' => en.key(J, direction.cast()),
                'K' | 'k' => en.key(K, direction.cast()),
                'L' | 'l' => en.key(L, direction.cast()),
                'M' | 'm' => en.key(M, direction.cast()),
                'N' | 'n' => en.key(N, direction.cast()),
                'O' | 'o' => en.key(O, direction.cast()),
                'P' | 'p' => en.key(P, direction.cast()),
                'Q' | 'q' => en.key(Q, direction.cast()),
                'R' | 'r' => en.key(R, direction.cast()),
                'S' | 's' => en.key(S, direction.cast()),
                'T' | 't' => en.key(T, direction.cast()),
                'U' | 'u' => en.key(U, direction.cast()),
                'V' | 'v' => en.key(V, direction.cast()),
                'W' | 'w' => en.key(W, direction.cast()),
                'X' | 'x' => en.key(X, direction.cast()),
                'Y' | 'y' => en.key(Y, direction.cast()),
                'Z' | 'z' => en.key(Z, direction.cast()),
                '0' => en.key(Num0, direction.cast()),
                '1' => en.key(Num1, direction.cast()),
                '2' => en.key(Num2, direction.cast()),
                '3' => en.key(Num3, direction.cast()),
                '4' => en.key(Num4, direction.cast()),
                '5' => en.key(Num5, direction.cast()),
                '6' => en.key(Num6, direction.cast()),
                '7' => en.key(Num7, direction.cast()),
                '8' => en.key(Num8, direction.cast()),
                '9' => en.key(Num9, direction.cast()),
                ' ' => en.key(Space, direction.cast()),
                _ => return Err(()),
            };
            res.map_err(|_| ())
        });

        match ya {
            Ok(_) => YAError::Success,
            Err(_) => YAError::InvalidInput, // signal caller to fallback to Unicode
        }
    }

    #[cfg(not(target_os = "windows"))]
    {
        // macOS/Linux: prefer Unicode injection for printable ASCII
        key_unicode_action(ch, direction)
    }
}
thread_local! {
    static ENIGO: RefCell<Option<Enigo>> = {
        init_logger();
        let enigo = match Enigo::new(&Settings::default()) {
            Ok(e) => Some(e),
            Err(err) => {
                error!("rs: Enigo init failed: {:?}", err);
                None
            }
        };
        RefCell::new(enigo)
    };
    static CLIPBOARD: RefCell<Option<Clipboard>> = {
        init_logger();
        let cb = match Clipboard::new() {
            Ok(c) => Some(c),
            Err(err) => {
                error!("rs: Clipboard init failed: {:?}", err);
                None
            }
        };
        RefCell::new(cb)
    };
}

fn with_enigo_mut<F, R>(f: F) -> Result<R, ()>
where
    F: FnOnce(&mut Enigo) -> Result<R, ()>,
{
    ENIGO.with(|cell| {
        // Try to initialize if missing
        if cell.borrow().is_none() {
            match Enigo::new(&Settings::default()) {
                Ok(e) => {
                    *cell.borrow_mut() = Some(e);
                }
                Err(err) => {
                    error!("rs: Enigo lazy init failed: {:?}", err);
                    return Err(());
                }
            }
        }
        // Borrow mutably and run the closure
        let mut borrow = cell.borrow_mut();
        match borrow.as_mut() {
            Some(en) => f(en),
            None => Err(()),
        }
    })
}

fn with_clipboard_mut<F, R>(f: F) -> Result<R, ()>
where
    F: FnOnce(&mut Clipboard) -> Result<R, ()>,
{
    CLIPBOARD.with(|cell| {
        // Try to initialize if missing
        if cell.borrow().is_none() {
            match Clipboard::new() {
                Ok(c) => {
                    *cell.borrow_mut() = Some(c);
                }
                Err(err) => {
                    error!("rs: Clipboard lazy init failed: {:?}", err);
                    return Err(());
                }
            }
        }
        let mut borrow = cell.borrow_mut();
        match borrow.as_mut() {
            Some(cb) => f(cb),
            None => Err(()),
        }
    })
}

#[no_mangle]
pub extern "C" fn move_mouse(x: i32, y: i32, coord: CCoordinate) -> YAError {
    match with_enigo_mut(|en| en.move_mouse(x, y, coord.cast()).map_err(|_| ())) {
        Ok(_) => YAError::Success,
        Err(_) => YAError::PlatformError,
    }
}

#[no_mangle]
pub extern "C" fn mouse_button(button: CButton, direction: CDirection) -> YAError {
    match with_enigo_mut(|en| en.button(button.cast(), direction.cast()).map_err(|_| ())) {
        Ok(_) => YAError::Success,
        Err(_) => YAError::PlatformError,
    }
}

#[no_mangle]
pub extern "C" fn key_action(key: CKey, direction: CDirection) -> YAError {
    match key.cast() {
        Ok(key) => match with_enigo_mut(|en| en.key(key, direction.cast()).map_err(|_| ())) {
            Ok(_) => YAError::Success,
            Err(_) => YAError::PlatformError,
        },
        Err(_) => YAError::InvalidInput,
    }
}

#[no_mangle]
pub extern "C" fn key_action_with_platform_code(code: u32, direction: CDirection) -> YAError {
    match with_enigo_mut(|en| en.key(Key::Other(code), direction.cast()).map_err(|_| ())) {
        Ok(_) => YAError::Success,
        Err(_) => YAError::PlatformError,
    }
}

#[no_mangle]
pub extern "C" fn key_action_with_code(key: c_char, direction: CDirection) -> YAError {
    let c_key = key as u8 as char;
    if !c_key.is_ascii() {
        return YAError::InvalidInput;
    }
    
    match with_enigo_mut(|en| en.key(Key::Unicode(c_key), direction.cast()).map_err(|_| ())) {
        Ok(_) => YAError::Success,
        Err(_) => YAError::PlatformError,
    }
}

#[no_mangle]
pub extern "C" fn enter_text(text: *const c_char) -> YAError {
    if text.is_null() {
        return YAError::InvalidInput;
    }
    
    unsafe {
        let c_str = match CStr::from_ptr(text).to_str() {
            Ok(s) => s,
            Err(_) => return YAError::InvalidInput,
        };
        match with_enigo_mut(|en| en.text(c_str).map_err(|_| ())) {
            Ok(_) => {
                YAError::Success
            },
            Err(_) => {
                error!("rs: enter_text enigo.text PlatformError");
                YAError::PlatformError
            },
        }
    }
}

#[no_mangle]
pub extern "C" fn clipboard_get() -> *const c_char {
    let text = match with_clipboard_mut(|cb| cb.get_text().map_err(|_| ())) {
        Ok(t) => t,
        Err(_) => return std::ptr::null(),
    };
    match CString::new(text) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => std::ptr::null(),
    }
}

#[no_mangle]
pub extern "C" fn clipboard_set(text: *const c_char) -> YAError {
    if text.is_null() {
        return YAError::InvalidInput;
    }
    
    unsafe {
        let c_str = match CStr::from_ptr(text).to_str() {
            Ok(s) => s,
            Err(_) => return YAError::InvalidInput,
        };

        match with_clipboard_mut(|cb| cb.set_text(c_str.to_owned()).map_err(|_| ())) {
            Ok(_) => YAError::Success,
            Err(_) => YAError::ClipboardError,
        }
    }
}

#[no_mangle]
pub extern "C" fn free_string(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe {
            let _ = CString::from_raw(ptr);
        }
    }
}
