use crate::bindings::MirKeyboardAction;

use super::bindings;
use bitflags::bitflags;

/// Event type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u32)]
pub enum EventType {
    /// Unused since Mir 0.26.
    Key = 0,
    /// Unused since Mir 0.26.
    Motion = 1,
    Window = 2,
    Resize = 3,
    PromptSessionStateChange = 4,
    Orientation = 5,
    CloseWindow = 6,
    Input = 7,
    /// Unused since Mir 0.26.
    InputConfiguration = 8,
    WindowOutput = 9,
    InputDeviceState = 10,
    WindowPlacement = 11,
}

impl From<EventType> for bindings::MirEventType {
    fn from(value: EventType) -> Self {
        value as bindings::MirEventType
    }
}

impl TryFrom<bindings::MirEventType> for EventType {
    type Error = ();

    fn try_from(value: bindings::MirEventType) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Key),
            1 => Ok(Self::Motion),
            2 => Ok(Self::Window),
            3 => Ok(Self::Resize),
            4 => Ok(Self::PromptSessionStateChange),
            5 => Ok(Self::Orientation),
            6 => Ok(Self::CloseWindow),
            7 => Ok(Self::Input),
            8 => Ok(Self::InputConfiguration),
            9 => Ok(Self::WindowOutput),
            10 => Ok(Self::InputDeviceState),
            11 => Ok(Self::WindowPlacement),
            _ => Err(()),
        }
    }
}

/// Input event type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum InputEventType {
    #[default]
    Key = 0,
    Touch = 1,
    Pointer = 2,
    KeyboardResync = 3,
}

impl From<InputEventType> for bindings::MirInputEventType {
    fn from(value: InputEventType) -> Self {
        value as bindings::MirInputEventType
    }
}

impl TryFrom<bindings::MirInputEventType> for InputEventType {
    type Error = ();

    fn try_from(value: bindings::MirInputEventType) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Key),
            1 => Ok(Self::Touch),
            2 => Ok(Self::Pointer),
            3 => Ok(Self::KeyboardResync),
            _ => Err(()),
        }
    }
}

bitflags! {
    /// Key modifier state flags.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct InputEventModifiers: u32 {
        const NONE         = 1 << 0;
        const ALT          = 1 << 1;
        const ALT_LEFT     = 1 << 2;
        const ALT_RIGHT    = 1 << 3;
        const SHIFT        = 1 << 4;
        const SHIFT_LEFT   = 1 << 5;
        const SHIFT_RIGHT  = 1 << 6;
        const SYM          = 1 << 7;
        const FUNCTION     = 1 << 8;
        const CTRL         = 1 << 9;
        const CTRL_LEFT    = 1 << 10;
        const CTRL_RIGHT   = 1 << 11;
        const META         = 1 << 12;
        const META_LEFT    = 1 << 13;
        const META_RIGHT   = 1 << 14;
        const CAPS_LOCK    = 1 << 15;
        const NUM_LOCK     = 1 << 16;
        const SCROLL_LOCK  = 1 << 17;
    }
}

impl From<InputEventModifiers> for bindings::MirInputEventModifiers {
    fn from(value: InputEventModifiers) -> Self {
        value.bits()
    }
}

impl From<bindings::MirInputEventModifiers> for InputEventModifiers {
    fn from(value: bindings::MirInputEventModifiers) -> Self {
        InputEventModifiers::from_bits_truncate(value)
    }
}

/// Keyboard action.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum KeyboardAction {
    /// A key has been released.
    #[default]
    Up = 0,
    /// A key has been pressed.
    Down = 1,
    /// System policy triggered a key repeat on an already-down key.
    Repeat = 2,
    /// Modifiers updated without a key event; keysym, scan_code and text are not set.
    Modifiers = 3,
}

impl From<KeyboardAction> for bindings::MirKeyboardAction {
    fn from(value: KeyboardAction) -> Self {
        value as bindings::MirKeyboardAction
    }
}

impl TryFrom<bindings::MirKeyboardAction> for KeyboardAction {
    type Error = ();

    fn try_from(value: bindings::MirKeyboardAction) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Up),
            1 => Ok(Self::Down),
            2 => Ok(Self::Repeat),
            3 => Ok(Self::Modifiers),
            _ => Err(()),
        }
    }
}

/// Per-touch action.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum TouchAction {
    /// This touch point is going up.
    #[default]
    Up = 0,
    /// This touch point is going down.
    Down = 1,
    /// Axis values have changed on this touch point.
    Change = 2,
}

impl From<TouchAction> for bindings::MirTouchAction {
    fn from(value: TouchAction) -> Self {
        value as bindings::MirTouchAction
    }
}

impl TryFrom<bindings::MirTouchAction> for TouchAction {
    type Error = ();

    fn try_from(value: bindings::MirTouchAction) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Up),
            1 => Ok(Self::Down),
            2 => Ok(Self::Change),
            _ => Err(()),
        }
    }
}

/// Touch axis identifier.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum TouchAxis {
    /// X coordinate of the touch.
    #[default]
    X = 0,
    /// Y coordinate of the touch.
    Y = 1,
    /// Pressure of the touch.
    Pressure = 2,
    /// Length of the major axis of the touch ellipse.
    TouchMajor = 3,
    /// Length of the minor axis of the touch ellipse.
    TouchMinor = 4,
    /// Diameter of a circle centered on the touch point.
    Size = 5,
}

impl From<TouchAxis> for bindings::MirTouchAxis {
    fn from(value: TouchAxis) -> Self {
        value as bindings::MirTouchAxis
    }
}

impl TryFrom<bindings::MirTouchAxis> for TouchAxis {
    type Error = ();

