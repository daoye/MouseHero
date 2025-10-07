use enigo::{Button, Coordinate, Direction, Key};

#[repr(C)]
pub enum CCoordinate {
    Abs,
    Rel,
}

impl CCoordinate {
    pub fn cast(&self) -> Coordinate {
        match self {
            CCoordinate::Abs => Coordinate::Abs,
            CCoordinate::Rel => Coordinate::Rel,
        }
    }
}

#[repr(C)]
pub enum CButton {
    Left,
    Middle,
    Right,
    Back,
    Forward,
    ScrollUp,
    ScrollDown,
    ScrollLeft,
    ScrollRight,
}

impl CButton {
    pub fn cast(&self) -> Button {
        match self {
            CButton::Left => Button::Left,
            CButton::Middle => Button::Middle,
            CButton::Right => Button::Right,
            CButton::ScrollUp => Button::ScrollUp,
            CButton::ScrollDown => Button::ScrollDown,
            CButton::ScrollLeft => Button::ScrollLeft,
            CButton::ScrollRight => Button::ScrollRight,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CButton::Back => Button::Back,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CButton::Forward => Button::Forward,
            #[cfg(not(any(target_os = "windows", all(unix, not(target_os = "macos")))))]
            CButton::Back => Button::Left,
            #[cfg(not(any(target_os = "windows", all(unix, not(target_os = "macos")))))]
            CButton::Forward => Button::Right,
        }
    }
}

#[repr(C)]
pub enum CDirection {
    Press,
    Release,
    Click,
}

impl CDirection {
    pub fn cast(&self) -> Direction {
        match self {
            CDirection::Press => Direction::Press,
            CDirection::Release => Direction::Release,
            CDirection::Click => Direction::Click,
        }
    }
}

#[repr(C)]
pub enum CKey {
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    AbntC1,
    AbntC2,
    Accept,
    Add,
    Alt,
    Apps,
    Attn,
    Backspace,
    Break,
    Begin,
    BrightnessDown,
    BrightnessUp,
    BrowserBack,
    BrowserFavorites,
    BrowserForward,
    BrowserHome,
    BrowserRefresh,
    BrowserSearch,
    BrowserStop,
    Cancel,
    CapsLock,
    Clear,
    Command,
    ContrastUp,
    ContrastDown,
    Control,
    Convert,
    Crsel,
    DBEAlphanumeric,
    DBECodeinput,
    DBEDetermineString,
    DBEEnterDLGConversionMode,
    DBEEnterIMEConfigMode,
    DBEEnterWordRegisterMode,
    DBEFlushString,
    DBEHiragana,
    DBEKatakana,
    DBENoCodepoint,
    DBENoRoman,
    DBERoman,
    DBESBCSChar,
    DBESChar,
    Decimal,
    Delete,
    Divide,
    DownArrow,
    Eject,
    End,
    Ereof,
    Escape_k,
    Execute,
    Exsel,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    F25,
    F26,
    F27,
    F28,
    F29,
    F30,
    F31,
    F32,
    F33,
    F34,
    F35,
    Function,
    Final,
    Find,
    GamepadA,
    GamepadB,
    GamepadDPadDown,
    GamepadDPadLeft,
    GamepadDPadRight,
    GamepadDPadUp,
    GamepadLeftShoulder,
    GamepadLeftThumbstickButton,
    GamepadLeftThumbstickDown,
    GamepadLeftThumbstickLeft,
    GamepadLeftThumbstickRight,
    GamepadLeftThumbstickUp,
    GamepadLeftTrigger,
    GamepadMenu,
    GamepadRightShoulder,
    GamepadRightThumbstickButton,
    GamepadRightThumbstickDown,
    GamepadRightThumbstickLeft,
    GamepadRightThumbstickRight,
    GamepadRightThumbstickUp,
    GamepadRightTrigger,
    GamepadView,
    GamepadX,
    GamepadY,
    Hangeul,
    Hangul,
    Hanja,
    Help,
    Home,
    Ico00,
    IcoClear,
    IcoHelp,
    IlluminationDown,
    IlluminationUp,
    IlluminationToggle,
    IMEOff,
    IMEOn,
    Insert,
    Junja,
    Kana,
    Kanji,
    LaunchApp1,
    LaunchApp2,
    LaunchMail,
    LaunchMediaSelect,
    Launchpad,
    LaunchPanel,
    LButton,
    LControl,
    LeftArrow,
    Linefeed,
    LMenu,
    LShift,
    LWin,
    MButton,
    MediaFast,
    MediaNextTrack,
    MediaPlayPause,
    MediaPrevTrack,
    MediaRewind,
    MediaStop,
    Meta,
    MissionControl,
    ModeChange,
    Multiply,
    NavigationAccept,
    NavigationCancel,
    NavigationDown,
    NavigationLeft,
    NavigationMenu,
    NavigationRight,
    NavigationUp,
    NavigationView,
    NoName,
    NonConvert,
    None,
    Numlock,
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    OEM1,
    OEM102,
    OEM2,
    OEM3,
    OEM4,
    OEM5,
    OEM6,
    OEM7,
    OEM8,
    OEMAttn,
    OEMAuto,
    OEMAx,
    OEMBacktab,
    OEMClear,
    OEMComma,
    OEMCopy,
    OEMCusel,
    OEMEnlw,
    OEMFinish,
    OEMFJJisho,
    OEMFJLoya,
    OEMFJMasshou,
    OEMFJRoya,
    OEMFJTouroku,
    OEMJump,
    OEMMinus,
    OEMNECEqual,
    OEMPA1,
    OEMPA2,
    OEMPA3,
    OEMPeriod,
    OEMPlus,
    OEMReset,
    OEMWsctrl,
    Option,
    PA1,
    Packet,
    PageDown,
    PageUp,
    Pause,
    Play,
    Power,
    PrintScr,
    Processkey,
    RButton,
    RCommand,
    RControl,
    Redo,
    Return,
    RightArrow,
    RMenu,
    ROption,
    RShift,
    RWin,
    Scroll,
    ScrollLock,
    Select,
    ScriptSwitch,
    Separator,
    Shift,
    ShiftLock,
    Sleep_k,
    Space,
    Subtract,
    Tab,
    Undo,
    UpArrow,
    VidMirror,
    VolumeDown,
    VolumeMute,
    VolumeUp,
    MicMute,
    XButton1,
    XButton2,
    Zoom,
}

