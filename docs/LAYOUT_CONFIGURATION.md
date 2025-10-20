# Layout Configuration System

## Overview

The new layout configuration system provides complete control over the game's UI positioning, sizing, and scaling. All layout coordinates are specified in **virtual space** and automatically transformed to physical screen coordinates.

## Key Concepts

### Virtual Coordinates

Instead of hard-coding pixel positions for specific resolutions, the game uses a virtual coordinate system (default: 1920×1080). The game automatically scales and transforms these coordinates to fit your actual screen resolution.

**Benefits:**
- Resolution-independent layout
- Easy to design for a standard resolution
- Automatically adapts to different screen sizes
- Consistent aspect ratio handling

### Scale Modes

Two scaling strategies are available:

1. **AUTO (Uniform Scaling)**
   - Maintains the virtual aspect ratio
   - Adds black bars if screen aspect doesn't match
   - No distortion
   - Best for pixel-perfect rendering

2. **STRETCH (Independent X/Y Scaling)**
   - Fills the entire screen
   - May distort if aspect ratios don't match
   - No black bars
   - Best for fullscreen experience

## Configuration Options

### Global Settings

```ini
# Virtual resolution (default: 1920×1080)
LAYOUT_VIRTUAL_WIDTH=1920
LAYOUT_VIRTUAL_HEIGHT=1080

# Scale mode: AUTO or STRETCH
LAYOUT_SCALE_MODE=AUTO

# Global border styling (applied to all panels)
PANEL_BORDER_RADIUS=10        # Corner radius in pixels
PANEL_BORDER_THICKNESS=2      # Outline thickness
```

### UI Element Configuration

Each UI element (BANNER, STATS, BOARD, HUD, NEXT) can be configured with:

#### Position & Size
```ini
ELEMENT_X=100              # X position in virtual coordinates
ELEMENT_Y=50               # Y position in virtual coordinates
ELEMENT_WIDTH=200          # Width in virtual coordinates
ELEMENT_HEIGHT=400         # Height in virtual coordinates
```

#### Colors
```ini
ELEMENT_BG_COLOR=#181820           # Background color (hex)
ELEMENT_OUTLINE_COLOR=#5A5A78      # Outline color (hex)
ELEMENT_TEXT_COLOR=#C8C8DC         # Text color (hex)
```

#### Alpha/Transparency
```ini
ELEMENT_BG_ALPHA=255               # Background alpha (0-255)
ELEMENT_OUTLINE_ALPHA=200          # Outline alpha (0-255)
```

#### Enable/Disable
```ini
ELEMENT_ENABLED=1                  # 1 = enabled, 0 = disabled
```

### Available Elements

1. **BANNER** - Vertical text banner on the left
2. **STATS** - Piece statistics panel
3. **BOARD** - Game board container (10×20 grid)
4. **HUD** - Score, level, lines panel
5. **NEXT** - Next piece preview

## Board Container

The `BOARD` element is special:
- It defines a **container** where the game board is drawn
- The actual 10×20 board is centered within this container
- If the container aspect ratio doesn't match 10×20, black bars are added
- The board maintains its aspect ratio within the container

**Example:**
```ini
BOARD_X=300
BOARD_Y=100
BOARD_WIDTH=600
BOARD_HEIGHT=880
```

This creates a 600×880 container. The 10×20 board will be centered inside it.

## Example Layouts

### Default Layout (1920×1080 Virtual)

```ini
LAYOUT_VIRTUAL_WIDTH=1920
LAYOUT_VIRTUAL_HEIGHT=1080
LAYOUT_SCALE_MODE=AUTO

BANNER_X=10
BANNER_Y=100
BANNER_WIDTH=80
BANNER_HEIGHT=880

STATS_X=95
STATS_Y=100
STATS_WIDTH=200
STATS_HEIGHT=880

BOARD_X=300
BOARD_Y=100
BOARD_WIDTH=600
BOARD_HEIGHT=880

HUD_X=920
HUD_Y=100
HUD_WIDTH=500
HUD_HEIGHT=880
```

### Fullscreen Stretch Layout

```ini
LAYOUT_VIRTUAL_WIDTH=1920
LAYOUT_VIRTUAL_HEIGHT=1080
LAYOUT_SCALE_MODE=STRETCH

# Same coordinates as above
# But now stretches to fill screen completely
```

### Compact Layout (1280×720 Virtual)

```ini
LAYOUT_VIRTUAL_WIDTH=1280
LAYOUT_VIRTUAL_HEIGHT=720
LAYOUT_SCALE_MODE=AUTO

BANNER_X=10
BANNER_Y=50
BANNER_WIDTH=60
BANNER_HEIGHT=620

STATS_X=75
STATS_Y=50
STATS_WIDTH=150
STATS_HEIGHT=620

BOARD_X=230
BOARD_Y=50
BOARD_WIDTH=400
BOARD_HEIGHT=620

HUD_X=640
HUD_Y=50
HUD_WIDTH=400
HUD_HEIGHT=620
```

## Migration from Old System

### Old System (Hardcoded)
- Layout calculated based on screen size
- Fixed aspect ratio (16:9)
- Limited customization
- Resolution-dependent calculations

### New System (Configurable)
- Layout defined in virtual coordinates
- Flexible aspect ratios
- Full customization via .cfg files
- Resolution-independent

### Backward Compatibility

The old system's globals (`BX`, `BY`, `GX`, `GY`, etc.) are still populated for compatibility during the transition. However, new code should use the new `LayoutCache` fields:
- `layout.bannerRect` - Physical rect for banner
- `layout.statsRect` - Physical rect for stats
- `layout.boardContainerRect` - Physical rect for board container
- `layout.hudRect` - Physical rect for HUD
- `layout.nextRect` - Physical rect for next preview

## Tips

1. **Start with AUTO mode** to avoid distortion
2. **Design for 1920×1080** as it's the most common resolution
3. **Test on different resolutions** to ensure proper scaling
4. **Use STRETCH mode** only if you want to fill the screen completely
5. **Keep borders consistent** using global PANEL_BORDER_* settings
6. **Consider black bars** when using AUTO mode on non-standard aspects

## Technical Details

### Transformation Pipeline

1. **Load Config** → `LayoutConfig` (virtual coordinates)
2. **Apply Config** → Copy to global `layoutConfig`
3. **Calculate Layout** → `VirtualLayout::calculate()` computes scale factors
4. **Transform** → `VirtualLayout::toPhysical()` converts to screen coordinates
5. **Render** → Use physical coordinates in `SDL_RenderFillRect()`

### Performance

- Layout calculation is **cached** and only recalculated on window resize
- Virtual-to-physical transformation is **O(1)** per coordinate
- No performance impact during gameplay

## Future Enhancements

- [ ] Per-element border radius/thickness override
- [ ] Rotation support for elements
- [ ] Z-order configuration
- [ ] Animation support
- [ ] Multiple layout profiles (switchable in-game)
- [ ] Grid/alignment helpers in config

