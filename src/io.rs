use std::collections::HashMap;

use glium::glutin::event::{VirtualKeyCode, ElementState};

// Keyboard state tracking
pub struct Keyboard {
    keys: HashMap <VirtualKeyCode, ElementState>
}

impl Keyboard {
    pub fn new() -> Keyboard {
        Keyboard {
            keys: HashMap::new()
        }
    }

    pub fn update(&mut self, key: VirtualKeyCode, state: ElementState) {
        self.keys.insert(key, state);
    }

    pub fn is_pressed(&self, key: VirtualKeyCode) -> bool {
        match self.keys.get(&key) {
            Some(&ElementState::Pressed) => true,
            _ => false
        }
    }

    pub fn is_released(&self, key: VirtualKeyCode) -> bool {
        match self.keys.get(&key) {
            Some(&ElementState::Released) => true,
            _ => false
        }
    }
}
