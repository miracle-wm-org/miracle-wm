/// Configuration types for the plugin `configure()` hook.
///
/// Return a [`Configuration`] from your [`Plugin::configure`] implementation
/// to override compositor configuration values on every config reload. Any field
/// left as `None` is ignored; the compositor keeps its own value for that field.
///
/// The `plugins` and `includes` keys cannot be set by plugins.
///
/// # Example
/// ```rust,ignore
/// fn configure(&mut self) -> Option<Configuration> {
///     Some(Configuration {
///         primary_modifier: Some(Modifier::Meta),
///         custom_key_actions: Some(vec![CustomKeyAction {
///             action: BindingAction::Down,
///             modifiers: vec![Modifier::Primary],
///             key: Key::new("Return"),
///             command: "kitty".to_string(),
///         }]),
///         inner_gaps: Some(Gaps { x: 5, y: 5 }),
///         ..Default::default()
///     })
/// }
/// ```
use serde::Serialize;

// ─── Modifier ────────────────────────────────────────────────────────────────

/// A keyboard modifier key for use in configuration bindings.
///
/// These names correspond exactly to the lowercase strings accepted by
/// miracle-wm's configuration parser. [`Modifier::Primary`] is a special
/// sentinel meaning "use whatever the user has configured as their primary
/// modifier key" — it is the recommended value for plugins that want to
/// integrate naturally with the user's keybinding preferences.
///
/// # Relationship to `input::InputEventModifiers`
/// At runtime, `Modifier::Meta` corresponds to `InputEventModifiers::META`,
/// `Modifier::Shift` to `InputEventModifiers::SHIFT`, etc. Config uses a
/// simple enum because the set of recognised modifiers is fixed and small.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Modifier {
    /// Either Alt key. Serializes as `"alt"`.
    Alt,
    /// Left Alt key. Serializes as `"alt_left"`.
    AltLeft,
    /// Right Alt key. Serializes as `"alt_right"`.
    AltRight,
    /// Either Shift key. Serializes as `"shift"`.
    Shift,
    /// Left Shift key. Serializes as `"shift_left"`.
    ShiftLeft,
    /// Right Shift key. Serializes as `"shift_right"`.
    ShiftRight,
    /// Sym key. Serializes as `"sym"`.
    Sym,
    /// Function key. Serializes as `"function"`.
    Function,
    /// Either Ctrl key. Serializes as `"ctrl"`.
    Ctrl,
    /// Left Ctrl key. Serializes as `"ctrl_left"`.
    CtrlLeft,
    /// Right Ctrl key. Serializes as `"ctrl_right"`.
    CtrlRight,
    /// Either Meta/Super/Windows key. Serializes as `"meta"`.
    /// This is the most common choice for compositor bindings.
    Meta,
    /// Left Meta key. Serializes as `"meta_left"`.
    MetaLeft,
    /// Right Meta key. Serializes as `"meta_right"`.
    MetaRight,
    /// Caps Lock. Serializes as `"caps_lock"`.
    CapsLock,
    /// Num Lock. Serializes as `"num_lock"`.
    NumLock,
    /// Scroll Lock. Serializes as `"scroll_lock"`.
    ScrollLock,
    /// Sentinel: "use the user's configured primary modifier key".
    /// Serializes as `"primary"`. Recommended for plugins that should
    /// respect the user's own modifier preference.
    Primary,
}

impl Modifier {
    /// Returns the string representation expected by the compositor.
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Alt         => "alt",
            Self::AltLeft     => "alt_left",
            Self::AltRight    => "alt_right",
            Self::Shift       => "shift",
            Self::ShiftLeft   => "shift_left",
            Self::ShiftRight  => "shift_right",
            Self::Sym         => "sym",
            Self::Function    => "function",
            Self::Ctrl        => "ctrl",
            Self::CtrlLeft    => "ctrl_left",
            Self::CtrlRight   => "ctrl_right",
            Self::Meta        => "meta",
            Self::MetaLeft    => "meta_left",
            Self::MetaRight   => "meta_right",
            Self::CapsLock    => "caps_lock",
            Self::NumLock     => "num_lock",
            Self::ScrollLock  => "scroll_lock",
            Self::Primary     => "primary",
        }
    }
}

