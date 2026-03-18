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
///         inner_gaps: Some(Gaps { x: 5, y: 5 }),
///         animations_enabled: Some(false),
///         ..Default::default()
///     })
/// }
/// ```
use serde::Serialize;

/// Gaps configuration. Both `x` (left/right) and `y` (top/bottom) are in pixels.
#[derive(Debug, Clone, Default, Serialize)]
pub struct Gaps {
    pub x: i32,
    pub y: i32,
}

/// A custom key binding that runs a shell command.
#[derive(Debug, Clone, Default, Serialize)]
pub struct CustomKeyAction {
    /// The keyboard action that triggers this binding (e.g. `"down"`, `"up"`, `"repeat"`).
    pub action: String,
    /// Modifier flags as a string (e.g. `"Mod4"`, `"Mod4+Shift"`).
    pub modifiers: String,
    /// The XKB keysym name (e.g. `"Return"`, `"a"`, `"Up"`).
    pub key: String,
    /// The shell command to execute.
    pub command: String,
}

/// Override the key binding for a built-in compositor action.
#[derive(Debug, Clone, Default, Serialize)]
pub struct BuiltInKeyCommandOverride {
    /// Name of the built-in action (e.g. `"terminal"`, `"close_window"`).
    pub name: String,
    /// The keyboard action that triggers this binding.
    pub action: String,
    /// Modifier flags as a string.
    pub modifiers: String,
    /// The XKB keysym name.
    pub key: String,
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
    /// Modifier string (e.g. `"Mod4+Shift"`).
    pub modifiers: String,
}

impl Default for DragAndDropConfiguration {
    fn default() -> Self {
        Self { enabled: true, modifiers: String::new() }
    }
}

/// A single built-in animation (one phase of an easing sequence).
#[derive(Debug, Clone, Default, Serialize)]
pub struct BuiltInAnimationPart {
    /// Built-in animation type: `"slide"`, `"grow"`, `"shrink"`, `"fade"`, `"disabled"`.
    #[serde(rename = "type")]
    pub type_: String,
    /// Easing function name (e.g. `"ease_in_out_cubic"`).
    pub function: String,
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
    /// The compositor event to animate: `"window_open"`, `"window_move"`,
    /// `"window_close"`, `"workspace_switch"`.
    pub event: String,
    /// Animation kind: `"built_in"` or `"plugin"`.
    #[serde(rename = "type")]
    pub type_: String,
    /// Duration in seconds.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub duration: Option<f32>,
    /// The list of animation phases (required when `type_` is `"built_in"`).
    #[serde(skip_serializing_if = "Vec::is_empty")]
    pub parts: Vec<BuiltInAnimationPart>,
}

/// Mouse pointer configuration.
#[derive(Debug, Clone, Default, Serialize)]
pub struct MouseConfiguration {
    /// Swap left and right buttons: `"right_handed"` or `"left_handed"`.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub handedness: Option<String>,
    /// Pointer acceleration profile: `"none"` or `"adaptive"`.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub acceleration: Option<String>,
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
    /// `"hover"` or `"click"`.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub focus_mode: Option<String>,
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
    /// `"none"`, `"button_areas"`, `"click_finger"`.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub click_mode: Option<String>,
    /// `"none"`, `"two_finger"`, `"edge"`, `"button"`.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub scroll_mode: Option<String>,
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
    /// The primary modifier key (e.g. `"Mod4"` for Super/Windows key).
    #[serde(skip_serializing_if = "Option::is_none", rename = "action_key")]
    pub primary_modifier: Option<String>,

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

    /// The modifier key used for window move operations (e.g. `"Mod4"`).
    #[serde(skip_serializing_if = "Option::is_none")]
    pub move_modifier: Option<String>,

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