    fn try_from(value: bindings::MirTouchAxis) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::X),
            1 => Ok(Self::Y),
            2 => Ok(Self::Pressure),
            3 => Ok(Self::TouchMajor),
            4 => Ok(Self::TouchMinor),
            5 => Ok(Self::Size),
            _ => Err(()),
        }
    }
}

/// Per-touch tool type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum TouchTooltype {
    /// Tool type could not be determined.
    #[default]
    Unknown = 0,
    /// Touch made with a finger.
    Finger = 1,
    /// Touch made with a stylus.
    Stylus = 2,
}

impl From<TouchTooltype> for bindings::MirTouchTooltype {
    fn from(value: TouchTooltype) -> Self {
        value as bindings::MirTouchTooltype
    }
}

impl TryFrom<bindings::MirTouchTooltype> for TouchTooltype {
    type Error = ();

    fn try_from(value: bindings::MirTouchTooltype) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::Unknown),
            1 => Ok(Self::Finger),
            2 => Ok(Self::Stylus),
            _ => Err(()),
        }
    }
}

/// Pointer action.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum PointerAction {
    /// A pointer button has been released.
    #[default]
    ButtonUp = 0,
    /// A pointer button has been pressed.
    ButtonDown = 1,
    /// The pointer entered the surface.
    Enter = 2,
    /// The pointer left the surface.
    Leave = 3,
    /// Axis values have changed.
    Motion = 4,
}

impl From<PointerAction> for bindings::MirPointerAction {
    fn from(value: PointerAction) -> Self {
        value as bindings::MirPointerAction
    }
}

impl TryFrom<bindings::MirPointerAction> for PointerAction {
    type Error = ();

    fn try_from(value: bindings::MirPointerAction) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::ButtonUp),
            1 => Ok(Self::ButtonDown),
            2 => Ok(Self::Enter),
            3 => Ok(Self::Leave),
            4 => Ok(Self::Motion),
            _ => Err(()),
        }
    }
}

/// Pointer axis identifier.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum PointerAxis {
    /// Absolute X coordinate of the pointer.
    #[default]
    X = 0,
    /// Absolute Y coordinate of the pointer.
    Y = 1,
    /// Vertical scroll wheel ticks.
    VScroll = 2,
    /// Horizontal scroll wheel ticks.
    HScroll = 3,
    /// Last reported X differential from the pointer.
    RelativeX = 4,
    /// Last reported Y differential from the pointer.
    RelativeY = 5,
    /// Physical vertical scroll wheel clicks.
    VScrollDiscrete = 6,
    /// Physical horizontal scroll wheel clicks.
    HScrollDiscrete = 7,
    /// Fractional values of 120 for high-res vertical scrolling.
    VScrollValue120 = 8,
    /// Fractional values of 120 for high-res horizontal scrolling.
    HScrollValue120 = 9,
}

impl From<PointerAxis> for bindings::MirPointerAxis {
    fn from(value: PointerAxis) -> Self {
        value as bindings::MirPointerAxis
    }
}

impl TryFrom<bindings::MirPointerAxis> for PointerAxis {
    type Error = ();

    fn try_from(value: bindings::MirPointerAxis) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::X),
            1 => Ok(Self::Y),
            2 => Ok(Self::VScroll),
            3 => Ok(Self::HScroll),
            4 => Ok(Self::RelativeX),
            5 => Ok(Self::RelativeY),
            6 => Ok(Self::VScrollDiscrete),
            7 => Ok(Self::HScrollDiscrete),
            8 => Ok(Self::VScrollValue120),
            9 => Ok(Self::HScrollValue120),
            _ => Err(()),
        }
    }
}

bitflags! {
    /// Pointer button state flags.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
    pub struct PointerButtons: u32 {
        const PRIMARY   = 1 << 0;
        const SECONDARY = 1 << 1;
        const TERTIARY  = 1 << 2;
        const BACK      = 1 << 3;
        const FORWARD   = 1 << 4;
        const SIDE      = 1 << 5;
        const EXTRA     = 1 << 6;
        const TASK      = 1 << 7;
    }
}

impl From<PointerButtons> for bindings::MirPointerButtons {
    fn from(value: PointerButtons) -> Self {
        value.bits()
    }
}

impl From<bindings::MirPointerButtons> for PointerButtons {
    fn from(value: bindings::MirPointerButtons) -> Self {
        PointerButtons::from_bits_truncate(value)
    }
}

/// Source of a pointer axis event.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[repr(u32)]
pub enum PointerAxisSource {
    #[default]
    None = 0,
    Wheel = 1,
    Finger = 2,
    Continuous = 3,
    WheelTilt = 4,
}

impl From<PointerAxisSource> for bindings::MirPointerAxisSource {
    fn from(value: PointerAxisSource) -> Self {
        value as bindings::MirPointerAxisSource
    }
}

impl TryFrom<bindings::MirPointerAxisSource> for PointerAxisSource {
    type Error = ();

    fn try_from(value: bindings::MirPointerAxisSource) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(Self::None),
            1 => Ok(Self::Wheel),
            2 => Ok(Self::Finger),
            3 => Ok(Self::Continuous),
            4 => Ok(Self::WheelTilt),
            _ => Err(()),
        }
    }
}

/// A keyboard event.
pub struct KeyboardEvent {
    /// The keyboard action.
    pub action: KeyboardAction,

    /// The xkeysym.
    ///
    /// You may use the xkeysym crate to match against this.
    pub keysym: u32,

    /// The raw scan code.
    ///
    /// Prefer using the keysym.
    pub scan_code: i32,

    /// The modifiers held during this event.
    pub modifiers: InputEventModifiers,
}