impl Serialize for Modifier {
    fn serialize<S: serde::Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        s.serialize_str(self.as_str())
    }
}

// ─── BindingAction ───────────────────────────────────────────────────────────

/// The keyboard event phase that triggers a key binding.
///
/// Named `BindingAction` (rather than `KeyboardAction`) to avoid confusion
/// with [`crate::input::KeyboardAction`], which carries additional runtime
/// variants that have no meaning in a configuration context.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub enum BindingAction {
    /// Key was pressed. This is the most common trigger for bindings.
    #[default]
    Down,
    /// Key was released.
    Up,
    /// Key is being held and auto-repeating.
    Repeat,
}

impl BindingAction {
    /// Returns the string representation expected by the compositor.
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Down   => "down",
            Self::Up     => "up",
            Self::Repeat => "repeat",
        }
    }
}

impl Serialize for BindingAction {
    fn serialize<S: serde::Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        s.serialize_str(self.as_str())
    }
}

// ─── Key ─────────────────────────────────────────────────────────────────────

/// An XKB keysym name for use in configuration bindings.
///
/// Examples: `Key::new("Return")`, `Key::new("a")`, `Key::new("Up")`,
/// `Key::new("F5")`.
///
/// The compositor validates the name using `xkb_keysym_from_name`. A full
/// list of valid names is available at:
/// <https://xkbcommon.org/doc/current/xkbcommon-keysyms_8h.html>
#[derive(Debug, Clone, PartialEq, Eq, Hash, Default)]
pub struct Key(pub String);

impl Key {
    /// Create a key from an XKB keysym name.
    pub fn new(name: impl Into<String>) -> Self {
        Self(name.into())
    }
}

impl<S: Into<String>> From<S> for Key {
    fn from(s: S) -> Self {
        Self(s.into())
    }
}

impl Serialize for Key {
    fn serialize<S: serde::Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        s.serialize_str(&self.0)
    }
}

// ─── Handedness ──────────────────────────────────────────────────────────────

/// Mouse button handedness.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum Handedness {
    #[default]
    Right,
    Left,
}

// ─── PointerAcceleration ─────────────────────────────────────────────────────

/// Pointer acceleration profile.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum PointerAcceleration {
    Adaptive,
    #[default]
    None,
}

// ─── CursorFocusMode ─────────────────────────────────────────────────────────

/// Whether focus follows the pointer or requires a click.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum CursorFocusMode {
    #[default]
    Hover,
    Click,
}

// ─── TouchpadClickMode ───────────────────────────────────────────────────────

/// Touchpad click emulation mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum TouchpadClickMode {
    #[default]
    None,
    AreaToClick,
    FingerCount,
}

// ─── TouchpadScrollMode ──────────────────────────────────────────────────────

/// Touchpad scroll method.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum TouchpadScrollMode {
    #[default]
    None,
    TwoFingerScroll,
    EdgeScroll,
    ButtonDownScroll,
}

// ─── AnimationPartType ───────────────────────────────────────────────────────

/// Built-in animation visual effect for one phase of an animation sequence.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum AnimationPartType {
    #[default]
    Disabled,
    Slide,
    Grow,
    Shrink,
    Fade,
}

// ─── EasingFunction ──────────────────────────────────────────────────────────

/// Easing function for animation timing.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum EasingFunction {
    #[default]
    Linear,
    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInQuart,
    EaseOutQuart,
    EaseInOutQuart,
    EaseInQuint,
    EaseOutQuint,
    EaseInOutQuint,
    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,
    EaseInCirc,
    EaseOutCirc,
    EaseInOutCirc,
    EaseInBack,
    EaseOutBack,
    EaseInOutBack,
    EaseInElastic,
    EaseOutElastic,
    EaseInOutElastic,
    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce,
}