impl CKey {
    pub fn cast(&self) -> Result<Key, &'static str> {
        Ok(match self {
            #[cfg(target_os = "windows")]
            CKey::Num0 => Key::Num0,
            #[cfg(target_os = "windows")]
            CKey::Num1 => Key::Num1,
            #[cfg(target_os = "windows")]
            CKey::Num2 => Key::Num2,
            #[cfg(target_os = "windows")]
            CKey::Num3 => Key::Num3,
            #[cfg(target_os = "windows")]
            CKey::Num4 => Key::Num4,
            #[cfg(target_os = "windows")]
            CKey::Num5 => Key::Num5,
            #[cfg(target_os = "windows")]
            CKey::Num6 => Key::Num6,
            #[cfg(target_os = "windows")]
            CKey::Num7 => Key::Num7,
            #[cfg(target_os = "windows")]
            CKey::Num8 => Key::Num8,
            #[cfg(target_os = "windows")]
            CKey::Num9 => Key::Num9,
            #[cfg(target_os = "macos")]
            CKey::Num0 => Key::Unicode('0'),
            #[cfg(target_os = "macos")]
            CKey::Num1 => Key::Unicode('1'),
            #[cfg(target_os = "macos")]
            CKey::Num2 => Key::Unicode('2'),
            #[cfg(target_os = "macos")]
            CKey::Num3 => Key::Unicode('3'),
            #[cfg(target_os = "macos")]
            CKey::Num4 => Key::Unicode('4'),
            #[cfg(target_os = "macos")]
            CKey::Num5 => Key::Unicode('5'),
            #[cfg(target_os = "macos")]
            CKey::Num6 => Key::Unicode('6'),
            #[cfg(target_os = "macos")]
            CKey::Num7 => Key::Unicode('7'),
            #[cfg(target_os = "macos")]
            CKey::Num8 => Key::Unicode('8'),
            #[cfg(target_os = "macos")]
            CKey::Num9 => Key::Unicode('9'),
            #[cfg(target_os = "windows")]
            CKey::A => Key::A,
            #[cfg(target_os = "windows")]
            CKey::B => Key::B,
            #[cfg(target_os = "windows")]
            CKey::C => Key::C,
            #[cfg(target_os = "windows")]
            CKey::D => Key::D,
            #[cfg(target_os = "windows")]
            CKey::E => Key::E,
            #[cfg(target_os = "windows")]
            CKey::F => Key::F,
            #[cfg(target_os = "windows")]
            CKey::G => Key::G,
            #[cfg(target_os = "windows")]
            CKey::H => Key::H,
            #[cfg(target_os = "windows")]
            CKey::I => Key::I,
            #[cfg(target_os = "windows")]
            CKey::J => Key::J,
            #[cfg(target_os = "windows")]
            CKey::K => Key::K,
            #[cfg(target_os = "windows")]
            CKey::L => Key::L,
            #[cfg(target_os = "windows")]
            CKey::M => Key::M,
            #[cfg(target_os = "windows")]
            CKey::N => Key::N,
            #[cfg(target_os = "windows")]
            CKey::O => Key::O,
            #[cfg(target_os = "windows")]
            CKey::P => Key::P,
            #[cfg(target_os = "windows")]
            CKey::Q => Key::Q,
            #[cfg(target_os = "windows")]
            CKey::R => Key::R,
            #[cfg(target_os = "windows")]
            CKey::S => Key::S,
            #[cfg(target_os = "windows")]
            CKey::T => Key::T,
            #[cfg(target_os = "windows")]
            CKey::U => Key::U,
            #[cfg(target_os = "windows")]
            CKey::V => Key::V,
            #[cfg(target_os = "windows")]
            CKey::W => Key::W,
            #[cfg(target_os = "windows")]
            CKey::X => Key::X,
            #[cfg(target_os = "windows")]
            CKey::Y => Key::Y,
            #[cfg(target_os = "windows")]
            CKey::Z => Key::Z,
            #[cfg(target_os = "macos")]
            CKey::A => Key::Unicode('a'),
            #[cfg(target_os = "macos")]
            CKey::B => Key::Unicode('b'),
            #[cfg(target_os = "macos")]
            CKey::C => Key::Unicode('c'),
            #[cfg(target_os = "macos")]
            CKey::D => Key::Unicode('d'),
            #[cfg(target_os = "macos")]
            CKey::E => Key::Unicode('e'),
            #[cfg(target_os = "macos")]
            CKey::F => Key::Unicode('f'),
            #[cfg(target_os = "macos")]
            CKey::G => Key::Unicode('g'),
            #[cfg(target_os = "macos")]
            CKey::H => Key::Unicode('h'),
            #[cfg(target_os = "macos")]
            CKey::I => Key::Unicode('i'),
            #[cfg(target_os = "macos")]
            CKey::J => Key::Unicode('j'),
            #[cfg(target_os = "macos")]
            CKey::K => Key::Unicode('k'),
            #[cfg(target_os = "macos")]
            CKey::L => Key::Unicode('l'),
            #[cfg(target_os = "macos")]
            CKey::M => Key::Unicode('m'),
            #[cfg(target_os = "macos")]
            CKey::N => Key::Unicode('n'),
            #[cfg(target_os = "macos")]
            CKey::O => Key::Unicode('o'),
            #[cfg(target_os = "macos")]
            CKey::P => Key::Unicode('p'),
            #[cfg(target_os = "macos")]
            CKey::Q => Key::Unicode('q'),
            #[cfg(target_os = "macos")]
            CKey::R => Key::Unicode('r'),
            #[cfg(target_os = "macos")]
            CKey::S => Key::Unicode('s'),
            #[cfg(target_os = "macos")]
            CKey::T => Key::Unicode('t'),
            #[cfg(target_os = "macos")]
            CKey::U => Key::Unicode('u'),
            #[cfg(target_os = "macos")]
            CKey::V => Key::Unicode('v'),
            #[cfg(target_os = "macos")]
            CKey::W => Key::Unicode('w'),
            #[cfg(target_os = "macos")]
            CKey::X => Key::Unicode('x'),
            #[cfg(target_os = "macos")]
            CKey::Y => Key::Unicode('y'),
            #[cfg(target_os = "macos")]
            CKey::Z => Key::Unicode('z'),
            #[cfg(target_os = "windows")]
            CKey::AbntC1 => Key::AbntC1,
            #[cfg(target_os = "windows")]
            CKey::AbntC2 => Key::AbntC2,
            #[cfg(target_os = "windows")]
            CKey::Accept => Key::Accept,
            #[cfg(target_os = "windows")]
            CKey::Add => Key::Add,
            CKey::Alt => Key::Alt,
            #[cfg(target_os = "windows")]
            CKey::Apps => Key::Apps,
            #[cfg(target_os = "windows")]
            CKey::Attn => Key::Attn,
            CKey::Backspace => Key::Backspace,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::Break => Key::Break,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::Begin => Key::Begin,
            #[cfg(target_os = "macos")]
            CKey::BrightnessDown => Key::BrightnessDown,
            #[cfg(target_os = "macos")]
            CKey::BrightnessUp => Key::BrightnessUp,
            #[cfg(target_os = "windows")]
            CKey::BrowserBack => Key::BrowserBack,
            #[cfg(target_os = "windows")]
            CKey::BrowserFavorites => Key::BrowserFavorites,
            #[cfg(target_os = "windows")]
            CKey::BrowserForward => Key::BrowserForward,
            #[cfg(target_os = "windows")]
            CKey::BrowserHome => Key::BrowserHome,
            #[cfg(target_os = "windows")]
            CKey::BrowserRefresh => Key::BrowserRefresh,
            #[cfg(target_os = "windows")]
            CKey::BrowserSearch => Key::BrowserSearch,
            #[cfg(target_os = "windows")]
            CKey::BrowserStop => Key::BrowserStop,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::Cancel => Key::Cancel,
            CKey::CapsLock => Key::CapsLock,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::Clear => Key::Clear,
            // CKey::Command => Key::Command,
            #[cfg(target_os = "macos")]
            CKey::ContrastUp => Key::ContrastUp,
            #[cfg(target_os = "macos")]
            CKey::ContrastDown => Key::ContrastDown,
            CKey::Control => Key::Control,
            #[cfg(target_os = "windows")]
            CKey::Convert => Key::Convert,
            #[cfg(target_os = "windows")]
            CKey::Crsel => Key::Crsel,
            #[cfg(target_os = "windows")]
            CKey::DBEAlphanumeric => Key::DBEAlphanumeric,
            #[cfg(target_os = "windows")]
            CKey::DBECodeinput => Key::DBECodeinput,
            #[cfg(target_os = "windows")]
            CKey::DBEDetermineString => Key::DBEDetermineString,
            #[cfg(target_os = "windows")]
            CKey::DBEEnterDLGConversionMode => Key::DBEEnterDLGConversionMode,
            #[cfg(target_os = "windows")]
            CKey::DBEEnterIMEConfigMode => Key::DBEEnterIMEConfigMode,
            #[cfg(target_os = "windows")]
            CKey::DBEEnterWordRegisterMode => Key::DBEEnterWordRegisterMode,
            #[cfg(target_os = "windows")]
            CKey::DBEFlushString => Key::DBEFlushString,
            #[cfg(target_os = "windows")]
            CKey::DBEHiragana => Key::DBEHiragana,
            #[cfg(target_os = "windows")]
            CKey::DBEKatakana => Key::DBEKatakana,
            #[cfg(target_os = "windows")]
            CKey::DBENoCodepoint => Key::DBENoCodepoint,
            #[cfg(target_os = "windows")]
            CKey::DBENoRoman => Key::DBENoRoman,
            #[cfg(target_os = "windows")]
            CKey::DBERoman => Key::DBERoman,
            #[cfg(target_os = "windows")]
            CKey::DBESBCSChar => Key::DBESBCSChar,
            #[cfg(target_os = "windows")]
            CKey::DBESChar => Key::DBESChar,
            #[cfg(target_os = "windows")]
            CKey::Decimal => Key::Decimal,
            CKey::Delete => Key::Delete,
            #[cfg(target_os = "windows")]
            CKey::Divide => Key::Divide,
            CKey::DownArrow => Key::DownArrow,
            #[cfg(target_os = "macos")]
            CKey::Eject => Key::Eject,
            CKey::End => Key::End,
            #[cfg(target_os = "windows")]
            CKey::Ereof => Key::Ereof,
            CKey::Escape_k => Key::Escape,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::Execute => Key::Execute,
            #[cfg(target_os = "windows")]
            CKey::Exsel => Key::Exsel,
            CKey::F1 => Key::F1,
            CKey::F2 => Key::F2,
            CKey::F3 => Key::F3,
            CKey::F4 => Key::F4,
            CKey::F5 => Key::F5,
            CKey::F6 => Key::F6,
            CKey::F7 => Key::F7,
            CKey::F8 => Key::F8,
            CKey::F9 => Key::F9,
            CKey::F10 => Key::F10,
            CKey::F11 => Key::F11,
            CKey::F12 => Key::F12,
            CKey::F13 => Key::F13,
            CKey::F14 => Key::F14,
            CKey::F15 => Key::F15,
            CKey::F16 => Key::F16,
            CKey::F17 => Key::F17,
            CKey::F18 => Key::F18,
            CKey::F19 => Key::F19,
            CKey::F20 => Key::F20,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::F21 => Key::F21,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::F22 => Key::F22,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::F23 => Key::F23,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::F24 => Key::F24,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F25 => Key::F25,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F26 => Key::F26,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F27 => Key::F27,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F28 => Key::F28,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F29 => Key::F29,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F30 => Key::F30,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F31 => Key::F31,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F32 => Key::F32,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F33 => Key::F33,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F34 => Key::F34,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::F35 => Key::F35,
            #[cfg(target_os = "macos")]
            CKey::Function => Key::Function,
            #[cfg(target_os = "windows")]
            CKey::Final => Key::Final,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::Find => Key::Find,
            #[cfg(target_os = "windows")]
            CKey::GamepadA => Key::GamepadA,
            #[cfg(target_os = "windows")]
            CKey::GamepadB => Key::GamepadB,
            #[cfg(target_os = "windows")]
            CKey::GamepadDPadDown => Key::GamepadDPadDown,
            #[cfg(target_os = "windows")]
            CKey::GamepadDPadLeft => Key::GamepadDPadLeft,
            #[cfg(target_os = "windows")]
            CKey::GamepadDPadRight => Key::GamepadDPadRight,
            #[cfg(target_os = "windows")]
            CKey::GamepadDPadUp => Key::GamepadDPadUp,
            #[cfg(target_os = "windows")]
            CKey::GamepadLeftShoulder => Key::GamepadLeftShoulder,
            #[cfg(target_os = "windows")]
            CKey::GamepadLeftThumbstickButton => Key::GamepadLeftThumbstickButton,
            #[cfg(target_os = "windows")]
            CKey::GamepadLeftThumbstickDown => Key::GamepadLeftThumbstickDown,
            #[cfg(target_os = "windows")]
            CKey::GamepadLeftThumbstickLeft => Key::GamepadLeftThumbstickLeft,
            #[cfg(target_os = "windows")]
            CKey::GamepadLeftThumbstickRight => Key::GamepadLeftThumbstickRight,
            #[cfg(target_os = "windows")]
            CKey::GamepadLeftThumbstickUp => Key::GamepadLeftThumbstickUp,
            #[cfg(target_os = "windows")]
            CKey::GamepadLeftTrigger => Key::GamepadLeftTrigger,
            #[cfg(target_os = "windows")]
            CKey::GamepadMenu => Key::GamepadMenu,
            #[cfg(target_os = "windows")]
            CKey::GamepadRightShoulder => Key::GamepadRightShoulder,
            #[cfg(target_os = "windows")]
            CKey::GamepadRightThumbstickButton => Key::GamepadRightThumbstickButton,
            #[cfg(target_os = "windows")]
            CKey::GamepadRightThumbstickDown => Key::GamepadRightThumbstickDown,
            #[cfg(target_os = "windows")]
            CKey::GamepadRightThumbstickLeft => Key::GamepadRightThumbstickLeft,
            #[cfg(target_os = "windows")]
            CKey::GamepadRightThumbstickRight => Key::GamepadRightThumbstickRight,
            #[cfg(target_os = "windows")]
            CKey::GamepadRightThumbstickUp => Key::GamepadRightThumbstickUp,
            #[cfg(target_os = "windows")]
            CKey::GamepadRightTrigger => Key::GamepadRightTrigger,
            #[cfg(target_os = "windows")]
            CKey::GamepadView => Key::GamepadView,
            #[cfg(target_os = "windows")]
            CKey::GamepadX => Key::GamepadX,
            #[cfg(target_os = "windows")]
            CKey::GamepadY => Key::GamepadY,
            #[cfg(target_os = "windows")]
            CKey::Hangeul => Key::Hangeul,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::Hangul => Key::Hangul,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::Hanja => Key::Hanja,
            CKey::Help => Key::Help,
            CKey::Home => Key::Home,
            #[cfg(target_os = "windows")]
            CKey::Ico00 => Key::Ico00,
            #[cfg(target_os = "windows")]
            CKey::IcoClear => Key::IcoClear,
            #[cfg(target_os = "windows")]
            CKey::IcoHelp => Key::IcoHelp,
            #[cfg(target_os = "macos")]
            CKey::IlluminationDown => Key::IlluminationDown,
            #[cfg(target_os = "macos")]
            CKey::IlluminationUp => Key::IlluminationUp,
            #[cfg(target_os = "macos")]
            CKey::IlluminationToggle => Key::IlluminationToggle,
            #[cfg(target_os = "windows")]
            CKey::IMEOff => Key::IMEOff,
            #[cfg(target_os = "windows")]
            CKey::IMEOn => Key::IMEOn,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::Insert => Key::Insert,
            #[cfg(target_os = "windows")]
            CKey::Junja => Key::Junja,
            #[cfg(target_os = "windows")]
            CKey::Kana => Key::Kana,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::Kanji => Key::Kanji,
            #[cfg(target_os = "windows")]
            CKey::LaunchApp1 => Key::LaunchApp1,
            #[cfg(target_os = "windows")]
            CKey::LaunchApp2 => Key::LaunchApp2,
            #[cfg(target_os = "windows")]
            CKey::LaunchMail => Key::LaunchMail,
            #[cfg(target_os = "windows")]
            CKey::LaunchMediaSelect => Key::LaunchMediaSelect,
            #[cfg(target_os = "macos")]
            CKey::Launchpad => Key::Launchpad,
            #[cfg(target_os = "macos")]
            CKey::LaunchPanel => Key::LaunchPanel,
            #[cfg(target_os = "windows")]
            CKey::LButton => Key::LButton,
            CKey::LControl => Key::LControl,
            CKey::LeftArrow => Key::LeftArrow,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::Linefeed => Key::Linefeed,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::LMenu => Key::LMenu,
            CKey::LShift => Key::LShift,
            #[cfg(target_os = "windows")]
            CKey::LWin => Key::LWin,
            #[cfg(target_os = "windows")]
            CKey::MButton => Key::MButton,
            #[cfg(target_os = "macos")]
            CKey::MediaFast => Key::MediaFast,
            CKey::MediaNextTrack => Key::MediaNextTrack,
            CKey::MediaPlayPause => Key::MediaPlayPause,
            CKey::MediaPrevTrack => Key::MediaPrevTrack,
            #[cfg(target_os = "macos")]
            CKey::MediaRewind => Key::MediaRewind,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::MediaStop => Key::MediaStop,
            CKey::Meta => Key::Meta,
            #[cfg(target_os = "macos")]
            CKey::MissionControl => Key::MissionControl,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::ModeChange => Key::ModeChange,
            #[cfg(target_os = "windows")]
            CKey::Multiply => Key::Multiply,
            #[cfg(target_os = "windows")]
            CKey::NavigationAccept => Key::NavigationAccept,
            #[cfg(target_os = "windows")]
            CKey::NavigationCancel => Key::NavigationCancel,
            #[cfg(target_os = "windows")]
            CKey::NavigationDown => Key::NavigationDown,
            #[cfg(target_os = "windows")]
            CKey::NavigationLeft => Key::NavigationLeft,
            #[cfg(target_os = "windows")]
            CKey::NavigationMenu => Key::NavigationMenu,
            #[cfg(target_os = "windows")]
            CKey::NavigationRight => Key::NavigationRight,
            #[cfg(target_os = "windows")]
            CKey::NavigationUp => Key::NavigationUp,
            #[cfg(target_os = "windows")]
            CKey::NavigationView => Key::NavigationView,
            #[cfg(target_os = "windows")]
            CKey::NoName => Key::NoName,
            #[cfg(target_os = "windows")]
            CKey::NonConvert => Key::NonConvert,
            #[cfg(target_os = "windows")]
            CKey::None => Key::None,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::Numlock => Key::Numlock,
            #[cfg(target_os = "windows")]
            CKey::Numpad0 => Key::Numpad0,
            #[cfg(target_os = "windows")]
            CKey::Numpad1 => Key::Numpad1,
            #[cfg(target_os = "windows")]
            CKey::Numpad2 => Key::Numpad2,
            #[cfg(target_os = "windows")]
            CKey::Numpad3 => Key::Numpad3,
            #[cfg(target_os = "windows")]
            CKey::Numpad4 => Key::Numpad4,
            #[cfg(target_os = "windows")]
            CKey::Numpad5 => Key::Numpad5,
            #[cfg(target_os = "windows")]
            CKey::Numpad6 => Key::Numpad6,
            #[cfg(target_os = "windows")]
            CKey::Numpad7 => Key::Numpad7,
            #[cfg(target_os = "windows")]
            CKey::Numpad8 => Key::Numpad8,
            #[cfg(target_os = "windows")]
            CKey::Numpad9 => Key::Numpad9,
            #[cfg(target_os = "windows")]
            CKey::OEM1 => Key::OEM1,
            #[cfg(target_os = "windows")]
            CKey::OEM102 => Key::OEM102,
            #[cfg(target_os = "windows")]
            CKey::OEM2 => Key::OEM2,
            #[cfg(target_os = "windows")]
            CKey::OEM3 => Key::OEM3,
            #[cfg(target_os = "windows")]
            CKey::OEM4 => Key::OEM4,
            #[cfg(target_os = "windows")]
            CKey::OEM5 => Key::OEM5,
            #[cfg(target_os = "windows")]
            CKey::OEM6 => Key::OEM6,
            #[cfg(target_os = "windows")]
            CKey::OEM7 => Key::OEM7,
            #[cfg(target_os = "windows")]
            CKey::OEM8 => Key::OEM8,
            #[cfg(target_os = "windows")]
            CKey::OEMAttn => Key::OEMAttn,
            #[cfg(target_os = "windows")]
            CKey::OEMAuto => Key::OEMAuto,
            #[cfg(target_os = "windows")]
            CKey::OEMAx => Key::OEMAx,
            #[cfg(target_os = "windows")]
            CKey::OEMBacktab => Key::OEMBacktab,
            #[cfg(target_os = "windows")]
            CKey::OEMClear => Key::OEMClear,
            #[cfg(target_os = "windows")]
            CKey::OEMComma => Key::OEMComma,
            #[cfg(target_os = "windows")]
            CKey::OEMCopy => Key::OEMCopy,
            #[cfg(target_os = "windows")]
            CKey::OEMCusel => Key::OEMCusel,
            #[cfg(target_os = "windows")]
            CKey::OEMEnlw => Key::OEMEnlw,
            #[cfg(target_os = "windows")]
            CKey::OEMFinish => Key::OEMFinish,
            #[cfg(target_os = "windows")]
            CKey::OEMFJJisho => Key::OEMFJJisho,
            #[cfg(target_os = "windows")]
            CKey::OEMFJLoya => Key::OEMFJLoya,
            #[cfg(target_os = "windows")]
            CKey::OEMFJMasshou => Key::OEMFJMasshou,
            #[cfg(target_os = "windows")]
            CKey::OEMFJRoya => Key::OEMFJRoya,
            #[cfg(target_os = "windows")]
            CKey::OEMFJTouroku => Key::OEMFJTouroku,
            #[cfg(target_os = "windows")]
            CKey::OEMJump => Key::OEMJump,
            #[cfg(target_os = "windows")]
            CKey::OEMMinus => Key::OEMMinus,
            #[cfg(target_os = "windows")]
            CKey::OEMNECEqual => Key::OEMNECEqual,
            #[cfg(target_os = "windows")]
            CKey::OEMPA1 => Key::OEMPA1,
            #[cfg(target_os = "windows")]
            CKey::OEMPA2 => Key::OEMPA2,
            #[cfg(target_os = "windows")]
            CKey::OEMPA3 => Key::OEMPA3,
            #[cfg(target_os = "windows")]
            CKey::OEMPeriod => Key::OEMPeriod,
            #[cfg(target_os = "windows")]
            CKey::OEMPlus => Key::OEMPlus,
            #[cfg(target_os = "windows")]
            CKey::OEMReset => Key::OEMReset,
            #[cfg(target_os = "windows")]
            CKey::OEMWsctrl => Key::OEMWsctrl,
            CKey::Option => Key::Option,
            #[cfg(target_os = "windows")]
            CKey::PA1 => Key::PA1,
            #[cfg(target_os = "windows")]
            CKey::Packet => Key::Packet,
            CKey::PageDown => Key::PageDown,
            CKey::PageUp => Key::PageUp,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::Pause => Key::Pause,
            #[cfg(target_os = "windows")]
            CKey::Play => Key::Play,
            #[cfg(target_os = "macos")]
            CKey::Power => Key::Power,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::PrintScr => Key::PrintScr,
            #[cfg(target_os = "windows")]
            CKey::Processkey => Key::Processkey,
            #[cfg(target_os = "windows")]
            CKey::RButton => Key::RButton,
            #[cfg(target_os = "macos")]
            CKey::RCommand => Key::RCommand,
            CKey::RControl => Key::RControl,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::Redo => Key::Redo,
            CKey::Return => Key::Return,
            CKey::RightArrow => Key::RightArrow,
            #[cfg(target_os = "windows")]
            CKey::RMenu => Key::RMenu,
            #[cfg(target_os = "macos")]
            CKey::ROption => Key::ROption,
            CKey::RShift => Key::RShift,
            #[cfg(target_os = "windows")]
            CKey::RWin => Key::RWin,
            #[cfg(target_os = "windows")]
            CKey::Scroll => Key::Scroll,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::ScrollLock => Key::ScrollLock,
            #[cfg(any(target_os = "windows", all(unix, not(target_os = "macos"))))]
            CKey::Select => Key::Select,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::ScriptSwitch => Key::ScriptSwitch,
            #[cfg(target_os = "windows")]
            CKey::Separator => Key::Separator,
            CKey::Shift => Key::Shift,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::ShiftLock => Key::ShiftLock,
            #[cfg(target_os = "windows")]
            CKey::Sleep_k => Key::Sleep,
            CKey::Space => Key::Space,
            #[cfg(target_os = "windows")]
            CKey::Subtract => Key::Subtract,
            CKey::Tab => Key::Tab,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::Undo => Key::Undo,
            CKey::UpArrow => Key::UpArrow,
            #[cfg(target_os = "macos")]
            CKey::VidMirror => Key::VidMirror,
            CKey::VolumeDown => Key::VolumeDown,
            CKey::VolumeMute => Key::VolumeMute,
            CKey::VolumeUp => Key::VolumeUp,
            #[cfg(all(unix, not(target_os = "macos")))]
            CKey::MicMute => Key::MicMute,
            #[cfg(target_os = "windows")]
            CKey::XButton1 => Key::XButton1,
            #[cfg(target_os = "windows")]
            CKey::XButton2 => Key::XButton2,
            #[cfg(target_os = "windows")]
            CKey::Zoom => Key::Zoom,
            _ => return Err("Not support this key in this platform."),
        })
    }
}

