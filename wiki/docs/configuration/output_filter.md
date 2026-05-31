# Output Filter
Apply a custom output filter to the final render via a shader.

## Example
This example applies a grayscale filter to the output:

```yaml
# ~/.config/miracle-wm/config.yaml

output_filter:
    shader_path: ~/.config/miracle-wm/shaders/filters/grayscale.frag
```

Here is the corresponding shader file:

```c
// ~/.config/miracle-wm/shaders/filters/grayscale.frag

uniform sampler2D tex;
vec4 sample_to_rgba(in vec2 texcoord) {
    vec4 col = texture2D(tex, texcoord);
    float s = (col[0] + col[1] + col[2]) / 3.0;
    return vec4(s, s, s, col[3]);
}
```

## Schema

```yaml
output_filter:
  shader_path: <string>
```

## Properties

### `shader_path`

:   <small>required</small> **type:** String (filesystem path)

    Path to the fragment shader code for this filter. Tildes (`~`) are respected and will resolve to the appropriate home directory. Shaders can be placed in any directory.
    
    The shader must specify a function named `sample_to_rgba` with the following signature:
    
    ```c
    // The 'tex' uniform must be manually provided by the shader author.
    // This is the texture for the output.
    uniform sampler2D tex;
    
    // This method takes in a texture coordinate and expects the user
    // to output a color. Note that this texture corresponds to a corner
    // of the screen, not an individual window.
    vec4 sample_to_rgba(in vec2 texcoord) {
        vec4 originalColor = texture2D(tex, texcoord);
        /// Do some changes on the color...
        return vec4(...);  /// Return a new color
    }
    ```
    
    Miracle will call this method to resolve the color.
    
    The shader will automatically reload when it is changed. If the shader fails to compile for whatever reason (e.g., bad path or poor program), then the error will be logged to the console and Miracle will use the default shader instead.
    
    Miracle ships a number of default shaders that are installed in `/usr/share/miracle-wm/shaders/output_filter`.

## Default
```yaml
output_filter:
  shader_path: null
```