// ─── AnimationEvent ──────────────────────────────────────────────────────────

/// The compositor event that an animation definition applies to.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum AnimationEvent {
    #[default]
    WindowOpen,
    WindowMove,
    WindowClose,
    WorkspaceSwitch,
}

// ─── AnimationKind ───────────────────────────────────────────────────────────

/// Whether an animation is driven by a built-in effect or a plugin callback.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum AnimationKind {
    #[default]
    BuiltIn,
    Plugin,
}

// ─── Config structs ──────────────────────────────────────────────────────────

/// Gaps configuration. Both `x` (left/right) and `y` (top/bottom) are in pixels.
#[derive(Debug, Clone, Default, Serialize)]
pub struct Gaps {
    pub x: i32,
    pub y: i32,
}

/// A custom key binding that runs a shell command.
#[derive(Debug, Clone, Serialize)]
pub struct CustomKeyAction {
    /// The keyboard event phase that triggers this binding.
    pub action: BindingAction,
    /// The modifier keys required for this binding.
    pub modifiers: Vec<Modifier>,
    /// The XKB keysym name (e.g. `Key::new("Return")`, `Key::new("a")`).
    pub key: Key,
    /// The shell command to execute.
    pub command: String,
}

/// Override the key binding for a built-in compositor action.
#[derive(Debug, Clone, Serialize)]
pub struct BuiltInKeyCommandOverride {
    /// Name of the built-in action (e.g. `"terminal"`, `"close_window"`).
    pub name: String,
    /// The keyboard event phase that triggers this binding.
    pub action: BindingAction,
    /// The modifier keys required for this binding.
    pub modifiers: Vec<Modifier>,
    /// The XKB keysym name.
    pub key: Key,
}

/// An application to start on compositor launch.
#[derive(Debug, Clone, Default, Serialize)]
pub struct StartupApp {
    pub command: String,
    #[serde(skip_serializing_if = "is_false")]
    pub restart_on_death: bool,
    #[serde(skip_serializing_if = "is_false")]
    pub no_startup_id: bool,
    #[serde(skip_serializing_if = "is_false")]
    pub should_halt_compositor_on_death: bool,
    #[serde(skip_serializing_if = "is_false")]
    pub in_systemd_scope: bool,
}

/// An environment variable to set in the compositor's environment.
#[derive(Debug, Clone, Default, Serialize)]
pub struct EnvironmentVariable {
    pub key: String,
    pub value: String,
}

/// Window border appearance.
#[derive(Debug, Clone, Default, Serialize)]
pub struct BorderConfig {
    pub size: i32,
    pub radius: f32,
    /// Color as a hex string (`"RRGGBBAA"`) or an RGBA array `[r, g, b, a]` (0–255).
    pub color: String,
    /// Focused-window color as a hex string or RGBA array.
    pub focus_color: String,
}

/// Workspace configuration entry.
#[derive(Debug, Clone, Default, Serialize)]
pub struct WorkspaceConfig {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub number: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub name: Option<String>,
}

/// Drag-and-drop behaviour.
#[derive(Debug, Clone, Serialize)]
pub struct DragAndDropConfiguration {
    pub enabled: bool,
    /// The modifier keys required to initiate a drag-and-drop operation.
    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub modifiers: Vec<Modifier>,
}

impl Default for DragAndDropConfiguration {
    fn default() -> Self {
        Self { enabled: true, modifiers: Vec::new() }
    }
}

/// A single built-in animation (one phase of an easing sequence).
#[derive(Debug, Clone, Default, Serialize)]
pub struct BuiltInAnimationPart {
    /// The visual effect for this animation phase.
    #[serde(rename = "type")]
    pub type_: AnimationPartType,
    /// The easing function that controls timing.
    pub function: EasingFunction,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub c1: Option<f32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub c2: Option<f32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub c3: Option<f32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub c4: Option<f32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub n1: Option<f32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub d1: Option<f32>,
}

