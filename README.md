# Air Dribble Helper

A BakkesMod plugin that visualizes **optimal hit zones**, **actual ball contact points**, and **flip reset touches** to improve air dribbling mechanics.

---

## 🚀 Overview

**Air Dribble Helper** renders:

### 🔵 Ring Indicator
A configurable ring displayed under the ball at the optimal height for controlled air dribbles.

### 🔴 Touch Markers
Cross-shaped markers showing the exact point where your car hits the ball.

### 🟣 Flip Reset Marker
Special colored markers for **flip reset touches**.

---

## ✨ Features

### **Ring Indicator**
- Adjustable ring size / distance.
- Custom RGBA color.
- Smooth, closed ring drawing.

### **Hit / Contact Markers**
- Shows hit position directly on ball surface.
- Markers fade after a configurable duration.
- Limit the max number of markers visible simultaneously.

### **Flip Reset Detection**
- Detects `HasFlip` transitions (airborne).
- Filters out training pack resets using distance validation.
- Unique colors for correct / incorrect / flip reset hits.

### **Full Customization**
All settings available in GUI:

> **F2 → Plugins → Air Dribble Indicator Ring**

---

## 🔧 Configuration

### **Ring**
| Setting | Description |
|--------|-------------|
| `ring_enabled` | Enables ring rendering |
| `ring_size` | Distance from ball bottom |
| `ring_color` | RGBA |

### **Touch Markers**
| Setting | Description |
|--------|-------------|
| `touch_marker_enabled` | Enables hit markers |
| `touch_marker_size` | Marker size in px |
| `touch_marker_duration` | Fade-out duration |
| `max_touch_markers` | Maximum active markers |
| `touch_marker_correct_color` | Correct touch color |
| `touch_marker_incorrect_color` | Incorrect touch color |
| `touch_marker_reset_color` | Flip reset color |

---

## 🧠 How It Works

### Rendering
- Uses BakkesMod’s `CanvasWrapper` every frame.
- Projects 3D ring points into 2D with overlap to close gaps.
- Touch markers are drawn relative to ball position so they follow naturally.

### Hit Detection
The plugin hooks:
```
Car_TA.OnHitBall
```
This provides hit location, normal, and ball reference.

### Flip Reset Detection
Uses:
- Monitoring `HasFlip` transitions.
- Checking `!AnyWheelTouchingGround()`.
- Validating car→ball distance < **250uu** to remove false resets from training pack shot resets.

---

## 🎯 Use Cases

- Air dribble consistency training  
- Learning ideal hit height  
- Visualizing contact points  
- Practicing flip resets  
- Reviewing touches during replays or freeplay  

---

## ⚠ Limitations

- Wheel touches do not fire `OnHitBall`, flip reset detection relies on flip state logic.  
- Extreme camera angles can slightly distort projected markers.  
- Flip reset point is an approximation (game does not expose exact collision mesh).  

---

## 👤 Credits

**Author:** raistlinsk 
**Documentation & tooling:** ChatGPT  

---