/// An animation definition for one animatable event.
#[derive(Debug, Clone, Default, Serialize)]
pub struct AnimationDefinition {
    /// The compositor event to animate.
    pub event: AnimationEvent,
    /// Whether to use a built-in animation effect or a plugin callback.
    #[serde(rename = "type")]
    pub type_: AnimationKind,
    /// Duration in seconds.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub duration: Option<f32>,
    /// The list of animation phases (required when `type_` is `BuiltIn`).
    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub parts: Vec<BuiltInAnimationPart>,
}

/// Mouse pointer configuration.
#[derive(Debug, Clone, Default, Serialize)]
pub struct MouseConfiguration {
    /// Swap left and right buttons.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub handedness: Option<Handedness>,
    /// Pointer acceleration profile.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub acceleration: Option<PointerAcceleration>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub acceleration_bias: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub vscroll_speed: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub hscroll_speed: Option<f64>,
}

/// Keymap (keyboard layout) configuration.
#[derive(Debug, Clone, Default, Serialize)]
pub struct KeymapConfiguration {
    pub language: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub variant: Option<String>,
    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub options: Vec<String>,
}

/// Keyboard repeat and layout configuration.
#[derive(Debug, Clone, Default, Serialize)]
pub struct KeyboardConfiguration {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub repeat_delay: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub repeat_rate: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub keymap: Option<KeymapConfiguration>,
}

/// Hover-click (dwell click) configuration.
#[derive(Debug, Clone, Default, Serialize)]
pub struct HoverClickConfiguration {
    pub enabled: bool,
    /// How long (ms) the pointer must hover before a click is generated.
    #[serde(skip_serializing_if = "Option::is_none", rename = "hover_duration")]
    pub hover_duration_ms: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cancel_displacement_threshold: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub reclick_displacement_threshold: Option<i32>,
}

/// Simulated secondary (right) click via long-press.
#[derive(Debug, Clone, Default, Serialize)]
pub struct SimulatedSecondaryClickConfiguration {
    pub enabled: bool,
    /// How long (ms) to hold before the secondary click is generated.
    #[serde(skip_serializing_if = "Option::is_none", rename = "hold_duration")]
    pub hold_duration_ms: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub displacement_threshold: Option<i32>,
}

/// Output (display) filter shader.
#[derive(Debug, Clone, Default, Serialize)]
pub struct OutputFilterConfiguration {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub shader_path: Option<String>,
}

/// Cursor appearance and focus behaviour.
#[derive(Debug, Clone, Default, Serialize)]
pub struct CursorConfiguration {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub scale: Option<f32>,
    /// Whether focus follows hover or requires a click.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub focus_mode: Option<CursorFocusMode>,
}

/// Slow keys (accessibility) configuration.
#[derive(Debug, Clone, Default, Serialize)]
pub struct SlowKeysConfiguration {
    pub enabled: bool,
    /// How long (ms) a key must be held before it registers.
    #[serde(skip_serializing_if = "Option::is_none", rename = "hold_delay")]
    pub hold_delay_ms: Option<u32>,
}

/// Sticky keys (accessibility) configuration.
#[derive(Debug, Clone, Default, Serialize)]
pub struct StickyKeysConfiguration {
    pub enabled: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub should_disable_if_two_keys_are_pressed_together: Option<bool>,
}

/// Touchpad configuration.
#[derive(Debug, Clone, Default, Serialize)]
pub struct TouchpadConfiguration {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub disable_while_typing: Option<bool>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub disable_with_external_mouse: Option<bool>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub acceleration_bias: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub vscroll_speed: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub hscroll_speed: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub tap_to_click: Option<bool>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub middle_mouse_button_emulation: Option<bool>,
    /// Touchpad click emulation mode.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub click_mode: Option<TouchpadClickMode>,
    /// Touchpad scroll method.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub scroll_mode: Option<TouchpadScrollMode>,
}

/// Screen magnifier configuration.
#[derive(Debug, Clone, Default, Serialize)]
pub struct MagnifierConfiguration {
    pub enabled: bool,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub scale: Option<f32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub scale_increment: Option<f32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub width: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub height: Option<i32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub size_increment: Option<i32>,
}

/// Configuration overrides that a plugin may return from [`Plugin::configure`].
///
/// Every field is optional. `None` means "do not override this value". The
/// compositor merges all loaded plugins' results and then merges the combined
/// result with the file-based configuration (plugin values win on conflict).
///
/// The `plugins` and `includes` keys of the compositor config cannot be set
/// by plugins and are intentionally absent from this struct.
#[derive(Debug, Clone, Default, Serialize)]
pub struct Configuration {
    /// The primary modifier key (e.g. `Modifier::Meta` for the Super/Windows key).
    #[serde(skip_serializing_if = "Option::is_none", rename = "action_key")]
    pub primary_modifier: Option<Modifier>,

    /// Custom key bindings that run shell commands.
    #[serde(skip_serializing_if = "Option::is_none", rename = "custom_actions")]
    pub custom_key_actions: Option<Vec<CustomKeyAction>>,

    /// Overrides for built-in compositor key bindings.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub default_action_overrides: Option<Vec<BuiltInKeyCommandOverride>>,

    /// Inner (between windows) gap size.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub inner_gaps: Option<Gaps>,

    /// Outer (screen edge) gap size.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub outer_gaps: Option<Gaps>,

    /// Applications to launch on startup.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub startup_apps: Option<Vec<StartupApp>>,

    /// Override the default terminal emulator command.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub terminal: Option<String>,

    /// Pixel amount to jump when resizing with keyboard shortcuts.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub resize_jump: Option<i32>,

    /// Extra environment variables to set in the compositor process.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub environment_variables: Option<Vec<EnvironmentVariable>>,

    /// Window border appearance.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub border: Option<BorderConfig>,

    /// Workspace layout definitions.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub workspaces: Option<Vec<WorkspaceConfig>>,

    /// Animation definitions per event. Each entry names an `event` plus the definition.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub animations: Option<Vec<AnimationDefinition>>,

    /// Whether animations are globally enabled.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub enable_animations: Option<bool>,

    /// The modifier keys used for window move operations.
    /// Use `vec![Modifier::Primary]` to follow the user's primary modifier.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub move_modifier: Option<Vec<Modifier>>,

    /// Drag-and-drop behaviour.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub drag_and_drop: Option<DragAndDropConfiguration>,

    /// Mouse pointer settings.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub mouse: Option<MouseConfiguration>,

    /// Touchpad settings.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub touchpad: Option<TouchpadConfiguration>,

    /// Keyboard repeat rate, delay, and keymap.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub keyboard: Option<KeyboardConfiguration>,

    /// Hover-click (dwell click) accessibility feature.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub hover_click: Option<HoverClickConfiguration>,

    /// Simulated secondary click accessibility feature.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub simulated_secondary_click: Option<SimulatedSecondaryClickConfiguration>,

    /// Output (display) post-processing filter.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub output_filter: Option<OutputFilterConfiguration>,

    /// Cursor appearance and focus behaviour.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cursor: Option<CursorConfiguration>,

    /// Slow keys accessibility feature.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub slow_keys: Option<SlowKeysConfiguration>,

    /// Sticky keys accessibility feature.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub sticky_keys: Option<StickyKeysConfiguration>,

    /// Screen magnifier.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub magnifier: Option<MagnifierConfiguration>,

    /// Whether switching to the current workspace goes back to the previous one.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub workspace_back_and_forth: Option<bool>,
}

fn is_false(v: &bool) -> bool {
    !v
}
